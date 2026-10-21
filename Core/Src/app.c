/**
  ******************************************************************************
  * @file    app.c
  * @brief   界面实现：演示 / 个人信息 / 水平滑动 / 垂直滑动 / 图案动画 / 温湿度
  *
  *  六个界面每 5 秒自动切换一次；也可以用串口发 1~6 直接跳转。
  ******************************************************************************
  */
#include "app.h"
#include "oled.h"
#include "oled_fonts.h"
#include "aht20.h"
#include "app_config.h"
#include <string.h>

static uint8_t  s_screen;
static uint32_t s_switch_tick;
static uint32_t s_boot_tick;

static float    s_temp, s_humi;
static uint8_t  s_sensor_ok, s_sensor_tried;
static uint32_t s_sensor_tick;

/* ---------------------------- 小工具 ---------------------------- */
static uint8_t fmt_f1(char *buf, float v)
{
  uint8_t n = 0;
  int32_t t = (int32_t)(v * 10.0f + (v >= 0.0f ? 0.5f : -0.5f));
  char tmp[8];
  uint8_t k = 0;

  if (t < 0) { buf[n++] = '-'; t = -t; }

  int32_t ip = t / 10;
  int32_t fp = t % 10;
  if (ip == 0) tmp[k++] = '0';
  while (ip > 0) { tmp[k++] = (char)('0' + ip % 10); ip /= 10; }
  while (k > 0) buf[n++] = tmp[--k];

  buf[n++] = '.';
  buf[n++] = (char)('0' + fp);
  buf[n] = '\0';
  return n;
}

/* 顶部标题条：标题 + 一条分隔线 */
static void draw_header(const char *title)
{
  oled_SetColor(1);
  oled_DrawStr(2, 0, title);
  oled_DrawHLine(0, 17, 128);
}

/* 一行“中文标签 + 数值”，返回整行宽度 */
static int16_t draw_kv(int16_t x, int16_t y, const char *label, const char *value)
{
  int16_t w = oled_DrawUTF8(x, y, label);
  int16_t w2 = oled_DrawUTF8(x + w, y, value);
  return (int16_t)(w + w2);
}

/* ---------------------------- 界面 0：图形库演示 ---------------------------- */
static void screen_demo(uint32_t t)
{
  int16_t bx = (int16_t)(4 + (t / 10) % 112);
  if (((t / 10) / 112) & 1) bx = (int16_t)(116 - bx);

  oled_SetColor(1);

  oled_DrawFrame(0, 0, 128, 64);
  oled_DrawStr(8, 2, "SSD1306 128x64");
  oled_DrawHLine(3, 19, 122);

  oled_DrawCircle(22, 37, 9);
  oled_DrawRFrame(42, 27, 28, 20, 5);
  oled_DrawLine(80, 27, 120, 47);
  oled_DrawLine(80, 47, 120, 27);

  oled_DrawBox(bx, 50, 4, 4);

  oled_DrawFrame(4, 57, 120, 5);
  oled_DrawBox(6, 59, (int16_t)(1 + (t / 12) % 116), 1);
}

/* ---------------------------- 界面 1：个人信息 ---------------------------- */
static void screen_info(void)
{
  oled_SetColor(1);
  oled_DrawUTF8(24, 0, "姓名");
  oled_DrawUTF8(56, 0, STUDENT_NAME);
  draw_kv(0, 22, "学号", STUDENT_ID);
  oled_DrawUTF8(16, 44, STUDENT_CLASS);
}

/* ---------------------------- 界面 2：水平滑动 ---------------------------- */
static void screen_scroll_h(uint32_t t)
{
  static const char *msg  = "欢迎来到重庆交通大学物联网205实训室！";
  static const char *msg2 = "Hello, CQJTU IoT Lab 205!";

  int16_t w  = (int16_t)oled_GetUTF8Width(msg);
  int16_t w2 = (int16_t)oled_GetStrWidth(msg2);
  int16_t off  = (int16_t)((t / 14) % (w + 128));
  int16_t off2 = (int16_t)((t / 9)  % (w2 + 128));

  draw_header("H-SCROLL");
  oled_SetColor(1);
  oled_DrawUTF8((int16_t)(128 - off), 26, msg);
  oled_DrawStr((int16_t)(128 - off2), 47, msg2);
}

