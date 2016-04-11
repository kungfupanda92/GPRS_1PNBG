#include "read_para.h"
#include "save_flash.h"
#include "main.h"
//------------------------------------------------------------------------------
extern _rtc_flag rtc_flag;
extern _uart_frame buffer_rx, buffer_tx;
extern __attribute ((aligned(32))) char my_bl_data[256];
extern unsigned int total_times;
extern char buff_rssi[5];

extern char buffer_PLC[MAX_BUFFER];
extern char commands[COMMANDS];
extern char buf_send_server[MAX_BUFFER_TX];
extern char buff_time[10];
extern _RTC_time time_server;
extern char buff_contain_data_read_metter[200];
extern char buff_contain_data_add_send_server[200];
extern char code_server[5];
extern char code_meter[5];
extern uint16_t table_control_code[LEN_BUFF_FREEZE][4];

extern PARA_PLC para_plc;
extern unsigned char check_frame;
extern unsigned char start_frame;
extern unsigned char stop_frame;
extern unsigned char stop_frame_immediately;
extern _system_flag system_flag;

extern CONFIG_NEWWORK config_network;

extern char out_data[15];
extern char data_PLC[50];
extern char temp_data1[50];
/******************************************************************************/
/* T Y P E D E F   N E W    V A R I A B L E */
/******************************************************************************/
typedef enum {
	normal = 0,
	volt,
	not_change_data,
	current_reverse,
	time_max_demand,
	in_active_power,
	in_reactive_power
} mode_lookup;

/******************************************************************************/
/* P R I V A T E   F U N C T I O N   P R O T O T Y P E S */
/******************************************************************************/
void clear_para(uint8_t start, uint8_t stop, bool _state);
void read_para(void);	//return: IP, APN, Port, ID DCU

flag_system load_data_meter(char* code_4_byte, char* data, unsigned char mode);
void write_data(char* frame_tx, char* code_server, char* code_meter, int i,
		int len, unsigned char mode);
void write_data_metter(char *frame_tx, unsigned int len_command);

void read_metter_directmode(char *frame_tx, unsigned int len_command);
flag_system return_data(char* frame, char* in_mode);
/******************************************************************************/

/******************************************************************************/
/*            clear_para                        */
/******************************************************************************/
/**
 * @brief  xoa buffer
 * @param  None
 * @retval None
 */
void clear_para(uint8_t start, uint8_t stop, bool _state) {
	uint32_t i;
	for (i = 0; i < MAX_BUFFER; i++)
		buffer_PLC[i] = 0;
	check_frame = false;
	start_frame = start;
	stop_frame = stop;
	stop_frame_immediately = _state;
}
//---------------------------------------------------------------------------------
void set_time_from_rtc(void) {
	char out_data[15];
	char data_PLC[50];
	uint32_t time_out;

	sign_in();

	clear_para(STX, ETX, false);
	//read ID
	sprintf(out_data, "%cR2%cC100()%c%c", SOH, STX, ETX, 0x12);
	UART0_Send(out_data);
	time_out = 0;
	while (check_frame == false) {
		delay_ms(1);
		if (++time_out >= 1000)
			return;
	}
	//check ok
	check_buffer(para_plc._ID, buffer_PLC);

	clear_para(STX, ETX, false);
	//read Time
	sprintf(out_data, "%cR2%cC001()%c%c", SOH, STX, ETX, 0x12);		//read IP1
	UART0_Send(out_data);
	time_out = 0;
	while (check_frame == false) {
		delay_ms(1);
		if (++time_out >= 1000)
			return;
	}
	//check ok
	check_buffer(data_PLC, buffer_PLC);
	//
	YEAR = 2000 + (data_PLC[2] - 0x30) * 10 + (data_PLC[3] - 0x30);
	MONTH = (data_PLC[4] - 0x30) * 10 + (data_PLC[5] - 0x30);
	DOM = (data_PLC[6] - 0x30) * 10 + (data_PLC[7] - 0x30);
	HOUR = (data_PLC[8] - 0x30) * 10 + (data_PLC[9] - 0x30);
	MIN = (data_PLC[10] - 0x30) * 10 + (data_PLC[11] - 0x30);
	SEC = (data_PLC[12] - 0x30) * 10 + (data_PLC[13] - 0x30);
}
/******************************************************************************/
/*            read_para                      */
/******************************************************************************/
/**
 * @brief  Doc thong so co ban tu cong to:
 +IP1
 +IP2
 +APN
 +PORT
 * @param  None
 * @retval None
 */
