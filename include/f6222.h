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

/* ═══════════════════════════════════════════════════════════════
 * Register Addresses
 * ═══════════════════════════════════════════════════════════════ */

#define F6222_REG_CTRL_CFG 0x00u      /* global power-down, sub-array index */
#define F6222_REG_PTAT_BIAS_CFG 0x01u /* master bias + temp compensation    */
#define F6222_REG_SCRATCH 0x02u       /* scratch (read-back test)           */
#define F6222_REG_MO_MEM_ACT 0x03u    /* active mode / LUT state (read-only)*/
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

/* CTRL_CFG (0x00) — reset 0x0010, typical 0x0000 */
#define F6222_CTRL_GLOBAL_PWD_BIT 4u /* 1 = all channels off    */
#define F6222_CTRL_GLOBAL_PWD (1u << 4)
#define F6222_CTRL_SCAN_MODE (1u << 3) /* write-only scan mode */
#define F6222_CTRL_SA_INDEX_MASK 0x0007u /* sub-array index [2:0]  */
#define F6222_CTRL_POR 0x0010u           /* hardware reset value     */

/* PTAT_BIAS_CFG (0x01) — reset 0x0080, typical 0x088A
 *
 *  Bit field summary:
 *   [11:9] MB_PT2_SLOPE  — PTAT2 slope trim (effective only when PT2_EN=1)
 *   [8]    MB_PT2_EN     — enable PTAT2 temperature compensation
 *   [7:5]  MB_PT_ADJ     — ±30% bias reference adjustment (typical = 4)
 *   [3]    MB_EN         — master bias generator enable
 *   [1]    MB_BG_SEL     — must be set to 1 (datasheet)
 *   [0]    MB_R_SEL      — reference resistor select (0 = internal, 1 = external)
 *
 *  PTAT mode  (7.4 dB gain flatness vs temp):  MB_EN=1, PT2_EN=0 → 0x0088
 *  PTAT2 mode (1.5 dB gain flatness vs temp):  MB_EN=1, PT2_EN=1, SLOPE=4 → 0x0788
 */
#define F6222_BIAS_MB_PT2_SLOPE_SHIFT 9u
#define F6222_BIAS_MB_PT2_SLOPE_MASK (0x07u << 9)
#define F6222_BIAS_MB_PT2_EN (1u << 8)
#define F6222_BIAS_MB_PT_ADJ_SHIFT 5u
#define F6222_BIAS_MB_PT_ADJ_MASK (0x07u << 5)
#define F6222_BIAS_MB_EN (1u << 3)
#define F6222_BIAS_MB_BG_SEL (1u << 1)
#define F6222_BIAS_MB_R_SEL (1u << 0)
#define F6222_BIAS_TYPICAL 0x088Au     /* datasheet typical operating value */
#define F6222_BIAS_TYPICAL_PT2 0x0788u /* PTAT2 mode, SLOPE=4             */

/* MO_MEM_ACT (0x03) — read-only */
#define F6222_MO_ACTIVE_MODE_SHIFT 13u
#define F6222_MO_ACTIVE_MODE_MASK (0x07u << 13)
#define F6222_MO_ACTIVE_LUT_SHIFT 5u
#define F6222_MO_ACTIVE_LUT_MASK (0xFFu << 5)

/* CHn_SET (0x22/26/.../5E) — reset 0x03F9, typical 0x03F8
 *
 *  Bit layout:
 *   [15:10]  PS_SET   — 6-bit phase code  (0 = 0°, 63 ≈ 354°, step ≈ 5.6°)
 *   [9:4]    VGA_SET  — 6-bit gain code   (0–31 lower range, 32–63 upper range)
 *   [3]      LNA_SW   — 1 = DA enabled (nominal), 0 = DA bypassed (high linearity)
 *   [0]      CH_PWD   — 1 = channel powered down, 0 = enabled
 *
 *  Gain step ≈ 0.45 dB; total range ≈ 28.4 dB.
 */
