/*
 * spi.c
 *
 *  Created on: Mar 31, 2016
 *      Author: CuongVN
 */
#include <string.h>

//#include "FreeRTOS.h"
//#include "task.h"
#include "main.h"
#include "spi.h"
#include "m25lc512.h"
extern unsigned char indexEEprom;
extern __attribute ((aligned(32))) char my_bl_data[256];
extern char buffer_frezze[];
//
//
//

#define SR_WPEN 0x80  // Write Protect Enable
#define SR_NU6  0x40  // Not used, returns 0
#define SR_NU5  0x20  // Not used, returns 0
#define SR_NU4  0x10  // Not used, returns 0
#define SR_BP1  0x08  // Block Protect 1
#define SR_BP0  0x04  // Block Protect 0
#define SR_WEL  0x02  // Write Enable Latch
#define SR_WIP  0x01  // Write In Progress

//
//
//
typedef enum {
	MC25LC_WRSR = 0x01,  // Write STATUS register
	MC25LC_WRITE = 0x02, // Write data to memory array beginning at selected address
	MC25LC_READ = 0x03, // Read data from memory array beginning at selected address
	MC25LC_WRDI = 0x04,  // Reset the write enable latch
	MC25LC_RDSR = 0x05,  // Read STATUS register
	MC25LC_WREN = 0x06,  // Set the write enable latch
	MC25LC_PE = 0x42,  // Page erase (pages are 128 bytes)
	MC25LC_RDID = 0xab, // Release from Deep power-down and read electronic signature
	MC25LC_DPD = 0xb9,  // Deep Power-Down mode
	MC25LC_CE = 0xc7,  // Chip erase (0x0000-0xffff)
	MC25LC_SE = 0xd8 // Sector erase (0x0000-0x3fff, 0x4000-0x7fff, 0x8000-0xbfff, 0xc000-0xffff)
} mc25lc_e;

typedef struct m25lc512ErrorList_s {
	m25lc_e e;
	const char *t;
} m25lc512ErrorList_t;

//
//
//
static U32 rwAddress;

static const m25lc512ErrorList_t m25lc512ErrorList[] = { { M25LC_OK, "OK" }, {
		M25LC_TIMEOUT, "Timeout" }, { M25LC_NODEVICE, "No device present" }, {
		M25LC_ADDRERR, "Address range error" }, { M25LC_SECTORERR,
		"Sector range error" }, { M25LC_PAGEERR, "Page range error" }, {
		M25LC_WP, "Write protected" }, { M25LC_WIP, "Write in progress" }, {
		M25LC_LAST, "Unknown error" }, };

//
//
//
static U8 m25lc512SendByte(U8 byte) {
	m25lc512_Select ();

	byte = spiPut(byte);

	m25lc512_Deselect ();

	return byte;
}

static m25lc_e m25lc512CheckWriteInProgress(void) {
	U8 status;

	m25lc512StatusRead(&status);
	delay_ns(10000);
	//printf("\r\rstatus=%uend\r\r", status);

	if ((status & (SR_NU6 | SR_NU5 | SR_NU4 | SR_WIP)) == SR_WIP)
		return M25LC_WIP;

	return M25LC_OK;
}

static m25lc_e m25lc512WaitNotWriting(void) {

	if (m25lc512CheckWriteInProgress() == M25LC_WIP) {
		int i;
		for (i = 0; i < 10; i++) {
			//vTaskDelay (10 / portTICK_RATE_MS);
//			for (j = 0; j < 55298; j++)
//				;
			delay_ns(55298);
			if (m25lc512CheckWriteInProgress() == M25LC_OK)
				return M25LC_OK;
		}

		return M25LC_TIMEOUT;
	}

	return M25LC_OK;
}

static m25lc_e m25lc512SendByteEx(U8 byte) {
	m25lc_e r;

	if ((r = m25lc512WaitNotWriting()) != M25LC_OK)
		return r;

	m25lc512SendByte(byte);

	return M25LC_OK;
}

