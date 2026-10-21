/**
  ******************************************************************************
  * @file    aht20.h
  * @brief   AHT20 温湿度传感器驱动（I2C，地址 0x38）
  ******************************************************************************
  */
#ifndef __AHT20_H
#define __AHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define AHT20_ADDR  0x70   /* 7 位地址 0x38 左移一位 */

uint8_t aht20_Init(void);
uint8_t aht20_Read(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif /* __AHT20_H */