#define F6222_SET_PS_SHIFT 10u
#define F6222_SET_PS_MASK (0x3Fu << 10)
#define F6222_SET_PS_MAX 63u
#define F6222_SET_VGA_SHIFT 4u
#define F6222_SET_VGA_MASK (0x3Fu << 4)
#define F6222_SET_VGA_MAX 63u
#define F6222_SET_LNA_SW (1u << 3)
#define F6222_SET_CH_PWD (1u << 0)
#define F6222_SET_TYPICAL 0x03F8u /* PS=0°, gain=max, LNA_SW=1, enabled */
#define F6222_SET_POR 0x03F9u     /* hardware reset; CH_PWD=1             */

/* CHn_CTRL (0x21/25/.../5D) — reset 0x20FF, typical 0x2FFF */
#define F6222_CTRL_ATT_BITS_SHIFT 10u
#define F6222_CTRL_ATT_BITS_MASK (0x07u << 10)
#define F6222_CTRL_PS_PHT_SHIFT 8u
#define F6222_CTRL_PS_PHT_MASK (0x03u << 8)
#define F6222_CTRL_CH_MODE (1u << 7)  /* 1 = nominal bias, 0 = low-bias (~18% IDD reduction) */
#define F6222_CTRL_LNA_MODE (1u << 6)
#define F6222_CTRL_PS_MODE (1u << 5)
#define F6222_CTRL_ATT_MODE (1u << 4)
#define F6222_CTRL_LNA_EN (1u << 3)
#define F6222_CTRL_VGA_EN (1u << 2)
#define F6222_CTRL_PS_EN (1u << 1)
#define F6222_CTRL_ATT_EN (1u << 0)
#define F6222_CTRL_CH_TYPICAL 0x2FFFu

/* CHn_BIAS (0x20/24/.../5C) — reset 0x70DB, typical 0x6CDB */
#define F6222_BIAS_LNA_SHIFT 13u
#define F6222_BIAS_LNA_MASK (0x07u << 13)
#define F6222_BIAS_VGA_SHIFT 9u
#define F6222_BIAS_VGA_MASK (0x0Fu << 9)
#define F6222_BIAS_PS_SHIFT 6u
#define F6222_BIAS_PS_MASK (0x07u << 6)
#define F6222_BIAS_ATT_SHIFT 3u
#define F6222_BIAS_ATT_MASK (0x07u << 3)
#define F6222_BIAS_CH_SHIFT 0u
#define F6222_BIAS_CH_MASK 0x07u
#define F6222_BIAS_CH_TYPICAL 0x6CDBu

/* LNAIREF1 (0x69) */
#define F6222_LNA_IREF_SHUTDOWN (1u << 5)
#define F6222_LNA_IREF1_CONT_MASK 0x001Fu

/* LNAIREF2 (0x6A) */
#define F6222_LNA_IREF_MULT_SHIFT 5u
#define F6222_LNA_IREF_MULT_MASK (0x07u << 5)
#define F6222_LNA_IREF2_CONT_MASK 0x001Fu

/* LNAIREF3/4 (0x6B/0x6C) */
#define F6222_LNA_IREFn_CONT_MASK 0x001Fu

/* ADC_TEST (0x0A) — typical 0x0003 */
#define F6222_ADC_TEST_TYPICAL 0x0003u
#define F6222_ADC_CHOPPER_EN (1u << 1)
#define F6222_ADC_I_X2 (1u << 0)

/* CLK_CTRL (0x05) — typical 0x0B30 (60 MHz / 12 → 5 MHz) */
#define F6222_CLK_CTRL_TYPICAL 0x0B30u

/* ADC_CTRL (0x06) */
#define F6222_ADC_CTRL_TYPICAL 0x0000u
#define F6222_ADC_OSC_EN (1u << 10)
#define F6222_ADC_CTRL_TEST_MUX (1u << 9)
#define F6222_ADC_TRIG_TEMP (1u << 8)
#define F6222_ADC_CTRL_START_TEMP (F6222_ADC_TRIG_TEMP | F6222_ADC_OSC_EN) /* 0x0500 */

