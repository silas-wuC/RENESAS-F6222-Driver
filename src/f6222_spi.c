/*
 * f6222_spi.c — SPI frame construction (Step 1)
 *
 * Datasheet: Table 5 read/write modes (001/000/110/111/011/010).
 */

#include "f6222.h"

f6222_status_t f6222_local_reg_write(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t reg, uint16_t val) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE || chip_addr > F6222_CHIP_ADDR_MAX || reg > F6222_SPI_REG_ADDR_MASK)
        return F6222_ERR_INVALID_ARG;

    uint8_t tx[5] = {0};
    int ret;

    tx[0] = 0x20u | ((rf_load & 0x03u) << 3);
    tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;
    tx[2] = reg & F6222_SPI_REG_ADDR_MASK;
    tx[3] = (uint8_t)(val >> F6222_SPI_DATA_HIGH_SHIFT);
    tx[4] = (uint8_t)(val & F6222_SPI_DATA_LOW_MASK);

    ret = dev->spi_xfer(dev->ctx, tx, NULL, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    return F6222_OK;
}

f6222_status_t f6222_local_reg_read(f6222_dev_t* dev, uint8_t chip_addr, uint8_t reg, uint16_t* val) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX || reg > F6222_SPI_REG_ADDR_MASK || val == NULL) return F6222_ERR_INVALID_ARG;

    uint8_t tx[5] = {0};
    uint8_t rx[5] = {0};
    int ret;

    tx[0] = 0x00u;
    tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;
    tx[2] = reg & F6222_SPI_REG_ADDR_MASK;
    tx[3] = F6222_SPI_PAD_BYTE;
    tx[4] = F6222_SPI_PAD_BYTE;
    ret = dev->spi_xfer(dev->ctx, tx, rx, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    *val = ((uint16_t)rx[3] << F6222_SPI_DATA_HIGH_SHIFT) | rx[4];
    return F6222_OK;
}

f6222_status_t f6222_local_lut_write(f6222_dev_t* dev, uint8_t ch, uint8_t chip_addr, uint8_t lut_addr, uint16_t val) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (!F6222_CH_IS_VALID(ch) || chip_addr > F6222_CHIP_ADDR_MAX || lut_addr >= F6222_LUT_ENTRIES)
        return F6222_ERR_INVALID_ARG;

    uint8_t tx[5] = {0};
    uint8_t ch_idx = F6222_CH_TO_IDX(ch);
    int ret;

    tx[0] = 0xC0u | ((ch_idx & 0x0Fu) << 1);
    tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;
    tx[2] = lut_addr & F6222_SPI_LUT_ADDR_MASK;
    tx[3] = (uint8_t)(val >> F6222_SPI_DATA_HIGH_SHIFT);
    tx[4] = (uint8_t)(val & F6222_SPI_DATA_LOW_MASK);
    ret = dev->spi_xfer(dev->ctx, tx, NULL, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    return F6222_OK;
}

f6222_status_t f6222_local_lut_read(f6222_dev_t* dev, uint8_t lut_ch, uint8_t chip_addr, uint8_t lut_addr,
                                    uint16_t* val) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (!F6222_CH_IS_VALID(lut_ch) || chip_addr > F6222_CHIP_ADDR_MAX || lut_addr >= F6222_LUT_ENTRIES || val == NULL)
        return F6222_ERR_INVALID_ARG;

    uint8_t tx[5] = {0};
    uint8_t rx[5] = {0};
    int ret;

    uint8_t ch_idx = F6222_CH_TO_IDX(lut_ch);
    tx[0] = (F6222_SPI_M_LOCAL_LUT_READ << 5) | ((ch_idx & 0x0Fu) << 1);
    tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;
    tx[2] = lut_addr & F6222_SPI_LUT_ADDR_MASK;
    tx[3] = F6222_SPI_PAD_BYTE;
    tx[4] = F6222_SPI_PAD_BYTE;

    ret = dev->spi_xfer(dev->ctx, tx, rx, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    *val = ((uint16_t)rx[3] << F6222_SPI_DATA_HIGH_SHIFT) | rx[4];

    return F6222_OK;
}

f6222_status_t f6222_global_reg_write(f6222_dev_t* dev, bool sa_op_enable, uint8_t sa_index, uint8_t reg,
                                      uint16_t val) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (sa_index > 7u || reg > F6222_SPI_REG_ADDR_MASK) return F6222_ERR_INVALID_ARG;

    uint8_t tx[4] = {0};
    int ret;

    tx[0] = 0x60u | ((sa_op_enable ? 1u : 0u) << 3) | (sa_index & 0x07u);
    tx[1] = reg & F6222_SPI_REG_ADDR_MASK;
    tx[2] = (uint8_t)(val >> F6222_SPI_DATA_HIGH_SHIFT);
    tx[3] = (uint8_t)(val & F6222_SPI_DATA_LOW_MASK);
    ret = dev->spi_xfer(dev->ctx, tx, NULL, sizeof(tx));
    if (ret < 0) return F6222_ERR_SPI;

    return F6222_OK;
}
