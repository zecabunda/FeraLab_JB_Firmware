

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#ifdef CONFIG_ARCH_MSM_ARM11
#include "msm-pcm.h"
#else
#include "qsd-pcm.h"
#endif

struct snd_soc_dai msm_dais[] = {
{
	.name = "CODEC_DAI",
	.playback = {
		.stream_name = "Playback",
		.channels_max = USE_CHANNELS_MAX,
		.rates = USE_RATE,
		.rate_min = USE_RATE_MIN,
		.rate_max = USE_RATE_MAX,
		.formats = USE_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_max = USE_CHANNELS_MAX,
		.rate_min = USE_RATE_MIN,
		.rates = USE_RATE,
		.formats = USE_FORMATS,
	},
},
{
	.name = "CPU_DAI",
	.id = 0,
	.playback = {
		.channels_min = USE_CHANNELS_MIN,
		.channels_max = USE_CHANNELS_MAX,
		.rates = USE_RATE,
		.rate_min = USE_RATE_MIN,
		.rate_max = USE_RATE_MAX,
		.formats = USE_FORMATS,
	},
	.capture = {
		.channels_min = USE_CHANNELS_MIN,
		.channels_max = USE_CHANNELS_MAX,
		.rate_min = USE_RATE_MIN,
		.rates = USE_RATE,
		.formats = USE_FORMATS,
	},
},
};
EXPORT_SYMBOL_GPL(msm_dais);

int msm_pcm_probe(struct platform_device *devptr)
{
	struct snd_soc_codec *codec;
	int ret;

	struct snd_soc_device *socdev = platform_get_drvdata(devptr);

	printk(KERN_ERR "msm_soc: create pcms\n");
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	codec->name = "MSM-CARD";
	codec->owner = THIS_MODULE;
	socdev->card->codec = codec;
	mutex_init(&codec->mutex);

	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "msm_soc: failed to create pcms\n");
		goto __nopcm;
	}

	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "msm_soc: failed to register card\n");
		goto __nodev;
	}

	return 0;

__nodev:
	snd_soc_free_pcms(socdev);
__nopcm:
	kfree(codec);
	return ret;
}

struct snd_soc_codec_device soc_codec_dev_msm = {
	.probe          = msm_pcm_probe,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_msm);


static int __init msm_dai_init(void)
{
	return snd_soc_register_dais(msm_dais, ARRAY_SIZE(msm_dais));
}

static void __exit msm_dai_exit(void)
{
	snd_soc_unregister_dais(msm_dais, ARRAY_SIZE(msm_dais));
}

module_init(msm_dai_init);
module_exit(msm_dai_exit);


MODULE_DESCRIPTION("MSM Codec/Cpu Dai driver");
MODULE_LICENSE("GPL v2");