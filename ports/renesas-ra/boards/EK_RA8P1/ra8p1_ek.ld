/*
                  Linker File for RA8P1 MCU
                  Cortex-M85 (CM85) core
*/

/* Linker script to configure memory regions. */
MEMORY
{
  FLASH (rx)         : ORIGIN = 0x02000000, LENGTH = 0x00200000  /* 2MB / 4MB */
  FLASH_ROMFS (r)    : ORIGIN = 0x02200000, LENGTH = 0x00100000  /* 1MB / 4MB */
  FLASH_FS (r)       : ORIGIN = 0x02300000, LENGTH = 0x00100000  /* 1MB / 4MB */
  RAM (rwx)          : ORIGIN = 0x22000000, LENGTH = 0x00100000  /* 1MB */
  SDRAM (rwx)        : ORIGIN = 0x68000000, LENGTH = 0x00800000  /* 8MB external SDRAM */
  DATA_FLASH (rx)    : ORIGIN = 0x40100000, LENGTH = 0x00002000  /* 8KB */
  QSPI_FLASH (rx)    : ORIGIN = 0x60000000, LENGTH = 0x01000000  /* 16MB */
  OSPI_FLASH (rx)    : ORIGIN = 0x70000000, LENGTH = 0x01000000  /* 16MB */
  ID_CODE (rx)       : ORIGIN = 0x0100A150, LENGTH = 0x00000010  /* 32bytes */
}

/* Library configurations */
/*GROUP(libgcc.a libc.a libm.a libnosys.a) */

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        _stext = .;
        __ROM_Start = .;

        /* Vector table - RA8P1 has 256-entry VTOR */
        KEEP(*(.fixed_vectors*))
        KEEP(*(.application_vectors*))
        __Vectors_End = .;
        __end__ = .;

        /* ROM Registers start at address 0x00000400 */
        . = __ROM_Start + 0x400;
        KEEP(*(.rom_registers*))

        /* Reserving 0x100 bytes of space for ROM registers. */
        . = __ROM_Start + 0x500;

        *(.text*)

        KEEP(*(.version))
        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)

        KEEP(*(.eh_frame*))

        __ROM_End = .;
        _etext = .;
    } > FLASH = 0xFF

    __Vectors_Size = __Vectors_End - __Vectors;

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    __etext = .;

    /* If DTC is used, put the DTC vector table at the start of SRAM. */
    .fsp_dtc_vector_table (NOLOAD) :
    {
        . = ORIGIN(RAM);
        *(.fsp_dtc_vector_table)
    } > RAM

    /* Initialized data section. */
    .data :
    {
        _sidata = .;
        _sdata = .;
        __data_start__ = .;
        *(vtable)
        *(.data.*)

        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);

        __Code_In_RAM_Start = .;
        KEEP(*(.code_in_ram*))
        __Code_In_RAM_End = .;

        __data_end__ = .;
        _edata = .;
    } > RAM AT > FLASH

    .noinit (NOLOAD):
    {
        . = ALIGN(4);
        __noinit_start = .;
        KEEP(*(.noinit*))
        __noinit_end = .;
    } > RAM

    .sdram_noinit (NOLOAD):
    {
        . = ALIGN(64);
        __sdram_noinit$$Base = .;
        KEEP(*(.sdram_noinit))
        __sdram_noinit$$Limit = .;
    } > SDRAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        _ebss = .;
    } > RAM

    .heap (NOLOAD):
    {
        . = ALIGN(8);
        __HeapBase = .;
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
        __HeapLimit = .;
    } > RAM

    /* Stacks are stored in this section. */
    .stack_dummy (NOLOAD):
    {
        . = ALIGN(8);
        _sstack = .;
        __StackLimit = .;
        KEEP(*(.stack))
        __StackTop = .;
        KEEP(*(.stack*))
        __StackTopAll = .;
        _estack = .;
    } > RAM

    PROVIDE(__stack = __StackTopAll);

    __RAM_segment_used_end__ = ALIGN(__StackTopAll , 4);

    /* Data flash. */
    .data_flash :
    {
        __Data_Flash_Start = .;
        KEEP(*(.data_flash*))
        __Data_Flash_End = .;
    } > DATA_FLASH

    .id_code :
    {
        __ID_Code_Start = .;
        KEEP(*(.id_code*))
        __ID_Code_End = .;
    } > ID_CODE
}

/* Define the top end of the stack. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* RAM extents for the garbage collector */
_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
/* Reserve 64KB at top of RAM for the C stack on RA8P1 bring-up. */
_ra8p1_stack_reserve = 0x00010000;
_heap_start = __HeapBase;
_heap_end = ORIGIN(RAM) + LENGTH(RAM) - _ra8p1_stack_reserve;
_sstack = _heap_end;
__StackLimit = _sstack;
__StackTopAll = ORIGIN(RAM) + LENGTH(RAM);
__stack = __StackTopAll;
_estack = __StackTopAll;

_micropy_hw_internal_flash_storage_start = ORIGIN(FLASH_FS);
_micropy_hw_internal_flash_storage_end   = ORIGIN(FLASH_FS) + LENGTH(FLASH_FS);

_micropy_hw_romfs_part0_start            = ORIGIN(FLASH_ROMFS);
_micropy_hw_romfs_part0_size             = LENGTH(FLASH_ROMFS);
