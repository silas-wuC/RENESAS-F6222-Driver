/*
 * f6222_spi.c — SPI frame construction (Step 1)
 *
 * Datasheet: Table 5 read/write modes (001/000/110/111/011/010).
 */

#include "f6222.h"

/* CH1: bias=0x20 … CH16: bias=0x5C; bias/ctrl/set stride +4 per channel */
const f6222_ch_regs_t f6222_ch_reg_map[F6222_NUM_CHANNELS] = {
    [0] = {.ch_bias = 0x20u, .ch_ctrl = 0x21u, .ch_set = 0x22u},
    [1] = {.ch_bias = 0x24u, .ch_ctrl = 0x25u, .ch_set = 0x26u},
    [2] = {.ch_bias = 0x28u, .ch_ctrl = 0x29u, .ch_set = 0x2Au},
    [3] = {.ch_bias = 0x2Cu, .ch_ctrl = 0x2Du, .ch_set = 0x2Eu},
    [4] = {.ch_bias = 0x30u, .ch_ctrl = 0x31u, .ch_set = 0x32u},
    [5] = {.ch_bias = 0x34u, .ch_ctrl = 0x35u, .ch_set = 0x36u},
    [6] = {.ch_bias = 0x38u, .ch_ctrl = 0x39u, .ch_set = 0x3Au},
    [7] = {.ch_bias = 0x3Cu, .ch_ctrl = 0x3Du, .ch_set = 0x3Eu},
    [8] = {.ch_bias = 0x40u, .ch_ctrl = 0x41u, .ch_set = 0x42u},
    [9] = {.ch_bias = 0x44u, .ch_ctrl = 0x45u, .ch_set = 0x46u},
    [10] = {.ch_bias = 0x48u, .ch_ctrl = 0x49u, .ch_set = 0x4Au},
    [11] = {.ch_bias = 0x4Cu, .ch_ctrl = 0x4Du, .ch_set = 0x4Eu},
    [12] = {.ch_bias = 0x50u, .ch_ctrl = 0x51u, .ch_set = 0x52u},
    [13] = {.ch_bias = 0x54u, .ch_ctrl = 0x55u, .ch_set = 0x56u},
    [14] = {.ch_bias = 0x58u, .ch_ctrl = 0x59u, .ch_set = 0x5Au},
    [15] = {.ch_bias = 0x5Cu, .ch_ctrl = 0x5Du, .ch_set = 0x5Eu},
};

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

f6222_status_t f6222_lut_write_global(f6222_dev_t* dev, bool lut_all_channels, uint8_t ch, uint8_t lut_addr,
                                      uint16_t val, const uint16_t* extra_vals, size_t extra_count) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if ((!lut_all_channels && !F6222_CH_IS_VALID(ch)) || lut_addr >= F6222_LUT_ENTRIES ||
        (extra_count > 0u && extra_vals == NULL) || extra_count > F6222_LUT_WRITE_MAX_EXTRA)
        return F6222_ERR_INVALID_ARG;

    size_t tx_len = 4u + extra_count * 2u;
    uint8_t tx[4u + 2u * F6222_LUT_WRITE_MAX_EXTRA] = {0};
    uint8_t ch_idx = F6222_CH_TO_IDX(ch);
    size_t i;
    int ret;

    tx[0] =
        (F6222_SPI_M_GLOBAL_LUT_WRITE << 5) | ((lut_all_channels ? 1u : 0u) << F6222_SPI_LM_SHIFT) | (ch_idx & 0x0Fu);
    tx[1] = lut_addr & F6222_SPI_LUT_ADDR_MASK;
    tx[2] = (uint8_t)(val >> F6222_SPI_DATA_HIGH_SHIFT);
    tx[3] = (uint8_t)(val & F6222_SPI_DATA_LOW_MASK);

    for (i = 0; i < extra_count; i++) {
        tx[4u + i * 2u] = (uint8_t)(extra_vals[i] >> F6222_SPI_DATA_HIGH_SHIFT);
        tx[4u + i * 2u + 1u] = (uint8_t)(extra_vals[i] & F6222_SPI_DATA_LOW_MASK);
    }

    ret = dev->spi_xfer(dev->ctx, tx, NULL, tx_len);
    if (ret < 0) return F6222_ERR_SPI;

    return F6222_OK;
}
