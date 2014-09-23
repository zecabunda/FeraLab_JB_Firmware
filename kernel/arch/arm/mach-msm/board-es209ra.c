/*FeraLab*/

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/mfd/tps65023.h>
#include <linux/power_supply.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/sirc.h>
#include <mach/dma.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_hsusb.h>
#include <mach/msm_hsusb_hw.h>
#include <mach/msm_serial_hs.h>
#include <mach/pmic.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_spi.h>
#include <linux/max17040.h>
#include <linux/akm8973.h>
#include "devices.h"
#include "timer.h"
#include "socinfo.h"
#include "msm-keypad-devices.h"
#include "pm.h"
#include "smd_private.h"
#include "proc_comm.h"
#include <linux/msm_kgsl.h>
#include <linux/usb/android_composite.h>
#include "board-es209ra.h"
#include "board-es209ra-keypad.h"
#include "es209ra_headset.h"
#include <linux/spi/es209ra_touch_mt.h>
#include <asm/setup.h>
#include <linux/keyreset.h>
#include <mach/semc_low_batt_shutdown.h>
#include <linux/semc/msm_pmic_vibrator.h>
#include <linux/bma150_ng.h>
#include "qdsp6/q6audio.h"
#include <linux/nt35580.h>
#include "../../../drivers/video/msm/msm_fb_panel.h"
#include "../../../drivers/video/msm/mddihost.h"

#define AKM8973_GPIO_RESET_PIN	2
#define AKM8973_GPIO_RESET_ON	0
#define AKM8973_GPIO_RESET_OFF	1
#define SMEM_SPINLOCK_I2C	"S:6"
#define PMIC_VREG_WLAN_LEVEL	2600
#define PMIC_VREG_GP6_LEVEL	2850
#define TPS65023_MAX_DCDC1	1600
#define PMEM_KERNEL_EBI1_SIZE	0x48000
#define MSM_PMEM_MDP_SIZE	0x1C91000
#define MSM_PMEM_ADSP_SIZE	0x2196000
#define MSM_SHARED_RAM_PHYS	0x00100000
#define MSM_PMEM_SMI_SIZE	0x01500000
#define MSM_PMEM_SMI_BASE	0x02B00000
#define MSM_FB_BASE		0x02B00000
#define MSM_FB_SIZE		0x500000
#define MSM_GPU_PHYS_SIZE	SZ_8M
#define MSM_GPU_PHYS_BASE	(MSM_FB_BASE + MSM_FB_SIZE)
#define MSM_PMEM_SMIPOOL_BASE	(MSM_GPU_PHYS_BASE + MSM_GPU_PHYS_SIZE)
#define MSM_PMEM_SMIPOOL_SIZE	(MSM_PMEM_SMI_SIZE - MSM_FB_SIZE - MSM_GPU_PHYS_SIZE)

static int msm7227_platform_set_vib_voltage(u16 volt_mv)
{
        int rc = pmic_vib_mot_set_volt(volt_mv);
        if (rc)
                printk(KERN_ERR "%s: Failed to set motor voltage\n", __func__);
        return rc;
}

static int msm7227_platform_init_vib_hw(void)
{
        int rc = pmic_vib_mot_set_mode(PM_VIB_MOT_MODE__MANUAL);
        if (rc) {
                printk(KERN_ERR "%s: Failed to set pin mode\n", __func__);
                return rc;
        }
        return pmic_vib_mot_set_volt(0);
}

static struct msm_pmic_vibrator_platform_data vibrator_platform_data = {
        .min_voltage = 1200,
        .max_voltage = 1800,
        .off_voltage = 0,
        .default_voltage = 1400,
        .mimimal_on_time = 20,
        .platform_set_vib_voltage = msm7227_platform_set_vib_voltage,
        .platform_init_vib_hw = msm7227_platform_init_vib_hw,
};
static struct platform_device vibrator_device = {
        .name = "msm_pmic_vibrator",
        .id = -1,
        .dev = {
                .platform_data = &vibrator_platform_data,
        },
};

static char *usb_func_msc[] = {
	"usb_mass_storage",
};
static char *usb_func_msc_adb[] = {
	"usb_mass_storage",
	"adb",
};
static char *usb_func_rndis[] = {
	"rndis",
};
static char *usb_func_adb_rndis[] = {
	"rndis",
	"adb",
};

static char *usb_func_msc_adb_eng[] = {
	"usb_mass_storage",
	"adb",
	"modem",
	"nmea",
	"diag",
};

static char *usb_functions_all[] = {
	"rndis",
	"usb_mass_storage",
	"adb",
	"modem",
	"nmea",
	"diag",
};

static struct android_usb_product android_usb_products[] = {
	{
		.product_id = 0xE12E,
		.functions = usb_func_msc,
		.num_functions = ARRAY_SIZE(usb_func_msc),
	},
	{
		.product_id = 0x612E,
		.functions = usb_func_msc_adb,
		.num_functions = ARRAY_SIZE(usb_func_msc_adb),
	},
	{
		.product_id = 0x712E,
		.functions = usb_func_rndis,
		.num_functions = ARRAY_SIZE(usb_func_rndis),
	},
	{
		.product_id = 0x812E,
		.functions = usb_func_adb_rndis,
		.num_functions = ARRAY_SIZE(usb_func_adb_rndis),
	},
	{
		.product_id = 0x6146,
		.functions = usb_func_msc_adb_eng,
		.num_functions = ARRAY_SIZE(usb_func_msc_adb_eng),
	}
};

