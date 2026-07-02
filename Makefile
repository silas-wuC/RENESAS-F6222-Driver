# F6222 driver — build as a portable static library
#
# Usage:
#   make              — build libf6222.a
#   make clean        — remove build artefacts
#   make check        — compile only (no link); useful on any host
#
# Cross-compile example (STM32):
#   make CC=arm-none-eabi-gcc AR=arm-none-eabi-ar \
#        CFLAGS_EXTRA="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard"

CC        ?= gcc
AR        ?= ar

# -Os + per-function/per-data sections let the final link drop every unused
# mode (LUT, FBS, ADC, …) so a firmware that calls only a subset of the API
# pays no flash cost for the rest.  -MMD -MP emit .d files so editing a header
# (e.g. include/f6222.h) triggers a rebuild of every dependent object.
CFLAGS     = -std=c99 -Wall -Wextra -Wpedantic -Werror -Warray-bounds \
             -Os -ffunction-sections -fdata-sections \
             -MMD -MP \
             -I include \
             $(CFLAGS_EXTRA)

# The caller links against libf6222.a; pass these to GC the unused sections:
#   $(CC) ... main.c -L. -lf6222 $(LDFLAGS)
LDFLAGS    = -Wl,--gc-sections

SRCS       = src/f6222_spi.c \
             src/f6222_init.c \
             src/f6222_channel.c \
             src/f6222_adc.c

OBJS       = $(SRCS:.c=.o)
DEPS       = $(OBJS:.o=.d)
LIB        = libf6222.a

.PHONY: all clean check

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

check:
	$(CC) $(CFLAGS) -fsyntax-only $(SRCS)

clean:
	rm -f $(OBJS) $(DEPS) $(LIB)

-include $(DEPS)