void read_para(void) {
//	char out_data[15];
//	char data_PLC[50];
	char temp[20];
	sign_in();

	return_data(para_plc._ID, "C100");		//read APN

	return_data(data_PLC, "C105");		//read IP1
	StringToHex(temp, data_PLC);
	sprintf(config_network._IP1, "%u.%u.%u.%u", temp[0], temp[1], temp[2],
			temp[3]);

	return_data(data_PLC, "C106");		//read PORT
	sprintf(config_network._PORT, "%u", (uint32_t) strtol(data_PLC, NULL, 16));

	return_data(data_PLC, "C107");		//read APN
	StringToHex(config_network._APN, data_PLC);

	return_data(data_PLC, "C108");		//read IP2
	StringToHex(temp, data_PLC);
	sprintf(config_network._IP2, "%u.%u.%u.%u", temp[0], temp[1], temp[2],
			temp[3]);

	//---------------------------------------------------------------
	//send break command
	sprintf(out_data, "%cB0%c%c", SOH, ETX, 0x71);
	UART0_Send(out_data);
}
/******************************************************************************/
/*            sign_in                                               */
/******************************************************************************/
/**
 * @brief  dang nhap vao cong to
 * @param  None
 * @retval None
 */
flag_system sign_in(void) {
	//char out_data[50];
	uint32_t time_out;
	start: clear_para('/', 0x0A, false);
	sprintf(out_data, "/?!\r\n");
	UART0_Send(out_data);
	time_out = 0;
	while (check_frame == false && (++time_out <= 1000)) {
		delay_ms(1);
	}
	if (time_out >= 1000)
		goto start;
	//check ok

	clear_para(SOH, ETX, false);
	sprintf(out_data, "%c021\r\n", ACK);
	UART0_Send(out_data);
	time_out = 0;
	while (check_frame == false) {
		delay_ms(1);
		if (++time_out >= 1000)
			return not_enter_program_mode;
	}
	//check ok
	clear_para(ACK, NULL, true);
	sprintf(out_data, "%cP1%c(0000)%c%c", SOH, STX, ETX, 0x61);
	UART0_Send(out_data);
	time_out = 0;
	while (check_frame == false) {
		delay_ms(1);
		if (++time_out >= 1000)
			return not_enter_program_mode;
	}
	//check ok
	return sign_in_ok;
}
/******************************************************************************/
/*            return_data                        */
/******************************************************************************/
/**
 * @brief  Tra ve gia tri doc tu cong to
 * @param  
 *frame: tra ve gia tri tro den frame
 *in_mode: ma doc phia cong to
 * @retval 
 tra ve co he thong
 */
flag_system return_data(char* frame, char* in_mode) {
	uint32_t time_out;
	uint32_t i;
	//char out_data[15];
	char result_BCC = 0;
	clear_para(STX, ETX, false);
	//read commands
	sprintf(out_data, "%cR2%c%s()%c", SOH, STX, in_mode, ETX);
	for (i = 1; i < strlen(out_data); i++) {
		result_BCC ^= out_data[i];
	}
	strcat(out_data, &result_BCC);
	UART0_Send(out_data);
	time_out = 0;
	while (check_frame == false) {
		delay_ms(1);
		if (++time_out >= 1000)
			return commands_error;
	}
//check ok
	//check_buffer(frame, buffer_PLC);
	if (check_buffer(frame, buffer_PLC) == 0)
		return commands_error;
	return commands_ok;
}
/******************************************************************************/
/*            caculate_checksum                        */
/******************************************************************************/
/**
 * @brief  easy understand
 * @param  
 * @retval 
 */
