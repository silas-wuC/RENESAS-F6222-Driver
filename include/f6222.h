/*
 * f6222.h — Renesas F6222 16-channel Ka-band Rx Beamforming IC driver
 *
 * Platform-agnostic: no OS, no stdlib, no SPI implementation.
 * The caller supplies a single spi_xfer() callback; everything else is
 * handled here in pure C99.
 *
 * Datasheet: R35DS0065EU0100 Rev.1.00 (2025-01-27)
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Naming convention (R35DS0065EU0100 §9):
 *
 *   F6222_REG_<RegisterName>              register address
 *   F6222_<RegisterName>_<FieldName>      field bit or composite value
 *   F6222_<RegisterName>_<FieldName>_SHIFT / _MASK
 *   F6222_<RegisterName>_RESET            full-register reset value
 *   F6222_<RegisterName>_TYPICAL          full-register typical value
 *
 * Per-channel registers use literal CHn: F6222_CHn_SET_PS_SET, etc.
 * C macros uppercase datasheet mixed-case fields (SA_Index → SA_INDEX).
 */

/* ═══════════════════════════════════════════════════════════════
 * Register Addresses
 * ═══════════════════════════════════════════════════════════════ */

#define F6222_REG_CTRL_CFG 0x00u      /* global power-down, sub-array index */
#define F6222_REG_PTAT_BIAS_CFG 0x01u /* master bias + temp compensation    */
#define F6222_REG_SCRATCH 0x02u       /* scratch (read-back test)           */
#define F6222_REG_MO_MEM_ACT 0x03u    /* active mode / LUT state (read-only)*/
#define F6222_REG_SILICON_ID 0x04u    /* readiness / device ID (GUI only)   */
#define F6222_REG_CLK_CTRL 0x05u      /* ADC sample clock divider           */
#define F6222_REG_ADC_CTRL 0x06u      /* ADC trigger + oscillator enable    */
#define F6222_REG_ADC_TEST 0x0Au      /* ADC chopper / bias (typical 0x0003)*/
#define F6222_REG_TEMP_DATA 0x0Bu     /* temperature ADC result (read-only) */

/* Per-channel register base addresses (stride = 4 between channels) */
#define F6222_REG_CH1_BIAS 0x20u /* reference bias levels per channel  */
#define F6222_REG_CH1_CTRL 0x21u /* component enable / coarse settings */
#define F6222_REG_CH1_SET 0x22u  /* phase, gain, LNA bypass, power-down */

#define F6222_REG_LNAIREF1 0x69u /* LNA IDAC1 bias (controls all 4 outputs) */
#define F6222_REG_LNAIREF2 0x6Au /* LNA IDAC2 bias + multiplier            */
#define F6222_REG_LNAIREF3 0x6Bu /* LNA IDAC3 bias                         */
#define F6222_REG_LNAIREF4 0x6Cu /* LNA IDAC4 bias                         */

/* Stride between consecutive channel register blocks (BIAS/CTRL/SET) */
#define F6222_CH_STRIDE 0x04u

/* Per-channel offsets within the 4-byte channel block */
#define F6222_CH_OFF_BIAS 0u
#define F6222_CH_OFF_CTRL 1u
#define F6222_CH_OFF_SET 2u

/* ═══════════════════════════════════════════════════════════════
 * Register Field Masks & Shifts
 * ═══════════════════════════════════════════════════════════════ */

/* §9.2.1 Register Name: CTRL_CFG — reset 0x0010, typical 0x0000 */
#define F6222_CTRL_CFG_GLOBAL_PWD (1u << 4)
#define F6222_CTRL_CFG_SCAN_MODE (1u << 3)
#define F6222_CTRL_CFG_SA_INDEX_MASK 0x0007u
#define F6222_CTRL_CFG_RESET 0x0010u
#define F6222_CTRL_CFG_TYPICAL 0x0000u

/* §9.2.2 Register Name: PTAT_BIAS_CFG — reset 0x0080, typical 0x088A
 *
 *  PTAT mode  (7.4 dB gain flatness vs temp):  MB_EN=1, MB_PT2_EN=0 → 0x0088
 *  PTAT2 mode (1.5 dB gain flatness vs temp):  MB_EN=1, MB_PT2_EN=1, MB_PT2_SLOPE=4 → 0x0788
 */
