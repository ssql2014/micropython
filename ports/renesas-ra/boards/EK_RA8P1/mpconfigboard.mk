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

# Source-level recovery image. This keeps the board reachable over RTT while
# full FSP clock/SDRAM startup is still being debugged.
RA8P1_SAFE_BOOT ?= 1
CFLAGS += -DMICROPY_RA8P1_SAFE_BOOT=$(RA8P1_SAFE_BOOT)

RA8P1_SAFE_BOOT_CLOCKS ?= 0
CFLAGS += -DMICROPY_RA8P1_SAFE_BOOT_CLOCKS=$(RA8P1_SAFE_BOOT_CLOCKS)

RA8P1_SAFE_BOOT_SDRAM ?= 0
CFLAGS += -DMICROPY_RA8P1_SAFE_BOOT_SDRAM=$(RA8P1_SAFE_BOOT_SDRAM)

RA8P1_SAFE_BOOT_SDRAM_HEAP ?= 0
CFLAGS += -DMICROPY_RA8P1_SAFE_BOOT_SDRAM_HEAP=$(RA8P1_SAFE_BOOT_SDRAM_HEAP)

# Display smoke test gate (C-side GLCDC bring-up before REPL).
# Set to 0 to revert to the no-display image.
RA8P1_BRINGUP_DISPLAY_TEST ?= 0
ifeq ($(RA8P1_BRINGUP_DISPLAY_TEST), 1)
CFLAGS += -DMICROPY_RA8P1_BRINGUP_DISPLAY_TEST=1
endif

# MIPI-DSI smoke gate. This is kept separate from the GLCDC display smoke test
# because enabling it changes the GLCDC PHY path and requires DSI/PHY IRQs.
RA8P1_BRINGUP_MIPI_DSI_TEST ?= 0
ifeq ($(RA8P1_BRINGUP_MIPI_DSI_TEST), 1)
CFLAGS += -DMICROPY_RA8P1_BRINGUP_MIPI_DSI_TEST=1
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_mipi_dsi/r_mipi_dsi.c
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_mipi_phy/r_mipi_phy.c
endif

# CEU/parallel camera smoke gate. This is mutually exclusive with MIPI camera
# work because the board-level camera routing and pin mux requirements differ.
RA8P1_BRINGUP_CEU_TEST ?= 0

# MIPI-CSI/VIN smoke gate. Keep this separate from MIPI-DSI because both
# currently claim the same generated MIPI PHY instance name.
RA8P1_BRINGUP_MIPI_CSI_TEST ?= 0
RA8P1_CSI_TRY_RA_I2C ?= 0
RA8P1_CSI_USE_FSP_IIC ?= 0
RA8P1_CSI_MIPI_SEL_STATE ?= 1
RA8P1_CSI_VERIFY_CAMERA_WRITES ?= 1
RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE ?= 0
ifeq ($(RA8P1_BRINGUP_MIPI_CSI_TEST), 1)
ifeq ($(RA8P1_BRINGUP_MIPI_DSI_TEST), 1)
$(error RA8P1_BRINGUP_MIPI_CSI_TEST and RA8P1_BRINGUP_MIPI_DSI_TEST are mutually exclusive)
endif
ifeq ($(RA8P1_BRINGUP_CEU_TEST), 1)
$(error RA8P1_BRINGUP_MIPI_CSI_TEST and RA8P1_BRINGUP_CEU_TEST are mutually exclusive)
endif
ifeq ($(RA8P1_CSI_TRY_RA_I2C), 1)
ifeq ($(RA8P1_CSI_USE_FSP_IIC), 1)
$(error RA8P1_CSI_TRY_RA_I2C and RA8P1_CSI_USE_FSP_IIC are mutually exclusive)
endif
endif
CFLAGS += -DMICROPY_RA8P1_BRINGUP_MIPI_CSI_TEST=1
CFLAGS += -DMICROPY_RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE=$(RA8P1_BRINGUP_MIPI_CSI_BOOT_PROBE)
CFLAGS += -DRA8P1_CSI_MIPI_SEL_STATE=$(RA8P1_CSI_MIPI_SEL_STATE)
CFLAGS += -DRA8P1_CSI_VERIFY_CAMERA_WRITES=$(RA8P1_CSI_VERIFY_CAMERA_WRITES)
ifeq ($(RA8P1_CSI_TRY_RA_I2C), 1)
CFLAGS += -DRA8P1_CSI_TRY_RA_I2C=1
endif
ifeq ($(RA8P1_CSI_USE_FSP_IIC), 1)
CFLAGS += -DRA8P1_CSI_USE_FSP_IIC=1
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_iic_master/r_iic_master.c
endif
INC += -I$(TOP)/$(HAL_DIR)/ra/fsp/src/r_mipi_csi
INC += -I$(TOP)/$(HAL_DIR)/ra/fsp/src/r_vin
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_mipi_csi/r_mipi_csi.c
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_mipi_phy/r_mipi_phy.c
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_vin/r_vin.c
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_gpt/r_gpt.c
endif

ifeq ($(RA8P1_BRINGUP_CEU_TEST), 1)
ifeq ($(RA8P1_BRINGUP_MIPI_DSI_TEST), 1)
$(error RA8P1_BRINGUP_CEU_TEST and RA8P1_BRINGUP_MIPI_DSI_TEST are mutually exclusive)
endif
CFLAGS += -DMICROPY_RA8P1_BRINGUP_CEU_TEST=1
INC += -I$(TOP)/$(HAL_DIR)/ra/fsp/src/r_ceu
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_ceu/r_ceu.c
HAL_SRC_C += $(HAL_DIR)/ra/fsp/src/r_gpt/r_gpt.c
endif