unsigned char caculate_checksum(char *string_data) {
	unsigned char checksum;
	unsigned int length_string;		//length of string input
	unsigned int i;
	char *p;

	length_string = strlen(string_data);	//get length of data input
	if (length_string == 0) {
		system_flag.bits.ERROR_CACULATE_CHECKSUM = 1;
		return 0;	//length = 0
	}
	if (length_string % 2) {
		system_flag.bits.ERROR_CACULATE_CHECKSUM = 1;
		return 0;	//length = 0
	}
	p = string_data;
	for (i = 0; i < length_string; i++) {	//check value
		if (*(p + i) < 0x30) {
			system_flag.bits.ERROR_CACULATE_CHECKSUM = 1;
			return 0;	//value error
		}
		if ((0x39 < *(p + i)) && (*(p + i) < 0x41)) {
			system_flag.bits.ERROR_CACULATE_CHECKSUM = 1;
			return 0;	//value error
		}
		if ((0x46 < *(p + i)) && (*(p + i) < 0x61)) {
			system_flag.bits.ERROR_CACULATE_CHECKSUM = 1;
			return 0;	//value error
		}
		if (*(p + i) > 0x66) {
			system_flag.bits.ERROR_CACULATE_CHECKSUM = 1;
			return 0;	//value error
		}
	}
	//Ok frame, start caculate byte checksum
	p = string_data;
	checksum = 0;
	for (i = 0; i < length_string; i += 2)
		checksum += convert_string2hex(p + i);
	return checksum;
}
//---------------------------------------------------------------------------------------
unsigned char process_server_reading_data_save(char *data_server,
		uint16_t *len_full_buff_tx) {
	unsigned char data_return;

	char ID_hex[7];
	/////////////////////////////
	unsigned char checksum;
	unsigned int len_data_meter, len, i;
	WORD_UNSIGNED length_data;

	for (i = 0; i < 8; i++) {
		buffer_tx.data_frame[i] = buffer_rx.data_frame[i];
	}
	buffer_tx.frame.id_meter[0] = 0x01;
	buffer_tx.frame.id_meter[1] = 0x01;
	buffer_tx.frame.id_meter[2] = 0x00;
	buffer_tx.frame.id_meter[3] = 0x01;

	buffer_tx.frame.id_meter[4] = buffer_rx.frame.id_meter[0];
	buffer_tx.frame.id_meter[5] = buffer_rx.frame.id_meter[1];
	//set time
	for (i = 0; i < 5; i++) {
		buffer_tx.data_frame[i + 17] = buffer_rx.data_frame[i + 17];
	}
	//set ID
	StringToHex(ID_hex, para_plc._ID);
	for (i = 0; i < 6; i++) {
		buffer_tx.data_frame[i + 22] = ID_hex[5 - i];
	}
	buffer_tx.data_frame[28] = 0x9B;

	//set time
	time_server.year = bcd2hex(buffer_rx.frame.meter_index[0]);
	time_server.month = bcd2hex(buffer_rx.frame.meter_index[1]);
	time_server.day_of_month = bcd2hex(buffer_rx.frame.byte_ff[0]);
	time_server.hour = bcd2hex(buffer_rx.frame.byte_ff[1]);
	time_server.minute = bcd2hex(buffer_rx.frame.byte_ff[2]);
	//end time

	buf_send_server[0] = 0;
#ifdef EXTERNAL_EEPROM
	if (readFreezeFrame(time_server, buf_send_server) == 0) {	//no data freeze in flash
		buffer_tx.data_frame[28] = 0x00;
		//strcat(buf_send_server, "00");
	}
#else
	if (read_freeze_frame(time_server, buf_send_server) == 0) {	//no data freeze in flash
		buffer_tx.data_frame[28] = 0x00;
		//strcat(buf_send_server, "00");
	}
#endif

	len_data_meter = strlen(buf_send_server);
	len_data_meter /= 2;
	length_data.val = len_data_meter + 18;
	StringToHex(buffer_tx.data_frame + 29, buf_send_server);

	buffer_tx.frame.command = buffer_rx.frame.command | 0x80;
	buffer_tx.frame.length_data[0] = length_data.byte.byte0;
	buffer_tx.frame.length_data[1] = length_data.byte.byte1;

	checksum = 0;
	len = len_data_meter + 29;
	for (i = 0; i < len; i++) {
		checksum += buffer_tx.data_frame[i];
	}
	buffer_tx.data_frame[len] = checksum;
	buffer_tx.data_frame[len + 1] = 0x16;

	/*---------end protocol ------------------*/
	*len_full_buff_tx = len + 2;

	return data_return;
}
//---------------------------------------------------------------------------------------
void process_syntime(char *buff) {
	WORD_UNSIGNED length_data;

	length_data.byte.byte0 = buffer_rx.frame.meter_index[1]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.meter_index[0]; // byte MSB

	switch (length_data.val) {
	case 0x1688: //Setup time free_zone
		strcat(buff, "0000168800"); //data
		break;
	case 0x3080:
		//SYN time for module gprs
		SEC = bcd2hex(buffer_rx.frame.byte_ff[0]);
		MIN = bcd2hex(buffer_rx.frame.byte_ff[1]);
		HOUR = bcd2hex(buffer_rx.frame.byte_ff[2]);
		DOM = bcd2hex(buffer_rx.frame.byte_ff[3]);
		MONTH = bcd2hex(buffer_rx.frame.byte_ff[4]);
		YEAR = bcd2hex(buffer_rx.frame.byte_ff[5]) + 2000;
		strcat(buff, "0000308000");			//data
		break;
	default:
		break;
	}
}
unsigned char process_server_syntime_module(char *data_server,
		uint16_t *len_full_buff_tx) {
	unsigned char checksum;
	unsigned int len_data_meter, len, i;
	WORD_UNSIGNED length_data;

	for (i = 0; i < 17; i++) {
		buffer_tx.data_frame[i] = buffer_rx.data_frame[i];
	}
	length_data.byte.byte0 = buffer_rx.frame.length_data[0]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.length_data[1]; // byte MSB

	len = length_data.val - 6;

	buf_send_server[0] = 0;
	process_syntime(buf_send_server);

	len_data_meter = strlen(buf_send_server);
	len_data_meter /= 2;
	length_data.val = len_data_meter + 6;
	StringToHex(buffer_tx.data_frame + 17, buf_send_server);

	buffer_tx.frame.command = buffer_rx.frame.command | 0x80;
	buffer_tx.frame.length_data[0] = length_data.byte.byte0;
	buffer_tx.frame.length_data[1] = length_data.byte.byte1;

	checksum = 0;
	len = len_data_meter + 17;
	for (i = 0; i < len; i++) {
		checksum += buffer_tx.data_frame[i];
	}
	buffer_tx.data_frame[len] = checksum;
	buffer_tx.data_frame[len + 1] = 0x16;

	/*---------end protocol ------------------*/
	*len_full_buff_tx = len + 2;

	return true;
}
//---------------------------------------------------------------------------------------
void process_commands(char *data) {
	char code_str[5];
	unsigned int len, _index;
	char tem_data[15];

	len = strlen(commands);
	_index = 0;
	do {
		strncpy(code_str, commands + _index, 4);
		code_str[4] = 0;
		strncat(buf_send_server, code_str, 4);
		if (strstr(code_str, "3080")) {	//read time module
			//prepare time
			tem_data[0] = (SEC / 10) + 0x30;
			tem_data[1] = (SEC % 10) + 0x30;

			tem_data[2] = (MIN / 10) + 0x30;
			tem_data[3] = (MIN % 10) + 0x30;

			tem_data[4] = (HOUR / 10) + 0x30;
			tem_data[5] = (HOUR % 10) + 0x30;

			tem_data[6] = (DOM / 10) + 0x30;
			tem_data[7] = (DOM % 10) + 0x30;

			tem_data[8] = (MONTH / 10) + 0x30;
			tem_data[9] = (MONTH % 10) + 0x30;

			tem_data[10] = ((YEAR - 2000) / 10) + 0x30;
			tem_data[11] = ((YEAR - 2000) % 10) + 0x30;

			tem_data[12] = 0;

//			strcat(buf_send_server, "1000"); //length data Tx (16 bytes)
//			strncat(buf_send_server, ptr + 22, 20); //data of rx frame
			strncat(buf_send_server, tem_data, 12); //data of rx frame
		} else if (strstr(code_str, "1688")) { //Doc chu ky chot data
//			strcat(buf_send_server, "0C00"); //length data Tx (16 bytes)
//			strncat(buf_send_server, ptr + 22, 20); //data of rx frame

			if (rtc_flag.bits.mode_save_one_hour == 1) {
				strcat(buf_send_server, "0001"); //data - 60p
			} else {
				strcat(buf_send_server, "3000"); //data - 30p
			}
		} else if (strstr(code_str, "0988")) {
			strcat(buf_send_server, "0100030316207777"); //version FW GPRS
		} else if (strstr(code_str, "0A88")) { //code HW
			strcat(buf_send_server, "0601"); //version HW GPRS
		} else if (strstr(code_str, "3180")) { //read RSSI
			strncat(buf_send_server, buff_rssi, 2);
		} else if (strstr(code_str, "3280")) { //read total_offline
			read_time_offline(buf_send_server, 0);
		} else if (strstr(code_str, "3380")) { //date_offline
			read_time_offline(buf_send_server, 1);
		}

		_index += 4;
	} while (_index < len);
}
unsigned char process_server_readtime_module(char *data_server,
		uint16_t *len_full_buff_tx) {
	unsigned int len, i;
	unsigned char checksum;
	char string_data[4];
	unsigned int len_data_meter;

	WORD_UNSIGNED length_data;

	for (i = 0; i < 19; i++) {
		buffer_tx.data_frame[i] = buffer_rx.data_frame[i];
	}
	length_data.byte.byte0 = buffer_rx.frame.length_data[0]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.length_data[1]; // byte MSB

	len = length_data.val - 8;

	commands[0] = 0;
	for (i = 0; i < len; i++) {
		sprintf(string_data, "%02X", buffer_rx.data_frame[i + 19]);
		strcat(commands, string_data);
	}
	slog("comand", commands);
	buf_send_server[0] = 0;
	process_commands(buf_send_server);
	slog("buf_send_server", buf_send_server);
	len_data_meter = strlen(buf_send_server);
	len_data_meter /= 2;
	length_data.val = len_data_meter + 8;
	StringToHex(buffer_tx.data_frame + 19, buf_send_server);

	buffer_tx.frame.command = buffer_rx.frame.command | 0x80;
	buffer_tx.frame.length_data[0] = length_data.byte.byte0;
	buffer_tx.frame.length_data[1] = length_data.byte.byte1;

	checksum = 0;
	len = len_data_meter + 19;
	for (i = 0; i < len; i++) {
		checksum += buffer_tx.data_frame[i];
	}
	buffer_tx.data_frame[len] = checksum;
	buffer_tx.data_frame[len + 1] = 0x16;

	/*---------end protocol ------------------*/
	*len_full_buff_tx = len + 2;

	return true;
}
/******************************************************************************/
/*            process_server_write_mode                       */
/******************************************************************************/
/**
 * @brief  Ghi gia tri tu server vao cong to
 * @param  None
 * @retval None
 */
