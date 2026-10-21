/**
  ******************************************************************************
  * @file    oled.h
  * @brief   SSD1306 128x64 OLED 驱动（I2C1: PB6=SCL, PB7=SDA）
  *
  *          提供一帧显存（1024 字节）与常用绘图接口，接口风格参照 u8g2，
  *          便于在课程作业中替换为官方 u8g2 库。
  ******************************************************************************
  */
#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_I2C_ADDR   0x78   /* 7 位地址 0x3C，左移一位后为 0x78 */

typedef struct
{
  uint8_t buf[OLED_WIDTH * OLED_HEIGHT / 8];  /* 显存，页式排列 */
  uint8_t color;                              /* 0=清除 1=置位 2=异或 */
} oled_t;

extern oled_t oled;

/* 初始化与刷新 */
void oled_Init(void);
void oled_Clear(void);
void oled_Refresh(void);
void oled_SetColor(uint8_t color);
void oled_SetContrast(uint8_t contrast);
void oled_SetPower(uint8_t on);
uint8_t oled_IsPresent(void);

/* 基本图元 */
void oled_DrawPixel(int16_t x, int16_t y);
void oled_DrawHLine(int16_t x, int16_t y, int16_t w);
void oled_DrawVLine(int16_t x, int16_t y, int16_t h);
void oled_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void oled_DrawFrame(int16_t x, int16_t y, int16_t w, int16_t h);
void oled_DrawBox(int16_t x, int16_t y, int16_t w, int16_t h);
void oled_DrawRFrame(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r);
void oled_DrawCircle(int16_t x, int16_t y, uint8_t r);
void oled_DrawDisc(int16_t x, int16_t y, uint8_t r);

/* 位图（页式，同 u8g2_DrawXBMP） */
void oled_DrawBitmap(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bitmap);

/* 文本：ASCII 用 8x16，汉字用 16x16 */
int16_t  oled_DrawStr(int16_t x, int16_t y, const char *str);
int16_t  oled_DrawUTF8(int16_t x, int16_t y, const char *str);
uint16_t oled_GetStrWidth(const char *str);
uint16_t oled_GetUTF8Width(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H */
