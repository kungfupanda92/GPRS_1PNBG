#ifndef __GPRS_COM_H
#define __GPRS_COM_H

#include <LPC213x.H>                       /* LPC213x definitions  */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "main.h"
#include "init_system.h"
#include "gpio.h"
#include "uart0.h"
#include "uart1.h"
#include "timer.h"
#include "system.h"
#include "delay.h"
#include "stringconvert.h"
#include "read_para.h"
#include "LPC213x_isr.h"
//---------------------------------------------------------------------------------------
#define UART_STATE_NOTHING              	     	0x00
#define UART_STATE_RECEIVING              	     	0x01
#define UART_STATE_BLOCK												0x02
//---------------------------------------------------------------------------------------
#define BUF_RX_EMPTY		              	     	0x00
#define BUF_RX_FULL			              	     	0x01
//---------------------------------------------------------------------------------------
#define MODULE_NO_RESPONSE              	     	0x00
#define MODULE_RESPONSE_ERROR              	     	0x01
#define MODULE_RESPONSE_OK              	     	0x02
#define MODULE_CONNECT_OK               	     	0x03

#define MODULE_SEND_OK               	     	    0x04
#define MODULE_SEND_ERROR              	     	    0x05
#define MODULE_PRESEND_OK              	     	    0x06

//---------------------------------------------------------------------------------------
#define SIM800_NO_RESPONSE              	     	0x00
#define SIM800_RESPONSE_ERROR              	     	0x01
#define SIM800_RESPONSE_OK              	     	0x02
#define SIM800_RESPONSE_CORRECT            	     	0x03


//define state of uart-------------------------------------------------------------------------------
#define UART_IDLE              	    			 	0x00
//#define UART_WAIT_COMMAND_RESPONSE               	0x01
//#define UART_WAIT_DATA_GPRS             	        0x02
//#define UART_DO_NOTHING	  	             	        0x03
//#define UART_SEND_DATA								0x04
//#define UART_CHECK_PARA								0x05
//#define UART_START_MODULE_GPRS						0x06
//#define UART_WAIT_PRESEND							0x07
#define UART_WAIT_RESPONDE							0x08
//---------------------------------------------------------------------------------------------------
#define TIME_READ_COMMAND  	             	        30	//second
#define TIME_SEND_SOCKET  	             	        150	//second
#define TIME_WAIT_RESPONSE_LOGIN					30	//15 second
//---------------------------------------------------------------------------------------------------
#define COMMAND_LOGIN  	             	   	        0xA1	//login to server
#define COMMAND_SOCKET  	             	        0xA4	//hold the socket of the transmission
//---------------------------------------------------------------------------------------------------
#define COMMAND_READ_PARA_DIRECT					0x11 	//server reading direct
#define COMMAND_ACCEPT_LOGIN						0x21	//server accept login
#define COMMAND_ACCEPT_SOCKET						0x24	//server accept socket
#define COMMAND_LOAD_CURVE							0x08

#define COMMAND_READ_DATA_SAVED						0x35
#define COMMAND_SYN_TIME_MODULE						0x06
#define COMMAND_READ_TIME_MODULE					0x01
//---------------------------------------------------------------------------------------------------
void convert_array_hex2string(unsigned char *hex_array, char *string, unsigned int length_hex);
void get_RSSI_signal(void);
bool ON_OFF_mudule_GPRS(void);
void prepare_command_gprs(unsigned char process, char header_rx);

void quick_connect(void);
void prepare_trans_data(uint16_t len_frame);
void prepare_trans_data_test(void);

//unsigned char wait_response_command_gprs (unsigned char second);
//void check_para (void);
//void get_time_from_meter(_RTC_time time);
void prepare_config_mudule(void);
void trans_data_server(uint16_t len_frame);
void trans_data_server_test(void);
void send_command_to_server(unsigned char command);
void get_command_from_server(char *data_process);
flag_system check_data_rx_server (char *data_server);
void process_data_rx_from_server (char *data_server,unsigned char control_code);
void send_data_to_server (void);
void clear_array(char *array, unsigned int length);

#endif	/*  */

