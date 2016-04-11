/*
 * spi.h
 *
 *  Created on: Mar 31, 2016
 *      Author: CuongVN
 */
#ifndef _SPI_H_
#define _SPI_H_

//#include "FreeRTOS.h"
#include "main.h"
//
//
/* SPI 0 PCR register */
#define SPI0_BE		0x00000004
#define SPI0_CPHA	0x00000008
#define SPI0_CPOL	0x00000010
#define SPI0_MSTR	0x00000020
#define SPI0_LSBF	0x00000040
#define SPI0_SPIE	0x00000080

#define SPI0_SEL	(1<<25)
#define SPIF		1 << 7
//
void spiInit (void);
U8 spiPut (U8 valueIn);

#endif /* _SPI_H_ */