static m25lc_e m25lc512CheckWriteEnabled(void) {
	U8 status;

	m25lc512StatusRead(&status);

	if ((status & (SR_NU6 | SR_NU5 | SR_NU4 | SR_WEL)) != SR_WEL)
		return M25LC_WP;

	return M25LC_OK;
}

//
//  m25lc512Init() never called because main() does spiInit().  But you *could*
//  call it...
//
void m25lc512Init(void) {
	spiInit();
}

const char *m25lc512Strerror(m25lc_e e) {
	m25lc_e i;

	for (i = 0; m25lc512ErrorList[i].e != M25LC_LAST; i++)
		if (e == m25lc512ErrorList[i].e)
			return m25lc512ErrorList[i].t;

	return m25lc512ErrorList[i].t;
}

m25lc_e m25lc512ReadID(U8 *id) {
	U8 rdid;
	m25lc_e r;

	if ((r = m25lc512WaitNotWriting()) != M25LC_OK)
		return r;

	m25lc512_Select ();

	spiPut(MC25LC_RDID);
	spiPut(0);
	spiPut(0);

	rdid = spiPut(0);

	m25lc512_Deselect ();

	if (!rdid || (rdid == 0xff))
		return M25LC_NODEVICE;

	*id = rdid;

	return M25LC_OK;
}

m25lc_e m25lc512DeepPowerDown(void) {
	return m25lc512SendByteEx(MC25LC_DPD);
}

// Check the Serial EEPROM status register
U8 ReadSR(void) {
	U8 rdsr;
	m25lc512_Select ();

	spiPut(MC25LC_RDSR);
	rdsr = spiPut(0);

	m25lc512_Deselect ();
	return rdsr;
}
m25lc_e m25lc512StatusRead(U8 *rdsr) {
	m25lc512_Select ();

	spiPut(MC25LC_RDSR);
	*rdsr = spiPut(0);

	m25lc512_Deselect ();

	return M25LC_OK;
}

m25lc_e m25lc512StatusWrite(U8 wrsr) {
	m25lc_e r;

	if ((r = m25lc512WaitNotWriting()) != M25LC_OK)
		return r;

	m25lc512_Select ();

	spiPut(MC25LC_WRSR);
	spiPut(wrsr);

	m25lc512_Deselect ();

	return M25LC_OK;
}

m25lc_e m25lc512WriteEnable(void) {
	return m25lc512SendByteEx(MC25LC_WREN);
}

m25lc_e m25lc512WriteDisable(void) {
	return m25lc512SendByteEx(MC25LC_WRDI);
}

m25lc_e m25lc512ErasePage(U32 page) {
	if (page > M25LC512_LASTPAGE)
		return M25LC_PAGEERR;

	page *= M25LC512_PAGESIZE;

	m25lc512_Select ();

	spiPut(MC25LC_PE);
	spiPut(page >> 8);
	spiPut(page & 0xff);

	m25lc512_Deselect ();

	return M25LC_OK;
}

m25lc_e m25lc512EraseSector(U32 sector) {
	if (sector > M25LC512_LASTSECTOR)
		return M25LC_SECTORERR;

	sector *= M25LC512_SECTORSIZE;

	m25lc512_Select ();

	spiPut(MC25LC_SE);
	spiPut(sector >> 8);
	spiPut(sector & 0xff);

	m25lc512_Deselect ();

	return M25LC_OK;
}

m25lc_e m25lc512EraseChip(void) {
	return m25lc512SendByteEx(MC25LC_CE);
}

m25lc_e m25lc512SetAddress(U32 address) {
	if (address >= M25LC512_SIZE)
		return M25LC_ADDRERR;

	rwAddress = address;

	return M25LC_OK;
}

int m25lc512GetAddress(void) {
	return rwAddress;
}

