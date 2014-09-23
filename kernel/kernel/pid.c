#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/rculist.h>
#include <linux/bootmem.h>
#include <linux/hash.h>
#include <linux/pid_namespace.h>
#include <linux/init_task.h>
#include <linux/syscalls.h>

#define pid_hashfn(nr, ns)	\
	hash_long((unsigned long)nr + (unsigned long)ns, pidhash_shift)
static struct hlist_head *pid_hash;
static int pidhash_shift;
struct pid init_struct_pid = INIT_STRUCT_PID;

int pid_max = PID_MAX_DEFAULT;

#define RESERVED_PIDS		300

int pid_max_min = RESERVED_PIDS + 1;
int pid_max_max = PID_MAX_LIMIT;

#define BITS_PER_PAGE		(PAGE_SIZE*8)
#define BITS_PER_PAGE_MASK	(BITS_PER_PAGE-1)

static inline int mk_pid(struct pid_namespace *pid_ns,
		struct pidmap *map, int off)
{
	return (map - pid_ns->pidmap)*BITS_PER_PAGE + off;
}

#define find_next_offset(map, off)					\
		find_next_zero_bit((map)->page, BITS_PER_PAGE, off)

struct pid_namespace init_pid_ns = {
	.kref = {
		.refcount       = ATOMIC_INIT(2),
	},
	.pidmap = {
		[ 0 ... PIDMAP_ENTRIES-1] = { ATOMIC_INIT(BITS_PER_PAGE), NULL }
	},
	.last_pid = 0,
	.level = 0,
	.child_reaper = &init_task,
};
EXPORT_SYMBOL_GPL(init_pid_ns);

int is_container_init(struct task_struct *tsk)
{
	int ret = 0;
	struct pid *pid;

	rcu_read_lock();
	pid = task_pid(tsk);
	if (pid != NULL && pid->numbers[pid->level].nr == 1)
		ret = 1;
	rcu_read_unlock();

	return ret;
}
EXPORT_SYMBOL(is_container_init);

static  __cacheline_aligned_in_smp DEFINE_SPINLOCK(pidmap_lock);

static void free_pidmap(struct upid *upid)
{
	int nr = upid->nr;
	struct pidmap *map = upid->ns->pidmap + nr / BITS_PER_PAGE;
	int offset = nr & BITS_PER_PAGE_MASK;

	clear_bit(offset, map->page);
	atomic_inc(&map->nr_free);
}

static int pid_before(int base, int a, int b)
	    {
	return (unsigned)(a - base) < (unsigned)(b - base);
	    }

static void set_last_pid(struct pid_namespace *pid_ns, int base, int pid)
	{
	int prev;
	int last_write = base;
	do {
	prev = last_write;
	last_write = cmpxchg(&pid_ns->last_pid, prev, pid);
	} while ((prev != last_write) && (pid_before(base, last_write, pid)));
	}

static int alloc_pidmap(struct pid_namespace *pid_ns)
{
	int i, offset, max_scan, pid, last = pid_ns->last_pid;
	struct pidmap *map;

	pid = last + 1;
	if (pid >= pid_max)
		pid = RESERVED_PIDS;
	offset = pid & BITS_PER_PAGE_MASK;
	map = &pid_ns->pidmap[pid/BITS_PER_PAGE];
	max_scan = (pid_max + BITS_PER_PAGE - 1)/BITS_PER_PAGE - !offset;
	for (i = 0; i <= max_scan; ++i) {
		if (unlikely(!map->page)) {
			void *page = kzalloc(PAGE_SIZE, GFP_KERNEL);
			spin_lock_irq(&pidmap_lock);
			if (map->page)
				kfree(page);
			else
				map->page = page;
			spin_unlock_irq(&pidmap_lock);
			if (unlikely(!map->page))
				break;
		}
		if (likely(atomic_read(&map->nr_free))) {
			do {
				if (!test_and_set_bit(offset, map->page)) {
					atomic_dec(&map->nr_free);
					set_last_pid(pid_ns, last, pid);
					return pid;
				}
				offset = find_next_offset(map, offset);
				pid = mk_pid(pid_ns, map, offset);
			} while (offset < BITS_PER_PAGE && pid < pid_max &&
					(i != max_scan || pid < last ||
					    !((last+1) & BITS_PER_PAGE_MASK)));
		}
		if (map < &pid_ns->pidmap[(pid_max-1)/BITS_PER_PAGE]) {
			++map;
			offset = 0;
		} else {
			map = &pid_ns->pidmap[0];
			offset = RESERVED_PIDS;
			if (unlikely(last == offset))
				break;
		}
		pid = mk_pid(pid_ns, map, offset);
	}
	return -1;
}

