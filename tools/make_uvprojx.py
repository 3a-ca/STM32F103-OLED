# -*- coding: utf-8 -*-
"""Rebuild MDK-ARM/OLED_Demo.uvprojx by literal replacement in the template."""
import io
import os

TPL = r"C:\Users\30477\STM32Cube\Repository\STM32Cube_FW_F1_V1.8.7\Projects\STM32F103RB-Nucleo\Templates\MDK-ARM\Project.uvprojx"
OUT = r"C:\Users\30477\Documents\deepseek-harness\default-workspace\stm32_oled\MDK-ARM\OLED_Demo.uvprojx"

text = io.open(TPL, encoding="utf-8").read()


def rep(old, new):
    global text
    if old not in text:
        raise SystemExit("not found: " + old[:70])
    text = text.replace(old, new, 1)


def rep_block(tag, new_block):
    global text
    start = text.index("<%s>" % tag)
    end = text.index("</%s>" % tag) + len("</%s>" % tag)
    text = text[:start] + new_block + text[end:]


rep("<TargetName>STM32F103RB_Nucleo</TargetName>", "<TargetName>OLED_Demo</TargetName>")
rep("<uAC6>0</uAC6>", "<uAC6>1</uAC6>")
rep("<Device>STM32F103RB</Device>", "<Device>STM32F103C8</Device>")
rep("<PackID>Keil.STM32F1xx_DFP.2.3.0</PackID>", "<PackID>Keil.STM32F1xx_DFP.2.4.0</PackID>")
rep('<Cpu>IRAM(0x20000000-0x20004FFF) IROM(0x8000000-0x801FFFF) CLOCK(8000000) CPUTYPE("Cortex-M3")</Cpu>',
    '<Cpu>IRAM(0x20000000-0x20004FFF) IROM(0x8000000-0x800FFFF) CLOCK(8000000) CPUTYPE("Cortex-M3")</Cpu>')
rep(r"<SFDFile>$$Device:STM32F103RB$SVD\STM32F103xx.svd</SFDFile>",
    r"<SFDFile>$$Device:STM32F103C8$SVD\STM32F103xx.svd</SFDFile>")
rep(r"<OutputDirectory>STM32F103RB_Nucleo\</OutputDirectory>",
    r"<OutputDirectory>OLED_Demo\</OutputDirectory>")
rep("<OutputName>STM32F103RB_Nucleo</OutputName>", "<OutputName>OLED_Demo</OutputName>")
rep("<CreateHexFile>0</CreateHexFile>", "<CreateHexFile>1</CreateHexFile>")
rep("<InvalidFlash>1</InvalidFlash>", "<InvalidFlash>0</InvalidFlash>")
rep("<Define>USE_HAL_DRIVER,STM32F103xB,USE_STM32F1xx_NUCLEO</Define>",
    "<Define>USE_HAL_DRIVER,STM32F103xB</Define>")
rep("<IncludePath>../Inc;../../../../Drivers/CMSIS/Device/ST/STM32F1xx/Include;"
    "../../../../Drivers/STM32F1xx_HAL_Driver/Inc;../../../../Drivers/BSP/STM32F1xx_Nucleo;"
    "../../../../Drivers/BSP/Adafruit_Shield</IncludePath>",
    r"<IncludePath>..\Core\Inc;..\Drivers\CMSIS\Include;"
    r"..\Drivers\CMSIS\Device\ST\STM32F1xx\Include;..\Drivers\STM32F1xx_HAL_Driver\Inc</IncludePath>")

rep_block("IROM", "<IROM>\n                <Type>1</Type>\n"
                  "                <StartAddress>0x8000000</StartAddress>\n"
                  "                <Size>0x10000</Size>\n              </IROM>")

CORE = ["main", "app", "aht20", "oled", "oled_font8x16", "oled_fontcjk", "oled_image",
        "stm32f1xx_hal_msp", "stm32f1xx_it", "system_stm32f1xx"]
HAL = ["hal", "hal_cortex", "hal_rcc", "hal_rcc_ex", "hal_gpio", "hal_dma", "hal_i2c",
       "hal_uart", "hal_flash", "hal_flash_ex", "hal_pwr", "hal_exti"]


def file_xml(name, ftype, path):
    return ("        <File>\n"
            "          <FileName>%s</FileName>\n"
            "          <FileType>%d</FileType>\n"
            "          <FilePath>%s</FilePath>\n"
            "        </File>\n" % (name, ftype, path))


g = "      <Groups>\n"
g += "        <Group>\n          <GroupName>Startup</GroupName>\n          <Files>\n"
g += file_xml("startup_stm32f103xb.s", 2, r"startup_stm32f103xb.s")
g += "          </Files>\n        </Group>\n"
g += "        <Group>\n          <GroupName>Core</GroupName>\n          <Files>\n"
for n in CORE:
    g += file_xml(n + ".c", 1, r"..\Core\Src\%s.c" % n)
g += "          </Files>\n        </Group>\n"
g += "        <Group>\n          <GroupName>HAL</GroupName>\n          <Files>\n"
for n in HAL:
    g += file_xml("stm32f1xx_%s.c" % n, 1,
                  r"..\Drivers\STM32F1xx_HAL_Driver\Src\stm32f1xx_%s.c" % n)
g += "          </Files>\n        </Group>\n      </Groups>\n"

# 只保留第一个 target，并用我们自己的 Groups 整体替换
head = text[:text.index("<Groups>")]
text = head + g.strip() + "\n    </Target>\n  </Targets>\n</Project>\n"

os.makedirs(os.path.dirname(OUT), exist_ok=True)
io.open(OUT, "w", encoding="utf-8", newline="\n").write(text)
print("written", OUT, len(text))