static struct usb_mass_storage_platform_data mass_storage_pdata = {
        .nluns = 1,
        .vendor = "SEMC",
        .product = "Mass Storage",
        .release = 0x0100,
};

static struct platform_device usb_mass_storage_device = {
        .name = "usb_mass_storage",
        .id = -1,
        .dev = {
                .platform_data = &mass_storage_pdata,
                },
};

static struct usb_ether_platform_data rndis_pdata = {
	.vendorID	= 0x0FCE,
	.vendorDescr	= "SEMC",
};

static struct platform_device rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= 0x0FCE,
	.product_id		= 0xE12E,
	.version		= 0x0100,
	.product_name		= "SEMC HSUSB Device",
	.manufacturer_name	= "SEMC",
	.serial_number		= "1234567890ABCDEF",
	.num_products		= ARRAY_SIZE(android_usb_products),
	.products		= android_usb_products,
	.num_functions		= ARRAY_SIZE(usb_functions_all),
	.functions		= usb_functions_all,
};
static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
 
static struct platform_device hs_device = {
       .name   = "msm-handset",
       .id     = -1,
       .dev    = {
               .platform_data = "8k_handset",
       },
};

#ifdef CONFIG_USB_FS_HOST
static struct msm_gpio fsusb_config[] = {
	{ GPIO_CFG(139, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "fs_dat" },
	{ GPIO_CFG(140, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "fs_se0" },
	{ GPIO_CFG(141, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "fs_oe_n" },
};

static int fsusb_gpio_init(void)
{
	return msm_gpios_request(fsusb_config, ARRAY_SIZE(fsusb_config));
}

static void msm_fsusb_setup_gpio(unsigned int enable)
{
	if (enable)
		msm_gpios_enable(fsusb_config, ARRAY_SIZE(fsusb_config));
	else
		msm_gpios_disable(fsusb_config, ARRAY_SIZE(fsusb_config));

}
#endif

#define MSM_USB_BASE              ((unsigned)addr)
static unsigned ulpi_read(void __iomem *addr, unsigned reg)
{
	unsigned timeout = 90000;

	writel(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg),
	       USB_ULPI_VIEWPORT);

	while ((readl(USB_ULPI_VIEWPORT) & ULPI_RUN) && (--timeout))
		cpu_relax();

	if (timeout == 0) {
		printk(KERN_ERR "ulpi_read: timeout %08x\n",
			readl(USB_ULPI_VIEWPORT));
		return 0xffffffff;
	}
	return ULPI_DATA_READ(readl(USB_ULPI_VIEWPORT));
}

static int ulpi_write(void __iomem *addr, unsigned val, unsigned reg)
{
	unsigned timeout = 9000;

	writel(ULPI_RUN | ULPI_WRITE |
	       ULPI_ADDR(reg) | ULPI_DATA(val),
	       USB_ULPI_VIEWPORT);

	while ((readl(USB_ULPI_VIEWPORT) & ULPI_RUN) && (--timeout))
		cpu_relax();

	if (timeout == 0) {
		printk(KERN_ERR "ulpi_write: timeout\n");
		return -1;
	}

	return 0;
}

struct clk *hs_clk, *phy_clk;
#define CLKRGM_APPS_RESET_USBH      37
#define CLKRGM_APPS_RESET_USB_PHY   34
static void msm_hsusb_apps_reset_link(int reset)
{
	if (reset)
		clk_reset(hs_clk, CLK_RESET_ASSERT);
	else
		clk_reset(hs_clk, CLK_RESET_DEASSERT);
}

static void msm_hsusb_apps_reset_phy(void)
{
	clk_reset(phy_clk, CLK_RESET_ASSERT);
	msleep(1);
	clk_reset(phy_clk, CLK_RESET_DEASSERT);
}

#define ULPI_VERIFY_MAX_LOOP_COUNT  3
static int msm_hsusb_phy_verify_access(void __iomem *addr)
{
	int temp;

	for (temp = 0; temp < ULPI_VERIFY_MAX_LOOP_COUNT; temp++) {
		if (ulpi_read(addr, ULPI_DEBUG) != (unsigned)-1)
			break;
		msm_hsusb_apps_reset_phy();
	}

	if (temp == ULPI_VERIFY_MAX_LOOP_COUNT) {
		pr_err("%s: ulpi read failed for %d times\n",
				__func__, ULPI_VERIFY_MAX_LOOP_COUNT);
		return -1;
	}

	return 0;
}

static unsigned msm_hsusb_ulpi_read_with_reset(void __iomem *addr, unsigned reg)
{
	int temp;
	unsigned res;

	for (temp = 0; temp < ULPI_VERIFY_MAX_LOOP_COUNT; temp++) {
		res = ulpi_read(addr, reg);
		if (res != -1)
			return res;
		msm_hsusb_apps_reset_phy();
	}

	pr_err("%s: ulpi read failed for %d times\n",
			__func__, ULPI_VERIFY_MAX_LOOP_COUNT);

	return -1;
}

static int msm_hsusb_ulpi_write_with_reset(void __iomem *addr,
		unsigned val, unsigned reg)
{
	int temp;
	int res;

	for (temp = 0; temp < ULPI_VERIFY_MAX_LOOP_COUNT; temp++) {
		res = ulpi_write(addr, val, reg);
		if (!res)
			return 0;
		msm_hsusb_apps_reset_phy();
	}

	pr_err("%s: ulpi write failed for %d times\n",
			__func__, ULPI_VERIFY_MAX_LOOP_COUNT);
	return -1;
}

static int msm_hsusb_phy_caliberate(void __iomem *addr)
{
	int ret;
	unsigned res;

	ret = msm_hsusb_phy_verify_access(addr);
	if (ret)
		return -ETIMEDOUT;

	res = msm_hsusb_ulpi_read_with_reset(addr, ULPI_FUNC_CTRL_CLR);
	if (res == -1)
		return -ETIMEDOUT;

	res = msm_hsusb_ulpi_write_with_reset(addr,
			res | ULPI_SUSPENDM,
			ULPI_FUNC_CTRL_CLR);
	if (res)
		return -ETIMEDOUT;

	msm_hsusb_apps_reset_phy();

	return msm_hsusb_phy_verify_access(addr);
}

#define USB_LINK_RESET_TIMEOUT      (msecs_to_jiffies(10))
static int msm_hsusb_native_phy_reset(void __iomem *addr)
{
	u32 temp;
	unsigned long timeout;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa())
		return msm_hsusb_phy_reset();