flag_system process_server_write_mode(char *data_server, uint16_t *len_buff) {
	WORD_UNSIGNED length_data;
	unsigned char checksum;
	unsigned int len, len_data_meter;
	char string_data[4];
	flag_system flag_sign_in;
	unsigned int i = 0;

	for (i = 0; i < 8; i++) {
		buffer_tx.data_frame[i] = buffer_rx.data_frame[i];
	}
//	buffer_tx.frame.command = buffer_rx.frame.command | 0x80;
	buffer_tx.data_frame[11] = 0; //default
	buffer_tx.data_frame[12] = 0; //default

	length_data.byte.byte0 = buffer_rx.frame.length_data[0]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.length_data[1]; // byte MSB

	len = length_data.val - 6;

	commands[0] = 0;
	for (i = 0; i < len; i++) {
		sprintf(string_data, "%02X", buffer_tx.data_frame[i + 17]);
		strcat(commands, string_data);
	}

//------------check_command length------------------------
	len = strlen(commands);
//	if (len > NUMBER_COMMAND_MAX * 4)
//		return commands_overload;
	/*---------------------------Protocol mode C--------------------------------*/
	/*---------Sign in meter----------*/
	flag_sign_in = sign_in();
	if (flag_sign_in != sign_in_ok)
		return not_enter_program_mode;
//---------write metter--------------------
	buf_send_server[0] = 0;
	write_data_metter(buf_send_server, len);
	/*---------Save data to frame tx ---------*/
	len_data_meter = strlen(buf_send_server);
	len_data_meter /= 2;
	length_data.val = len_data_meter + 6;
	StringToHex(buffer_tx.data_frame + 17, buf_send_server);

	buffer_tx.frame.command = buffer_rx.frame.command | 0x80;
	buffer_tx.frame.length_data[0] = length_data.byte.byte0;
	buffer_tx.frame.length_data[1] = length_data.byte.byte1;

	checksum = 0;
	len = len_data_meter + 17;
	for (i = 0; i < len; i++) {
		checksum += buffer_tx.data_frame[i];
	}
	buffer_tx.data_frame[len] = checksum;
	buffer_tx.data_frame[len + 1] = 0x16;

	/*---------end protocol ------------------*/
	*len_buff = len + 2;
	return process_data_ok;

}
/******************************************************************************/
/*            process_server_reading_direct                        */
/******************************************************************************/
/**
 * @brief  Doc data truc tiep
 * @param  None
 * @retval None
 */
