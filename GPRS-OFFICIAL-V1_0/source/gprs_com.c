#include "gprs_com.h"
//---------------------------------------------------------------------------------
extern _system_flag system_flag;
extern _program_counter program_counter;
extern _uart1_rx_frame uart1_rx;
extern _uart_frame uart1_frame;
extern _uart_frame buffer_rx, buffer_tx;
extern char buf_send_server[MAX_BUFFER_TX];
extern PARA_PLC para_plc;
extern CONFIG_NEWWORK config_network;
extern unsigned int lenght_buff_tx;
extern __stateModule stateModule;

extern char buff_rssi[5];
//-------------------------------------------------------------------------------------------------
unsigned char check_responde_SIM800(char *string_compare,
		unsigned char second_wait);
void restart_gprs(void);
//-------------------------------------------------------------------------------------------------
void convert_array_hex2string(unsigned char *hex_array, char *string,
		unsigned int length_hex) {
	unsigned int i, j;
	unsigned char a;
	for (i = 0, j = 0; i < length_hex; i++) {
		a = (hex_array[i] >> 4);
		if (a <= 0x09)
			string[j] = a + 0x30;
		else
			string[j] = a + 0x37;
		//------------------------
		a = hex_array[i] & 0x0f;
		if (a <= 0x09)
			string[j + 1] = a + 0x30;
		else
			string[j + 1] = a + 0x37;
		//------------------------
		j += 2;
	}
	string[j] = 0;	//add NULL to end string
	uart1_rx.para_rx.state_buf_rx = BUF_RX_FULL;
}
//-------------------------------------------------------------------------------------------------
void get_RSSI_signal(void) {
	char *ptr_data;
//	if (system_flag.bits.GET_RSSI == 0)
//		return;
//	system_flag.bits.GET_RSSI = 0; //clear flag
	prepare_command_gprs(UART_WAIT_RESPONDE, '+');
	printf("AT+CSQ\r");
	//wait_response_command_gprs(2);
	check_responde_SIM800("OK", 3);

	ptr_data = strstr(uart1_rx.buffer_rx.buf_response_command, "+CSQ:");
	if (ptr_data != NULL) {
		if (*(ptr_data + 7) == ',') {
			buff_rssi[1] = *(ptr_data + 6);
			buff_rssi[0] = '0';

		} else {
			buff_rssi[0] = *(ptr_data + 6);
			buff_rssi[1] = *(ptr_data + 7);
		}
		buff_rssi[2] = 0;
	}
}
void checkPowerOnOffGPRS(void) {
	uint8_t loop, i;
	/*check respond from gprs*/
	for (loop = 0; loop < 3; loop++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT\r");
		for (i = 0; i < 20; i++) {
			delay_n50ms(2);
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "AT OK")) {
				stateModule = OFF_GPRS;
				return;

			} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "OK")) {
				stateModule = OFF_GPRS;
				return;
			}
		}
	}
	/*state off module*/
	stateModule = ON_GPRS;
}
void offModuleGprs(void) {
	uint8_t loop, i;
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 1);
	delay_nsecond(3);	//delay_3s
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 0);
	//*****-----*****-----
	delay_nsecond(3);	//delay 3s

//	/*wait status gprs*/
//	for (loop = 0; loop < 1; loop++) {
//		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
//		//printf("AT\r");
//		for (i = 0; i < 200; i++) {
//			delay_n50ms(2);
//			if (strstr(uart1_rx.buffer_rx.buf_response_command,
//					"NORMAL POWER DOWN")) {
//				stateModule = ON_GPRS;
//				return;
//			}
////			if (strstr(uart1_rx.buffer_rx.buf_response_command, "AT OK")) {
////				stateModule = ON_GPRS;
////				return;
////
////			} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "OK")) {
////				stateModule = ON_GPRS;
////				return;
////			}
//		}
//	}
	stateModule = ON_GPRS;
}

//		}