	msm_hsusb_apps_reset_link(1);
	msm_hsusb_apps_reset_phy();
	msm_hsusb_apps_reset_link(0);

	temp = (readl(USB_PORTSC) & ~PORTSC_PTS);
	writel(temp | PORTSC_PTS_ULPI, USB_PORTSC);

	if (msm_hsusb_phy_caliberate(addr))
		return -1;

	writel(USBCMD_RESET, USB_USBCMD);
	timeout = jiffies + USB_LINK_RESET_TIMEOUT;
	while (readl(USB_USBCMD) & USBCMD_RESET) {
		if (time_after(jiffies, timeout)) {
			pr_err("usb link reset timeout\n");
			break;
		}
		msleep(1);
	}

	return 0;
}

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
};

#ifdef CONFIG_USB_FS_HOST
static struct msm_usb_host_platform_data msm_usb_host2_pdata = {
	.phy_info	= USB_PHY_SERIAL_PMIC,
	.config_gpio = msm_fsusb_setup_gpio,
	.vbus_power = msm_hsusb_vbus_power,
};
#endif

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_smipool_pdata = {
	.name = "pmem_smipool",
	.start = MSM_PMEM_SMIPOOL_BASE,
	.size = MSM_PMEM_SMIPOOL_SIZE,
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_smipool_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_smipool_pdata },
};

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	int ret = -EPERM;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		if (!strncmp(name, "mddi_toshiba_wvga_pt", 20))
			ret = 0;
		else
			ret = -ENODEV;
	} else if ((machine_is_qsd8x50_surf() || machine_is_qsd8x50a_surf())
			&& !strcmp(name, "lcdc_external"))
		ret = 0;

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA1200000,
		.end	= 0xA1200000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

static struct platform_device qsd_device_spi = {
	.name	        = "spi_qsd",
	.id	        = 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

static struct es209ra_touch_platform_data es209ra_touch_data = {
	.gpio_irq_pin		= 37,
	.gpio_reset_pin		= 30,
	.x_min			= 0,
	.x_max			= 480,
	.y_min			= 0,
	.y_max			= 854,
};

static struct spi_board_info msm_spi_board_info[] __initdata = {
	{
		.modalias	= "es209ra_touch",
		.mode		= SPI_MODE_0,
		.irq		= INT_ES209RA_GPIO_TOUCHPAD,
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 2000000,
		.platform_data  = &es209ra_touch_data,
	}
};

#define CT_CSR_PHYS		0xA8700000
#define TCSR_SPI_MUX		(ct_csr_base + 0x54)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_csr_base = 0;
	u32 spi_mux;
	int ret = 0;

	ct_csr_base = ioremap(CT_CSR_PHYS, PAGE_SIZE);
	if (!ct_csr_base) {
		pr_err("%s: Could not remap %x\n", __func__, CT_CSR_PHYS);
		return -1;
	}

	spi_mux = readl(TCSR_SPI_MUX);
	switch (spi_mux) {
	case (1):
		qsd_spi_resources[4].start  = DMOV_HSUART1_RX_CHAN;
		qsd_spi_resources[4].end    = DMOV_HSUART1_TX_CHAN;
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[4].start  = DMOV_HSUART2_RX_CHAN;
		qsd_spi_resources[4].end    = DMOV_HSUART2_TX_CHAN;
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[4].start  = DMOV_CE_OUT_CHAN;
		qsd_spi_resources[4].end    = DMOV_CE_IN_CHAN;
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -1;
	}

	iounmap(ct_csr_base);
	return ret;
}

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(17, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), "spi_clk" },
	{ GPIO_CFG(18, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "spi_mosi" },
	{ GPIO_CFG(19, 1, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), "spi_miso" },
	{ GPIO_CFG(20, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "spi_cs0" },
};

static int msm_qsd_spi_gpio_config(void)
{
	int rc;

	rc = msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
	if (rc)
		return rc;

	return 0;
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 19200000,
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}

static int mddi_power_save_on;
static void msm_fb_mddi_power_save(int on)
{
	int flag_on = !!on;
	int ret;


	if (mddi_power_save_on == flag_on)
		return;

	mddi_power_save_on = flag_on;

	ret = pmic_lp_mode_control(flag_on ? OFF_CMD : ON_CMD,
		PM_VREG_LP_MSME2_ID);
	if (ret)
		printk(KERN_ERR "%s: pmic_lp_mode failed!\n", __func__);
}

static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
	*clk_rate *= 2;
	return 0;
}