flag_system process_server_reading_direct(char *data_server,
		uint16_t *len_full_buff_tx) {
	unsigned char checksum;
	WORD_UNSIGNED length_data;
	unsigned int len, len_data_meter;
	flag_system flag_sign_in;
	unsigned int i = 0;
	/*-------Check buffer-------------------------------------------------------*/
//flag_check_buffer = check_buffer_server(frame_tx, commands, buffer);
//-----*****-----*****-----
	for (i = 0; i < 17; i++) {
		buffer_tx.data_frame[i] = buffer_rx.data_frame[i];
	}

	length_data.byte.byte0 = buffer_rx.frame.length_data[0]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.length_data[1]; // byte MSB
//

	len = length_data.val - 33;
	printf("len=%u", len);
	printf("code=%0.4X", buffer_rx.frame.data[0]);
	/*---------------------------Protocol mode C--------------------------------*/
	/*---------Sign in meter----------*/
	flag_sign_in = sign_in();
	if (flag_sign_in != sign_in_ok)
		return not_enter_program_mode;
//---------read metter--------------------
	buf_send_server[0] = 0;
	read_metter_directmode(buf_send_server, len);
	/*---------Save data to frame tx ---------*/

	len_data_meter = strlen(buf_send_server);
	len_data_meter /= 2;
	printf("len=%u", len_data_meter);
	printf("buf=%s", buf_send_server);
	length_data.val = len_data_meter + 6;
	StringToHex(buffer_tx.data_frame + 17, buf_send_server);

	buffer_tx.frame.command = buffer_rx.frame.command | 0x80;
	buffer_tx.frame.length_data[0] = length_data.byte.byte0;
	buffer_tx.frame.length_data[1] = length_data.byte.byte1;

	checksum = 0;
	len = len_data_meter + 17;
	for (i = 0; i < len; i++) {
		checksum += buffer_tx.data_frame[i];
	}
	buffer_tx.data_frame[len] = checksum;
	buffer_tx.data_frame[len + 1] = 0x16;

	/*---------end protocol ------------------*/
	*len_full_buff_tx = len + 2;
	return process_data_ok;
}
/******************************************************************************/
/*            load_data_meter                        */
/******************************************************************************/
/**
 * @brief  
 * @param  None
 * @retval None
 */
