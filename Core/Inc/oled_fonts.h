/**
  ******************************************************************************
  * @file    oled_fonts.h
  * @brief   OLED 点阵字库与图像数据声明
  ******************************************************************************
  */
#ifndef __OLED_FONTS_H
#define __OLED_FONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 8x16 ASCII 点阵：95 个字符（0x20~0x7E），每个字符 8 字节，列优先，bit0 为最上面一点 */
extern const uint8_t oled_ascii8x16[95 * 16];

/* 16x16 汉字点阵：每个汉字 32 字节，行优先，每行 2 字节（高字节为左 8 点） */
typedef struct
{
  uint16_t code;          /* Unicode 码位 */
  const uint8_t *bitmap;  /* 32 字节点阵 */
} oled_cjk_glyph_t;

extern const oled_cjk_glyph_t oled_cjk_table[];
extern const uint16_t oled_cjk_count;

/* 48x48 头像（3 帧：睁眼 / 闭眼 / 眨眼），每帧 48*6 字节，页式排列 */
extern const uint8_t *const oled_avatar_frames[3];

/* 16x16 心形图标，32 字节，页式排列 */
extern const uint8_t oled_heart16[16 * 2];

#ifdef __cplusplus
}
#endif

#endif /* __OLED_FONTS_H */
