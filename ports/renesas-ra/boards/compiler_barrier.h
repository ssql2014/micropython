// we need to provide a definition of __COMPILER_BARRIER() defined in cmsis/cmsis_gcc.h V5.4.1 for FSP v4.4.0
#ifndef   __COMPILER_BARRIER
  #define __COMPILER_BARRIER()                   __ASM volatile ("" ::: "memory")
#endif

#ifndef FSP_HEADER
#define FSP_HEADER
#endif
#ifndef FSP_FOOTER
#define FSP_FOOTER
#endif

// Provide NAN and INFINITY via compiler builtins for math.h compatibility
#ifndef NAN
#define NAN (__builtin_nanf(""))
#endif

#ifndef INFINITY
#define INFINITY (__builtin_inff())
#endif

// FSP header bug workaround: BSP_ICU_VECTOR_NUM_ENTRIES used but only MAX defined
#ifndef BSP_ICU_VECTOR_NUM_ENTRIES
#if defined(BSP_ICU_VECTOR_MAX_ENTRIES)
#define BSP_ICU_VECTOR_NUM_ENTRIES BSP_ICU_VECTOR_MAX_ENTRIES
#endif
#endif
#ifndef BSP_ICU_VECTOR_MAX_ENTRIES
#if defined(BSP_ICU_VECTOR_NUM_ENTRIES)
#define BSP_ICU_VECTOR_MAX_ENTRIES BSP_ICU_VECTOR_NUM_ENTRIES
#endif
#endif

// Stub assert that actually works
#undef assert
#define assert(x) ((void)0)

/* FSP clock configuration for USB - set to no divider */
#define BSP_CFG_UCLK_DIV BSP_CLOCKS_USB_CLOCK_DIV_1


// RA8P1 additional clock configuration (needed by FSP 6.2.0 bsp_clocks.c)
#ifndef BSP_CFG_USB60CLK_DIV
#define BSP_CFG_USB60CLK_DIV (0)
#endif
#ifndef BSP_CFG_USB60CLK_SOURCE
#define BSP_CFG_USB60CLK_SOURCE (0)
#endif
#ifndef BSP_CFG_UCLK_DIV
#define BSP_CFG_UCLK_DIV (0)
#endif
#ifndef BSP_CFG_ADCCLK_DIV
#define BSP_CFG_ADCCLK_DIV (0)
#endif
#ifndef BSP_CFG_ADCCLK_SOURCE
#define BSP_CFG_ADCCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_ESWCLK_DIV
#define BSP_CFG_ESWCLK_DIV (0)
#endif
#ifndef BSP_CFG_ESWCLK_SOURCE
#define BSP_CFG_ESWCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_ESWPHYCLK_DIV
#define BSP_CFG_ESWPHYCLK_DIV (0)
#endif
#ifndef BSP_CFG_ESWPHYCLK_SOURCE
#define BSP_CFG_ESWPHYCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_ETHPHYCLK_DIV
#define BSP_CFG_ETHPHYCLK_DIV (0)
#endif
#ifndef BSP_CFG_ETHPHYCLK_SOURCE
#define BSP_CFG_ETHPHYCLK_SOURCE (0)
#endif

// RA8P1 additional FSP 6.2.0 clock/PLL configuration
#ifndef BSP_CFG_PLODIVR
#define BSP_CFG_PLODIVR (0)
#endif
#ifndef BSP_CFG_PLODIVQ
#define BSP_CFG_PLODIVQ (0)
#endif
#ifndef BSP_CFG_PLODIVP
#define BSP_CFG_PLODIVP (0)
#endif
#ifndef BSP_CFG_UCLK_SOURCE
#define BSP_CFG_UCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_OCTACLK_SOURCE
#define BSP_CFG_OCTACLK_SOURCE (0)
#endif
#ifndef BSP_CFG_OCTACLK_DIV
#define BSP_CFG_OCTACLK_DIV (0)
#endif
#ifndef BSP_CFG_CANFDCLK_DIV
#define BSP_CFG_CANFDCLK_DIV (0)
#endif
#ifndef BSP_CFG_CANFDCLK_SOURCE
#define BSP_CFG_CANFDCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_SCICLK_DIV
#define BSP_CFG_SCICLK_DIV (0)
#endif
#ifndef BSP_CFG_SCICLK_SOURCE
#define BSP_CFG_SCICLK_SOURCE (0)
#endif
#ifndef BSP_CFG_SPICLK_DIV
#define BSP_CFG_SPICLK_DIV (0)
#endif
#ifndef BSP_CFG_SPICLK_SOURCE
#define BSP_CFG_SPICLK_SOURCE (0)
#endif
#ifndef BSP_CFG_GPTCLK_DIV
#define BSP_CFG_GPTCLK_DIV (0)
#endif
#ifndef BSP_CFG_GPTCLK_SOURCE
#define BSP_CFG_GPTCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_I3CCLK_DIV
#define BSP_CFG_I3CCLK_DIV (0)
#endif
#ifndef BSP_CFG_I3CCLK_SOURCE
#define BSP_CFG_I3CCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_LCDCLK_DIV
#define BSP_CFG_LCDCLK_DIV (0)
#endif
#ifndef BSP_CFG_LCDCLK_SOURCE
#define BSP_CFG_LCDCLK_SOURCE (0)
#endif

