/* RA8P1 uses generated linker-init metadata from bsp_linker_info.h. */
#if defined(RA8P1) && __has_include("../boards/EK_RA8P1/ra_gen/bsp_linker_info.h")
#include "../boards/EK_RA8P1/ra_gen/bsp_linker_info.h"
#else
/* Stub for FSP system.c g_init_info */
#ifndef _G_INIT_INFO_H
#define _G_INIT_INFO_H

#include <stdint.h>
#include <stdbool.h>

typedef struct st_bsp_init_mem_type
{
    uint32_t source_type;
    uint32_t external;
} bsp_init_mem_type_t;

typedef struct st_bsp_init_zero_info
{
    void * p_base;
    void * p_limit;
    bsp_init_mem_type_t type;
} bsp_init_zero_info_t;

typedef struct st_bsp_init_copy_info
{
    void * p_base;
    void * p_limit;
    void const * p_load;
    bsp_init_mem_type_t type;
} bsp_init_copy_info_t;

typedef struct st_bsp_init_nocache_info
{
    void * p_base;
    void * p_limit;
} bsp_init_nocache_info_t;

typedef struct st_bsp_init_info
{
    uint32_t zero_count;
    uint32_t copy_count;
    uint32_t nocache_count;
    bsp_init_zero_info_t * p_zero_list;
    bsp_init_copy_info_t * p_copy_list;
    bsp_init_nocache_info_t * p_nocache_list;
} init_info_t;

#ifndef INIT_MEM_SIP_FLASH
#define INIT_MEM_SIP_FLASH (1U)
#endif

extern const init_info_t g_init_info;

#endif
#endif
