/* PC 端仿真：用同一份绘图代码把 6 个界面渲染成 PGM 图片 */
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "oled.h"
#include "app.h"

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

static uint32_t s_tick;

uint32_t HAL_GetTick(void) { return s_tick; }
void HAL_Delay(uint32_t ms) { s_tick += ms; }
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state) { (void)port; (void)pin; (void)state; }
void HAL_GPIO_TogglePin(void *port, uint16_t pin) { (void)port; (void)pin; }

/* 模拟 AHT20：返回固定温湿度 */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t addr, uint8_t *p, uint16_t n, uint32_t t)
{
  (void)h; (void)addr; (void)p; (void)n; (void)t;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h, uint16_t addr, uint8_t *p, uint16_t n, uint32_t t)
{
  (void)h; (void)addr; (void)t;
  if (n == 1 && p)
  {
    p[0] = 0x08;         /* 已校准 */
    return HAL_OK;
  }
  if (n == 7 && p)
  {
    /* 26.5 C, 58.0 %RH */
    uint32_t raw_t = (uint32_t)((26.5f + 50.0f) * 1048576.0f / 200.0f) & 0xFFFFF;
    uint32_t raw_h = (uint32_t)(58.0f * 1048576.0f / 100.0f) & 0xFFFFF;
    p[0] = 0x18;
    p[1] = (uint8_t)(raw_h >> 12);
    p[2] = (uint8_t)(raw_h >> 4);
    p[3] = (uint8_t)(((raw_h & 0x0F) << 4) | (raw_t >> 16));
    p[4] = (uint8_t)(raw_t >> 8);
    p[5] = (uint8_t)raw_t;
    p[6] = 0;
    return HAL_OK;
  }
  return HAL_OK;
}

static void dump_pgm(const char *path)
{
  FILE *f = fopen(path, "wb");
  if (!f) return;
  fprintf(f, "P5\n%d %d\n255\n", OLED_WIDTH, OLED_HEIGHT);
  for (uint8_t y = 0; y < OLED_HEIGHT; y++)
  {
    for (uint8_t x = 0; x < OLED_WIDTH; x++)
    {
      uint8_t byte = oled.buf[(y >> 3) * OLED_WIDTH + x];
      uint8_t on = (byte >> (y & 7)) & 1;
      fputc(on ? 255 : 0, f);
    }
  }
  fclose(f);
}

int main(void)
{
  const char *names[] = {"sim0_demo", "sim1_info", "sim2_hscroll", "sim3_vscroll", "sim4_image", "sim5_sensor"};

  oled_Init();
  app_Init();
  s_tick = 0;

  for (uint8_t i = 0; i < 6; i++)
  {
    char path[64];
    app_Select(i);
    for (uint16_t frame = 0; frame < 90; frame++)
    {
      app_Task();
    }
    sprintf(path, "sim_out/%s.pgm", names[i]);
    dump_pgm(path);
  }
  printf("simulation done\n");
  return 0;
}