static struct mddi_platform_data mddi_pdata = {
	.mddi_power_save = msm_fb_mddi_power_save,
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 98,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", &mddi_pdata);
	msm_fb_register_device("emdh", &mddi_pdata);
	msm_fb_register_device("tvenc", 0);
	msm_fb_register_device("lcdc", 0);
}

#define NT35580_GPIO_XRST 100
static struct vreg *vreg_mmc;
static struct vreg *vreg_gp2;

static void tmd_wvga_lcd_power_on(void)
{
	int rc = 0;

	local_irq_disable();

	rc = vreg_enable(vreg_gp2);
	if (rc) {
		local_irq_enable();
		printk(KERN_ERR"%s:vreg_enable(gp2)err. rc=%d\n", __func__, rc);
		return;
	}
	rc = vreg_enable(vreg_mmc);
	if (rc) {
		local_irq_enable();
		printk(KERN_ERR"%s:vreg_enable(mmc)err. rc=%d\n", __func__, rc);
		return;
	}
	local_irq_enable();
	msleep(45);
	gpio_set_value(NT35580_GPIO_XRST, 1);
	msleep(9);
	gpio_set_value(NT35580_GPIO_XRST, 0);
	msleep(1);
	gpio_set_value(NT35580_GPIO_XRST, 1);
	msleep(210);
}

static void tmd_wvga_lcd_power_off(void)
{
	gpio_set_value(NT35580_GPIO_XRST, 0);
	msleep(9);
	local_irq_disable();
	vreg_disable(vreg_mmc);
	vreg_disable(vreg_gp2);
	local_irq_enable();
}

static struct panel_data_ext tmd_wvga_panel_ext = {
	.power_on = tmd_wvga_lcd_power_on,
	.power_off = tmd_wvga_lcd_power_off,
};

static struct msm_fb_panel_data tmd_wvga_panel_data;

static struct platform_device mddi_tmd_wvga_display_device = {
	.name = "mddi_tmd_wvga",
	.id = -1,
};

static void __init msm_mddi_tmd_fwvga_display_device_init(void)
{
	struct msm_fb_panel_data *panel_data = &tmd_wvga_panel_data;
	printk(KERN_DEBUG "%s \n", __func__);
	panel_data->panel_info.yres = 854;
        panel_data->panel_info.xres = 480;
	panel_data->panel_info.type = MDDI_PANEL;
	panel_data->panel_info.pdest = DISPLAY_1;
	panel_data->panel_info.wait_cycle = 0;
	panel_data->panel_info.bpp = 32;
	panel_data->panel_info.clk_rate = 235000000;
	panel_data->panel_info.clk_min = 192000000;
	panel_data->panel_info.clk_max = 256000000;
	panel_data->panel_info.fb_num = 3;
	panel_data->panel_info.mddi.vdopkt = MDDI_DEFAULT_PRIM_PIX_ATTR;
	panel_data->panel_info.lcd.vsync_enable = FALSE;
	panel_data->panel_info.lcd.v_back_porch = 12;
	panel_data->panel_info.lcd.v_front_porch = 2;
	panel_data->panel_info.lcd.v_pulse_width = 0;
	panel_data->panel_info.lcd.hw_vsync_mode = TRUE;
	panel_data->panel_info.lcd.vsync_notifier_period = 0;
	panel_data->panel_info.lcd.refx100 = 7468;
	panel_data->panel_info.width = 51;
	panel_data->panel_info.height = 89;
	panel_data->panel_ext = &tmd_wvga_panel_ext;
	mddi_tmd_wvga_display_device.dev.platform_data = &tmd_wvga_panel_data;

	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		printk(KERN_ERR "%s: vreg_get(gp2) err.\n", __func__);
		return;
	}
	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg_get(mmc) err.\n", __func__);
		return;
	}

	platform_device_register(&mddi_tmd_wvga_display_device);
}

static struct resource msm_audio_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 68,
		.end    = 68,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 69,
		.end    = 69,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 70,
		.end    = 70,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 71,
		.end    = 71,
		.flags  = IORESOURCE_IO,
	},
	{
		.name	= "audio_base_addr",
		.start	= 0xa0700000,
		.end	= 0xa0700000 + 4,
		.flags	= IORESOURCE_MEM,
	},
};

static unsigned audio_gpio_on[] = {
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
};

static void __init audio_gpio_init(void)
{
	int pin, rc;

	for (pin = 0; pin < ARRAY_SIZE(audio_gpio_on); pin++) {
		rc = gpio_tlmm_config(audio_gpio_on[pin],
			GPIO_ENABLE);
	}
	set_audio_gpios(msm_audio_resources[1].start);
}

static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 21,
		.end	= 21,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 29,
		.end	= 29,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= INT_ES209RA_GPIO_BT_HOSTWAKE,
		.end	= INT_ES209RA_GPIO_BT_HOSTWAKE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

#ifdef CONFIG_BT
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

