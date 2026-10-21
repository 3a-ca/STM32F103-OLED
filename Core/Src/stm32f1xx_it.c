/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务函数
  ******************************************************************************
  */
#include "main.h"
#include "stm32f1xx_it.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

void NMI_Handler(void)
{
  while (1) {}
}

void HardFault_Handler(void)
{
  while (1) {}
}

void MemManage_Handler(void)
{
  while (1) {}
}

void BusFault_Handler(void)
{
  while (1) {}
}

void UsageFault_Handler(void)
{
  while (1) {}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

void I2C1_EV_IRQHandler(void)
{
  HAL_I2C_EV_IRQHandler(&hi2c1);
}

void I2C1_ER_IRQHandler(void)
{
  HAL_I2C_ER_IRQHandler(&hi2c1);
}

void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
