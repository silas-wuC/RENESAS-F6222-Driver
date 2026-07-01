/*
 * f6222_channel.c — Per-channel phase, gain, LNA bypass, enable (Step 3)
 */

#include "f6222.h"

f6222_status_t f6222_set_phase(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t ps_set) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;
    if (!F6222_CH_IS_VALID(ch)) return F6222_ERR_INVALID_ARG;
    if (ps_set > F6222_CHn_SET_PS_SET_MAX) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    uint16_t readback = 0;
    uint8_t ch_idx = F6222_CH_TO_IDX(ch);

    /* READ: CHn_SET holds phase, gain, LNA_SW, and CH_PWD — read before patch. */
    st = f6222_local_reg_read(dev, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, &readback);
    if (st != F6222_OK) return st;

    /* Patch PS_SET [15:10] only; leave VGA_SET, LNA_SW, and CH_PWD unchanged. */
    readback = (readback & (~F6222_CHn_SET_PS_SET_MASK)) | ((uint16_t)ps_set << F6222_CHn_SET_PS_SET_SHIFT);

    /* WRITE: latch patched CHn_SET; rf_load controls buffer vs immediate RF update. */
    st = f6222_local_reg_write(dev, rf_load, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, readback);
    if (st != F6222_OK) return st;

    return F6222_OK;
}

f6222_status_t f6222_set_gain(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t vga_set) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;
    if (!F6222_CH_IS_VALID(ch)) return F6222_ERR_INVALID_ARG;
    if (vga_set > F6222_CHn_SET_VGA_SET_MAX) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    uint16_t readback = 0;
    uint8_t ch_idx = F6222_CH_TO_IDX(ch);

    /* READ: CHn_SET holds phase, gain, LNA_SW, and CH_PWD — read before patch. */
    st = f6222_local_reg_read(dev, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, &readback);
    if (st != F6222_OK) return st;

    /* Patch VGA_SET [9:4] only; leave PS_SET, LNA_SW, and CH_PWD unchanged. */
    readback = (readback & (~F6222_CHn_SET_VGA_SET_MASK)) | ((uint16_t)vga_set << F6222_CHn_SET_VGA_SET_SHIFT);

    /* WRITE: latch patched CHn_SET; rf_load controls buffer vs immediate RF update. */
    st = f6222_local_reg_write(dev, rf_load, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, readback);
    if (st != F6222_OK) return st;

    return F6222_OK;
}

f6222_status_t f6222_set_lna_sw(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, bool lna_sw_on) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;
    if (!F6222_CH_IS_VALID(ch)) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    uint16_t readback = 0;
    uint8_t ch_idx = F6222_CH_TO_IDX(ch);

    /* READ: CHn_SET holds phase, gain, LNA_SW, and CH_PWD — read before patch. */
    st = f6222_local_reg_read(dev, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, &readback);
    if (st != F6222_OK) return st;

    /* Patch LNA_SW [3] only; leave PS_SET, VGA_SET, and CH_PWD unchanged. */
    if (lna_sw_on) {
        readback |= F6222_CHn_SET_LNA_SW;
    } else {
        readback &= (uint16_t)(~F6222_CHn_SET_LNA_SW);
    }

    /* WRITE: latch patched CHn_SET; rf_load controls buffer vs immediate RF update. */
    st = f6222_local_reg_write(dev, rf_load, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, readback);
    if (st != F6222_OK) return st;

    return F6222_OK;
}

f6222_status_t f6222_set_channel_enable(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, bool enable) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE || chip_addr > F6222_CHIP_ADDR_MAX || !F6222_CH_IS_VALID(ch))
        return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    uint16_t val = 0;
    uint8_t reg = (uint8_t)(F6222_REG_CH1_SET + (F6222_CH_TO_IDX(ch) * F6222_CH_STRIDE));

    st = f6222_local_reg_read(dev, chip_addr, reg, &val);
    if (st != F6222_OK) return st;

    if (enable) {
        val &= (uint16_t)(~F6222_CHn_SET_CH_PWD);
    } else {
        val |= F6222_CHn_SET_CH_PWD;
    }

    return f6222_local_reg_write(dev, rf_load, chip_addr, reg, val);
}