enum {
	BT_SYSRST,
	BT_WAKE,
	BT_HOST_WAKE,
	BT_VDD_IO,
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
	BT_VDD_FREG
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(29, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	"BT_WAKE" },
	{ GPIO_CFG(21, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	"HOST_WAKE" },
	{ GPIO_CFG(77, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "BT_VDD_IO" },
	{ GPIO_CFG(157, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "UART1DM_RFR" },
	{ GPIO_CFG(141, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "UART1DM_CTS" },
	{ GPIO_CFG(139, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "UART1DM_RX" },
	{ GPIO_CFG(140, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "UART1DM_TX" }
};
#if 0
static unsigned bt_config_power_off[] = {
	GPIO_CFG(29, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* WAKE */
	GPIO_CFG(21, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* HOST_WAKE */
	GPIO_CFG(77, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* PWR_EN */
	GPIO_CFG(157, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* RFR */
	GPIO_CFG(141, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* CTS */
	GPIO_CFG(139, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Rx */
	GPIO_CFG(140, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* Tx */
};
#endif

#if 1
static int bluetooth_power(int on)
{
	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	gpio_set_value(77, on);

	return 0;
}

static void __init bt_power_init(void)
{
	int pin, rc;

	for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
		rc = gpio_tlmm_config(bt_config_power_on[pin].gpio_cfg,
				      GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR
			       "%s: gpio_tlmm_config(%#x)=%d\n",
			       __func__, bt_config_power_on[pin].gpio_cfg, rc);
			return;
		}
	}
	gpio_set_value(77, 0);

	msm_bt_power_device.dev.platform_data = &bluetooth_power;
	printk(KERN_DEBUG "Bluetooth power switch initialized\n");
}
#else
static int bluetooth_power(int on)
{
	struct vreg *vreg_bt;
	struct vreg *vreg_wlan;
	int pin, rc;

	vreg_bt = vreg_get(NULL, "gp6");

	if (IS_ERR(vreg_bt)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_bt));
		return PTR_ERR(vreg_bt);
	}

	vreg_wlan = vreg_get(NULL, "wlan");

	if (IS_ERR(vreg_wlan)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_wlan));
		return PTR_ERR(vreg_wlan);
	}

	if (on) {
		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_on); pin++) {
			rc = gpio_tlmm_config(bt_config_power_on[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_on[pin], rc);
				return -EIO;
			}
		}

		rc = vreg_set_level(vreg_bt, PMIC_VREG_GP6_LEVEL);
		if (rc) {
			printk(KERN_ERR "%s: vreg bt set level failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = vreg_enable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg bt enable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}

		rc = vreg_set_level(vreg_wlan, PMIC_VREG_WLAN_LEVEL);
		if (rc) {
			printk(KERN_ERR "%s: vreg wlan set level failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = vreg_enable(vreg_wlan);
		if (rc) {
			printk(KERN_ERR "%s: vreg wlan enable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}

		gpio_set_value(22, on);
		gpio_set_value(18, on);
	} else {
		gpio_set_value(18, on);
		gpio_set_value(22, on);

		rc = vreg_disable(vreg_wlan);
		if (rc) {
			printk(KERN_ERR "%s: vreg wlan disable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = vreg_disable(vreg_bt);
		if (rc) {
			printk(KERN_ERR "%s: vreg bt disable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}

		for (pin = 0; pin < ARRAY_SIZE(bt_config_power_off); pin++) {
			rc = gpio_tlmm_config(bt_config_power_off[pin],
					      GPIO_ENABLE);
			if (rc) {
				printk(KERN_ERR
				       "%s: gpio_tlmm_config(%#x)=%d\n",
				       __func__, bt_config_power_off[pin], rc);
				return -EIO;
			}
		}
	}

	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}

static void __init bt_power_init(void)
{
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
}
#endif
#else
#define bt_power_init(x) do {} while (0)
#endif

static struct resource kgsl_resources[] = {
       {
		.name  = "kgsl_reg_memory",
		.start = 0xA0000000,
		.end = 0xA001ffff,
		.flags = IORESOURCE_MEM,
       },
       {
		.name = "kgsl_yamato_irq",
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
       },
};

static struct kgsl_platform_data kgsl_pdata = {
	.high_axi_3d = 192000,
	.max_grp2d_freq = 0,
	.min_grp2d_freq = 0,
	.set_grp2d_async = NULL,
        .max_grp3d_freq = 0,
        .min_grp3d_freq = 0,
	.set_grp3d_async = NULL,
	.imem_clk_name = "imem_clk",
	.grp3d_clk_name = "grp_clk",
	.grp2d0_clk_name = NULL,
	.idle_timeout_3d = HZ/5,
	.idle_timeout_2d = 0,
	.pt_va_size = SZ_128M,
	.pt_max_count = 1,
};

static struct platform_device msm_device_kgsl = {
       .name = "kgsl",
       .id = -1,
       .num_resources = ARRAY_SIZE(kgsl_resources),
       .resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};

struct es209ra_headset_platform_data es209ra_headset_data = {
	.keypad_name = "es209ra_keypad",
	.gpio_detout = 114,
	.gpio_detin = 132,
	.wait_time = 700,
};

static int es209ra_reset_keys_up[] = {
  KEY_VOLUMEDOWN,
  0
};

static struct keyreset_platform_data es209ra_reset_keys_pdata = {
  .keys_up = es209ra_reset_keys_up,
  .keys_down = {
    KEY_VOLUMEUP,
    KEY_HOME,
    0
  },
  .down_time_ms = 2700,
};

struct platform_device es209ra_reset_keys_device = {
  .name = KEYRESET_NAME,
  .dev  = {
    .platform_data = &es209ra_reset_keys_pdata,
  },
};

static struct platform_device es209ra_audio_jack_device = {
	.name		= "es209ra_audio_jack",
	.dev = {
        .platform_data = &es209ra_headset_data,
    },
};


static int qsd8x50_tps65023_set_dcdc1(int mVolts)
{
	int rc = 0;
	rc = tps65023_set_dcdc1_level(mVolts);

	if (rc == -ENODEV && mVolts <= CONFIG_QSD_PMIC_DEFAULT_DCDC1)
		rc = 0;
	return rc;
}

static struct msm_acpu_clock_platform_data qsd8x50_clock_data = {
	.acpu_switch_time_us = 20,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.max_vdd = TPS65023_MAX_DCDC1,
	.acpu_set_vdd = qsd8x50_tps65023_set_dcdc1,
};

static struct max17040_i2c_platform_data max17040_platform_data = {
	.data = &max17040_dev_data
};

static int ak8973_gpio_config(int enable)
{
	if (enable) {
		if (gpio_request(AKM8973_GPIO_RESET_PIN, "akm8973_xres")) {
			printk(KERN_ERR "%s: gpio_req xres"
				" - Fail!", __func__);
			return -EIO;
		}
		if (gpio_tlmm_config(GPIO_CFG(AKM8973_GPIO_RESET_PIN, 0,
			GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),GPIO_ENABLE)) {
			printk(KERN_ERR "%s: gpio_tlmm_conf xres"
				" - Fail!", __func__);
			goto ak8973_gpio_fail_0;
		}
		gpio_set_value(AKM8973_GPIO_RESET_PIN, 1);

	} else {
		gpio_free(AKM8973_GPIO_RESET_PIN);
	}
	return 0;

ak8973_gpio_fail_0:
	gpio_free(AKM8973_GPIO_RESET_PIN);
	return -EIO;
}

static int ak8973_xres(void)
{
	gpio_set_value(AKM8973_GPIO_RESET_PIN, 0);
	msleep(9);
	gpio_set_value(AKM8973_GPIO_RESET_PIN, 1);
	msleep(18);
	return 0;
}

static struct akm8973_i2c_platform_data akm8973_platform_data = {
	.gpio_config = ak8973_gpio_config,
	.xres = ak8973_xres
};

static int bma150_gpio_setup(bool request)
{
	if (request)
		return gpio_request(ES209RA_GPIO_ACCEL, "bma150_irq");
	else
		gpio_free(ES209RA_GPIO_ACCEL);
	return 0;
}

struct bma150_platform_data bma150_ng_platform_data = {
	.gpio_setup = &bma150_gpio_setup,
};

static struct i2c_board_info msm_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("tps65023", 0x48),
	},
	{
		I2C_BOARD_INFO("max17040_fuel_gauge", 0x36),
		.platform_data = &max17040_platform_data,
	},
	{
		I2C_BOARD_INFO("lv5219lg", 0x74),
	},
	{
		I2C_BOARD_INFO("akm8973", 0x1C),
		.platform_data = &akm8973_platform_data,
	},
	{
		I2C_BOARD_INFO("bma150", 0x38), 
		.irq		   =  INT_ES209RA_GPIO_ACCEL,
		.platform_data = &bma150_ng_platform_data,
	},
	{
		I2C_BOARD_INFO("semc_imx046_camera", 0x1F),
		.irq		   =  INT_ES209RA_GPIO_CAM_ISP,
	},
};

static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(0,  0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(1,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(27, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(43, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(142,0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(0,  0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(1,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(12, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(27, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(43, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
	GPIO_CFG(142, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static void config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

static struct resource msm_camera_resources[] = {
	{
		.start	= 0xA0F00000,
		.end	= 0xA0F00000 + SZ_2M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};

static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
	._fsrc.pmic_src.low_current  = 20,
	._fsrc.pmic_src.high_current = 90,
};

static struct msm_camera_sensor_flash_data flash_semc_imx046_camera = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_semc_imx046_camera_data = {
	.sensor_name    = "semc_imx046_camera",
	.sensor_reset   = 0,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_semc_imx046_camera,
	.mclk           = 15,
	.sensor_int     = INT_ES209RA_GPIO_CAM_ISP,
	.sensor_vsync   = INT_ES209RA_GPIO_CAM_VSYNC,
	.vcam_l1        = {.type = MSM_CAMERA_SENSOR_PWR_GPIO, .resource.number = 27,       },
	.vcam_l2        = {.type = MSM_CAMERA_SENSOR_PWR_GPIO, .resource.number = 43,       },
	.vcam_sd        = {.type = MSM_CAMERA_SENSOR_PWR_GPIO, .resource.number = 142,      },
	.vcam_io        = {.type = MSM_CAMERA_SENSOR_PWR_VREG, .resource.name   = "rfrx2",  },
	.vcam_af        = {.type = MSM_CAMERA_SENSOR_PWR_VREG, .resource.name   = "rftx",   },
	.vcam_sa        = {.type = MSM_CAMERA_SENSOR_PWR_VREG, .resource.name   = "gp1",    },
};

static struct platform_device msm_camera_sensor_semc_imx046_camera = {
	.name      = "msm_camera_semc_imx046_camera",
	.dev       = {
		.platform_data = &msm_camera_sensor_semc_imx046_camera_data,
	},
};

static struct platform_device msm_wlan_ar6000_pm_device = {
        .name           = "wlan_ar6000_pm_dev",
        .id             = 1,
        .num_resources  = 0,
        .resource       = NULL,
};

static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static struct msm_otg_platform_data msm_otg_pdata = {
	.rpc_connect	= hsusb_rpc_connect,
	.phy_reset	= msm_hsusb_native_phy_reset,
	.pmic_notif_init         = msm_pm_app_rpc_init,
	.pmic_notif_deinit       = msm_pm_app_rpc_deinit,
	.pmic_register_vbus_sn   = msm_pm_app_register_vbus_sn,
	.pmic_unregister_vbus_sn = msm_pm_app_unregister_vbus_sn,
	.pmic_enable_ldo         = msm_pm_app_enable_usb_ldo,
};

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;

static struct lbs_platform_data lbs_data = {
	.threshold_vol = 3400,
};

static struct platform_device lbs_device = {
	.name	= "Low-Battery Shutdown",
	.id	= -1,
	.dev	= {
		.platform_data = &lbs_data,
	},
};

static struct platform_device pmic_time_device = {
	.name = "pmic_time",
};

static struct platform_device *devices[] __initdata = {
	&msm_wlan_ar6000_pm_device,
	&msm_fb_device,
	&msm_device_smd,
	&msm_device_dmov,
	&android_pmem_kernel_ebi1_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_smipool_device,
	&msm_device_nand,
	&msm_device_i2c,
	&qsd_device_spi,
	&rndis_device,
	&usb_mass_storage_device,
	&android_usb_device,
	&msm_device_tssc,
	&msm_device_uart1,
	&msm_bluesleep_device,
	&msm_bt_power_device,
	&msm_device_uart_dm2,
	&msm_device_kgsl,
	&hs_device,
	&msm_camera_sensor_semc_imx046_camera,
	&vibrator_device,
	&es209ra_audio_jack_device,
	&lbs_device,
	&pmic_time_device,
};

static void __init es209ra_init_irq(void)
{
	msm_init_irq();
	msm_init_sirc();
}

static void __init es209ra_init_usb(void)
{
	hs_clk = clk_get(NULL, "usb_hs_clk");
	if (IS_ERR(hs_clk)) {
		printk(KERN_ERR "%s: hs_clk clk get failed\n", __func__);
		return;
	}

	phy_clk = clk_get(NULL, "usb_phy_clk");
	if (IS_ERR(phy_clk)) {
		printk(KERN_ERR "%s: phy_clk clk get failed\n", __func__);
		return;
	}

#ifdef CONFIG_USB_MSM_OTG_72K
	platform_device_register(&msm_device_otg);
#endif

#ifdef CONFIG_USB_FUNCTION_MSM_HSUSB
	platform_device_register(&msm_device_hsusb_peripheral);
#endif

#ifdef CONFIG_USB_MSM_72K
	platform_device_register(&msm_device_gadget_peripheral);
#endif
}

static void sdcc_gpio_init(void)
{
	if (gpio_request(23, "sdc1_status_irq"))
		pr_err("failed to request gpio sdc1_status_irq\n");
	if (gpio_request(51, "sdc1_data_3"))
		pr_err("failed to request gpio sdc1_data_3\n");
	if (gpio_request(52, "sdc1_data_2"))
		pr_err("failed to request gpio sdc1_data_2\n");
	if (gpio_request(53, "sdc1_data_1"))
		pr_err("failed to request gpio sdc1_data_1\n");
	if (gpio_request(54, "sdc1_data_0"))
		pr_err("failed to request gpio sdc1_data_0\n");
	if (gpio_request(55, "sdc1_cmd"))
		pr_err("failed to request gpio sdc1_cmd\n");
	if (gpio_request(56, "sdc1_clk"))
		pr_err("failed to request gpio sdc1_clk\n");
	if (gpio_request(62, "sdc2_clk"))
		pr_err("failed to request gpio sdc2_clk\n");
	if (gpio_request(63, "sdc2_cmd"))
		pr_err("failed to request gpio sdc2_cmd\n");
	if (gpio_request(64, "sdc2_data_3"))
		pr_err("failed to request gpio sdc2_data_3\n");
	if (gpio_request(65, "sdc2_data_2"))
		pr_err("failed to request gpio sdc2_data_2\n");
	if (gpio_request(66, "sdc2_data_1"))
		pr_err("failed to request gpio sdc2_data_1\n");
	if (gpio_request(67, "sdc2_data_0"))
		pr_err("failed to request gpio sdc2_data_0\n");
}

static unsigned sdcc_cfg_data[][6] = {
	{
	GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	},
	{
	GPIO_CFG(62, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA),
	GPIO_CFG(63, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(64, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(65, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(66, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	GPIO_CFG(67, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA),
	}
};

static unsigned long vreg_sts, gpio_sts;
static struct vreg *vreg_mmc;

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int i, rc;

	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable)
		set_bit(dev_id, &gpio_sts);
	else
		clear_bit(dev_id, &gpio_sts);

	for (i = 0; i < ARRAY_SIZE(sdcc_cfg_data[dev_id - 1]); i++) {
		rc = gpio_tlmm_config(sdcc_cfg_data[dev_id - 1][i],
			enable ? GPIO_ENABLE : GPIO_DISABLE);
		if (rc)
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, sdcc_cfg_data[dev_id - 1][i], rc);
	}
}

static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
		}
		return 0;
	}

	if (!vreg_sts) {
		rc = vreg_set_level(vreg_mmc, PMIC_VREG_GP6_LEVEL);

		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}

static unsigned int es209ra_sdcc_slot_status(struct device *dev)
{
	unsigned int ret=0;
	if(gpio_get_value(23))
		ret = 0;
	else
		ret = 1;
       return ret;
}

static struct mmc_platform_data es209ra_sdcc_data1 = {
	.ocr_mask	    = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.status		    = es209ra_sdcc_slot_status,
	.status_irq	    = INT_ES209RA_GPIO_CARD_INS_N,
	.irq_flags      = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
};

static struct mmc_platform_data es209ra_sdcc_data2 = {
	.ocr_mask	= MMC_VDD_23_24 | MMC_VDD_25_26,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
};

static void __init es209ra_init_mmc(void)
{
	vreg_mmc = vreg_get(NULL, "gp6");
	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_mmc));
		return;
	}

	sdcc_gpio_init();
	msm_add_sdcc(1, &es209ra_sdcc_data1);
	msm_add_sdcc(2, &es209ra_sdcc_data2);
}

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 8594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 23740,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 4594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 23740,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 0,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 443,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 1098,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency = 2,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].residency = 0,
};

static void msm_i2c_gpio_config(int iface, int config_type)
{
	int gpio_scl;
	int gpio_sda;

	switch (iface)
	{
		case 0:
			gpio_scl = 95;
			gpio_sda = 96;
			break;
		case 1: 
		default:
			return;
	}

	if (config_type)
	{
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	}
	else
	{
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 384000,
	.rsl_id = SMEM_SPINLOCK_I2C,
	.pri_clk = 95,
	.pri_dat = 96,
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

static void __init msm_device_i2c_init(void)
{
	if (gpio_request(95, "i2c_pri_clk"))
		pr_err("failed to request gpio i2c_pri_clk\n");
	if (gpio_request(96, "i2c_pri_dat"))
		pr_err("failed to request gpio i2c_pri_dat\n");

	msm_i2c_pdata.rmutex = (uint32_t)smem_alloc(SMEM_I2C_MUTEX, 8);
	msm_i2c_pdata.pm_lat =
		msm_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;
	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static void __init pmem_mdp_size_setup(char **p)
{
	pmem_mdp_size = memparse(*p, p);
}
__early_param("pmem_mdp_size=", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);

unsigned int es209ra_startup_reason = 0;

static int __init es209ra_startup_reason_setup(char *str)
{
	es209ra_startup_reason = simple_strtoul(str, NULL, 16);
	return 1;
}
__setup_param("startup=", es209ra_startup_reason_setup_1, es209ra_startup_reason_setup, 0);

static int es209ra_hw_version = 0;
static int __init es209ra_hw_version_setup(char *str)
{
	es209ra_hw_version = simple_strtoul(str, NULL, 0);

	return 1;
}
__setup_param("hwversion=", es209ra_hw_version_setup_1, es209ra_hw_version_setup, 0);
__setup_param("semcandroidboot.hwversion=", es209ra_hw_version_setup_2, es209ra_hw_version_setup, 0);
__setup_param("semcandroidboot.startup=", es209ra_startup_reason_setup_2, es209ra_startup_reason_setup, 0);
int get_predecode_repair_cache(void);
int set_predecode_repair_cache(void);
static void __init es209ra_init(void)
{
	smsm_wait_for_modem();
	printk(KERN_INFO "%s: startup_reason: 0x%08x\n",
					__func__, es209ra_startup_reason);
	printk(KERN_ERR "PVR0F2 (1): %x\n", get_predecode_repair_cache());
	set_predecode_repair_cache();
	printk(KERN_ERR "PVR0F2 (2): %x\n", get_predecode_repair_cache());
	msm_acpu_clock_init(&qsd8x50_clock_data);
	msm_hsusb_pdata.swfi_latency = msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;
	platform_add_devices(devices, ARRAY_SIZE(devices));
	platform_device_register(&es209ra_reset_keys_device);
	msm_fb_add_devices();
#ifdef CONFIG_MSM_CAMERA
	config_camera_off_gpios();
#endif
	es209ra_init_usb();
	es209ra_init_mmc();
	bt_power_init();
	audio_gpio_init();
	msm_device_i2c_init();
	msm_qsd_spi_init();
	i2c_register_board_info(0, msm_i2c_board_info,
				ARRAY_SIZE(msm_i2c_board_info));
	spi_register_board_info(msm_spi_board_info,
				ARRAY_SIZE(msm_spi_board_info));
	msm_pm_set_platform_data(msm_pm_data);
	platform_device_register(&es209ra_keypad_device);
	msm_mddi_tmd_fwvga_display_device_init();
}

static void __init es209ra_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_mdp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for mdp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = MSM_FB_SIZE;
	addr = (void *)MSM_FB_BASE;
	msm_fb_resources[0].start = (unsigned long)addr;
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("using %lu bytes of SMI at %lx physical for fb\n",
	       size, (unsigned long)addr);
}

static void __init es209ra_fixup(struct machine_desc *desc, struct tag *tags,
                               char **cmdline, struct meminfo *mi)
{
	mi->nr_banks=2;
	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = (232*1024*1024);
	mi->bank[1].start = 0x30000000;
	mi->bank[1].size = (127*1024*1024);
}

static void __init es209ra_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_qsd8x50_io();
	es209ra_allocate_memory_regions();
	msm_clock_init(msm_clocks_8x50, msm_num_clocks_8x50);
}


MACHINE_START(ES209RA, "ES209RA")
	.boot_params	= PHYS_OFFSET + 0x100,
	.fixup          = es209ra_fixup,
	.map_io		= es209ra_map_io,
	.init_irq	= es209ra_init_irq,
	.init_machine	= es209ra_init,
	.timer = &msm_timer,
MACHINE_END
