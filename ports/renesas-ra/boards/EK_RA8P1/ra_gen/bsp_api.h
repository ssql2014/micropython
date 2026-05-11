// Local bsp_api.h shim for EK_RA8P1 bring-up
#ifndef BSP_API_H
#define BSP_API_H

#include "fsp_common_api.h"
#include "bsp_cfg.h"
#include "../../inc/fsp_features.h"
#include "../../src/bsp/mcu/all/bsp_exceptions.h"
#include "../../src/bsp/cmsis/Device/RENESAS/Include/renesas.h"
#include "../../src/bsp/cmsis/Device/RENESAS/Include/system.h"
#include "../../src/bsp/mcu/all/bsp_common.h"
#ifndef BSP_ICU_VECTOR_NUM_ENTRIES
#define BSP_ICU_VECTOR_NUM_ENTRIES (BSP_VECTOR_TABLE_MAX_ENTRIES - BSP_CORTEX_VECTOR_TABLE_ENTRIES)
#endif
#include "../../src/bsp/mcu/all/bsp_register_protection.h"
#include "../../src/bsp/mcu/all/bsp_irq.h"
#include "vector_data.h"
#include "../../src/bsp/mcu/all/bsp_io.h"
#include "../../src/bsp/mcu/all/bsp_group_irq.h"
#include "../../src/bsp/mcu/all/bsp_clocks.h"
#include "../../src/bsp/mcu/all/bsp_module_stop.h"
#include "../../src/bsp/mcu/all/bsp_security.h"
#include "../../src/bsp/mcu/all/bsp_delay.h"
#include "../../src/bsp/mcu/all/bsp_mcu_api.h"

#endif