#define F6222_PTAT_BIAS_CFG_MB_PT2_SLOPE_SHIFT 9u
#define F6222_PTAT_BIAS_CFG_MB_PT2_SLOPE_MASK (0x07u << 9)
#define F6222_PTAT_BIAS_CFG_MB_PT2_EN (1u << 8)
#define F6222_PTAT_BIAS_CFG_MB_PT_ADJ_SHIFT 5u
#define F6222_PTAT_BIAS_CFG_MB_PT_ADJ_MASK (0x07u << 5)
#define F6222_PTAT_BIAS_CFG_MB_EN (1u << 3)
#define F6222_PTAT_BIAS_CFG_MB_BG_SEL (1u << 1)
#define F6222_PTAT_BIAS_CFG_MB_R_SEL (1u << 0)
#define F6222_PTAT_BIAS_CFG_RESET 0x0080u
#define F6222_PTAT_BIAS_CFG_TYPICAL 0x088Au
#define F6222_PTAT_BIAS_CFG_TYPICAL_PTAT2 0x0788u

/* §9.2.3 Register Name: SCRATCH — reset 0x0000 */
#define F6222_SCRATCH_RESET 0x0000u

/* §9.2.4 Register Name: MO_MEM_ACT — reset 0x0000 */
#define F6222_MO_MEM_ACT_ACTIVE_MODE_SHIFT 13u
#define F6222_MO_MEM_ACT_ACTIVE_MODE_MASK (0x07u << 13)
#define F6222_MO_MEM_ACT_ACTIVE_LUT_STATE_SHIFT 5u
#define F6222_MO_MEM_ACT_ACTIVE_LUT_STATE_MASK (0xFFu << 5)
#define F6222_MO_MEM_ACT_RESET 0x0000u

/* §9.2.5 Register Name: CLK_CTRL — reset 0x0B30, typical 0x0B30 */
#define F6222_CLK_CTRL_BASE_CLK_CTRL_SHIFT 8u
#define F6222_CLK_CTRL_BASE_CLK_CTRL_MASK (0x0Fu << 8)
#define F6222_CLK_CTRL_ADC_CLK_HIGH_SHIFT 4u
#define F6222_CLK_CTRL_ADC_CLK_HIGH_MASK (0x0Fu << 4)
#define F6222_CLK_CTRL_ADC_CLK_LOW_SHIFT 0u
#define F6222_CLK_CTRL_ADC_CLK_LOW_MASK 0x000Fu
#define F6222_CLK_CTRL_RESET 0x0B30u
#define F6222_CLK_CTRL_TYPICAL 0x0B30u

/* §9.2.6 Register Name: ADC_CTRL — reset 0x0000, typical 0x0000 */
#define F6222_ADC_CTRL_OSC_FREQ_SHIFT 11u
#define F6222_ADC_CTRL_OSC_FREQ_MASK (0x03u << 11)
#define F6222_ADC_CTRL_OSC_EN (1u << 10)
#define F6222_ADC_CTRL_TEST_MUX (1u << 9)
#define F6222_ADC_CTRL_TEMP (1u << 8)
#define F6222_ADC_CTRL_RESET 0x0000u
#define F6222_ADC_CTRL_TYPICAL 0x0000u
#define F6222_ADC_CTRL_PREPARE_OSC F6222_ADC_CTRL_OSC_EN                        /* 0x0400 — GUI temp read step 1 */
#define F6222_ADC_CTRL_START_TEMP (F6222_ADC_CTRL_TEMP | F6222_ADC_CTRL_OSC_EN) /* 0x0500 */

/* §9.2.7 Register Name: ADC_TEST — reset 0x0000, typical 0x0003 */
#define F6222_ADC_TEST_CHOPPER_EN (1u << 1)
#define F6222_ADC_TEST_ADC_I_X2 (1u << 0)
#define F6222_ADC_TEST_RESET 0x0000u
#define F6222_ADC_TEST_TYPICAL 0x0003u

/* §9.2.8 Register Name: TEMP_DATA — reset 0x0000 */
#define F6222_TEMP_DATA_ADC_DONE (1u << 10)
#define F6222_TEMP_DATA_DATA_MASK 0x03FFu
#define F6222_TEMP_DATA_ADC_POLL_MAX 1000u