flag_system load_data_meter(char* code_4_byte, char* data, unsigned char mode) {
	uint32_t time_out;
	uint32_t i;

	char result_BCC = 0;
	clear_para(ACK, NULL, true);
	//write commands
	if (mode == 0)
		sprintf(temp_data1, "%cW2%c%s(%s)%c", SOH, STX, code_4_byte, data,
		ETX);
	else
		sprintf(temp_data1, "%cW2%c%s(01%s)%c", SOH, STX, code_4_byte, data,
		ETX);
	for (i = 1; i < strlen(temp_data1); i++) {
		result_BCC ^= temp_data1[i];
	}
	strcat(temp_data1, &result_BCC);
	UART0_Send(temp_data1);
	time_out = 0;
	while (check_frame == false) {
		delay_ms(1);
		if (++time_out >= 3000)
			return commands_error;
	}
	return commands_ok;
}
/******************************************************************************/
/*            write_data                      */
/******************************************************************************/
/**
 * @brief  
 * @param  None
 * @retval None
 */
void write_data(char* frame_tx, char* code_server, char* code_meter, int i,
		int len, unsigned char mode) {
	char in_data[20];
	char out_data[20];
	unsigned int lenght, n;

	strncpy(in_data, commands + i + 4, len);
	in_data[len] = 0;
	lenght = strlen(in_data);
	out_data[0] = 0;
	for (n = 0; n < lenght; n += 2) {
		strncat(out_data, in_data + lenght - 2 - n, 2);
	}
	if (load_data_meter(code_meter, out_data, mode) != commands_ok)
		return;
	strcat(frame_tx, code_server);
	strcat(frame_tx, "00");
}
/******************************************************************************/
/*            write_data_metter                       */
/******************************************************************************/
/**
 * @brief  This function handles UART1 global interrupt request.
 * @param  None
 * @retval None
 */
