#include <stddef.h>
#include <stdint.h>

#include "bsp_linker_info.h"

extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __etext;

static const bsp_init_zero_info_t zero_list[] = {
    {
        .p_base = &__bss_start__,
        .p_limit = &__bss_end__,
        .type = {
            .copy_64 = 0,
            .external = 0,
            .source_type = INIT_MEM_ZERO,
            .destination_type = INIT_MEM_RAM,
        },
    },
};

static const bsp_init_copy_info_t copy_list[] = {
    {
        .p_base = &__data_start__,
        .p_limit = &__data_end__,
        .p_load = &__etext,
        .type = {
            .copy_64 = 0,
            .external = 0,
            .source_type = INIT_MEM_FLASH,
            .destination_type = INIT_MEM_RAM,
        },
    },
};

const bsp_init_info_t g_init_info = {
    .zero_count = sizeof(zero_list) / sizeof(zero_list[0]),
    .p_zero_list = zero_list,
    .copy_count = sizeof(copy_list) / sizeof(copy_list[0]),
    .p_copy_list = copy_list,
    .nocache_count = 0,
    .p_nocache_list = NULL,
};
