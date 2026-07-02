/*
 * f6222_spi_stm32.c — STM32 HAL SPI transport for F6222 driver
 */

#include "f6222_spi_stm32.h"

#include "main.h"

int f6222_spi_stm32_xfer(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len) {
    if (ctx == NULL || tx == NULL || len == 0U) {
        return -1;
    }

    f6222_spi_stm32_t* hal = (f6222_spi_stm32_t*)ctx;
    GPIO_TypeDef* cs_port = (GPIO_TypeDef*)hal->cs_gpio_port;
    HAL_StatusTypeDef status;

    HAL_GPIO_WritePin(cs_port, hal->cs_pin, GPIO_PIN_RESET);

    if (rx != NULL) {
        status = HAL_SPI_TransmitReceive(hal->hspi, (uint8_t*)tx, rx, len, hal->timeout_ms);
    } else {
        status = HAL_SPI_Transmit(hal->hspi, (uint8_t*)tx, len, hal->timeout_ms);
    }

    HAL_GPIO_WritePin(cs_port, hal->cs_pin, GPIO_PIN_SET);

    return (status == HAL_OK) ? 0 : -1;
}

void f6222_spi_stm32_bind(f6222_dev_t* dev, f6222_spi_stm32_t* hal) {
    if (dev == NULL || hal == NULL) {
        return;
    }
    dev->spi_xfer = f6222_spi_stm32_xfer;
    dev->ctx = hal;
}
