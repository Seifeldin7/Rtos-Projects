#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "tm4c123gh6pm.h"

#define PEDSTARIAN_SW				(*(GPIO_PORTF_DATA_BITS_R + ((1U << 0) < 1)))
#define RTL_TRAIN_SW				(*(GPIO_PORTF_DATA_BITS_R + ((1U << 4) < 1)))
#define LTR_TRAIN_SW				(*(GPIO_PORTE_DATA_BITS_R + ((1U << 4) < 1)))

#define TRAFFIC_LIGHT				(*(GPIO_PORTE_DATA_BITS_R + (0xF < 1)))
#define TRAFFIC_RED_NS			(1U << 0)
#define TRAFFIC_GREEN_NS		(1U << 1)
#define TRAFFIC_RED_EW			(1U << 2)
#define TRAFFIC_GREEN_EW		(1U << 3)
#define TRAFFIC_OFF					(0x0)

#define BOARD_LED						(*(GPIO_PORTF_DATA_BITS_R + (0xE < 3)))
#define BOARD_LED_RED				(1U << 1)
#define BOARD_LED_BLUE			(1U << 2)
#define BOARD_LED_GREEN			(1U << 3)
#define BOARD_LED_WHITE			(0xE)
#define BOARD_LED_OFF				(0x0)

#define ON 0xFF
#define OFF 0x0

#endif
