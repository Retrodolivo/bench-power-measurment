/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mercury.h"
#include "sd_card.h"
#include "socket.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "tcp.h"
#include "rtc.h"

#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OSC32_IN_Pin GPIO_PIN_14
#define OSC32_IN_GPIO_Port GPIOC
#define OSC32_OUT_Pin GPIO_PIN_15
#define OSC32_OUT_GPIO_Port GPIOC
#define OSC_IN_Pin GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOF
#define OSC_OUT_Pin GPIO_PIN_1
#define OSC_OUT_GPIO_Port GPIOF
#define B1_Pin GPIO_PIN_0
#define B1_GPIO_Port GPIOA
#define MERC_DE_Pin GPIO_PIN_1
#define MERC_DE_GPIO_Port GPIOA
#define MERC_TX_Pin GPIO_PIN_2
#define MERC_TX_GPIO_Port GPIOA
#define MERC_RX_Pin GPIO_PIN_3
#define MERC_RX_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_PIN_4
#define SD_CS_GPIO_Port GPIOA
#define SD_SCK_Pin GPIO_PIN_5
#define SD_SCK_GPIO_Port GPIOA
#define SD_MISO_Pin GPIO_PIN_6
#define SD_MISO_GPIO_Port GPIOA
#define SD_MOSI_Pin GPIO_PIN_7
#define SD_MOSI_GPIO_Port GPIOA
#define STATE_RED_Pin GPIO_PIN_9
#define STATE_RED_GPIO_Port GPIOE
#define STATE_RED_EXTI_IRQn EXTI9_5_IRQn
#define STATE_YELLOW_Pin GPIO_PIN_10
#define STATE_YELLOW_GPIO_Port GPIOE
#define STATE_YELLOW_EXTI_IRQn EXTI15_10_IRQn
#define STATE_GREEN_Pin GPIO_PIN_11
#define STATE_GREEN_GPIO_Port GPIOE
#define STATE_GREEN_EXTI_IRQn EXTI15_10_IRQn
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define W5500_CS_Pin GPIO_PIN_15
#define W5500_CS_GPIO_Port GPIOA
#define W5500_SCK_Pin GPIO_PIN_10
#define W5500_SCK_GPIO_Port GPIOC
#define W5500_MISO_Pin GPIO_PIN_11
#define W5500_MISO_GPIO_Port GPIOC
#define W5500_MOSI_Pin GPIO_PIN_12
#define W5500_MOSI_GPIO_Port GPIOC
#define W5500_RST_Pin GPIO_PIN_0
#define W5500_RST_GPIO_Port GPIOD
#define W5500_INT_Pin GPIO_PIN_1
#define W5500_INT_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define W5500_SPI_PORT	&hspi3
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
