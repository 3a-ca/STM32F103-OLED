/**
  ******************************************************************************
  * @file    oled.c
  * @brief   SSD1306 128x64 OLED 驱动实现
  *
  *  硬件：STM32F103C8T6 I2C1，PB6 = SCL，PB7 = SDA，3.3V 供电
  *  显存：128 * 64 / 8 = 1024 字节，8 页，每页 128 列，LSB 在上
  ******************************************************************************
  */
#include "oled.h"
#include "oled_fonts.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

oled_t oled;
static uint8_t s_present = 0;

/* ------------------------------ 底层 I2C 接口 ------------------------------ */
static void OLED_WriteCmd(uint8_t cmd)
{
  uint8_t buf[2] = {0x00, cmd};   /* 0x00: 后续为命令 */
  if (HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, 2, 20) == HAL_OK)
  {
    s_present = 1;
  }
}

static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
  uint8_t buf[129];
  if (len > 128) len = 128;
  buf[0] = 0x40;                  /* 0x40: 后续为显存数据 */
  memcpy(&buf[1], data, len);
  HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, len + 1, 50);
}

/* ------------------------------ 初始化 ------------------------------ */
void oled_Init(void)
{
  HAL_Delay(100);                 /* 等 OLED 上电稳定 */

  static const uint8_t init_seq[] = {
    0xAE,        /* 关显示 */
    0xD5, 0x80,  /* 时钟分频 */
    0xA8, 0x3F,  /* 复用率 1/64 */
    0xD3, 0x00,  /* 显示偏移 */
    0x40,        /* 起始行 0 */
    0x8D, 0x14,  /* 电荷泵开 */
    0x20, 0x00,  /* 水平寻址 */
    0xA1,        /* 段重映射 */
    0xC8,        /* COM 扫描方向 */
    0xDA, 0x12,  /* COM 引脚配置 */
    0x81, 0xCF,  /* 对比度 */
    0xD9, 0xF1,  /* 预充电 */
    0xDB, 0x40,  /* VCOMH */
    0xA4,        /* 显示跟随显存 */
    0xA6,        /* 正常显示 */
    0x2E,        /* 关滚动 */
    0xAF         /* 开显示 */
  };

  memset(oled.buf, 0x00, sizeof(oled.buf));
  oled.color = 1;

  for (uint16_t i = 0; i < sizeof(init_seq); i++)
  {
    OLED_WriteCmd(init_seq[i]);
    HAL_Delay(1);
  }
}

uint8_t oled_IsPresent(void) { return s_present; }

void oled_SetColor(uint8_t color) { oled.color = color; }

void oled_SetContrast(uint8_t contrast)
{
  OLED_WriteCmd(0x81);
  OLED_WriteCmd(contrast);
}

void oled_SetPower(uint8_t on) { OLED_WriteCmd(on ? 0xAF : 0xAE); }

void oled_Clear(void)
{
  memset(oled.buf, 0x00, sizeof(oled.buf));
}

/* 把显存推送到 OLED，8 页逐页发送 */
void oled_Refresh(void)
{
  for (uint8_t page = 0; page < 8; page++)
  {
    OLED_WriteCmd(0xB0 + page);      /* 页地址 */
    OLED_WriteCmd(0x00);             /* 列低 4 位 */
    OLED_WriteCmd(0x10);             /* 列高 4 位 */
    OLED_WriteData(&oled.buf[page * OLED_WIDTH], OLED_WIDTH);
  }
}

/* ------------------------------ 像素与图元 ------------------------------ */
void oled_DrawPixel(int16_t x, int16_t y)
{
  if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;

  uint16_t idx = (uint16_t)((y >> 3) * OLED_WIDTH + x);
  uint8_t mask = (uint8_t)(1u << (y & 7));

  switch (oled.color)
  {
    case 0: oled.buf[idx] &= (uint8_t)~mask; break;
    case 1: oled.buf[idx] |= mask;           break;
    default: oled.buf[idx] ^= mask;          break;
  }
}

void oled_DrawHLine(int16_t x, int16_t y, int16_t w)
{
  for (int16_t i = 0; i < w; i++) oled_DrawPixel(x + i, y);
}

void oled_DrawVLine(int16_t x, int16_t y, int16_t h)
{
  for (int16_t i = 0; i < h; i++) oled_DrawPixel(x, y + i);
}

void oled_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx - dy;

  while (1)
  {
    oled_DrawPixel(x0, y0);
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = (int16_t)(err * 2);
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 <  dx) { err += dx; y0 += sy; }
  }
}

void oled_DrawFrame(int16_t x, int16_t y, int16_t w, int16_t h)
{
  oled_DrawHLine(x, y, w);
  oled_DrawHLine(x, y + h - 1, w);
  oled_DrawVLine(x, y, h);
  oled_DrawVLine(x + w - 1, y, h);
}

void oled_DrawBox(int16_t x, int16_t y, int16_t w, int16_t h)
{
  for (int16_t i = 0; i < h; i++) oled_DrawHLine(x, y + i, w);
}

