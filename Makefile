##############################################################################
# STM32F103C8T6 OLED 工程 —— GCC 构建脚本（用于命令行验证/生成 hex bin）
#   用法： make          编译并生成 build/OLED_Demo.elf/.hex/.bin
#          make clean    清理
#   需要 arm-none-eabi-gcc（Keil 用户可忽略本文件，直接用 MDK-ARM 工程）
##############################################################################
TARGET   = OLED_Demo
BUILD    = build

C_SOURCES  = $(wildcard Core/Src/*.c)
C_SOURCES += Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_i2c.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
             Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c

ASM_SOURCES = Startup/startup_stm32f103xb.s

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

CPU   = -mcpu=cortex-m3
MCU   = $(CPU) -mthumb -mfloat-abi=soft

C_DEFS = -DUSE_HAL_DRIVER -DSTM32F103xB

C_INCLUDES = -ICore/Inc \
             -IDrivers/CMSIS/Include \
             -IDrivers/CMSIS/Device/ST/STM32F1xx/Include \
             -IDrivers/STM32F1xx_HAL_Driver/Inc \
             -IDrivers/STM32F1xx_HAL_Driver/Inc/Legacy

CFLAGS  = $(MCU) $(C_DEFS) $(C_INCLUDES) -O2 -Wall -Wextra -std=gnu11 \
          -ffunction-sections -fdata-sections -g -gdwarf-2
ASFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -g
LDSCRIPT = STM32F103C8TX_FLASH.ld
LDFLAGS  = $(MCU) -T$(LDSCRIPT) -specs=nano.specs -specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map,--cref -Wl,--print-memory-usage

OBJECTS  = $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin

$(BUILD)/%.o: %.c Makefile | $(BUILD)
	$(CC) -c $(CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" $< -o $@

$(BUILD)/%.o: %.s Makefile | $(BUILD)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD)/%.hex: $(BUILD)/%.elf
	$(CP) -O ihex $< $@

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(CP) -O binary -S $< $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)

.PHONY: all clean