/* §9.2.9 Register Name: CHn_BIAS — reset 0x70DB, typical 0x6CDB */
#define F6222_CHn_BIAS_LNA_BIAS_SHIFT 13u
#define F6222_CHn_BIAS_LNA_BIAS_MASK (0x07u << 13)
#define F6222_CHn_BIAS_VGA_BIAS_SHIFT 9u
#define F6222_CHn_BIAS_VGA_BIAS_MASK (0x0Fu << 9)
#define F6222_CHn_BIAS_PS_BIAS_SHIFT 6u
#define F6222_CHn_BIAS_PS_BIAS_MASK (0x07u << 6)
#define F6222_CHn_BIAS_ATT_BIAS_SHIFT 3u
#define F6222_CHn_BIAS_ATT_BIAS_MASK (0x07u << 3)
#define F6222_CHn_BIAS_CH_BIAS_SHIFT 0u
#define F6222_CHn_BIAS_CH_BIAS_MASK 0x07u
#define F6222_CHn_BIAS_RESET 0x70DBu
#define F6222_CHn_BIAS_TYPICAL 0x6CDBu

/* §9.2.10 Register Name: CHn_CTRL — reset 0x20FF, typical 0x2FFF */
#define F6222_CHn_CTRL_ATT_BITS_SHIFT 10u
#define F6222_CHn_CTRL_ATT_BITS_MASK (0x07u << 10)
#define F6222_CHn_CTRL_PS_PHT_SHIFT 8u
#define F6222_CHn_CTRL_PS_PHT_MASK (0x03u << 8)
#define F6222_CHn_CTRL_CH_MODE (1u << 7)
#define F6222_CHn_CTRL_LNA_MODE (1u << 6)
#define F6222_CHn_CTRL_PS_MODE (1u << 5)
#define F6222_CHn_CTRL_ATT_MODE (1u << 4)
#define F6222_CHn_CTRL_LNA_EN (1u << 3)
#define F6222_CHn_CTRL_VGA_EN (1u << 2)
#define F6222_CHn_CTRL_PS_EN (1u << 1)
#define F6222_CHn_CTRL_ATT_EN (1u << 0)
#define F6222_CHn_CTRL_RESET 0x20FFu
#define F6222_CHn_CTRL_TYPICAL 0x2FFFu

/* §9.2.11 Register Name: CHn_SET — reset 0x03F9, typical 0x03F8
 *
 *  [15:10] PS_SET  — 6-bit phase (0–63, step ≈ 5.6°)
 *  [9:4]   VGA_SET — 6-bit gain (0–31 lower, 32–63 upper; step ≈ 0.45 dB)
 *  [3]     LNA_SW  — 1 = DA enabled, 0 = DA bypassed
 *  [0]     CH_PWD  — 1 = channel powered down
 */
#define F6222_CHn_SET_PS_SET_SHIFT 10u
#define F6222_CHn_SET_PS_SET_MASK (0x3Fu << 10)
#define F6222_CHn_SET_PS_SET_MAX 63u
#define F6222_CHn_SET_VGA_SET_SHIFT 4u
#define F6222_CHn_SET_VGA_SET_MASK (0x3Fu << 4)
#define F6222_CHn_SET_VGA_SET_MAX 63u
#define F6222_CHn_SET_LNA_SW (1u << 3)
#define F6222_CHn_SET_CH_PWD (1u << 0)
#define F6222_CHn_SET_RESET 0x03F9u
#define F6222_CHn_SET_TYPICAL 0x03F8u

/* §9.2.12 Register Name: LNAIREFn */
#define F6222_LNAIREF1_LNA_IREF_SHUTDOWN (1u << 5)
#define F6222_LNAIREF1_LNA_IREF1_CONT_MASK 0x001Fu
#define F6222_LNAIREF1_RESET 0x0040u

#define F6222_LNAIREF2_LNA_IREF_MULT_SHIFT 5u
#define F6222_LNAIREF2_LNA_IREF_MULT_MASK (0x07u << 5)
#define F6222_LNAIREF2_LNA_IREF2_CONT_MASK 0x001Fu
#define F6222_LNAIREF2_RESET 0x0020u

#define F6222_LNAIREF3_LNA_IREF3_CONT_MASK 0x001Fu
#define F6222_LNAIREF3_RESET 0x0000u

#define F6222_LNAIREF4_LNA_IREF4_CONT_MASK 0x001Fu
#define F6222_LNAIREF4_RESET 0x0000u