int next_pidmap(struct pid_namespace *pid_ns, int last)
{
	int offset;
	struct pidmap *map, *end;

	offset = (last + 1) & BITS_PER_PAGE_MASK;
	map = &pid_ns->pidmap[(last + 1)/BITS_PER_PAGE];
	end = &pid_ns->pidmap[PIDMAP_ENTRIES];
	for (; map < end; map++, offset = 0) {
		if (unlikely(!map->page))
			continue;
		offset = find_next_bit((map)->page, BITS_PER_PAGE, offset);
		if (offset < BITS_PER_PAGE)
			return mk_pid(pid_ns, map, offset);
	}
	return -1;
}

void put_pid(struct pid *pid)
{
	struct pid_namespace *ns;

	if (!pid)
		return;

	ns = pid->numbers[pid->level].ns;
	if ((atomic_read(&pid->count) == 1) ||
	     atomic_dec_and_test(&pid->count)) {
		kmem_cache_free(ns->pid_cachep, pid);
		put_pid_ns(ns);
	}
}
EXPORT_SYMBOL_GPL(put_pid);

static void delayed_put_pid(struct rcu_head *rhp)
{
	struct pid *pid = container_of(rhp, struct pid, rcu);
	put_pid(pid);
}

void free_pid(struct pid *pid)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&pidmap_lock, flags);
	for (i = 0; i <= pid->level; i++)
		hlist_del_rcu(&pid->numbers[i].pid_chain);
	spin_unlock_irqrestore(&pidmap_lock, flags);

	for (i = 0; i <= pid->level; i++)
		free_pidmap(pid->numbers + i);

	call_rcu(&pid->rcu, delayed_put_pid);
}

struct pid *alloc_pid(struct pid_namespace *ns)
{
	struct pid *pid;
	enum pid_type type;
	int i, nr;
	struct pid_namespace *tmp;
	struct upid *upid;

	pid = kmem_cache_alloc(ns->pid_cachep, GFP_KERNEL);
	if (!pid)
		goto out;

	tmp = ns;
	for (i = ns->level; i >= 0; i--) {
		nr = alloc_pidmap(tmp);
		if (nr < 0)
			goto out_free;

		pid->numbers[i].nr = nr;
		pid->numbers[i].ns = tmp;
		tmp = tmp->parent;
	}

	get_pid_ns(ns);
	pid->level = ns->level;
	atomic_set(&pid->count, 1);
	for (type = 0; type < PIDTYPE_MAX; ++type)
		INIT_HLIST_HEAD(&pid->tasks[type]);

	spin_lock_irq(&pidmap_lock);
	for (i = ns->level; i >= 0; i--) {
		upid = &pid->numbers[i];
		hlist_add_head_rcu(&upid->pid_chain,
				&pid_hash[pid_hashfn(upid->nr, upid->ns)]);
	}
	spin_unlock_irq(&pidmap_lock);

out:
	return pid;

out_free:
	while (++i <= ns->level)
		free_pidmap(pid->numbers + i);

	kmem_cache_free(ns->pid_cachep, pid);
	pid = NULL;
	goto out;
}

struct pid *find_pid_ns(int nr, struct pid_namespace *ns)
{
	struct hlist_node *elem;
	struct upid *pnr;

	hlist_for_each_entry_rcu(pnr, elem,
			&pid_hash[pid_hashfn(nr, ns)], pid_chain)
		if (pnr->nr == nr && pnr->ns == ns)
			return container_of(pnr, struct pid,
					numbers[ns->level]);

	return NULL;
}
EXPORT_SYMBOL_GPL(find_pid_ns);

struct pid *find_vpid(int nr)
{
	return find_pid_ns(nr, current->nsproxy->pid_ns);
}
EXPORT_SYMBOL_GPL(find_vpid);

void attach_pid(struct task_struct *task, enum pid_type type,
		struct pid *pid)
{
	struct pid_link *link;

	link = &task->pids[type];
	link->pid = pid;
	hlist_add_head_rcu(&link->node, &pid->tasks[type]);
}

static void __change_pid(struct task_struct *task, enum pid_type type,
			struct pid *new)
{
	struct pid_link *link;
	struct pid *pid;
	int tmp;

	link = &task->pids[type];
	pid = link->pid;

	hlist_del_rcu(&link->node);
	link->pid = new;

	for (tmp = PIDTYPE_MAX; --tmp >= 0; )
		if (!hlist_empty(&pid->tasks[tmp]))
			return;

	free_pid(pid);
}

void detach_pid(struct task_struct *task, enum pid_type type)
{
	__change_pid(task, type, NULL);
}

void change_pid(struct task_struct *task, enum pid_type type,
		struct pid *pid)
{
	__change_pid(task, type, pid);
	attach_pid(task, type, pid);
}

void transfer_pid(struct task_struct *old, struct task_struct *new,
			   enum pid_type type)
{
	new->pids[type].pid = old->pids[type].pid;
	hlist_replace_rcu(&old->pids[type].node, &new->pids[type].node);
}

