/**
  ******************************************************************************
  * @file    app.h
  * @brief   界面调度接口
  ******************************************************************************
  */
#ifndef __APP_H
#define __APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define APP_SCREEN_COUNT  6

void app_Init(void);
void app_Task(void);
void app_Next(void);
void app_Select(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* __APP_H */
