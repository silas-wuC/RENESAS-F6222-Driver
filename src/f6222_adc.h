/*
 * f6222_adc.h — Temperature and PDET ADC (WIP)
 *
 * Scope: on-chip temperature sensor and per-channel PDET readback.
 * Implementation pending Renesas evaluation reference flow.
 *
 * Datasheet: R35DS0065EU0100 Rev.1.00 (2025-01-27)
 */

#pragma once

#include "f6222.h"

/*
 * f6222_read_temp_raw() — trigger and read the temperature ADC.
 *
 * Reference flow (mirrors f6522_adc.c):
 *   Stage 1 — prepare temperature ADC
 *   Stage 2 — trigger ADC conversion
 *   Stage 3 — read measurement result
 *   Stage 4 — cleanup & restore defaults
 *
 * Public declaration: include/f6222.h
 * Implementation:     src/f6222_adc.c (WIP)
 */