// RA8P1 PLL2 and PLL output frequency configuration
#ifndef BSP_CFG_PLL1P_FREQUENCY_HZ
#define BSP_CFG_PLL1P_FREQUENCY_HZ (0)
#endif
#ifndef BSP_CFG_PLL1Q_FREQUENCY_HZ
#define BSP_CFG_PLL1Q_FREQUENCY_HZ (0)
#endif
#ifndef BSP_CFG_PLL1R_FREQUENCY_HZ
#define BSP_CFG_PLL1R_FREQUENCY_HZ (0)
#endif
#ifndef BSP_CFG_PLL2P_FREQUENCY_HZ
#define BSP_CFG_PLL2P_FREQUENCY_HZ (0)
#endif
#ifndef BSP_CFG_PLL2Q_FREQUENCY_HZ
#define BSP_CFG_PLL2Q_FREQUENCY_HZ (0)
#endif
#ifndef BSP_CFG_PLL2R_FREQUENCY_HZ
#define BSP_CFG_PLL2R_FREQUENCY_HZ (0)
#endif
#ifndef BSP_CFG_PLL2_MUL
#define BSP_CFG_PLL2_MUL (0)
#endif
#ifndef BSP_CFG_PLL2_DIV
#define BSP_CFG_PLL2_DIV (0)
#endif
#ifndef BSP_CFG_PL2ODIVR
#define BSP_CFG_PL2ODIVR (0)
#endif
#ifndef BSP_CFG_PL2ODIVQ
#define BSP_CFG_PL2ODIVQ (0)
#endif
#ifndef BSP_CFG_PL2ODIVP
#define BSP_CFG_PL2ODIVP (0)
#endif
#ifndef BSP_PRV_STARTUP_SCKDIVCR2
#define BSP_PRV_STARTUP_SCKDIVCR2 (0)
#endif

// RA8P1 additional clock dividers (bsp_clocks.c requires these)
#ifndef BSP_CFG_CPUCLK_DIV
#define BSP_CFG_CPUCLK_DIV (0)
#endif
#ifndef BSP_CFG_CPUCLK1_DIV
#define BSP_CFG_CPUCLK1_DIV (0)
#endif
#ifndef BSP_CFG_NPUCLK_DIV
#define BSP_CFG_NPUCLK_DIV (0)
#endif
#ifndef BSP_CFG_MRICLK_DIV
#define BSP_CFG_MRICLK_DIV (0)
#endif
#ifndef BSP_CFG_PCLKE_DIV
#define BSP_CFG_PCLKE_DIV (0)
#endif
#ifndef BSP_CFG_HOCO_DIV
#define BSP_CFG_HOCO_DIV (0)
#endif
#ifndef BSP_CFG_MOCO_DIV
#define BSP_CFG_MOCO_DIV (0)
#endif
#ifndef BSP_CFG_BCLKA_DIV
#define BSP_CFG_BCLKA_DIV (0)
#endif
#ifndef BSP_CFG_BCLKA_SOURCE
#define BSP_CFG_BCLKA_SOURCE (0)
#endif
#ifndef BSP_CFG_DSMIFCLK_DIV
#define BSP_CFG_DSMIFCLK_DIV (0)
#endif
#ifndef BSP_CFG_DSMIFCLK_SOURCE
#define BSP_CFG_DSMIFCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_CECCLK_DIV
#define BSP_CFG_CECCLK_DIV (0)
#endif
#ifndef BSP_CFG_CECCLK_SOURCE
#define BSP_CFG_CECCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_ESCCLK_DIV
#define BSP_CFG_ESCCLK_DIV (0)
#endif
#ifndef BSP_CFG_ESCCLK_SOURCE
#define BSP_CFG_ESCCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_IICCLK_DIV
#define BSP_CFG_IICCLK_SOURCE (0)
#endif
#ifndef BSP_CFG_IICCLK_SOURCE
#define BSP_CFG_IICCLK_DIV (0)
#endif
#ifndef BSP_CFG_FSXP_SOURCE
#define BSP_CFG_FSXP_SOURCE (0)
#endif
#ifndef BSP_CFG_ETHPHY_SOURCE
#define BSP_CFG_ETHPHY_SOURCE (0)
#endif
#ifndef BSP_CFG_CLKOUT_DIV
#define BSP_CFG_CLKOUT_DIV (0)
#endif
#ifndef BSP_CFG_CLKOUT_SOURCE
#define BSP_CFG_CLKOUT_SOURCE (0)
#endif
#ifndef BSP_CFG_CLKOUT1_DIV
#define BSP_CFG_CLKOUT1_SOURCE (0)
#endif
#ifndef BSP_CFG_CLKOUT1_SOURCE
#define BSP_CFG_CLKOUT1_DIV (0)
#endif
#ifndef BSP_CFG_PLL2_ENABLE
#define BSP_CFG_PLL2_ENABLE (0)
#endif

#ifndef BSP_TZ_NONSECURE_BUILD
#define BSP_TZ_NONSECURE_BUILD (0)
#endif
#ifndef BSP_CFG_IOPORT_VOLTAGE_MODE_VCC2
#define BSP_CFG_IOPORT_VOLTAGE_MODE_VCC2 (0)
#endif
#ifndef BSP_CFG_IOPORT_VOLTAGE_MODE_VCC
#define BSP_CFG_IOPORT_VOLTAGE_MODE_VCC (0)
#endif
