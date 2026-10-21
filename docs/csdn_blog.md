# STM32F103 + 0.96寸 OLED 图形界面实战：从 CubeMX 到 u8g2 风格移植（附完整源码）

> 课程：嵌入式系统设计　｜　平台：STM32F103C8T6 + SSD1306（I2C）＋ Keil MDK
> 效果：6 个界面自动轮播——图形演示、姓名学号、水平滑动、垂直滑动、头像动画、AHT20 温湿度

![在这里插入图片描述](docs/img/hw_photo.jpg)

---

## 一、先看效果

上电后每 5 秒自动切换一个界面，也可以用串口发 `1~6` 直接跳转。

![演示](docs/img/sim0_demo.png)
![个人信息](docs/img/sim1_info.png)
![水平滑动](docs/img/sim2_hscroll.png)
![垂直滑动](docs/img/sim3_vscroll.png)
![图案动画](docs/img/sim4_image.png)
![温湿度](docs/img/sim5_sensor.png)

---

## 二、硬件与接线

| OLED 引脚 | 开发板 | 说明 |
|---|---|---|
| VCC | 3.3V | **千万别接 5V**，接错会烧屏 |
| GND | GND | 共地 |
| SCL | PB6 | I2C1_SCL |
| SDA | PB7 | I2C1_SDA |

AHT20 温湿度传感器挂在同一组 I2C 上（地址 0x38），没有也能跑，界面会显示“未连接”。

USART1（PA9/PA10，115200）可选，用来打印调试信息和切屏。

---

## 三、原理速览

### 3.1 SSD1306 显存

128×64 点阵 = 1024 字节。显存分 8 页，每页 128 列，一个字节的 bit0~bit7 对应这一列自上而下的 8 个像素。

写屏流程：设页地址（0xB0+page）→ 设列地址（0x00~0x0F / 0x10~0x1F）→ 连续写 128 字节数据。每个字节前加控制字节：`0x00` 表示命令、`0x40` 表示显存数据。

```c
for (page = 0; page < 8; page++) {
    OLED_WriteCmd(0xB0 + page);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);
    OLED_WriteData(&buf[page * 128], 128);
}
```

### 3.2 I2C 时序

起始位（SCL 高时 SDA 下降）→ 7 位地址 + 读写位 → ACK → 数据字节 + ACK → 停止位。OLED 的 7 位地址是 0x3C，HAL 里传 0x78。

400 kHz 下每字节约 22.5 μs，整屏 1024 字节加控制字节大约 24 ms 刷新一次，约 40 帧/秒。

### 3.3 汉字点阵

16×16 汉字每个 32 字节，按行存，每行 2 字节（高字节是左边 8 点）。ASCII 用 8×16，每字符 16 字节。

字库不是网上下的，而是用**宋体 16 px 栅格化**脚本生成的：ASCII 95 个（1520 B）+ 界面用到的汉字 56 个（1792 B），全部 `const` 放 Flash，查表用 Unicode 码位。

```c
static uint16_t utf8_decode(const char **s);   /* UTF-8 -> Unicode */
const uint8_t *g = find_cjk(code);             /* 查 16x16 点阵 */
draw_cjk(x, y, g);  x += 16;
```

---

## 四、工程搭建

1. **CubeMX**：选 STM32F103C8Tx，RCC → HSE 8 MHz，时钟树拉到 72 MHz；I2C1 400 kHz（PB6/PB7）；USART1 115200；PC13 输出。工具链选 MDK-ARM V5，生成代码。
2. **Keil**：把 `oled.c / app.c / aht20.c / 字库.c` 加进工程，Include 路径加上 `Core/Inc`。
3. 编译下载即可。

工程同时带了一份 GCC Makefile（`arm-none-eabi-gcc`），不装 Keil 也能编：

```bash
make            # 生成 build/OLED_Demo.hex / .bin
```

编译结果：Keil 下 Code 26 384 B、RO 5 140 B、ZI 2 260 B，FLASH 用了 34%，RAM 用了 14%。

---

## 五、代码结构

```
Core/Inc   app_config.h   ← 只改这里就能换姓名学号
           oled.h  oled_fonts.h  aht20.h  app.h
Core/Src   main.c        ← 时钟、I2C、串口、主循环
           oled.c        ← 显存 + 图元 + 文本（u8g2 风格接口）
           oled_font8x16.c / oled_fontcjk.c / oled_image.c  ← 字库与图案
           app.c         ← 六个界面 + 调度
           aht20.c       ← 温湿度
MDK-ARM    OLED_Demo.uvprojx
tools/     字库生成、界面仿真、报告生成脚本
```

