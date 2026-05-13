# hash-bench-nes — cc65 / NES build
#
# `cl65` is the cc65 one-shot compile+assemble+link driver. We feed it
# the .c sources, point it at our project-local include path for
# hashes.h, and select target=nes which pulls in nes.cfg + nes.lib +
# the static joystick driver.
#
# Output: hash-bench-nes.nes (NROM, 32 KB PRG + 8 KB CHR, mapper 0).
#
# Requires: cc65 (cl65 in PATH). On Windows we expect cl65.exe at
# I:\cc65\bin\cl65.exe — build.bat prepends that to PATH.

TARGET   := hash-bench-nes
SRCDIR   := source
INCDIR   := include
BUILD    := build

CC65     ?= cl65
CFLAGS   := -t nes -O -I $(INCDIR) --standard c99
LDFLAGS  := -t nes -m $(BUILD)/$(TARGET).map

# Pull every .c in source/ — keeps Makefile honest if we add algos.
CFILES   := $(wildcard $(SRCDIR)/*.c)
OFILES   := $(patsubst $(SRCDIR)/%.c,$(BUILD)/%.o,$(CFILES))

.PHONY: all clean
all: $(TARGET).nes

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRCDIR)/%.c | $(BUILD)
	$(CC65) $(CFLAGS) -c -o $@ $<

$(TARGET).nes: $(OFILES) | $(BUILD)
	$(CC65) $(LDFLAGS) -o $@ $(OFILES)

clean:
	@rm -rf $(BUILD) $(TARGET).nes