void write_data_metter(char *frame_tx, unsigned int len_command) {
	unsigned int len;
	char four_bytes[4];
	char tem[10];
	unsigned int i;
	//---------------------------------------------------------------------
	len = len_command;
	i = 0;
	do {
		strncpy(four_bytes, commands + i, 4);
		if (strncmp(four_bytes, "1FD3", 4) == 0) {		//DoThiPhuTai
			write_data(frame_tx, "1FD3", "D31F", i, 12, 0);
			i += 12;
		} else if (strncmp(four_bytes, "02D3", 4) == 0) {	//DoThiPhuTai
			write_data(frame_tx, "02D3", "D302", i, 4, 0);
			i += 4;
		} else if (strncmp(four_bytes, "3080", 4) == 0) {	//DoThiPhuTai
			write_data(frame_tx, "3080", "C001", i, 12, 1);
			i += 12;
		} else if (strncmp(four_bytes, "05C1", 4) == 0) {	//IP1
			write_data(frame_tx, "05C1", "C105", i, 8, 0);
			i += 20;
		} else if (strncmp(four_bytes, "08C1", 4) == 0) {	//IP2
			write_data(frame_tx, "05C1", "C108", i, 8, 0);
			i += 20;
		} else if (strncmp(four_bytes, "06C1", 4) == 0) {	//PORT
			write_data(frame_tx, "06C1", "C106", i, 4, 0);
			i += 4;
		} else if (strncmp(four_bytes, "07C1", 4) == 0) {	//APN
			write_data(frame_tx, "07C1", "C107", i, 32, 0);
			system_flag.bits.RESET_CONFIG = 1;
			i += 32;
		}

		i += 1;
	} while (i < len);
	//send break command
	sprintf(tem, "%cB0%c%c", SOH, ETX, 0x71);
	UART0_Send(tem);
}
/******************************************************************************/
/*            convert_data                        */
/******************************************************************************/
/**
 * @brief  Hoan doi vi tri byte cao byte thap
 * @param  None
 * @retval None
 */
void convert_data(char* sour, char* des, unsigned int len) {
	unsigned int i;
	des[0] = 0;
	for (i = 0; i < len; i += 2) {
		strncat(des, sour + len - 2 - i, 2);
	}
}

/******************************************************************************/
/*            read_data_meter                        */
/******************************************************************************/
/**
 * @brief  
 * @param  None
 * @retval None
 */