//-------------------------------------------------------------------------------------------------
void onOffModuleGprs(void) {
	uint8_t loop, i;
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 1);
	delay_nsecond(1);	//delay_500ms
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 0);
//*****-----*****-----
	delay_nsecond(3);	//delay 3s

	/*wait status gprs
	 * max respond 10s
	 * */
	for (loop = 0; loop < 10; loop++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CGATT?\r");
		for (i = 0; i < 40; i++) {
			delay_n50ms(2);
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "CGATT: 1")) {
				stateModule = READ_PARA_CONFIG;
				return;
			} else if (strstr(uart1_rx.buffer_rx.buf_response_command,
					"CGATT: 0")) {
				break;
			} else if (strstr(uart1_rx.buffer_rx.buf_response_command,
					"ERROR")) {
				break;

			}
		}
	}
	//stateModule = STATE_MODULE;

}
bool ON_OFF_mudule_GPRS(void) {
	unsigned char i, j;
//restart_gprs();
	for (i = 0; i < 7; i++) {
		//*****-----*****-----
		GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 1);
		delay_nsecond(1);	//delay_500ms
		GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 0);
		//*****-----*****-----
		delay_nsecond(10);	//delay 3s
		//----------
		clear_array(uart1_rx.buffer_rx.buf_response_command, 70);
		uart1_rx.para_rx.state_uart = UART_WAIT_RESPONDE;
		uart1_rx.para_rx.counter_rx = 0;
		printf("AT\r");
		delay_nsecond(2);
		//*****-----*****-----
		if (strstr(uart1_rx.buffer_rx.buf_response_command, "AT OK")) {
			delay_nsecond(20);
			return true;
		} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "OK")) {
			delay_nsecond(20);
			return true;
		}
		j = false;
	}
	return j;
}
//-------------------------------------------------------------------------------------------------
void prepare_command_gprs(unsigned char process, char header_rx) {
	switch (process) {
	case UART_WAIT_RESPONDE:
		uart1_rx.para_rx.flag.data_bits = 0x00;
		clear_array(uart1_rx.buffer_rx.buf_response_command, 70);
		uart1_rx.para_rx.state_uart = UART_WAIT_RESPONDE;
		uart1_rx.para_rx.counter_rx_command = 0;
		break;
	default:
		break;
	}
}
//-------------------------------------------------------------------------------------------------
unsigned char check_responde_SIM800(char *string_compare,
		unsigned char second_wait) {
	unsigned int i, time_wait;
	unsigned char ret;

	time_wait = second_wait * 10;
	ret = SIM800_NO_RESPONSE;

	for (i = 0; i < time_wait; i++) {
		if (strstr(uart1_rx.buffer_rx.buf_response_command, string_compare)) {
			ret = SIM800_RESPONSE_CORRECT;
			break;
		} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
			ret = SIM800_RESPONSE_ERROR;
			break;
		} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "OK")) {
			ret = SIM800_RESPONSE_OK;
			break;
		} else {
			delay_n50ms(2);
		}
	}
//delay_n50ms(1);
	return ret;
}
//void init_gprs(){
//
//}
//-------------------------------------------------------------------------------------------------
void establishTCPClient(void) {
	unsigned char i, j;

	/*disable echo AT command*/
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("ATE0&W\r");
		if (check_responde_SIM800("OK", 3) == SIM800_RESPONSE_CORRECT)
			break;
	}
	/*close the connection*/
	prepare_command_gprs(UART_WAIT_RESPONDE, LF);
	printf("AT+CIPSHUT\r");
	for (i = 0; i < 30; i++) {
		delay_n50ms(2);
		if (strstr(uart1_rx.buffer_rx.buf_response_command, "SHUT OK")) {
			break;
		} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
			break;
		}
	}
	/*	Start task and set APN
	 * The default APN is "CMNET" with no username and password
	 * */
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CSTT=\"CMNET\"\r");
		j = check_responde_SIM800("OK", 3);
		if (j == SIM800_RESPONSE_CORRECT)
			break;
		else if (j == SIM800_RESPONSE_ERROR)
			break;
	}
	/*
	 * Bring up wireless connection
	 */
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CIICR\r");
		j = check_responde_SIM800("OK", 3);
		if (j == SIM800_RESPONSE_CORRECT)
			break;
		else if (j == SIM800_RESPONSE_ERROR)
			break;
	}
	/*
	 * Get local IP address
	 */
	printf("AT+CIFSR\r");
	delay_nsecond(2);