/* ═══════════════════════════════════════════════════════════════
 * §8 SPI Protocol — Mode Selection M[2:0] (Table 5, Table 6)
 *
 * Table 5 — Read/Write Modes:
 *
 *   M[2:0]  Mode Description         R/W    Memory Location  Scope   Cmd (bits)
 *   ──────  ───────────────────────  ─────  ───────────────  ──────  ──────────
 *   001     Local Register Write     Write  Static Registers Local   40
 *   000     Local Register Read      Read   Static Registers Local   24
 *   110     Local LUT Write          Write  LUT              Local   40
 *   111     Local LUT Read           Read   LUT              Local   24
 *   011     Global Register Write    Write  Static Registers Global  32
 *   010     Global LUT Write         Write  LUT              Global  32
 *
 * M[2:0] in SPI command byte 0, bits [7:5].  Read modes clock 16-bit
 * data after the command word (driver uses 40-bit SPI exchange).
 * ═══════════════════════════════════════════════════════════════ */

#define F6222_SPI_M_LOCAL_REG_WRITE 0x01u  /* 001 — Local Register Write, 40-bit */
#define F6222_SPI_M_LOCAL_REG_READ 0x00u   /* 000 — Local Register Read, 24-bit  */
#define F6222_SPI_M_LOCAL_LUT_WRITE 0x06u  /* 110 — Local LUT Write, 40-bit      */
#define F6222_SPI_M_LOCAL_LUT_READ 0x07u   /* 111 — Local LUT Read, 24-bit       */
#define F6222_SPI_M_GLOBAL_REG_WRITE 0x03u /* 011 — Global Register Write, 32-bit*/
#define F6222_SPI_M_GLOBAL_LUT_WRITE 0x02u /* 010 — Global LUT Write, 32-bit     */
#define F6222_SPI_M_GLOBAL_FBS 0x04u       /* 100 — Global FBS (Table 6)         */
#define F6222_SPI_M_LOCAL_FBS 0x05u        /* 101 — Local FBS (Table 6)          */

/* RF Load (RL) — Local Register Write / FBS commands (Table 7, Table 11) */
#define F6222_SPI_RF_LOAD_00 0x00u /* buffer; latch on subsequent RL=01 */
#define F6222_SPI_RF_LOAD_01 0x01u /* immediate RF channel update */

/* Global FBS command fields (Table 14) */
#define F6222_SPI_TE_SHIFT 12u
#define F6222_SPI_TE_MASK (1u << 12)
#define F6222_SPI_SE_SHIFT 11u
#define F6222_SPI_SE_MASK (1u << 11)
#define F6222_SPI_SA_INDEX_SHIFT 8u
#define F6222_SPI_SA_INDEX_MASK (0x07u << 8)
#define F6222_SPI_LA_SHIFT 1u
#define F6222_SPI_LA_MASK (0x7Fu << 1)

/* Chip address ADD[4:0] encoded in SPI command word [28:24] / [12:8] */
#define F6222_SPI_ADD_SHIFT_LOCAL_WRITE 24u
#define F6222_SPI_ADD_SHIFT_LOCAL_READ 8u
#define F6222_SPI_ADD_MASK 0x1Fu

/* 7-bit register address RA[6:0]; bit7 reserved */
#define F6222_SPI_REG_ADDR_MASK 0x7Fu
/* 7-bit LUT address LA[6:0]; LA[7] must be 0 (Table 9/10) */
#define F6222_SPI_LUT_ADDR_MASK 0x7Fu

/* 16-bit payload D[15:0] in SPI command word */
#define F6222_SPI_DATA_HIGH_SHIFT 8u
#define F6222_SPI_DATA_LOW_MASK 0xFFu /* D[7:0] */

#define F6222_SPI_PAD_BYTE 0x00u /* MOSI padding during read/data clock-out */

/* Global LUT Write (§8.7) — LM bit in command byte 0 */
#define F6222_SPI_LM_SHIFT 4u

/* ═══════════════════════════════════════════════════════════════
 * Constants
 * ═══════════════════════════════════════════════════════════════ */

