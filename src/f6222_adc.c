/*
 * f6222_adc.c — Temperature ADC
 *
 * Temp read SPI flow from Renesas evaluation GUI (LA capture):
 *   0x2800060400 → 0x2800060500 → 0x00000B0000 (poll until ADC_DONE)
 */

#include "f6222.h"

f6222_status_t f6222_read_temp_raw(f6222_dev_t* dev, uint8_t chip_addr, uint16_t* raw) {
    if (dev == NULL || dev->spi_xfer == NULL || raw == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    f6222_status_t restore_st;

    /* Step 1: PREPARE_OSC (0x0400). Not in TEMP mode yet — on failure just
     * return; ADC_CTRL has not left its default, so no restore is needed. */
    st = f6222_local_reg_write(dev, F6222_RF_LOAD_IMMEDIATE, chip_addr, F6222_REG_ADC_CTRL, F6222_ADC_CTRL_PREPARE_OSC);
    if (st != F6222_OK) return st;

    /* From here ADC_CTRL has left TYPICAL — every failure path must restore. */
    /* Step 2: START_TEMP (0x0500). */
    st = f6222_local_reg_write(dev, F6222_RF_LOAD_IMMEDIATE, chip_addr, F6222_REG_ADC_CTRL, F6222_ADC_CTRL_START_TEMP);
    if (st != F6222_OK) goto restore;

    /* Step 3: read TEMP_DATA (0x0B) until ADC_DONE[10]=1 (GUI frame 0x00000B0000). */
    {
        uint16_t result = 0;
        uint32_t polls = 0;

        while (!(result & F6222_TEMP_DATA_ADC_DONE)) {
            if (polls >= F6222_TEMP_DATA_ADC_POLL_MAX) {
                st = F6222_ERR_ADC_TIMEOUT;
                goto restore;
            }
            st = f6222_local_reg_read(dev, chip_addr, F6222_REG_TEMP_DATA, &result);
            if (st != F6222_OK) goto restore;
            polls++;
        }
        *raw = result & F6222_TEMP_DATA_DATA_MASK;
    }

restore:
    /* Step 4: always attempt to restore ADC_CTRL to its default. */
    restore_st =
        f6222_local_reg_write(dev, F6222_RF_LOAD_IMMEDIATE, chip_addr, F6222_REG_ADC_CTRL, F6222_ADC_CTRL_TYPICAL);
    /* Let the restore error surface only if the main flow succeeded;
     * otherwise keep the original error code. */
    if (st == F6222_OK) st = restore_st;
    return st;
}

f6222_status_t f6222_read_temp(f6222_dev_t* dev, uint8_t chip_addr, uint16_t* raw, float* temp_c) {
    if (raw == NULL || temp_c == NULL) return F6222_ERR_INVALID_ARG;

    f6222_status_t st = f6222_read_temp_raw(dev, chip_addr, raw);
    if (st != F6222_OK) return st;

    /* Cast each operand before subtracting: (*raw - F6222_TEMP_C0) would be
     * computed in unsigned arithmetic and wrap when raw < C0 (below ~T0). */
    *temp_c = (((float)*raw - (float)F6222_TEMP_C0) / F6222_TEMP_SLOPE) + F6222_TEMP_T0_C;
    return F6222_OK;
}
