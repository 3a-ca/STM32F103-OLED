/**
  ******************************************************************************
  * @file    aht20.c
  * @brief   AHT20 温湿度传感器驱动实现
  *
  *  时序：上电 40ms -> 读状态字（bit3 = 已校准）
  *        未校准则发 0xBE 0x08 0x00 校准，等 10ms
  *        发 0xAC 0x33 0x00 触发测量，等 80ms
  *        读 7 字节：状态 + 湿度 20bit + 温度 20bit + CRC
  ******************************************************************************
  */
#include "aht20.h"

extern I2C_HandleTypeDef hi2c1;

uint8_t aht20_Init(void)
{
  uint8_t status = 0xFF;

  HAL_Delay(50);
  if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, &status, 1, 50) != HAL_OK)
  {
    return 0;                      /* 没有传感器，直接返回失败，不影响主程序 */
  }

  if ((status & 0x08) == 0)        /* bit3 = 0 表示未校准 */
  {
    uint8_t cmd[3] = {0xBE, 0x08, 0x00};
    HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, cmd, 3, 50);
    HAL_Delay(10);
  }
  return 1;
}

uint8_t aht20_Read(float *temperature, float *humidity)
{
  uint8_t cmd[3] = {0xAC, 0x33, 0x00};
  uint8_t buf[7] = {0};
  uint32_t raw_h, raw_t;

  if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, cmd, 3, 50) != HAL_OK)
  {
    return 0;
  }
  HAL_Delay(80);

  if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, buf, 7, 50) != HAL_OK)
  {
    return 0;
  }
  if (buf[0] & 0x80)               /* bit7 = 1 表示忙，数据无效 */
  {
    return 0;
  }

  raw_h = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
  raw_t = ((uint32_t)(buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

  *humidity    = (float)raw_h * 100.0f / 1048576.0f;              /* 2^20 */
  *temperature = (float)raw_t * 200.0f / 1048576.0f - 50.0f;
  return 1;
}
