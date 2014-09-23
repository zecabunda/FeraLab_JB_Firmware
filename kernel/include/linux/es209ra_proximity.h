#ifndef __ES209RA_PROXIMITY_H__
#define __ES209RA_PROXIMITY_H__
#define PROXIMITY_IOC_MAGIC			'A'
#define PROXIMITY_SET_POWER_STATE		_IOW(PROXIMITY_IOC_MAGIC, 0x00, unsigned char)
#define PROXIMITY_GET_POWER_STATE		_IOR(PROXIMITY_IOC_MAGIC, 0x01, unsigned char)
#define PROXIMITY_SET_DEVICE_MODE		_IOW(PROXIMITY_IOC_MAGIC, 0x02, unsigned char)
#define PROXIMITY_GET_DEVICE_MODE		_IOR(PROXIMITY_IOC_MAGIC, 0x03, unsigned char)
#define PROXIMITY_SET_LED_MODE			_IOW(PROXIMITY_IOC_MAGIC, 0x04, unsigned char)
#define PROXIMITY_GET_LED_MODE			_IOR(PROXIMITY_IOC_MAGIC, 0x05, unsigned char)
#define PROXIMITY_GET_DETECTION_STATE		_IOR(PROXIMITY_IOC_MAGIC, 0x06, unsigned char)
#define PROXIMITY_SET_BURST_ON_TIME		_IOW(PROXIMITY_IOC_MAGIC, 0x07, unsigned long)
#define PROXIMITY_GET_BURST_ON_TIME		_IOR(PROXIMITY_IOC_MAGIC, 0x08, unsigned long)
#define PROXIMITY_DO_SENSING			_IOW(PROXIMITY_IOC_MAGIC, 0x09, unsigned char)
#define PROXIMITY_SET_LOCK			_IOW(PROXIMITY_IOC_MAGIC, 0x0A, unsigned char)
#define PROXIMITY_SET_UNLOCK			_IOW(PROXIMITY_IOC_MAGIC, 0x0B, unsigned char)
#define PROXIMITY_IOC_MAXNR			11
#define PROXIMITY_GPIO_POWER_PIN		146
#define PROXIMITY_GPIO_ENBAR_PIN		109
#define PROXIMITY_GPIO_LEDON_PIN		22
#define PROXIMITY_GPIO_DOUT_PIN			108
#define PROXIMITY_GPIO_POWER_ON			1			/* high active */
#define PROXIMITY_GPIO_POWER_OFF		0
#define PROXIMITY_GPIO_ENBAR_ENABLE		0			/* low active */
#define PROXIMITY_GPIO_ENBAR_DISABLE		1
#define PROXIMITY_GPIO_LEDON_ENABLE		1			/* high active */
#define PROXIMITY_GPIO_LEDON_DISABLE		0
#define PROXIMITY_GPIO_DOUT_ON			0			/* low active */
#define PROXIMITY_GPIO_DOUT_OFF			1
#define PROXIMITY_BURST_ON_TIME_MAX		3000		/* 6.0ms */
#define PROXIMITY_BURST_ON_TIME_MIN		500			/* 0.5ms */
#define PROXIMITY_BURST_ON_TIME_DEFAULT		1800		/* 1.8ms */
#define PROXIMITY_BURST_OFF_TIME_DEFAULT	900000		/* 900ms */
#endif /* __ES209RA_PROXIMITY_H__ */
