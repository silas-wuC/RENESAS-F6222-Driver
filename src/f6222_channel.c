/*
 * f6222_channel.c — Per-channel enable control
 */

#include "f6222.h"

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