//	/*
//	 * Get APN
//	 */
//
//	for (i = 0; i < 3; i++) {
//		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
//		printf("AT+COPS?\r");
//		//wait_response_command_gprs(2);
//		delay_nsecond(2);
//		if ((strstr(uart1_rx.buffer_rx.buf_response_command, "Mobifone"))) {
//			strcpy(config_network._APN, "m-wap");
//			break;
//			//Mobifone:m-wap
//			//vinafone: m3-world
//			//viettel: e-connect (VIETTEL
//			//EVN: e-wap
//			//Vietnammobile: internet
//		} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "Viettel")) {
//			strcpy(config_network._APN, "e-connect");
//			break;
//		} else if ((strstr(uart1_rx.buffer_rx.buf_response_command,
//				"VN VINAPHONE"))
//				|| strstr(uart1_rx.buffer_rx.buf_response_command, "VINAFONE")
//				|| (strstr(uart1_rx.buffer_rx.buf_response_command, "GPC"))) {
//			strcpy(config_network._APN, "m3-world");
//			break;
//		}
//	}
}
//-------------------------------------------------------------------------------------------------
void connectToServerNow(void) {
	unsigned char i, k;
	char ip_server[15];

	strcpy(ip_server, config_network._IP1);	//connect to IP1
//=================================================
	for (i = 0; i < 3; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		system_flag.bits.CONNECT_OK = 0; //clear flag before send Connect command
		printf("AT+CIPSTART=\"TCP\",\"%s\",%s\r", ip_server,
				config_network._PORT); //send IP & Port for connect to server
		for (k = 0; k < 200; k++) {
			delay_n50ms(2);
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "CONNECT OK")) {
				stateModule = LOGIN;
				//delay_nsecond(1);
				break;
			} else if (strstr(uart1_rx.buffer_rx.buf_response_command,
					"ALREADY CONNECT")) {
				stateModule = READY_CONNECT;
				//delay_nsecond(1);
				break;
			} else if (strstr(uart1_rx.buffer_rx.buf_response_command,
					"CONNECT FAIL")) {
				break;
			}
		}

		if (stateModule == LOGIN || stateModule == READY_CONNECT) {
			break;
		}
		//delay_nsecond(2);
	}
	if (i >= 3)
		stateModule = INIT_GPRS;

}
void prepare_trans_data_test(void) {
	unsigned int length;
	unsigned char counter, k;

	length = strlen(buf_send_server);
	for (counter = 0; counter < 3; counter++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CIPSEND=%u\r", length / 2);
		for (k = 0; k < 100; k++) {
			delay_n50ms(1);
			if (uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK)
				break;
			else if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
				system_flag.bits.CONNECT_OK = 0;
				system_flag.bits.SEND_ERROR = 1;
				break;
			} else {
				system_flag.bits.CONNECT_OK = 0;
				system_flag.bits.SEND_ERROR = 1;
			}
		}
		//---------------------
		if (uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK)
			break;
		else
			delay_nsecond(2);
	}
}
//-------------------------------------------------------------------------------------------------
void prepare_trans_data(uint16_t len_frame) {

	unsigned char counter, k;

//	length = strlen(buf_send_server);
	for (counter = 0; counter < 3; counter++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CIPSEND=%u\r", lenght_buff_tx);
		for (k = 0; k < 100; k++) {
			delay_n50ms(1);
			if (uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK)
				return;
			else if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
				stateModule = LOST_CONNECT;
				break;
			}

		}
		//delay_nsecond(2);
	}
}
//-------------------------------------------------------------------------------------------------
//---Process: transmit frame data (buf_send_server) to SERVER--------------------------------------
//---buf_send_server: contain data by STRING, before transmit, convert to HEX data-----------------
void trans_data_server(uint16_t len_frame) {
	unsigned int i;

	unsigned char k;

	/*send lenght data*/
	prepare_trans_data(len_frame);

	if (uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK == 0) { //can not send data
		stateModule = LOST_CONNECT;
		return;
	}

	/*send data*/
	prepare_command_gprs(UART_WAIT_RESPONDE, LF);
	for (i = 0; i < lenght_buff_tx; i++) {
		UART1_send_HEX(buffer_tx.data_frame[i]);
	}
	UART1_send_HEX(0x1A);	//command to send data

	for (k = 0; k < 300; k++) {
		delay_n50ms(2);
		if (strstr(uart1_rx.buffer_rx.buf_response_command, "SEND OK")) {
			stateModule = READY_CONNECT;
			break;
		}
		if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
			stateModule = LOST_CONNECT; //Alarm send error <=> Can not send data to server
			break;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void send_command_to_server(unsigned char command) {
	unsigned char checksum;
	char hexstring[7];
	unsigned int len, i;

	StringToHex(hexstring, para_plc._ID);
	buffer_tx.frame.start_code = 0x68;
	buffer_tx.frame.id_array[0] = hexstring[2];
	buffer_tx.frame.id_array[1] = hexstring[3];
	buffer_tx.frame.id_array[2] = hexstring[5];
	buffer_tx.frame.id_array[3] = hexstring[4];
	buffer_tx.frame.check_count[0] = 0x00;
	buffer_tx.frame.check_count[1] = 0x00;
	buffer_tx.frame.restart_code = 0x68;
	buffer_tx.frame.command = command;

	buffer_tx.frame.length_data[0] = 0x03;
	buffer_tx.frame.length_data[1] = 0x00;

	/*password 3 bytes*/
	buffer_tx.frame.id_meter[0] = 0x11;
	buffer_tx.frame.id_meter[1] = 0x11;
	buffer_tx.frame.id_meter[2] = 0x11;

	len = 14;
	checksum = 0;
	for (i = 0; i < len; i++) {
		checksum += buffer_tx.data_frame[i];
	}
	buffer_tx.data_frame[len] = checksum;
	buffer_tx.data_frame[len + 1] = 0x16;

	lenght_buff_tx = 16;

//	trans_data_server_test();

}
//-------------------------------------------------------------------------------------------------
void get_command_from_server(char *data_process) {
	char *ptr_data;
	uint8_t uTemp;

	ptr_data = data_process;

//-------ptr_data is pointing the frame data (68xxxx16)--------------------------
	if (check_data_rx_server(ptr_data) != ok_frame)
		return;		//frame error
//---------frame rx from server OK------------------------------------------------

	process_data_rx_from_server(buffer_rx.data_frame, buffer_rx.frame.command);

	//uart1_rx.para_rx.state_uart = UART_IDLE;
	uart1_rx.para_rx.flag.bits.IDLE_RECEIVING = 0;
	uart1_rx.para_rx.uart_state = UART_STATE_NOTHING;
}
//---------------------------------------------------------------------------------------
flag_system check_data_rx_server(char *data_server) {
	WORD_UNSIGNED length_data;
	unsigned int i, lenFullFrame;
	unsigned char checksum;
	char hexstring[7];

//------check start_frame---------
	if (buffer_rx.frame.start_code != 0x68)
		return error_start_code;
//log("start code ok");
	/*-------Check oSverflow frame----------------------------------------------------*/

	/*-------Check byte check_count----------------------------------------------------*/

	/*-------Check re_start code-------------------------------------------------------*/
	if (buffer_rx.frame.restart_code != 0x68)
		return error_restart_code;
//log("restart code ok");
	/*-------Check ID DCU-------------------------------------------------------*/
	StringToHex(hexstring, para_plc._ID);
	swap_byte(&hexstring[4], &hexstring[5]);
	for (i = 0; i < 4; i++) {
		if (buffer_rx.frame.id_array[i] != hexstring[i + 2])
			return error_id_dcu;
	}
//log("check id meter ok");

	/*------------------------------------------------------------------------------*/
	/*-------Get Length of data & check----------------------------------------------*/
	length_data.byte.byte0 = buffer_rx.frame.length_data[0]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.length_data[1]; // byte MSB

//hLog("len", length_data.val);

	if (length_data.val > (NUMBER_COMMAND_MAX * 2 + 33))
		return commands_overload;

	lenFullFrame = length_data.val + 11; //position of checksum
	/*-------check byte end_frame----------------------------------------------*/
	if (buffer_rx.data_frame[lenFullFrame + 1] != 0x16)
		return error_end_frame;
	/*-------check byte checksum of frame--------------------------------------*/
	checksum = 0;
	for (i = 0; i < lenFullFrame; i++)
		checksum += buffer_rx.data_frame[i];
	if (checksum != buffer_rx.data_frame[lenFullFrame])
		return error_check_sum;
	/*----------------FRAME OK - NO ERROR----------------------------------------------*/
//log("check check_sum ok");
	return ok_frame;
}
//----------------------------------------------------------------------------------------------------------------------
void process_data_rx_from_server(char *data_server, unsigned char control_code) {
	char *ptr;
	unsigned char i;
	uint16_t len_buff;
//--------------
	ptr = data_server;
//--------------
	switch (control_code) {
	case COMMAND_READ_PARA_DIRECT: //Control_code = 0x11
		i = process_server_reading_direct(ptr, &len_buff);
		if (i == process_data_ok) {
//			trans_data_server(len_buff);
			stateModule = UPDATE_DATA_SERVER;
		} else
			stateModule = READY_CONNECT;
		break;
	case COMMAND_ACCEPT_LOGIN:
		stateModule = READY_CONNECT;
		break;
	case COMMAND_ACCEPT_SOCKET:
		stateModule = READY_CONNECT;
		break;
	case COMMAND_LOAD_CURVE:
		i = process_server_write_mode(ptr, &len_buff);
		if (i == process_data_ok) {
			stateModule = UPDATE_DATA_SERVER;
		} else
			stateModule = READY_CONNECT;

		break;
	case COMMAND_READ_DATA_SAVED:
		process_server_reading_data_save(ptr, &len_buff);
		stateModule = UPDATE_DATA_SERVER;
		break;
	case COMMAND_SYN_TIME_MODULE:
		i = process_server_syntime_module(ptr, &len_buff);
		if (i)
			stateModule = UPDATE_DATA_SERVER;
		else
			stateModule = READY_CONNECT;
		break;
	case COMMAND_READ_TIME_MODULE:
		process_server_readtime_module(ptr, &len_buff);
		stateModule = UPDATE_DATA_SERVER;
		break;
	default:
		stateModule = READY_CONNECT;
		break;
	}
}
//----------------------------------------------------------------------------------------------------------------------
void send_data_to_server(void) {

	if ((system_flag.bits.LOGIN_OK == 0)
			&& (system_flag.bits.TIMEOUT_WAIT_LOGIN)) { //login not yet
//system_flag.bits.SEND_LOGIN = 0;	//clear flag
		send_command_to_server(COMMAND_LOGIN);
	} else if (system_flag.bits.SEND_SOCKET) {
		system_flag.bits.SEND_SOCKET = 0;	//clear flag
		send_command_to_server(COMMAND_SOCKET);
	}
}
//-------------------------------------------------------------------------------------------------
void clear_array(char *array, unsigned int length) {
	unsigned int i;

	for (i = 0; i < length; i++)
		array[i] = 0x00;
}
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
void restart_gprs(void) {
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_3, 1);
	delay_nsecond(3);
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_3, 0);
}
//----------------------------------------------------------------------------------------------------------------------