m25lc_e m25lc512Read(U8 *buffer, U32 bufferLength) {
	U32 i;
	m25lc_e r;

	if ((r = m25lc512WaitNotWriting()) != M25LC_OK)
		return r;
//	while (ReadSR() & 0x1)
//		;         // check WIP

	m25lc512_Select ();

	spiPut(MC25LC_READ);
	spiPut(rwAddress >> 8);
	spiPut(rwAddress & 0xff);

	for (i = 0; i < bufferLength; i++)
		buffer[i] = spiPut(0);

	m25lc512_Deselect ();

	rwAddress += bufferLength;
	return M25LC_OK;
}

m25lc_e m25lc512ReadAddress(U32 address, U8 *buffer, U32 bufferLength) {
	m25lc_e r;

	if ((r = m25lc512SetAddress(address)) != M25LC_OK)
		return r;

	return m25lc512Read(buffer, bufferLength);
}

m25lc_e m25lc512Write(U8 *buffer, U32 bufferLength) {
	U32 i;
	m25lc_e r;

	if ((r = m25lc512WaitNotWriting()) != M25LC_OK)
		return r;
//	while (ReadSR() & 0x1)
//		;         // check WIP

	m25lc512SendByte(MC25LC_WREN);

	m25lc512_Select ();

	spiPut(MC25LC_WRITE);
	spiPut(rwAddress >> 8);
	spiPut(rwAddress & 0xff);

	for (i = 0; i < bufferLength; i++)
		spiPut(buffer[i]);

	m25lc512_Deselect ();

	rwAddress += bufferLength;
	delay_ns(1000);
	return M25LC_OK;
}

m25lc_e m25lc512WriteAddress(U32 address, U8 *buffer, U32 bufferLength) {
	int r;

	if ((r = m25lc512SetAddress(address)) != M25LC_OK)
		return r;

	return m25lc512Write(buffer, bufferLength);
}

m25lc_e m25lc512FillAddressCommon(U32 address, U8 *buffer, U32 bufferLength) {
	m25lc_e r;
	if ((r = m25lc512WriteEnable()) == M25LC_OK)
		if ((r = m25lc512SetAddress(address)) == M25LC_OK)
			r = m25lc512Write(buffer, bufferLength);

	return r;
}

m25lc_e m25lc512FillAddress(U32 address, U32 length, U8 fillValue) {
	m25lc_e r;
	U32 l;
	U32 i;
	U8 buffer[M25LC512_PAGESIZE];

	if (address + length > M25LC512_SIZE)
		return M25LC_ADDRERR;

	if ((r = m25lc512CheckWriteEnabled()) != M25LC_OK)
		return r;

	memset(buffer, fillValue, sizeof(buffer));

	l =
			(M25LC512_PAGESIZE
					- ((address + M25LC512_PAGESIZE) % M25LC512_PAGESIZE))
					% M25LC512_PAGESIZE;
	//l = MIN(l, length);

	if (l && ((r = m25lc512FillAddressCommon(address, buffer, l)) != M25LC_OK))
		if (((r = m25lc512FillAddressCommon(address, buffer, l)) != M25LC_OK))
			return r;

	address += l;
	length -= l;
	l = length - (length % M25LC512_PAGESIZE);

	for (i = 0; i < l;
			i += M25LC512_PAGESIZE, address += sizeof(buffer), length -=
					sizeof(buffer))
		if ((r = m25lc512FillAddressCommon(address, buffer, sizeof(buffer)))
				!= M25LC_OK)
			return r;

	if (length
			&& ((r = m25lc512FillAddressCommon(address, buffer, length))
					!= M25LC_OK))
		if (((r = m25lc512FillAddressCommon(address, buffer, length))
				!= M25LC_OK))
			return r;

	return M25LC_OK;
}

