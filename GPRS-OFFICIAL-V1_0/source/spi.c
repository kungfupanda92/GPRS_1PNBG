/*
 * spi.c
 *
 *  Created on: Mar 31, 2016
 *      Author: CuongVN
 */

#include "spi.h"
#include "main.h"

void spiInit (void)
{
	S0SPCR = 0x00;
  PINSEL0 &= 0xFFFF00FF;
  PINSEL0 |= 0x00001500;
	S0SPCR = SPI0_MSTR;
	
	IODIR1 |= SPI0_SEL;		
  IOSET1 = SPI0_SEL;
	
	S0SPCCR = 8;
}


U8 spiPut (U8 valueIn)
{
  S0SPDR = valueIn;
	
	while ( !(S0SPSR & SPIF) )
		;
	
  return S0SPDR;
}
