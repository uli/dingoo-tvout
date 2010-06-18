#ifndef CPU_H
#define CPU_H

/* Define this to the CPU frequency */
#define CPU_FREQ 336000000    /* CPU clock: 336 MHz */
#define CFG_EXTAL 12000000    /* EXT clock: 12 Mhz */

// SDRAM Timings, unit: ns
#define SDRAM_TRAS		45	/* RAS# Active Time */
#define SDRAM_RCD		20	/* RAS# to CAS# Delay */
#define SDRAM_TPC		20	/* RAS# Precharge Time */
#define SDRAM_TRWL		7	/* Write Latency Time */
#define SDRAM_TREF	        15625	/* Refresh period: 4096 refresh cycles/64ms */ 
//#define SDRAM_TREF      7812  /* Refresh period: 8192 refresh cycles/64ms */


void jz_cpuspeed(unsigned clockspeed);
void pll_init(unsigned int clock);
inline int sdram_convert(unsigned int pllin,unsigned int *sdram_freq);

#endif