#define F6222_NUM_CHANNELS 16u /* RF channels: CH1 … CH16         */
#define F6222_NUM_RF_INPUTS 8u /* RF input ports: RF1 … RF8       */
#define F6222_NUM_BEAMS 2u     /* RFC1 (odd chs), RFC2 (even chs) */
#define F6222_CH_MIN 1u        /* API channel number (datasheet 1-based) */
#define F6222_CH_MAX F6222_NUM_CHANNELS
#define F6222_CH_IS_VALID(ch) ((ch) >= F6222_CH_MIN && (ch) <= F6222_CH_MAX)
#define F6222_CH_TO_IDX(ch) ((uint8_t)((ch) - F6222_CH_MIN))
#define F6222_LUT_ENTRIES 128u  /* fast-beam-steering LUT depth     */
#define F6222_CHIP_ADDR_MAX 31u /* ADD1–ADD5 strap pins (ADD[4:0], 0–31) */
#define F6222_CHIP_ADDR_MASK 0x1Fu

/* RF input port → first channel driven (each RF port drives 2 channels) */
#define F6222_RF_TO_CH1(rf) ((uint8_t)(((rf) - 1u) * 2u + 1u)) /* rf = 1..8 */

/* Beam assignment: odd channels → RFC1 (beam 1), even → RFC2 (beam 2) */
#define F6222_CH_BEAM(ch) (((ch) & 1u) ? 1u : 2u)
#define F6222_CH_IS_ODD(ch) ((ch) & 1u)

/* Phase / gain step sizes (datasheet typical) */
#define F6222_PHASE_STEP_DEG_x10 56u /* 5.6° × 10 for integer math */
#define F6222_GAIN_STEP_DB_x100 45u  /* 0.45 dB × 100               */
#define F6222_GAIN_RANGE_DB_x10 284u /* 28.4 dB × 10                */

/**
 * f6222_ch_regs_t — register addresses for one RF channel.
 *
 * Fields are listed in ascending SPI address order (matches datasheet).
 * Use f6222_ch_reg_map[F6222_CH_TO_IDX(ch)] to look up a channel's addresses (ch = 1..16).
 */
typedef struct {
    uint8_t ch_bias; /* CHn_BIAS */
    uint8_t ch_ctrl; /* CHn_CTRL */
    uint8_t ch_set;  /* CHn_SET  */
} f6222_ch_regs_t;

/* Register address table; array index 0..15 maps to API ch 1..16 (CH1..CH16). */
extern const f6222_ch_regs_t f6222_ch_reg_map[F6222_NUM_CHANNELS];

/* Silicon ID / power-on ready polling (reg 0x04 — absent from datasheet map) */
#define F6222_SILICON_ID 0x0952u     /* F6222 identity; GUI expects this */
#define F6222_READY_POLL_MAX 500u    /* max reads while waiting for ready */
#define F6222_READY_CONFIRM_READS 5u /* GUI requires this many consecutive 0x0952 reads */

/**
 * f6222_status_t — return codes for all public driver APIs.
 *
 * spi_xfer() callbacks still return int; the driver maps negative values
 * to F6222_ERR_SPI before returning f6222_status_t to the caller.
 */
typedef enum {
    F6222_OK = 0,
    F6222_ERR_INVALID_ARG = -1,
    F6222_ERR_SPI = -2,
    F6222_ERR_SCRATCH_MISMATCH = -3,
    F6222_ERR_ADC_TIMEOUT = -4,
    F6222_ERR_READY_TIMEOUT = -5,
    F6222_ERR_SILICON_ID = -6,
} f6222_status_t;

/* RF Load field values (used in Local Register Write and FBS commands).
 * BUFFER:    writes CHn_SET to an internal buffer only; RF output stays unchanged
 *            until f6222_apply_rf() broadcasts RF_TRIG to latch all buffered settings.
 * IMMEDIATE: applies the write to RF output right away (also triggers a bus-wide update). */
#define F6222_RF_LOAD_BUFFER 0x00u
#define F6222_RF_LOAD_IMMEDIATE 0x01u

/*
 * Temperature sensor linear model (datasheet §6.5):
 *   code = 1.2 * (T − T0) + C0
 *   T    = (code − C0) / 1.2 + T0
 * C0 must be calibrated at a known T0 (single-point, device in standby).
 */
#define F6222_TEMP_T0_C 25.0f /* calibration reference temperature (°C) */
#define F6222_TEMP_C0 405u    /* ADC code at T0; update after single-point cal */
#define F6222_TEMP_SLOPE 1.2f /* LSB/°C, measured */

