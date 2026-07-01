/*
 * f6222_init.c — Power-on register programming.
 *
 * Provenance: register order and values from the Renesas evaluation GUI power-on
 * SPI trace — Logic Analyzer capture exported as F6222_INIT.csv (76 writes).
 * Not an independent translation of datasheet prose.
 *
 * f6222_init() sequence: scratch_test → set_init_pattern (76 writes)
 * → f6222_set_channel_enable(false) for CH1–CH8.
 */

#include "f6222.h"

/* 0x2474 / 0x9879 = TMYtek company ID 24749879, split into two 16-bit test values */
static const uint16_t f6222_scratch_patterns[] = {
    0x2474u, 0x9879u, 0x8964u, 0x55AAu, 0xF0F0u,
};

f6222_status_t f6222_scratch_test(f6222_dev_t* dev, uint8_t chip_addr) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    uint16_t readback = 0;
    size_t i;

    for (i = 0; i < sizeof(f6222_scratch_patterns) / sizeof(f6222_scratch_patterns[0]); i++) {
        uint16_t pattern = f6222_scratch_patterns[i];

        st = f6222_reg_write(dev, F6222_RF_LOAD_IMMEDIATE, chip_addr, F6222_REG_SCRATCH, pattern);
        if (st != F6222_OK) return st;

        st = f6222_reg_read(dev, chip_addr, F6222_REG_SCRATCH, &readback);
        if (st != F6222_OK) return st;

        if (readback != pattern) return F6222_ERR_SCRATCH_MISMATCH;
    }

    return F6222_OK;
}

typedef struct {
    uint8_t reg;
    uint16_t val;
} f6222_init_entry_t;

/* Source: Renesas official evaluation GUI init sequence, LA capture (F6222_INIT.csv).
 * 76 local register writes, RF Load = immediate. Not derived from datasheet prose. */
static const f6222_init_entry_t f6222_init_pattern[] = {
    {0x00u, 0x0010u},                   /* CTRL_CFG */
    {0x06u, 0x0000u},                   /* ADC_CTRL */
    {0x0Au, 0x0003u},                   /* ADC_TEST */
    {0x05u, 0x0B30u},                   /* CLK_CTRL */
    {0x64u, 0x0000u}, {0x01u, 0x0C82u}, /* PTAT_BIAS_CFG */
    {0x09u, 0x0000u}, {0x07u, 0x0000u}, {0x20u, 0x6CDBu}, {0x21u, 0x2FFFu},
    {0x22u, 0x03F9u}, {0x23u, 0x0000u},                                     /* CH1 */
    {0x24u, 0x6CDBu}, {0x25u, 0x2FFFu}, {0x26u, 0x03F9u}, {0x27u, 0x0000u}, /* CH2 */
    {0x28u, 0x6CDBu}, {0x29u, 0x2FFFu}, {0x2Au, 0x03F9u}, {0x2Bu, 0x0000u}, /* CH3 */
    {0x2Cu, 0x6CDBu}, {0x2Du, 0x2FFFu}, {0x2Eu, 0x03F9u}, {0x2Fu, 0x0000u}, /* CH4 */
    {0x30u, 0x6CDBu}, {0x31u, 0x2FFFu}, {0x32u, 0x03F9u}, {0x33u, 0x0000u}, /* CH5 */
    {0x34u, 0x6CDBu}, {0x35u, 0x2FFFu}, {0x36u, 0x03F9u}, {0x37u, 0x0000u}, /* CH6 */
    {0x38u, 0x6CDBu}, {0x39u, 0x2FFFu}, {0x3Au, 0x03F9u}, {0x3Bu, 0x0000u}, /* CH7 */
    {0x3Cu, 0x6CDBu}, {0x3Du, 0x2FFFu}, {0x3Eu, 0x03F9u}, {0x3Fu, 0x0000u}, /* CH8 */
    {0x40u, 0x6CDBu}, {0x41u, 0x2FFFu}, {0x42u, 0x03F9u}, {0x43u, 0x0000u}, /* CH9 */
    {0x44u, 0x6CDBu}, {0x45u, 0x2FFFu}, {0x46u, 0x03F9u}, {0x47u, 0x0000u}, /* CH10 */
    {0x48u, 0x6CDBu}, {0x49u, 0x2FFFu}, {0x4Au, 0x03F9u}, {0x4Bu, 0x0000u}, /* CH11 */
    {0x4Cu, 0x6CDBu}, {0x4Du, 0x2FFFu}, {0x4Eu, 0x03F9u}, {0x4Fu, 0x0000u}, /* CH12 */
    {0x50u, 0x6CDBu}, {0x51u, 0x2FFFu}, {0x52u, 0x03F9u}, {0x53u, 0x0000u}, /* CH13 */
    {0x54u, 0x6CDBu}, {0x55u, 0x2FFFu}, {0x56u, 0x03F9u}, {0x57u, 0x0000u}, /* CH14 */
    {0x58u, 0x6CDBu}, {0x59u, 0x2FFFu}, {0x5Au, 0x03F9u}, {0x5Bu, 0x0000u}, /* CH15 */
    {0x5Cu, 0x6CDBu}, {0x5Du, 0x2FFFu}, {0x5Eu, 0x03F9u}, {0x5Fu, 0x0000u}, /* CH16 */
    {0x69u, 0x0007u}, {0x6Au, 0x0087u}, {0x6Bu, 0x0007u}, {0x6Cu, 0x0007u}, /* LNAIREF */
};

#define F6222_INIT_PATTERN_COUNT (sizeof(f6222_init_pattern) / sizeof(f6222_init_pattern[0]))

static f6222_status_t f6222_set_init_pattern(f6222_dev_t* dev, uint8_t chip_addr) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;
    size_t i;

    for (i = 0; i < F6222_INIT_PATTERN_COUNT; i++) {
        st = f6222_reg_write(dev, F6222_RF_LOAD_IMMEDIATE, chip_addr, f6222_init_pattern[i].reg,
                             f6222_init_pattern[i].val);
        if (st != F6222_OK) return st;
    }

    return F6222_OK;
}

f6222_status_t f6222_init(f6222_dev_t* dev, uint8_t chip_addr) {
    if (dev == NULL || dev->spi_xfer == NULL) return F6222_ERR_INVALID_ARG;
    if (chip_addr > F6222_CHIP_ADDR_MAX) return F6222_ERR_INVALID_ARG;

    f6222_status_t st;

    st = f6222_scratch_test(dev, chip_addr);
    if (st != F6222_OK) return st;

    st = f6222_set_init_pattern(dev, chip_addr);
    if (st != F6222_OK) return st;

    for (uint8_t ch = F6222_CH_MIN; ch <= F6222_CH_MAX; ch++) {
        st = f6222_set_channel_enable(dev, F6222_RF_LOAD_BUFFER, chip_addr, ch, false);
        if (st != F6222_OK) return st;
    }

    return F6222_OK;
}
