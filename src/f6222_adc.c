/*
 * f6222_adc.c — Temperature and PDET ADC (WIP)
 *
 * Reference: RENESAS-F6522-driver/src/f6522_adc.c
 */

#include "f6222.h"

f6222_status_t f6222_read_temp_raw(f6222_dev_t* dev, uint8_t chip_addr, uint16_t* raw) {
    if (dev == NULL || dev->spi_xfer == NULL || raw == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;

    (void)dev;
    (void)chip_addr;
    (void)raw;

    /* ── Stage 1: prepare temperature ADC ─────────────────────── */
    /* TODO: Step 1 — ADC_CTRL (0x06), TEMP=1 (F6222_ADC_CTRL_TEMP). */

    /* ── Stage 2: trigger ADC conversion ──────────────────────── */
    /* TODO: Step 2 — ADC_CTRL OSC_EN=1, TEMP=1 (F6222_ADC_CTRL_START_TEMP). */
    /* TODO: Step 3 — clear OSC_EN, keep TEMP=1 (F6222_ADC_CTRL_TEMP). */

    /* ── Stage 3: read measurement result ─────────────────────── */
    /* TODO: Step 4 — poll TEMP_DATA (0x0B) until ADC_DONE[10]=1; *raw = DATA[9:0]. */

    /* ── Stage 4: cleanup & restore defaults ──────────────────── */
    /* TODO: Step 5 — clear ADC_CTRL (F6222_ADC_CTRL_TYPICAL). */

    return F6222_OK;
}