/* ---------------------------- 界面 3：垂直滑动 ---------------------------- */
static void screen_scroll_v(uint32_t t)
{
  static const char *lines[] = {
    "嵌入式系统设计",
    "图形库移植演示",
    "OLED 128x64 I2C",
    "STM32F103C8T6",
  };
  const int16_t line_h = 18;
  const int16_t area_y = 20;
  const int16_t cycle = (int16_t)(line_h * 4);
  int16_t off = (int16_t)((t / 26) % cycle);

  oled_SetColor(1);
  for (uint8_t i = 0; i < 8; i++)
  {
    int16_t y = (int16_t)(area_y + i * line_h - off);
    if (y > 70 || y + 16 < area_y) continue;
    if ((i & 3) < 2) oled_DrawUTF8(6, y, lines[i & 3]);
    else             oled_DrawStr(6, y, lines[i & 3]);
  }

  oled_SetColor(0);
  oled_DrawBox(0, 0, 128, 19);
  oled_SetColor(1);
  oled_DrawStr(2, 0, "V-SCROLL");
  oled_DrawHLine(0, 17, 128);

  oled_DrawPixel(122, 24); oled_DrawPixel(121, 25); oled_DrawPixel(123, 25);
  oled_DrawPixel(122, 60); oled_DrawPixel(121, 59); oled_DrawPixel(123, 59);
}

/* ---------------------------- 界面 4：图案与动画 ---------------------------- */
static void screen_image(uint32_t t)
{
  uint32_t ph = t % 3000;
  uint8_t frame = 0;
  int16_t heart_y = (int16_t)(40 + ((t / 300) % 2) * 2);

  if (ph > 2600 && ph < 2760)       frame = 1;
  else if (ph >= 2760 && ph < 2900) frame = 2;

  oled_SetColor(1);
  oled_DrawBitmap(4, 8, 48, 48, oled_avatar_frames[frame]);

  oled_DrawUTF8(60, 4, "图案动画");
  oled_DrawStr(60, 24, "3 FRAMES");
  oled_DrawBitmap(60, heart_y, 16, 16, oled_heart16);
  oled_DrawHLine(62, 58, (int16_t)(6 + 4 * ((t / 250) % 3)));
}

/* ---------------------------- 界面 5：温湿度 ---------------------------- */
static void screen_sensor(uint32_t t)
{
  char buf[12];
  uint8_t len;
  int16_t bar;

  (void)t;
  draw_header("AHT20");

  if (!s_sensor_ok)
  {
    oled_DrawUTF8(8, 26, "未连接");
    oled_DrawStr(8, 48, "check SDA/SCL");
    return;
  }

  oled_SetColor(1);
  oled_DrawUTF8(2, 20, "温度");
  len = fmt_f1(buf, s_temp);
  oled_DrawStr(44, 20, buf);
  oled_DrawStr((int16_t)(44 + len * 8), 20, "C");

  oled_DrawUTF8(2, 38, "湿度");
  len = fmt_f1(buf, s_humi);
  oled_DrawStr(44, 38, buf);
  oled_DrawStr((int16_t)(44 + len * 8), 38, "%");

  oled_DrawFrame(2, 56, 124, 7);
  bar = (int16_t)((s_humi > 100.0f ? 100.0f : (s_humi < 0.0f ? 0.0f : s_humi)) * 1.20f);
  if (bar > 0) oled_DrawBox(4, 58, bar, 3);
}

/* ---------------------------- 调度 ---------------------------- */
void app_Init(void)
{
  s_screen = 0;
  s_boot_tick = HAL_GetTick();
  s_switch_tick = s_boot_tick;
  s_sensor_ok = 0;
  s_sensor_tried = 0;
  s_sensor_tick = 0;
}

void app_Next(void)
{
  s_screen = (uint8_t)((s_screen + 1) % APP_SCREEN_COUNT);
  s_switch_tick = HAL_GetTick();
}

void app_Select(uint8_t index)
{
  if (index < APP_SCREEN_COUNT)
  {
    s_screen = index;
    s_switch_tick = HAL_GetTick();
  }
}

void app_Task(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t t = now - s_boot_tick;

  if (now - s_switch_tick >= SCREEN_PERIOD_MS)
  {
    s_screen = (uint8_t)((s_screen + 1) % APP_SCREEN_COUNT);
    s_switch_tick = now;
  }

  if (s_screen == 5)
  {
    if (!s_sensor_tried)
    {
      s_sensor_ok = aht20_Init();
      s_sensor_tried = 1;
      s_sensor_tick = now;
    }
    else if (s_sensor_ok && now - s_sensor_tick > 1000)
    {
      float tt, hh;
      if (aht20_Read(&tt, &hh)) { s_temp = tt; s_humi = hh; }
      s_sensor_tick = now;
    }
  }

  oled_Clear();
  switch (s_screen)
  {
    case 0:  screen_demo(t);     break;
    case 1:  screen_info();      break;
    case 2:  screen_scroll_h(t); break;
    case 3:  screen_scroll_v(t); break;
    case 4:  screen_image(t);    break;
    default: screen_sensor(t);   break;
  }
  oled_Refresh();

  LED_TOGGLE();
  HAL_Delay(20);
}
