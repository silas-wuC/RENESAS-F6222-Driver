/*
 * f6222_spi_stm32.h — STM32 HAL SPI transport for F6222 driver
 *
 * Implements the f6222_dev_t.spi_xfer callback using STM32 HAL.
 * Requires CubeMX-generated spi.h / gpio.h (or equivalent).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "f6222.h"

/* Forward-declare HAL types so this header stays usable without stm32 HAL includes. */
struct __SPI_HandleTypeDef;
typedef struct __SPI_HandleTypeDef SPI_HandleTypeDef;

typedef struct {
    SPI_HandleTypeDef* hspi;
    void* cs_gpio_port; /* GPIO_TypeDef* */
    uint16_t cs_pin;    /* GPIO pin mask, e.g. GPIO_PIN_4 */
    uint32_t timeout_ms;
} f6222_spi_stm32_t;

/**
 * spi_xfer callback — wire into f6222_dev_t.spi_xfer.
 * ctx must point to a f6222_spi_stm32_t.
 */
int f6222_spi_stm32_xfer(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len);

/**
 * Convenience: fill f6222_dev_t from STM32 SPI config.
 */
void f6222_spi_stm32_bind(f6222_dev_t* dev, f6222_spi_stm32_t* hal);
