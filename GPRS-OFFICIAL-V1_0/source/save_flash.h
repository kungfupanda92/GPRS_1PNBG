#ifndef SAVE_FLASH_H_
#define SAVE_FLASH_H_

#include "main.h"
#define PAGESIZE_FLASH 		128
#define add_INDEX_EEPROM	0
#define LENGTH_BUFF_SAVE	128
#define LENGTH_DATA			121
#define PAGESIZE_EEPROM		64
/* P U B L I C   F U N C T I O N   P R O T O T Y P E S */

void check_freeze_data(void);

void test_save(void);

uint32_t check_add_free(void);

void freeze_frame(void);

uint8_t read_freeze_frame(_RTC_time Time_server, char *return_buff);

uint8_t check_id(void);

void save_time_offline(char* time_off);

void vSaveTimeLostConnect(void);

void read_time_offline(char * return_buff, unsigned char mode);

uint32_t errase_day_old(void);

uint8_t compare_date(_RTC_time day1, _RTC_time day2);

uint8_t check_day_ok(unsigned long add_day);

/******************************************************************************/
/* 				S A V E   D A T A   E E P R O M 				*/
/******************************************************************************/
void eepromSaveFrame(void);
uint8_t readFreezeFrame(_RTC_time Time_server, char *return_buff);
uint32_t checkHaveDataFreeze(_RTC_time day_current);

#endif /*SAVE_FLASH_H_*/