struct task_struct *pid_task(struct pid *pid, enum pid_type type)
{
	struct task_struct *result = NULL;
	if (pid) {
		struct hlist_node *first;
		first = rcu_dereference(pid->tasks[type].first);
		if (first)
			result = hlist_entry(first, struct task_struct, pids[(type)].node);
	}
	return result;
}
EXPORT_SYMBOL(pid_task);

struct task_struct *find_task_by_pid_type_ns(int type, int nr,
		struct pid_namespace *ns)
{
	return pid_task(find_pid_ns(nr, ns), type);
}

EXPORT_SYMBOL(find_task_by_pid_type_ns);

struct task_struct *find_task_by_vpid(pid_t vnr)
{
	return find_task_by_pid_type_ns(PIDTYPE_PID, vnr,
			current->nsproxy->pid_ns);
}
EXPORT_SYMBOL(find_task_by_vpid);

struct task_struct *find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns)
{
	return find_task_by_pid_type_ns(PIDTYPE_PID, nr, ns);
}
EXPORT_SYMBOL(find_task_by_pid_ns);

struct pid *get_task_pid(struct task_struct *task, enum pid_type type)
{
	struct pid *pid;
	rcu_read_lock();
	pid = get_pid(task->pids[type].pid);
	rcu_read_unlock();
	return pid;
}

struct task_struct *get_pid_task(struct pid *pid, enum pid_type type)
{
	struct task_struct *result;
	rcu_read_lock();
	result = pid_task(pid, type);
	if (result)
		get_task_struct(result);
	rcu_read_unlock();
	return result;
}

struct pid *find_get_pid(pid_t nr)
{
	struct pid *pid;

	rcu_read_lock();
	pid = get_pid(find_vpid(nr));
	rcu_read_unlock();

	return pid;
}
EXPORT_SYMBOL_GPL(find_get_pid);

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns)
{
	struct upid *upid;
	pid_t nr = 0;

	if (pid && ns->level <= pid->level) {
		upid = &pid->numbers[ns->level];
		if (upid->ns == ns)
			nr = upid->nr;
	}
	return nr;
}

pid_t pid_vnr(struct pid *pid)
{
	return pid_nr_ns(pid, current->nsproxy->pid_ns);
}
EXPORT_SYMBOL_GPL(pid_vnr);

pid_t __task_pid_nr_ns(struct task_struct *task, enum pid_type type,
			struct pid_namespace *ns)
{
	pid_t nr = 0;

	rcu_read_lock();
	if (!ns)
		ns = current->nsproxy->pid_ns;
	if (likely(pid_alive(task))) {
		if (type != PIDTYPE_PID)
			task = task->group_leader;
		nr = pid_nr_ns(task->pids[type].pid, ns);
	}
	rcu_read_unlock();

	return nr;
}
EXPORT_SYMBOL(__task_pid_nr_ns);

pid_t task_tgid_nr_ns(struct task_struct *tsk, struct pid_namespace *ns)
{
	return pid_nr_ns(task_tgid(tsk), ns);
}
EXPORT_SYMBOL(task_tgid_nr_ns);

struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
	return ns_of_pid(task_pid(tsk));
}
EXPORT_SYMBOL_GPL(task_active_pid_ns);

struct pid *find_ge_pid(int nr, struct pid_namespace *ns)
{
	struct pid *pid;

	do {
		pid = find_pid_ns(nr, ns);
		if (pid)
			break;
		nr = next_pidmap(ns, nr);
	} while (nr > 0);

	return pid;
}

void __init pidhash_init(void)
{
	int i, pidhash_size;
	unsigned long megabytes = nr_kernel_pages >> (20 - PAGE_SHIFT);

	pidhash_shift = max(4, fls(megabytes * 4));
	pidhash_shift = min(12, pidhash_shift);
	pidhash_size = 1 << pidhash_shift;

	printk("PID hash table entries: %d (order: %d, %Zd bytes)\n",
		pidhash_size, pidhash_shift,
		pidhash_size * sizeof(struct hlist_head));

	pid_hash = alloc_bootmem(pidhash_size *	sizeof(*(pid_hash)));
	if (!pid_hash)
		panic("Could not alloc pidhash!\n");
	for (i = 0; i < pidhash_size; i++)
		INIT_HLIST_HEAD(&pid_hash[i]);
}

void __init pidmap_init(void)
{
	init_pid_ns.pidmap[0].page = kzalloc(PAGE_SIZE, GFP_KERNEL);
	set_bit(0, init_pid_ns.pidmap[0].page);
	atomic_dec(&init_pid_ns.pidmap[0].nr_free);

	init_pid_ns.pid_cachep = KMEM_CACHE(pid,
			SLAB_HWCACHE_ALIGN | SLAB_PANIC);
}