U8 writeEEPROM(U32 address, U8 *buffer, U32 bufferLength) {
	uint32_t uiTmp, i;
	U8 Buf[PAGESIZE_EEPROM];

	while (bufferLength > 0) {

		uiTmp = (bufferLength < PAGESIZE_EEPROM) ?
				bufferLength : PAGESIZE_EEPROM;
		for (i = 0; i < 3; i++) {

			if (m25lc512WriteAddress(address, buffer, uiTmp) != M25LC_OK)
				printf("save fail");

			if (m25lc512ReadAddress(address, Buf, uiTmp) != M25LC_OK)
				printf("read fail");

			if (0 == memcmp(buffer, Buf, uiTmp))
				break;
//			else
//				printf("\r loi luu");
		}
		if (i > 2) {
			//gErrState|=ERR_FRAM;
			return 0;
		}
		buffer += uiTmp;
		bufferLength -= uiTmp;
		address += uiTmp;
	}
//	printf("\r luu ok");
	return 0;
}
U8 readEEPROM(U32 address, U8 *buffer, U32 bufferLength) {
	uint32_t uiTmp, i;
	U8 Buf[PAGESIZE_EEPROM];

	while (bufferLength > 0) {

		uiTmp = (bufferLength < PAGESIZE_EEPROM) ?
				bufferLength : PAGESIZE_EEPROM;
		for (i = 0; i < 3; i++) {

			if (m25lc512ReadAddress(address, buffer, uiTmp) != M25LC_OK)
				printf("read fail");

			if (m25lc512ReadAddress(address, Buf, uiTmp) != M25LC_OK)
				printf("read fail");

			if (0 == memcmp(buffer, Buf, uiTmp))
				break;
//			else
//				printf("loi doc");
		}
		if (i > 2) {
			//gErrState|=ERR_FRAM;
			return 1;
		}
		buffer += uiTmp;
		bufferLength -= uiTmp;
		address += uiTmp;
	}
//	printf("doc ok");
	return 0;
}

void DF_ReadID(U8 *buf) {
	m25lc512_Select ();

	spiPut(0x9f);
	*buf++ = spiPut(0);
	*buf++ = spiPut(0);
	*buf++ = spiPut(0);

	m25lc512_Deselect ();
}

void test_fram(void) {
	//unsigned char output_buffer[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	_RTC_time day;
	//U8 ucReadData[8], id;
	//U32 add = 0, buff = 1;
	//U8 data = 100, __buff, i;
	U8 i;
	//m25lc_e r;

	IODIR0 |= (1 << 7);
	IODIR1 |= (1 << 24) | (1 << 25);
	IOSET1 |= (1 << 24) | (1 << 25);
	//IOCLR0 |= (1 << 7);
	m25lc512_ON();
	spiInit();
	printf("helo baby");
	//delay_nsecond(2);

//	r = m25lc512WriteAddress(0, output_buffer, 8);
//	printf("m25lc512Write() returned %d/%s\n", r, m25lc512Strerror(r));
//
//	//delay_nsecond(2);
//
//	r = m25lc512ReadAddress(0, ucReadData, 8);
//	printf("m25lc512Read() returned %d/%s\n", r, m25lc512Strerror(r));
//
//	for (i = 0; i < 8; i++) {
//		printf("Read data= %u\r", ucReadData[i]);
//	}
	indexEEprom = 90;
	eepromSaveFrame();
	printf("save ok\r");
	for (i = 0; i < LENGTH_DATA; i++) {
		printf("%02X", my_bl_data[i]);
	}
	//delay_nsecond(2);

	day.minute = 0;
	day.hour = 0;
	day.day_of_month = 5;
	day.month = 6;
	day.year = 16;

	readFreezeFrame(day, buffer_frezze);

	//m25lc512ReadAddress(90 * LENGTH_BUFF_SAVE, buffer_frezze, LENGTH_DATA);
//	readEEPROM(90 * LENGTH_BUFF_SAVE, buffer_frezze, LENGTH_DATA);
//	printf("\rread ok\r");
//	for (i = 0; i < LENGTH_DATA; i++) {
//		printf("%02X", buffer_frezze[i]);
//	}
////	printf("\rread ok\r");
////	printf("%s", buffer_frezze);
	while (1)
		;

}

