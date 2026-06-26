/*
 * f6222_spi.c — SPI frame construction (Step 1)
 *
 * Datasheet: mode 000/001/011/110/101/100 frame layouts.
 */

#include "f6222.h"

f6222_status_t f6222_local_reg_write(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t reg, uint16_t val) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE || chip_addr > F6222_CHIP_ADDR_MAX || reg > 0x7Fu)
        return F6222_ERR_INVALID_ARG;

    uint8_t tx[5];
    int ret;

    tx[0] = 0x20u | ((rf_load & 0x03u) << 3);
    tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;
    tx[2] = reg & 0x7Fu;
    tx[3] = (uint8_t)(val >> 8);
    tx[4] = (uint8_t)(val & 0xFFu);

    ret = dev->spi_xfer(dev->ctx, tx, NULL, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    return F6222_OK;
}

f6222_status_t f6222_local_reg_read(f6222_dev_t* dev, uint8_t chip_addr, uint8_t reg, uint16_t* val) {
    if (dev == NULL || dev->spi_xfer == NULL || val == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX || reg > 0x7Fu) return F6222_ERR_INVALID_ARG;

    uint8_t tx[5];
    uint8_t rx[5] = {0};
    int ret;

    tx[0] = 0x00u;
    tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;
    tx[2] = reg & 0x7Fu;
    tx[3] = 0x00u;
    tx[4] = 0x00u;
    ret = dev->spi_xfer(dev->ctx, tx, rx, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    *val = ((uint16_t)rx[3] << 8) | rx[4];
    return F6222_OK;
}