对外接口基本照抄 u8g2 的命名：`DrawPixel / DrawLine / DrawFrame / DrawBox / DrawCircle / DrawRFrame / DrawBitmap / DrawStr / DrawUTF8 / SendBuffer`。以后想换官方 u8g2，只要实现一个字节传输回调：

```c
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch (msg) {
    case U8X8_MSG_BYTE_SEND:
      HAL_I2C_Master_Transmit(&hi2c1, 0x78, (uint8_t*)arg_ptr, arg_int, 50); break;
  }
  return 1;
}
```

---

## 六、六个界面怎么做的

### 6.1 个人信息

```c
#define STUDENT_NAME   "覃晖杰"
#define STUDENT_ID     "632407030323"
#define STUDENT_CLASS  "通信2班"
```

3 行 16×16 汉字 + 8×16 数字，正好 128 像素宽。

### 6.2 水平滑动

文本宽度 w，偏移 `off = (t/14) % (w+128)`，绘制起点 `x = 128 - off`，越界像素由 DrawPixel 自动裁掉。第二行英文反向滚动，视觉上更活。

### 6.3 垂直滑动

偏移作用在 y 上，四行内容重复两遍保证窗口里始终有字；标题区先画文字再用背景色覆盖，形成“裁剪窗口”，最后补画标题和分隔线。

### 6.4 图案动画

48×48 头像做了 3 帧（睁眼/闭眼/单眼眨），每 3 秒触发一次约 300 ms 的眨眼；旁边 16×16 心形按 300 ms 周期上下跳动。

### 6.5 温湿度

AHT20：读状态字判断是否校准 → 发 0xAC 0x33 0x00 触发 → 等 80 ms → 读 7 字节 → 20 位数据换算：

```c
*RH = raw_h * 100.0f / 1048576.0f;
*T  = raw_t * 200.0f / 1048576.0f - 50.0f;
```

---

## 七、踩过的坑（都是真实调试记录）

| 现象 | 原因 | 解决 |
|---|---|---|
| 屏全黑 | VCC 接了 5V / SDA、SCL 接反 | 按表重接，万用表确认 3.3V |
| 中文全是问号 | 把含中文的串交给了只支持 ASCII 的 DrawStr | 统一走 DrawUTF8（UTF-8 解码后查 16×16 字库） |
| 长串被截断 | 起点为负没裁剪 | DrawPixel 里统一判 0≤x<128、0≤y<64 |
| 圆角矩形画成八边形 | 四角只画了一个像素 | 用中点画圆算法补全四段圆弧 |
| 垂直滚动盖住标题 | 没有裁剪窗口 | 先画文字→覆盖标题区→再画标题 |
| DAPLink 下载报 “FAILED to reset/halt” | 4 芯排线插到了电源排针上（B8/B9/5V/G），SWDIO/SWCLK 根本没接到 A13/A14 | 换成 4 根单线，按丝印接 CLK/DIO/GND/3V3 |

---

## 八、思考题：Keil 的逻辑分析仪能抓 I2C 波形吗？

**结论：抓不到真实的 SDA 波形。**

1. Keil 的 Logic Analyzer 是“变量观察器”，显示的是你添加的变量/寄存器地址的内容，不是引脚电平；
2. 采样靠调试器周期性读内存，等效采样率只有几十 kHz，而 I2C 400 kHz 一个位宽才 2.5 μs，边沿根本抓不住；
3. 仿真模式下没有 OLED 从机模型，ACK 都是假的。

**替代方案：**

- 双通道示波器接 SCL/SDA，触发在起始位（SDA 下降沿），时基 20~50 μs/格，能看清地址 0x78、ACK 拉低、控制字节 0x00/0x40；
- Saleae / DSLogic + PulseView 的 I2C 解码器，直接解出字节，还能导出 CSV 核对；
- 用定时器输入捕获或 EXTI 自己记录边沿时间戳；
- 第二块 MCU 做 I2C 从机监听；
- 纯软件验证：I2C 扫描确认 0x78/0x38 在线 + 串口打印每帧校验和，再结合显示结果反证时序。

---

## 九、资源与源码

- 完整工程（Keil + GCC + 源码 + 字库 + 报告）：见文末仓库 / 压缩包
- 主控：STM32F103C8T6，64 KB Flash / 20 KB RAM
- 占用：Flash 22.6 KB（GCC）/ 26.4 KB（Keil），RAM 2.8 KB

如果这篇文章帮你少踩了一个坑，点个赞再走呗 👍
