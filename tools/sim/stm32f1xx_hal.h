#ifndef STUB_HAL_H
#define STUB_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

typedef struct { int dummy; } I2C_HandleTypeDef;
typedef struct { int dummy; } UART_HandleTypeDef;

typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;

#define GPIO_PIN_6    0x0040u
#define GPIO_PIN_7    0x0080u
#define GPIO_PIN_9    0x0200u
#define GPIO_PIN_10   0x0400u
#define GPIO_PIN_13   0x2000u
#define GPIOA         ((void *)1)
#define GPIOB         ((void *)2)
#define GPIOC         ((void *)3)
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET   1

uint32_t HAL_GetTick(void);
void     HAL_Delay(uint32_t ms);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h, uint16_t addr, uint8_t *p, uint16_t n, uint32_t t);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h, uint16_t addr, uint8_t *p, uint16_t n, uint32_t t);
void HAL_GPIO_WritePin(void *port, uint16_t pin, int state);
void HAL_GPIO_TogglePin(void *port, uint16_t pin);

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

#define LED_PIN     GPIO_PIN_13
#define LED_PORT    GPIOC
#define LED_ON()    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()   HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE() HAL_GPIO_TogglePin(LED_PORT, LED_PIN)

#endif
