/*
 * spi.c
 *
 *  Created on: Mar 31, 2016
 *      Author: CuongVN
 */

#ifndef _M25LC512_H_
#define _M25LC512_H_
#include "main.h"
#define m25lc512_Select()   IOCLR1 = GPIO_PIN_25
#define m25lc512_Deselect() IOSET1 = GPIO_PIN_25
#define m25lc512_ON()   	IOCLR0 = GPIO_PIN_7
#define m25lc512_OFF() 		IOSET0 = GPIO_PIN_7
#define ns_delay			10

//
//
//
#define M25LC512_SIZE       (65536)
#define M25LC512_SECTORSIZE (16384)
#define M25LC512_PAGESIZE     (128)
#define M25LC512_LASTSECTOR   (511)
#define M25LC512_LASTPAGE       (3)

//
//
//
/* ATMEL 25010 SEEPROM command set */
#define WREN		0x06		/* MSB A8 is set to 0, simplifying test */
#define WRDI		0x04
#define RDSR		0x05
#define WRSR		0x01
#define READ		0x03
#define WRITE		0x02

typedef enum {
	M25LC_OK = 0,     // Life is good
	M25LC_TIMEOUT,    // Timed out waiting for write to complete
	M25LC_NODEVICE,   // No device present (ID == 0xff or 0x00)
	M25LC_ADDRERR,    // Address out of range
	M25LC_SECTORERR,  // Sector > M25LC512_LASTSECTOR
	M25LC_PAGEERR,    // Page > M25LC512_LASTPAGE
	M25LC_WP,         // Write protected
	M25LC_WIP,        // Write in progress
	M25LC_LAST
} m25lc_e;

//
//
//
void m25lc512Init(void);
const char *m25lc512Strerror(m25lc_e e);
m25lc_e m25lc512ReadID(U8 *id);
m25lc_e m25lc512DeepPowerDown(void);
m25lc_e m25lc512StatusRead(U8 *rdsr);
m25lc_e m25lc512StatusWrite(U8 wrsr);
m25lc_e m25lc512WriteEnable(void);
m25lc_e m25lc512WriteDisable(void);
m25lc_e m25lc512ErasePage(U32 page);
m25lc_e m25lc512EraseSector(U32 sector);
m25lc_e m25lc512EraseChip(void);
m25lc_e m25lc512SetAddress(U32 address);
int m25lc512GetAddress(void);
m25lc_e m25lc512Read(U8 *buffer, U32 bufferLength);
m25lc_e m25lc512ReadAddress(U32 address, U8 *buffer, U32 bufferLength);
m25lc_e m25lc512Write(U8 *buffer, U32 bufferLength);
m25lc_e m25lc512WriteAddress(U32 address, U8 *buffer, U32 bufferLength);
m25lc_e m25lc512FillAddress(U32 address, U32 length, U8 fillValue);

m25lc_e m25lc512FillAddressCommon(U32 address, U8 *buffer, U32 bufferLength);
U8 writeEEPROM(U32 address, U8 *buffer, U32 bufferLength);
U8 readEEPROM(U32 address, U8 *buffer, U32 bufferLength);
U8 ReadSR(void);
void DF_ReadID(U8 *buf);
void test_fram(void);
#endif
