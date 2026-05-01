CMSIS_MCU = RA8P1
MCU_SERIES = cm85
LD_FILES = boards/EK_RA8P1/ra8p1_ek.ld

# MicroPython settings
MICROPY_VFS_FAT = 1

# FSP settings
USE_FSP_QSPI = 0

# Debug channel
CFLAGS+=-DDEFAULT_DBG_CH=0

# FSP settings
USE_FSP_LPM = 0

# Display smoke test gate (C-side GLCDC bring-up before REPL).
# Set to 0 to revert to the no-display image.
RA8P1_BRINGUP_DISPLAY_TEST ?= 0
ifeq ($(RA8P1_BRINGUP_DISPLAY_TEST), 1)
CFLAGS += -DMICROPY_RA8P1_BRINGUP_DISPLAY_TEST=1
endif
