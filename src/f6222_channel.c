/*
 * f6222_channel.c — Per-channel phase, gain, LNA bypass, enable (Step 3)
 */

#include "f6222.h"

/*
 * Read-modify-write one field of CHn_SET.
 *
 * READ: CHn_SET holds phase, gain, LNA_SW, and CH_PWD — read before patch.
 * Patch: clear `clear_mask`, OR in `set_bits`; other fields unchanged.
 * WRITE: latch patched CHn_SET; rf_load controls buffer vs immediate RF update.
 *
 * Common argument validation lives here so it always runs before any SPI
 * traffic; callers only validate their own field range.
 */
static f6222_status_t f6222_ch_set_update(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch,
                                          uint16_t clear_mask, uint16_t set_bits) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;
    if (!F6222_CH_IS_VALID(ch)) return F6222_ERR_INVALID_ARG;
    if (rf_load > F6222_RF_LOAD_IMMEDIATE) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    uint16_t readback = 0;
    uint8_t ch_idx = F6222_CH_TO_IDX(ch);

    st = f6222_local_reg_read(dev, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, &readback);
    if (st != F6222_OK) return st;

    readback = (uint16_t)((readback & (uint16_t)(~clear_mask)) | set_bits);

    return f6222_local_reg_write(dev, rf_load, chip_addr, f6222_ch_reg_map[ch_idx].ch_set, readback);
}

f6222_status_t f6222_set_phase(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t ps_step) {
    if (ps_step > F6222_CHn_SET_PS_SET_MAX) return F6222_ERR_INVALID_ARG;

    /* Patch PS_SET [15:10] only; leave VGA_SET, LNA_SW, and CH_PWD unchanged. */
    return f6222_ch_set_update(dev, rf_load, chip_addr, ch, F6222_CHn_SET_PS_SET_MASK,
                               (uint16_t)((uint16_t)ps_step << F6222_CHn_SET_PS_SET_SHIFT));
}

f6222_status_t f6222_set_gain(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t vga_step) {
    if (vga_step > F6222_CHn_SET_VGA_SET_MAX) return F6222_ERR_INVALID_ARG;

    /* Patch VGA_SET [9:4] only; leave PS_SET, LNA_SW, and CH_PWD unchanged. */
    return f6222_ch_set_update(dev, rf_load, chip_addr, ch, F6222_CHn_SET_VGA_SET_MASK,
                               (uint16_t)((uint16_t)vga_step << F6222_CHn_SET_VGA_SET_SHIFT));
}

f6222_status_t f6222_set_lna_sw(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, bool lna_sw_on) {
    /* Patch LNA_SW [3] only; leave PS_SET, VGA_SET, and CH_PWD unchanged. */
    return f6222_ch_set_update(dev, rf_load, chip_addr, ch, F6222_CHn_SET_LNA_SW,
                               lna_sw_on ? (uint16_t)F6222_CHn_SET_LNA_SW : 0u);
}

f6222_status_t f6222_set_channel_enable(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, bool enable) {
    /* Patch CH_PWD [0] only; leave PS_SET, VGA_SET, and LNA_SW unchanged. */
    return f6222_ch_set_update(dev, rf_load, chip_addr, ch, F6222_CHn_SET_CH_PWD,
                               enable ? 0u : (uint16_t)F6222_CHn_SET_CH_PWD);
}

f6222_status_t f6222_apply_rf(f6222_dev_t* dev, uint8_t chip_addr) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;

    st = f6222_local_reg_write(dev, F6222_RF_LOAD_IMMEDIATE, chip_addr, F6222_REG_SCRATCH, F6222_SCRATCH_RESET);
    if (st != F6222_OK) return st;

    return F6222_OK;
}
