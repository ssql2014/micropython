// Stub for RA8P1 which uses OSPI external flash instead of internal flash
// Internal FLASH_HP is not available on RA8P1 - using OSPI instead
#ifndef R_FLASH_HP_H_
#define R_FLASH_HP_H_

#include <stdint.h>

typedef struct st_flash_hp_instance_ctrl {
    uint32_t dummy;
} flash_hp_instance_ctrl_t;

typedef struct st_flash_cfg {
    uint32_t dummy;
} flash_cfg_t;

typedef enum e_flash_result {
    FLASH_RESULT_SUCCESS,
    FLASH_RESULT_BLANK,
    FLASH_RESULT_NOT_BLANK,
    FLASH_RESULT_BGO_ACTIVE,
    FLASH_RESULT_INTEGRATION_ERROR
} flash_result_t;

typedef void (*flash_callback_t)(flash_callback_args_t *);
typedef struct {
    flash_result_t result;
    uint32_t remaining_bytes;
    void const *p_context;
} flash_callback_args_t;

typedef struct st_flash_instance {
    void const *p_api;
    void const *p_ctrl;
    void const *p_cfg;
} flash_instance_t;

#define g_flash0 (flash_instance_t){0}

#endif