/* 圆角矩形边框，用于界面卡片 */
void oled_DrawRFrame(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r)
{
  oled_DrawHLine(x + r, y, w - 2 * r);
  oled_DrawHLine(x + r, y + h - 1, w - 2 * r);
  oled_DrawVLine(x, y + r, h - 2 * r);
  oled_DrawVLine(x + w - 1, y + r, h - 2 * r);

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t px = 0;
  int16_t py = r;

  while (px < py)
  {
    if (f >= 0) { py--; ddF_y += 2; f += ddF_y; }
    px++;
    ddF_x += 2;
    f += ddF_x;

    oled_DrawPixel(x + r - px,         y + r - py);
    oled_DrawPixel(x + w - 1 - r + px, y + r - py);
    oled_DrawPixel(x + r - px,         y + h - 1 - r + py);
    oled_DrawPixel(x + w - 1 - r + px, y + h - 1 - r + py);

    oled_DrawPixel(x + r - py,         y + r - px);
    oled_DrawPixel(x + w - 1 - r + py, y + r - px);
    oled_DrawPixel(x + r - py,         y + h - 1 - r + px);
    oled_DrawPixel(x + w - 1 - r + py, y + h - 1 - r + px);
  }
}
void oled_DrawCircle(int16_t x0, int16_t y0, uint8_t r)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  oled_DrawPixel(x0, y0 + r);
  oled_DrawPixel(x0, y0 - r);
  oled_DrawPixel(x0 + r, y0);
  oled_DrawPixel(x0 - r, y0);

  while (x < y)
  {
    if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
    x++;
    ddF_x += 2;
    f += ddF_x;
    oled_DrawPixel(x0 + x, y0 + y);
    oled_DrawPixel(x0 - x, y0 + y);
    oled_DrawPixel(x0 + x, y0 - y);
    oled_DrawPixel(x0 - x, y0 - y);
    oled_DrawPixel(x0 + y, y0 + x);
    oled_DrawPixel(x0 - y, y0 + x);
    oled_DrawPixel(x0 + y, y0 - x);
    oled_DrawPixel(x0 - y, y0 - x);
  }
}

void oled_DrawDisc(int16_t x0, int16_t y0, uint8_t r)
{
  for (int16_t y = -r; y <= r; y++)
  {
    for (int16_t x = -r; x <= r; x++)
    {
      if (x * x + y * y <= r * r) oled_DrawPixel(x0 + x, y0 + y);
    }
  }
}

/* 页式位图：与 u8g2_DrawXBMP 数据排布一致 */
void oled_DrawBitmap(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bitmap)
{
  uint16_t pages = (h + 7) / 8;
  for (uint16_t p = 0; p < pages; p++)
  {
    for (uint8_t i = 0; i < w; i++)
    {
      uint8_t bits = bitmap[p * w + i];
      for (uint8_t b = 0; b < 8; b++)
      {
        if (bits & (1u << b)) oled_DrawPixel(x + i, y + p * 8 + b);
      }
    }
  }
}

/* ------------------------------ 文本 ------------------------------ */
static const uint8_t *find_cjk(uint16_t code)
{
  for (uint16_t i = 0; i < oled_cjk_count; i++)
  {
    if (oled_cjk_table[i].code == code) return oled_cjk_table[i].bitmap;
  }
  return NULL;
}

/* 解码一个 UTF-8 字符，返回码位，*s 前移 */
static uint16_t utf8_decode(const char **s)
{
  const uint8_t *p = (const uint8_t *)*s;
  uint16_t code = '?';
  uint8_t len = 1;

  if (p[0] < 0x80)            { code = p[0]; len = 1; }
  else if ((p[0] & 0xE0) == 0xC0) { code = (uint16_t)(((p[0] & 0x1F) << 6) | (p[1] & 0x3F)); len = 2; }
  else if ((p[0] & 0xF0) == 0xE0) { code = (uint16_t)(((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F)); len = 3; }
  else                        { code = '?'; len = 1; }

  *s += len;
  return code;
}

static void draw_ascii(int16_t x, int16_t y, uint8_t ch)
{
  if (ch < 0x20 || ch > 0x7E) ch = '?';
  const uint8_t *g = &oled_ascii8x16[(ch - 0x20) * 16];
  for (uint8_t row = 0; row < 16; row++)
  {
    uint8_t bits = g[row];
    for (uint8_t col = 0; col < 8; col++)
    {
      if (bits & (0x80u >> col)) oled_DrawPixel(x + col, y + row);
    }
  }
}

static void draw_cjk(int16_t x, int16_t y, const uint8_t *g)
{
  for (uint8_t row = 0; row < 16; row++)
  {
    uint8_t hi = g[row * 2];
    uint8_t lo = g[row * 2 + 1];
    for (uint8_t b = 0; b < 8; b++)
    {
      if (hi & (0x80 >> b)) oled_DrawPixel(x + b, y + row);
      if (lo & (0x80 >> b)) oled_DrawPixel(x + 8 + b, y + row);
    }
  }
}

int16_t oled_DrawStr(int16_t x, int16_t y, const char *str)
{
  int16_t x0 = x;
  while (*str)
  {
    draw_ascii(x, y, (uint8_t)*str++);
    x += 8;
  }
  return (int16_t)(x - x0);
}

int16_t oled_DrawUTF8(int16_t x, int16_t y, const char *str)
{
  int16_t x0 = x;
  while (*str)
  {
    uint16_t code = utf8_decode(&str);
    if (code < 0x80)
    {
      draw_ascii(x, y, (uint8_t)code);
      x += 8;
    }
    else
    {
      const uint8_t *g = find_cjk(code);
      if (g)
      {
        draw_cjk(x, y, g);
      }
      else
      {
        oled_DrawFrame(x, y, 16, 16);   /* 字库中没有的字画方框 */
      }
      x += 16;
    }
  }
  return (int16_t)(x - x0);
}

uint16_t oled_GetStrWidth(const char *str)
{
  return (uint16_t)(strlen(str) * 8);
}

uint16_t oled_GetUTF8Width(const char *str)
{
  uint16_t w = 0;
  while (*str)
  {
    uint16_t code = utf8_decode(&str);
    w += (code < 0x80) ? 8 : 16;
  }
  return w;
}