/* TEMP_DATA (0x0B) result fields */
#define F6222_ADC_DONE_BIT (1u << 10)
#define F6222_ADC_DATA_MASK 0x03FFu /* 10-bit result */
#define F6222_ADC_POLL_MAX 1000u

/* ═══════════════════════════════════════════════════════════════
 * SPI Protocol Mode Bits (M[2:0])
 * ═══════════════════════════════════════════════════════════════ */

#define F6222_SPI_MODE_LOCAL_REG_READ 0x00u
#define F6222_SPI_MODE_LOCAL_REG_WRITE 0x01u
#define F6222_SPI_MODE_GLOBAL_LUT_WRITE 0x02u
#define F6222_SPI_MODE_GLOBAL_REG_WRITE 0x03u
#define F6222_SPI_MODE_GLOBAL_FBS 0x04u
#define F6222_SPI_MODE_LOCAL_FBS 0x05u
#define F6222_SPI_MODE_LOCAL_LUT_WRITE 0x06u
#define F6222_SPI_MODE_LOCAL_LUT_READ 0x07u

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
#define F6222_PHASE_STEP_DEG_x10 56u  /* 5.6° × 10 for integer math */
#define F6222_GAIN_STEP_DB_x100 45u   /* 0.45 dB × 100               */
#define F6222_GAIN_RANGE_DB_x10 284u  /* 28.4 dB × 10                */

/**
 * f6222_ch_regs_t — register addresses for one RF channel.
 *
 * Fields are listed in ascending SPI address order (matches datasheet).
 * Use f6222_ch_reg_map[F6222_CH_TO_IDX(ch)] to look up a channel's addresses (ch = 1..16).
 */
typedef struct {
    uint8_t bias; /* CHn_BIAS register address */
    uint8_t ctrl; /* CHn_CTRL register address */
    uint8_t set;  /* CHn_SET register address (phase, gain, power-down) */
} f6222_ch_regs_t;

/* Register address table; array index 0..15 maps to API ch 1..16 (CH1..CH16). */
extern const f6222_ch_regs_t f6222_ch_reg_map[F6222_NUM_CHANNELS];

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
} f6222_status_t;

/* RF Load field values (used in Local Register Write and FBS commands).
 * BUFFER:    writes CHn_SET to an internal buffer only; RF output stays unchanged
 *            until a subsequent write with RF_LOAD_IMMEDIATE (e.g. f6222_apply_rf()).
 * IMMEDIATE: applies the write to RF output right away (also triggers a bus-wide update). */
#define F6222_RF_LOAD_BUFFER 0x00u
#define F6222_RF_LOAD_IMMEDIATE 0x01u

/*
 * Temperature sensor linear model (datasheet §6.5):
 *   code = 1.3 * (T − T0) + C0
 *   T    = (code − C0) / 1.3 + T0
 * C0 must be calibrated at a known T0 (single-point, device in standby).
 */
#define F6222_TEMP_SLOPE_INV_x100 77 /* 100/1.3 ≈ 77 (integer arithmetic) */

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
 * f6222_init() — post-reset register programming to typical operating state.
 *
 * Programs PTAT_BIAS_CFG, CLK_CTRL, ADC_TEST, LNAIREF, and per-channel
 * BIAS/CTRL to datasheet typical values.  Leaves GLOBAL_PWD set and all
 * channels disabled (CH_PWD=1).  Caller must clear GLOBAL_PWD and enable
 * desired channels before RF use.
 *
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 */
f6222_status_t f6222_init(f6222_dev_t* dev, uint8_t chip_addr);

/**
 * f6222_scratch_test() — write/read-verify SCRATCH (0x02) with a built-in pattern list.
 */
f6222_status_t f6222_scratch_test(f6222_dev_t* dev, uint8_t chip_addr);

/* ── Raw Register Access ─────────────────────────────────────── */

/**
 * f6222_reg_write() — local register write (single chip, 40-bit frame).
 *
 * @param rf_load    F6222_RF_LOAD_BUFFER or F6222_RF_LOAD_IMMEDIATE.
 * @param chip_addr  5-bit chip address matching hardware ADD[4:0] pins (0–31).
 * @param reg        7-bit register address.
 * @param val        16-bit data to write.
 */