/* ═══════════════════════════════════════════════════════════════
 * Hardware Abstraction Layer
 * ═══════════════════════════════════════════════════════════════
 *
 * The driver never calls SPI or GPIO directly.  The caller fills
 * f6222_dev_t with one required callback (spi_xfer); chip_addr is
 * passed per API call (0–31).
 *
 * HOW TO PORT TO YOUR MCU:
 *
 *   1. Write a function matching the spi_xfer signature below.
 *      It must: assert CSB low, clock out `len` bytes (CPOL=0,
 *      CPHA=0, MSB first, up to 50 MHz), capture MISO into rx[]
 *      if rx != NULL, then deassert CSB high.
 *
 *   2. Optionally assert/deassert RST before calling f6222_init()
 *      (assert RST low, delay ≥1 µs, deassert high).  RST is not
 *      part of f6222_dev_t — handle it in your board support code.
 *
 *   3. Tie MODE pin: low = single-beam (SB), high = dual-beam (DB).
 *
 *   4. Fill an f6222_dev_t on the stack or in BSS, then call
 *      f6222_init().
 */

typedef struct {
    /**
     * SPI transfer.
     *
     * @param ctx  Opaque pointer from f6222_dev_t.ctx (your SPI handle, etc.)
     * @param tx   Bytes to clock out on MOSI — always non-NULL.
     * @param rx   Buffer for MISO bytes; pass NULL when read-back not needed.
     * @param len  Number of bytes in the transaction.
     * @return     0 on success, negative value on error.
     *
     * You are responsible for driving CSB low before the first clock and
     * high after the last clock.  SCLK idles low (CPOL=0, CPHA=0).
     */
    int (*spi_xfer)(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len);

    void* ctx; /* passed verbatim as the first arg to spi_xfer */
} f6222_dev_t;

/* ═══════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════ */

/* ── Initialisation ─────────────────────────────────────────── */

/**
 * f6222_wait_ready() — poll reg 0x04 until the chip reports 0x0952.
 *
 * Mirrors the evaluation GUI handshake: requires F6222_READY_CONFIRM_READS
 * consecutive reads of 0x0952 on reg 0x04.  The datasheet does not document
 * this sequence; it waits for LDO / oscillator startup and confirms the
 * F6222 Silicon ID.
 *
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 * @return  F6222_OK on success,
 *          F6222_ERR_READY_TIMEOUT if 0x0952 never appears,
 *          F6222_ERR_SILICON_ID if the ID was seen but not confirmed
 *          consecutively,
 *          F6222_ERR_SPI if an underlying SPI transfer fails, or
 *          F6222_ERR_INVALID_ARG if dev is NULL or chip_addr is out of range.
 */
f6222_status_t f6222_wait_ready(f6222_dev_t* dev, uint8_t chip_addr);

/**
 * f6222_init() — post-reset register programming to typical operating state.
 *
 * Calls f6222_wait_ready(), f6222_scratch_test(), then replays 76 SPI writes
 * from the Renesas evaluation GUI Logic Analyzer trace (F6222_INIT.csv).
 * Leaves GLOBAL_PWD set and all channels disabled (CH_PWD=1).  Caller must
 * clear GLOBAL_PWD and enable desired channels before RF use.
 *
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 * @return  F6222_OK on success, or an error code from f6222_wait_ready(),
 *          f6222_scratch_test(), or the init pattern loop.
 */
f6222_status_t f6222_init(f6222_dev_t* dev, uint8_t chip_addr);

/**
 * f6222_scratch_test() — write/read-verify SCRATCH (0x02) with a built-in pattern list.
 */
f6222_status_t f6222_scratch_test(f6222_dev_t* dev, uint8_t chip_addr);

/* ── SPI Read/Write Modes (Table 5) ─────────────────────────── */

/**
 * f6222_local_reg_write() — Local Register Write, Mode 001 (40-bit frame).
 *
 * @param rf_load    F6222_SPI_RF_LOAD_00 (buffer) or F6222_SPI_RF_LOAD_01 (immediate).
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 * @param reg        7-bit register address.
 * @param val        16-bit data to write.
 */
f6222_status_t f6222_local_reg_write(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t reg, uint16_t val);

/**
 * f6222_local_reg_read() — Local Register Read, Mode 000.
 * 24-bit command word + 16-bit readback (40-bit SPI exchange).
 */
f6222_status_t f6222_local_reg_read(f6222_dev_t* dev, uint8_t chip_addr, uint8_t reg, uint16_t* val);

