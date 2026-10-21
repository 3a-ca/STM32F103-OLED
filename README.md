# STM32F103C8T6 + 0.96" I2C OLED 显示实验

嵌入式系统设计作业：CubeMX + HAL 框架，u8g2 风格的图形库，六个界面（演示 / 个人信息 / 水平滑动 / 垂直滑动 / 图案动画 / 温湿度）。

## 1. 硬件连接

| OLED | 开发板 | 说明 |
|---|---|---|
| VCC | 3.3V | **不要接 5V** |
| GND | GND | 共地 |
| SCL | PB6 | I2C1_SCL |
| SDA | PB7 | I2C1_SDA |

AHT20 挂在同一组 I2C 上（地址 0x38）；没有 AHT20 也能跑，界面会显示“未连接”。

## 2. 改个人信息

只改 `Core/Inc/app_config.h` 三行，然后重新编译：

```c
#define STUDENT_NAME   "黄诗凯"
#define STUDENT_ID     "632407030316"
#define STUDENT_CLASS  "通信工程2402"
```

## 3. 编译

**Keil MDK**：打开 `MDK-ARM/OLED_Demo.uvprojx` → Rebuild → Download。
（已验证：ARMCLANG V6.24，0 Error / 0 Warning，Code 26384 B）

**GCC**（可选）：

```bash
make            # 生成 build/OLED_Demo.hex / .bin
```

## 4. 烧录

- Keil：工具栏 Download（需要 ST-Link/DAP-Link，Options → Debug 里选好调试器）
- 命令行：`STM32_Programmer_CLI -c port=SWD -w build/OLED_Demo.hex -v -rst`

如果没有 8 MHz 晶振，把 `SystemClock_Config()` 改成 HSI，否则会卡在时钟初始化。

## 5. 界面切换

- 每 5 秒自动切换
- 串口（PA9/PA10，115200）发 `1`~`6` 直接跳转，发 `n` 下一个

## 6. 目录说明

```
Core/Inc     头文件（app_config.h 是个人信息）
Core/Src     驱动与应用（oled.c 图形库，app.c 界面，aht20.c 传感器）
Drivers      CMSIS + HAL（CubeF1 V1.8.7）
MDK-ARM      Keil 工程 + ARM 版启动文件
Startup      GCC 版启动文件
tools/       字库生成、仿真、报告生成脚本
docs/        仿真效果图
实验报告.md / .docx / .pdf
```

## 7. 说明

`oled.c` 是按 u8g2 分层（绘图 API / 显存 / 字节传输回调）写的精简实现，接口与 u8g2 对应，方便以后替换成官方 u8g2：实现 `u8x8_byte_hw_i2c` 回调后调用 `u8g2_Setup_ssd1306_i2c_128x64_noname_f()` 即可。

字库由 `tools/gen_fonts.py` 用宋体 16 px 栅格化生成：ASCII 95 个（8×16），汉字 56 个（16×16），全部放 Flash。
