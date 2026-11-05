/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
#define Triang_Pin GPIO_PIN_13
#define Triang_GPIO_Port GPIOC
#define Triang_EXTI_IRQn EXTI15_10_IRQn
#define Square_Pin GPIO_PIN_14
#define Square_GPIO_Port GPIOC
#define Square_EXTI_IRQn EXTI15_10_IRQn
#define XX_Pin GPIO_PIN_0
#define XX_GPIO_Port GPIOA
#define XX_EXTI_IRQn EXTI0_IRQn
#define Circle_Pin GPIO_PIN_1
#define Circle_GPIO_Port GPIOA
#define Circle_EXTI_IRQn EXTI1_IRQn
#define Joy_Der_Pin GPIO_PIN_2
#define Joy_Der_GPIO_Port GPIOA
#define Joy_Der_EXTI_IRQn EXTI2_IRQn
#define Start_Pin GPIO_PIN_5
#define Start_GPIO_Port GPIOA
#define Start_EXTI_IRQn EXTI9_5_IRQn
#define Analog_ON_Pin GPIO_PIN_6
#define Analog_ON_GPIO_Port GPIOA
#define Analog_ON_EXTI_IRQn EXTI9_5_IRQn
#define Joy_Izq_Pin GPIO_PIN_7
#define Joy_Izq_GPIO_Port GPIOA
#define Joy_Izq_EXTI_IRQn EXTI9_5_IRQn
#define Select_Pin GPIO_PIN_10
#define Select_GPIO_Port GPIOB
#define Der_Pin GPIO_PIN_11
#define Der_GPIO_Port GPIOB
#define Der_EXTI_IRQn EXTI15_10_IRQn
#define Ab_Pin GPIO_PIN_12
#define Ab_GPIO_Port GPIOB
#define Ab_EXTI_IRQn EXTI15_10_IRQn
#define Latch_Out_Pin GPIO_PIN_13
#define Latch_Out_GPIO_Port GPIOB
#define Izq_Pin GPIO_PIN_15
#define Izq_GPIO_Port GPIOB
#define Izq_EXTI_IRQn EXTI15_10_IRQn
#define Ar_Pin GPIO_PIN_10
#define Ar_GPIO_Port GPIOA
#define Ar_EXTI_IRQn EXTI15_10_IRQn
#define LED_Pin GPIO_PIN_15
#define LED_GPIO_Port GPIOA
#define L2_Pin GPIO_PIN_3
#define L2_GPIO_Port GPIOB
#define L2_EXTI_IRQn EXTI3_IRQn
#define L1_Pin GPIO_PIN_4
#define L1_GPIO_Port GPIOB
#define L1_EXTI_IRQn EXTI4_IRQn
#define V_USB_Sensing_Pin GPIO_PIN_5
#define V_USB_Sensing_GPIO_Port GPIOB
#define R1_Pin GPIO_PIN_8
#define R1_GPIO_Port GPIOB
#define R1_EXTI_IRQn EXTI9_5_IRQn
#define R2_Pin GPIO_PIN_9
#define R2_GPIO_Port GPIOB
#define R2_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