f6222_status_t f6222_reg_write(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t reg, uint16_t val);

/**
 * f6222_reg_read() — local register read (single chip, 40-bit exchange).
 */
f6222_status_t f6222_reg_read(f6222_dev_t* dev, uint8_t chip_addr, uint8_t reg, uint16_t* val);

/**
 * f6222_reg_write_global() — global register write (Mode 011, 32-bit frame).
 *
 * @param sa_op_enable  true = sub-array select enabled (SE = 1).
 * @param sa_index      Sub-array index SA[2:0] when sa_op_enable is true.
 */
f6222_status_t f6222_reg_write_global(f6222_dev_t* dev, bool sa_op_enable, uint8_t sa_index, uint8_t reg, uint16_t val);

/* ── Channel Control ─────────────────────────────────────────── */

/**
 * f6222_set_phase() — set the phase of one RF channel.
 *
 * @param ch       Channel number, 1 (CH1) … 16 (CH16), datasheet 1-based.
 * @param ps_step  6-bit phase step (PS_SET), 0–63.  Angle ≈ ps_step × 5.6°.
 *
 * Register CHn_SET (read-modify-write):
 *   CH1 0x22  CH2 0x26  …  CH16 0x5E  (stride 4)
 */
f6222_status_t f6222_set_phase(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t ps_step);

/**
 * f6222_set_gain() — set the gain of one RF channel.
 *
 * @param vga_set  6-bit gain code (VGA_SET), 0–63.
 *               Codes 0–31 = lower range, 32–63 = upper range; step ≈ 0.45 dB.
 */
f6222_status_t f6222_set_gain(f6222_dev_t* dev, uint8_t rf_load, uint8_t chip_addr, uint8_t ch, uint8_t vga_set);

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
 * Issues a local register write to SCRATCH with RF_LOAD_IMMEDIATE,
 * triggering a bus-wide update of all buffered CHn_SET values on every chip.
 */
f6222_status_t f6222_apply_rf(f6222_dev_t* dev, uint8_t chip_addr);

/* ── LUT / Fast Beam Steering ────────────────────────────────── */

/**
 * f6222_lut_write() — write one entry into the on-chip beam LUT (Mode 110).
 *
 * @param ch         Channel number, 1 (CH1) … 16 (CH16).
 * @param lut_addr   LUT entry index, 0–127.
 * @param val        16-bit CHn_SET equivalent value (phase + gain + enable).
 */
f6222_status_t f6222_lut_write(f6222_dev_t* dev, uint8_t ch, uint8_t chip_addr, uint8_t lut_addr, uint16_t val);

/**
 * f6222_lut_read() — read one LUT entry (Mode 111).
 */
f6222_status_t f6222_lut_read(f6222_dev_t* dev, uint8_t ch, uint8_t chip_addr, uint8_t lut_addr, uint16_t* val);

/**
 * f6222_lut_write_global() — broadcast LUT entry to all chips (Mode 010).
 */
f6222_status_t f6222_lut_write_global(f6222_dev_t* dev, bool lut_all_channels, uint8_t ch, uint8_t lut_addr, uint16_t val);

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

/* ── ADC / Monitoring ────────────────────────────────────────── */

/**
 * f6222_read_temp_raw() — trigger and read the temperature ADC.
 *
 * @param raw  Receives the 10-bit ADC code.
 *             Convert to °C: T = (raw - C0) * 100 / 130 + T0
 *             where C0 is calibrated at known temperature T0.
 */
f6222_status_t f6222_read_temp_raw(f6222_dev_t* dev, uint8_t chip_addr, uint16_t* raw);

/**
 * f6222_read_active_lut() — read the currently active LUT address from MO_MEM_ACT.
 */
f6222_status_t f6222_read_active_lut(f6222_dev_t* dev, uint8_t chip_addr, uint8_t* lut_addr);