/**
 * f6222_local_lut_write() — Local LUT Write, Mode 110 (40-bit frame).
 *
 * @param ch         Channel number, 1 (CH1) … 16 (CH16).
 * @param lut_addr   LUT entry index, 0–127.
 * @param val        16-bit CHn_SET equivalent value (phase + gain + enable).
 */
f6222_status_t f6222_local_lut_write(f6222_dev_t* dev, uint8_t ch, uint8_t chip_addr, uint8_t lut_addr, uint16_t val);

/**
 * f6222_local_lut_read() — Local LUT Read, Mode 111 (§8.4).
 *
 * 24-bit command word + 16-bit readback (40-bit SPI exchange).
 * Each entry is 16-bit CHn_SET equivalent (phase + gain + enable).
 * Continuous read is not supported.
 *
 * @param lut_ch     Channel number, 1 (CH1) … 16 (CH16).
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 * @param lut_addr   LUT entry index, 0–127.
 * @param val        Receives the 16-bit LUT data.
 */
f6222_status_t f6222_local_lut_read(f6222_dev_t* dev, uint8_t lut_ch, uint8_t chip_addr, uint8_t lut_addr,
                                    uint16_t* val);

/**
 * f6222_global_reg_write() — Global Register Write, Mode 011 (32-bit frame).
 *
 * Broadcasts a register write to all chips on the bus, or to one sub-array
 * when sa_op_enable is true.  RF channels update immediately.
 *
 * @param sa_op_enable  true = sub-array select enabled (SE = 1).
 * @param sa_index      Sub-array index SA[2:0] when sa_op_enable is true.
 * @param reg           7-bit register address RA[6:0].
 * @param val           16-bit data D[15:0].
 */
f6222_status_t f6222_global_reg_write(f6222_dev_t* dev, bool sa_op_enable, uint8_t sa_index, uint8_t reg, uint16_t val);

/**
 * f6222_lut_write_global() — Global LUT Write, Mode 010 (§8.7).
 *
 * Broadcasts a LUT write to all chips on the bus (no chip address).
 * 32-bit command word plus optional continuous 16-bit data in one CS transaction.
 *
 * LM (`lut_all_channels`):
 *   false — write `val` (+ extras) starting at one channel/LUT address; hardware
 *           auto-advances to the next channel, then the next LUT address.
 *   true  — write to all channels at `lut_addr`; each extra word advances LUT addr.
 *
 * @param lut_all_channels  LM bit: false = one channel, true = all channels.
 * @param ch                Channel 1..16; validated only when lut_all_channels is false.
 * @param lut_addr          Starting LUT entry, 0..127.
 * @param val               First 16-bit CHn_SET data in the command word.
 * @param extra_vals        Additional 16-bit data after val; NULL when extra_count is 0.
 * @param extra_count       Number of extra 16-bit words (not including val).
 */
f6222_status_t f6222_lut_write_global(f6222_dev_t* dev, bool lut_all_channels, uint8_t ch, uint8_t lut_addr,
                                      uint16_t val, const uint16_t* extra_vals, size_t extra_count);

/* ── Channel Control ─────────────────────────────────────────── */

/**
 * f6222_set_phase() — set the phase of one RF channel.
 *
 * @param ch       Channel number, 1 (CH1) … 16 (CH16), datasheet 1-based.
 * @param ps_step  6-bit phase step index, 0–63 (maps to CHn_SET PS_SET).
 *                 Step size ≈ 5.6°; angle ≈ ps_step × 5.6°.
 *
 * Register CHn_SET (read-modify-write):
 *   CH1  0x22   CH2  0x26   CH3  0x2A   CH4  0x2E
 *   CH5  0x32   CH6  0x36   CH7  0x3A   CH8  0x3E
 *   CH9  0x42   CH10 0x46   CH11 0x4A   CH12 0x4E
 *   CH13 0x52   CH14 0x56   CH15 0x5A   CH16 0x5E
 */
f6222_status_t f6222_set_phase(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t ps_step);

/**
 * f6222_set_gain() — set the gain of one RF channel.
 *
 * @param vga_step  6-bit gain step index, 0–63 (maps to CHn_SET VGA_SET).
 *                  Step size ≈ 0.45 dB; codes 0–31 lower range, 32–63 upper.
 */
f6222_status_t f6222_set_gain(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t vga_step);

