ccflags-y := -Iinclude/drm
CFLAGS_REMOVE_drm_edid.o += -Werror
drm-y       :=	drm_auth.o drm_bufs.o drm_cache.o \
		drm_context.o drm_dma.o drm_drawable.o \
		drm_drv.o drm_fops.o drm_gem.o drm_ioctl.o drm_irq.o \
		drm_lock.o drm_memory.o drm_proc.o drm_stub.o drm_vm.o \
		drm_agpsupport.o drm_scatter.o ati_pcigart.o drm_pci.o \
		drm_platform.o drm_sysfs.o drm_hashtab.o drm_sman.o drm_mm.o \
		drm_crtc.o drm_modes.o drm_edid.o \
		drm_info.o drm_debugfs.o drm_encoder_slave.o

drm-$(CONFIG_COMPAT) += drm_ioc32.o
drm_kms_helper-y := drm_fb_helper.o drm_crtc_helper.o
obj-$(CONFIG_DRM_KMS_HELPER) += drm_kms_helper.o
obj-$(CONFIG_DRM)	+= drm.o
obj-$(CONFIG_DRM_TTM)	+= ttm/