void read_data_meter(char *frame_tx, uint16_t code, uint16_t index,
		uint8_t freeze_mode) {
	uint32_t len, length, i;
	uint8_t mode = 0xFF;
	uint32_t meter_hex;
	char *ptr_buffer;

	/* look up table code*/
	for (i = 0; i < LEN_BUFF_FREEZE; i++) {
		if (code == table_control_code[i][0]) {
			meter_hex = table_control_code[i][1];
			mode = table_control_code[i][2];
			length = table_control_code[i][3];
			break;
		}
	}
	if (i >= LEN_BUFF_FREEZE)
		return;

	sprintf(code_server, "%0.4X", code);
	sprintf(code_meter, "%0.4X", meter_hex);

	if (return_data(buff_contain_data_read_metter, code_meter) != commands_ok)
		return;

	switch (mode) {
	case normal:
		len = strlen(buff_contain_data_read_metter);
		convert_data(buff_contain_data_read_metter,
				buff_contain_data_add_send_server, len);
		ptr_buffer = buff_contain_data_add_send_server;
		break;
	case volt:
		buff_contain_data_add_send_server[0] = buff_contain_data_read_metter[1];
		buff_contain_data_add_send_server[1] = buff_contain_data_read_metter[2];
		buff_contain_data_add_send_server[2] = '0';
		buff_contain_data_add_send_server[3] = buff_contain_data_read_metter[0];
		buff_contain_data_add_send_server[4] = 0;
		ptr_buffer = buff_contain_data_add_send_server;
		break;
	case not_change_data:
		ptr_buffer = buff_contain_data_read_metter;
		break;
	case current_reverse:
		buff_contain_data_add_send_server[0] = buff_contain_data_read_metter[1];
		buff_contain_data_add_send_server[1] = '0';
		buff_contain_data_add_send_server[2] = buff_contain_data_read_metter[3];
		buff_contain_data_add_send_server[3] = buff_contain_data_read_metter[0];
		buff_contain_data_add_send_server[4] = buff_contain_data_read_metter[5];
		buff_contain_data_add_send_server[5] = buff_contain_data_read_metter[2];
		ptr_buffer = buff_contain_data_add_send_server;
		break;
	case time_max_demand:
		index = 6;
		ptr_buffer = buff_contain_data_read_metter;
		break;

	case in_active_power:
		buff_contain_data_add_send_server[0] = buff_contain_data_read_metter[5];
		buff_contain_data_add_send_server[1] = '0';
		buff_contain_data_add_send_server[2] = buff_contain_data_read_metter[3];
		buff_contain_data_add_send_server[3] = buff_contain_data_read_metter[4];
		buff_contain_data_add_send_server[4] = buff_contain_data_read_metter[1];
		buff_contain_data_add_send_server[5] = buff_contain_data_read_metter[2];
		ptr_buffer = buff_contain_data_add_send_server;
		break;

	case in_reactive_power:
		buff_contain_data_add_send_server[0] = buff_contain_data_read_metter[3];
		buff_contain_data_add_send_server[1] = buff_contain_data_read_metter[4];
		buff_contain_data_add_send_server[2] = buff_contain_data_read_metter[1];
		buff_contain_data_add_send_server[3] = buff_contain_data_read_metter[2];
		ptr_buffer = buff_contain_data_add_send_server;
		break;
	case 0xFF:
		return;
	default:
		return;
	}

	if (freeze_mode == 0)
		strncat(frame_tx, code_server, 4);
	strncat(frame_tx, ptr_buffer + index, length);
}

/******************************************************************************/
/*            read_metter_directmode                        */
/******************************************************************************/
/**
 * @brief  
 * @param  None
 * @retval None
 */
void read_metter_directmode(char *frame_tx, unsigned int len_command) {

	unsigned int len;
	char tem[10];
	unsigned int i;
	WORD_UNSIGNED usxCodeServer, usxTempCode;
//---------------------------------------------------------------------
	len = len_command / 2;
	i = 0;
	for (i = 0; i < len; i++) {
		usxTempCode.val = buffer_rx.frame.data[i];
		usxCodeServer.byte.byte0 = usxTempCode.byte.byte1;
		usxCodeServer.byte.byte1 = usxTempCode.byte.byte0;
		read_data_meter(frame_tx, usxCodeServer.val, 0, 0);
		printf("code=%0.4X", usxCodeServer.val);
	}
//send break command
	sprintf(tem, "%cB0%c%c", SOH, ETX, 0x71);
	UART0_Send(tem);
}
//======================================================================================