/**
 * f6222_set_channel_enable() — power a channel on or off.
 *
 * Patches CH_PWD [0] only (0 = enabled, 1 = powered down).
 */
f6222_status_t f6222_set_channel_enable(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, bool enable);

/**
 * f6222_set_lna_sw() — enable or bypass the common input driver amplifier (DA).
 *
 * @param lna_sw_on  true = DA enabled (nominal gain), false = DA bypassed (high linearity).
 */
f6222_status_t f6222_set_lna_sw(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, bool lna_sw_on);

/**
 * f6222_set_global_pwd() — set or clear GLOBAL_PWD in CTRL_CFG.
 *
 * When GLOBAL_PWD=1, CH_PWD has no effect and all channels are off.
 */
f6222_status_t f6222_set_global_pwd(f6222_dev_t* dev, uint8_t chip_addr, bool power_down);

/**
 * f6222_apply_rf() — latch buffered phase/gain settings to RF output.
 *
 * Issues a local register write to SCRATCH with F6222_SPI_RF_LOAD_01,
 * triggering a bus-wide update of all buffered CHn_SET values on every chip.
 */
f6222_status_t f6222_apply_rf(f6222_dev_t* dev, uint8_t chip_addr);

/* ── Fast Beam Steering (Table 6) ────────────────────────────── */

/**
 * f6222_fbs_local() — switch this chip to a stored LUT beam state (Mode 101).
 */
f6222_status_t f6222_fbs_local(f6222_dev_t* dev, uint8_t chip_addr, uint8_t lut_addr, uint8_t rf_load);

/**
 * f6222_fbs_global() — switch chips to a stored LUT beam state (Mode 100).
 *
 * Command word field order: TE, SE, SA[2:0], LA[6:0].
 *
 * @param toggle_en           [12] TE: true = auto-increment LUT addr every 2 clocks.
 * @param sa_op_enable        [11] SE: true = target one sub-array only.
 * @param sa_index            [10:8] SA: sub-array index when sa_op_enable is true.
 * @param start_lut_addr      [7:1] LA: starting LUT entry, 0–127.
 * @param followup_lut_addrs  TE=0: extra LUT addresses; TE=1: NULL.
 * @param extra_count         Number of follow-up 8-bit blocks after the command word.
 */
f6222_status_t f6222_fbs_global(f6222_dev_t* dev, bool toggle_en, bool sa_op_enable, uint8_t sa_index,
                                uint8_t start_lut_addr, const uint8_t* followup_lut_addrs, size_t extra_count);

/* ── ADC / Monitoring (WIP: src/f6222_adc.c) ─────────────────── */

/**
 * f6222_read_temp_raw() — trigger and read the temperature ADC.
 *
 * Temp read SPI flow from Renesas evaluation GUI:
 *   write ADC_CTRL 0x0400 → 0x0500 → poll-read TEMP_DATA (0x0B) → restore ADC_CTRL.
 *
 * @param raw  Receives the 10-bit ADC code.
 *             Convert to °C: T = (raw - C0) * 100 / 130 + T0
 *             where C0 is calibrated at known temperature T0.
 */
f6222_status_t f6222_read_temp_raw(f6222_dev_t* dev, uint8_t chip_addr, uint16_t* raw);

/**
 * f6222_read_temp() — trigger temperature ADC and return raw code plus °C.
 *
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 * @param raw        Receives the 10-bit ADC code from TEMP_DATA (0x0B).
 * @param temp_c     Receives temperature in °C (float), using F6222_TEMP_T0_C,
 *                   F6222_TEMP_C0, and F6222_TEMP_SLOPE from datasheet §6.5.
 *
 * Blocks until ADC_DONE is set (typically < 1 ms).
 *
 * @return  F6222_OK on success, F6222_ERR_INVALID_ARG, F6222_ERR_ADC_TIMEOUT,
 *          or F6222_ERR_SPI.
 */
f6222_status_t f6222_read_temp(f6222_dev_t* dev, uint8_t chip_addr, uint16_t* raw, float* temp_c);

/* WIP: f6222_read_pdet_raw() — per-channel PDET readback (pending Renesas ref flow). */

/**
 * f6222_read_active_lut() — read the currently active LUT address from MO_MEM_ACT.
 */
f6222_status_t f6222_read_active_lut(f6222_dev_t* dev, uint8_t chip_addr, uint8_t* lut_addr);
