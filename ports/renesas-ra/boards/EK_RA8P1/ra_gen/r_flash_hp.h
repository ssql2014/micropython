// Stub for RA8P1 which uses OSPI external flash instead of internal flash
#ifndef R_FLASH_HP_H_
#define R_FLASH_HP_H_

#include <stdint.h>

// RA8P1 doesn't have internal FLASH_HP - use OSPI instead
typedef void (* flash_callback_t)(void);
typedef struct {
    uint32_t dummy;
} flash_hp_instance_ctrl_t;
typedef struct {
    uint32_t dummy;
} flash_cfg_t;
typedef enum {
    FLASH_RESULT_SUCCESS = 0
} flash_result_t;
typedef struct {
    void const *p_api;
    void const *p_ctrl;
    void const *p_cfg;
} flash_instance_t;
typedef struct {
    flash_result_t result;
    uint32_t remaining_bytes;
    void const *p_context;
} flash_callback_args_t;

#endif
