/**
  ******************************************************************************
  * @file    main.h
  * @brief   全局句柄与公共声明
  ******************************************************************************
  */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

void Error_Handler(void);

/* 板载 LED：PC13，低电平点亮 */
#define LED_PIN        GPIO_PIN_13
#define LED_PORT       GPIOC
#define LED_ON()       HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()      HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE()   HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
