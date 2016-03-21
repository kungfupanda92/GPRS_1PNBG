#include "gprs_com.h"
//---------------------------------------------------------------------------------
extern _system_flag system_flag;
extern _program_counter program_counter;
extern _uart1_rx_frame uart1_rx;
extern _uart_frame uart1_frame;
extern _uart_frame buffer_rx,buffer_tx;
extern char buf_send_server[MAX_BUFFER_TX];
extern PARA_PLC para_plc;
extern CONFIG_NEWWORK config_network;

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
	if (system_flag.bits.GET_RSSI == 0)
		return;
	system_flag.bits.GET_RSSI = 0; //clear flag
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
//-------------------------------------------------------------------------------------------------
bool ON_OFF_mudule_GPRS(void) {
	unsigned char i, j;
	//restart_gprs();
	for (i = 0; i < 7; i++) {
		//*****-----*****-----
		GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 1);
		delay_nsecond(2);	//delay_3s
		GPIO_WriteBit(GPIO_P0, GPIO_PIN_23, 0);
		//*****-----*****-----
		delay_nsecond(10);	//delay 10s
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
		uart1_rx.para_rx.counter_rx = 0;
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

	time_wait = second_wait * 100;
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
			delay_n50ms(1);
		}
	}
	delay_n50ms(1);
	return ret;
}

//-------------------------------------------------------------------------------------------------
void prepare_config_mudule(void) {
	unsigned char i, j;

	//read_para();	//load para from metter: ID, IP, PORT, APN
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("ATE0&W\r");
		if (check_responde_SIM800("OK", 3) == SIM800_RESPONSE_CORRECT)
			break;
	}

	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CIPCLOSE\r");
		j = check_responde_SIM800("OK", 3);
		if (j == SIM800_RESPONSE_CORRECT)
			break;
		else if (j == SIM800_RESPONSE_ERROR)
			break;
	}

	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CGATT=1\r");		//Attach to gprs
		if (check_responde_SIM800("OK", 5))
			break;
	}
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CGACT=1,1\r");
		if (check_responde_SIM800("OK", 5))
			break;
	}
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CGCLASS=\"B\"\r");
		j = check_responde_SIM800("OK", 3);
		if (j == SIM800_RESPONSE_CORRECT)
			break;
		else if (j == SIM800_RESPONSE_ERROR)
			break;
	}
	//delay_nsecond(2);
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CIPMUX=0\r");
		j = check_responde_SIM800("OK", 3);
		if (j == SIM800_RESPONSE_CORRECT)
			break;
		else if (j == SIM800_RESPONSE_ERROR)
			break;
	}

	for (i = 0; i < 3; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+COPS?\r");
		//wait_response_command_gprs(2);
		delay_nsecond(2);
		if ((strstr(uart1_rx.buffer_rx.buf_response_command, "Mobifone"))) {
			strcpy(config_network._APN, "m-wap");
			break;
			//vinaphone: m3-world
			//viettel: e-connect (VIETTEL
			//EVN: e-wap
			//Vietnammobile: internet
		} else if (strstr(uart1_rx.buffer_rx.buf_response_command, "Viettel")) {
			strcpy(config_network._APN, "e-connect");
			break;
		} else if ((strstr(uart1_rx.buffer_rx.buf_response_command,
				"VN VINAPHONE"))
				|| (strstr(uart1_rx.buffer_rx.buf_response_command, "GPC"))) {
			strcpy(config_network._APN, "m3-world");
			break;
		}
	}

	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CGDCONT=1,\"IP\",\"%s\"\r", config_network._APN);//Send APN
		j = check_responde_SIM800("OK", 3);
		if (j == SIM800_RESPONSE_CORRECT)
			break;
		else if (j == SIM800_RESPONSE_ERROR)
			break;
	}
	quick_connect();
}
//-------------------------------------------------------------------------------------------------
void quick_connect(void) {
	unsigned char i, k;
	char ip_server[15];
//-----------------------
	//if (system_flag.bits.IP_CONNECT == 0)
		strcpy(ip_server, config_network._IP1);	//connect to IP1
	//else
		//strcpy(ip_server, config_network._IP2);	//connect to IP2
//-----------------------
	for (i = 0; i < 2; i++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		system_flag.bits.CONNECT_OK = 0; //clear flag before send Connect command
		printf("AT+CIPSTART=\"TCP\",\"%s\",%s\r", ip_server,
				config_network._PORT); //send IP & Port for connect to server
		for (k = 0; k < 100; k++) {
			delay_n50ms(1);
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "CONNECT OK")) {
				//system_flag.bits.SEND_LOGIN = 1;			//send command login
				system_flag.bits.LOGIN_OK = 0;			//send command login
				system_flag.bits.TIMEOUT_WAIT_LOGIN = 1;
				system_flag.bits.CONNECT_OK = 1;
				delay_nsecond(1);
				break;
			} else if (strstr(uart1_rx.buffer_rx.buf_response_command,
					"ALREADY CONNECT")) {
				//system_flag.bits.SEND_LOGIN = 1;			//send command login
				system_flag.bits.LOGIN_OK = 0;			//send command login
				system_flag.bits.TIMEOUT_WAIT_LOGIN = 1;
				system_flag.bits.CONNECT_OK = 1;
				delay_nsecond(1);
				break;
			} else if (strstr(uart1_rx.buffer_rx.buf_response_command,
					"CONNECT FAIL")) {
				system_flag.bits.SEND_ERROR = 1;//can not send data to server
				system_flag.bits.CONNECT_OK = 0;
				break;
			}
		}

		if (system_flag.bits.CONNECT_OK == 1) {
			break;
		}
		delay_nsecond(2);
	}
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
	unsigned int length;
	unsigned char counter, k;

//	length = strlen(buf_send_server);
	for (counter = 0; counter < 3; counter++) {
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		printf("AT+CIPSEND=%u\r", len_frame);
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
//---Process: transmit frame data (buf_send_server) to SERVER--------------------------------------
//---buf_send_server: contain data by STRING, before transmit, convert to HEX data-----------------
void trans_data_server(uint16_t len_frame) {
	unsigned int length, i, j;
	unsigned char byte_send;
	unsigned char a, b;
	unsigned char counter, k;

	prepare_trans_data(len_frame);
//-------------------------------------------------------
	if (uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK == 0) { //can not send data
		if (uart1_rx.para_rx.quick_connect < 3) {
			quick_connect();
			uart1_rx.para_rx.quick_connect++;
		} else {
			printf("Deo re_connect duoc\r");
			return;
		}

		if (system_flag.bits.CONNECT_OK == 1) {
			prepare_trans_data(len_frame);
		} else {
			printf("AT+CIPCLOSE\r");
			delay_nsecond(5);
		}
	}
//	length = strlen(buf_send_server);
	for (counter = 0; counter < 3; counter++) {
		delay_n50ms(1);
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		for (i = 0; i < len_frame; i++) {
				UART1_send_HEX(buffer_tx.data_frame[i]);
		}
		UART1_send_HEX(0x1A);	//command to send data

		for (k = 0; k < 100; k++) {
			delay_n50ms(1);
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "SEND OK")) {
				system_flag.bits.SEND_ERROR = 0;	//send Data to sever ok
				uart1_rx.para_rx.quick_connect = 0;
				break;
			}
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
				system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
				break;
			} else {
				system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
			}
		}
		if (system_flag.bits.SEND_ERROR == 0) {
			uart1_rx.para_rx.state_uart = UART_IDLE;
			break;
		}
	}
//------------------------------------------------------------------
	if (counter >= 3) {
		system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
		system_flag.bits.LOGIN_OK = 0;
		system_flag.bits.SOCKET_OK = 0;
	}
//------------------------------------------------------------------
	printf("counter = %u\r", counter);
	if (system_flag.bits.SEND_ERROR) {
		printf("data send error\r");
	} else {
		printf("data send ok\r");
	}
}
//void trans_data_server(uint16_t len_frame) {
//	unsigned int i;

//	unsigned char counter, k;

//	prepare_command_gprs(UART_WAIT_PRESEND, LF);
//	printf("AT+CIPSEND=%u\r", len_frame);
//	for (counter = 0; counter < 3; counter++) {
//		if (wait_response_command_gprs(5) == MODULE_PRESEND_OK) {
//			delay_n50ms(2);
//			prepare_command_gprs(UART_SEND_DATA, LF);
//			for (i = 0; i < len_frame; i++) {
//				UART1_send_HEX(buffer_tx.data_frame[i]);
//			}
//			UART1_send_HEX(0x1A);	//command to send data

//			k = wait_response_command_gprs(5);
//			if (k == MODULE_SEND_OK) {
//				system_flag.bits.SEND_ERROR = 0;	//send Data to sever ok
//			} else {
//				system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
//				system_flag.bits.LOGIN_OK = 0;
//				system_flag.bits.SOCKET_OK = 0;
//			}
//			uart1_rx.para_rx.state_uart = UART_IDLE;
//			uart1_rx.para_rx.flag.data_bits = 0;

//			break;
//		}
//	}
//	//------------------------------------------------------------------
//	if (counter >= 3) {
//		system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
//		system_flag.bits.LOGIN_OK = 0;
//		system_flag.bits.SOCKET_OK = 0;
//	}
//	//------------------------------------------------------------------
//	printf("counter = %u\r", counter);
//	if (system_flag.bits.SEND_ERROR) {
//		printf("data send error\r");
//	} else {
//		printf("data send ok\r");
//	}
//}

void trans_data_server_test(void) {
		unsigned int length, i, j;
	unsigned char byte_send;
	unsigned char a, b;
	unsigned char counter, k;

	prepare_trans_data_test();
//-------------------------------------------------------
	if (uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK == 0) { //can not send data
		if (uart1_rx.para_rx.quick_connect < 3) {
			quick_connect();
			uart1_rx.para_rx.quick_connect++;
		} else {
			printf("Deo re_connect duoc\r");
			return;
		}

		if (system_flag.bits.CONNECT_OK == 1) {
			prepare_trans_data_test();
		} else {
			printf("AT+CIPCLOSE\r");
			delay_nsecond(5);
		}
	}
	length = strlen(buf_send_server);
	for (counter = 0; counter < 3; counter++) {
		delay_n50ms(1);
		prepare_command_gprs(UART_WAIT_RESPONDE, LF);
		for (j = 0, i = 0; i < length / 2; i++) {
			a = convert_1byte_string2hex(buf_send_server[j]);
			b = convert_1byte_string2hex(buf_send_server[j + 1]);
			byte_send = (a << 4) | b;
			UART1_send_HEX(byte_send);
			j += 2;
		}
		UART1_send_HEX(0x1A);	//command to send data

		for (k = 0; k < 100; k++) {
			delay_n50ms(1);
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "SEND OK")) {
				system_flag.bits.SEND_ERROR = 0;	//send Data to sever ok
				uart1_rx.para_rx.quick_connect = 0;
				break;
			}
			if (strstr(uart1_rx.buffer_rx.buf_response_command, "ERROR")) {
				system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
				break;
			} else {
				system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
			}
		}
		if (system_flag.bits.SEND_ERROR == 0) {
			uart1_rx.para_rx.state_uart = UART_IDLE;
			break;
		}
	}
//------------------------------------------------------------------
	if (counter >= 3) {
		system_flag.bits.SEND_ERROR = 1; //Alarm send error <=> Can not send data to server
		system_flag.bits.LOGIN_OK = 0;
		system_flag.bits.SOCKET_OK = 0;
	}
//------------------------------------------------------------------
	printf("counter = %u\r", counter);
	if (system_flag.bits.SEND_ERROR) {
		printf("data send error\r");
	} else {
		printf("data send ok\r");
	}
}
//-------------------------------------------------------------------------------------------------
void send_command_to_server(unsigned char command) {
	char _check_sum[2];

	buf_send_server[0] = 0;
	strcat(buf_send_server, "68");
	strncat(buf_send_server, para_plc._ID + 4, 4);
	strncat(buf_send_server, para_plc._ID + 10, 2);
	strncat(buf_send_server, para_plc._ID + 8, 2);

	strcat(buf_send_server, "0000");
	strcat(buf_send_server, "68");
//-----*****-----*****-----
	switch (command) {
	case COMMAND_LOGIN:
		program_counter.timer_wait_response_login = 0;
		program_counter.timer_send_socket = 0;
		system_flag.bits.TIMEOUT_WAIT_LOGIN = 0;
		system_flag.bits.LOGIN_OK = 0;
		system_flag.bits.SEND_SOCKET = 0;
		strcat(buf_send_server, "A1");
		break;
	case COMMAND_SOCKET:
		system_flag.bits.SOCKET_OK = 0;
		strcat(buf_send_server, "A4");
		break;
	}
//-----*****-----*****-----
	strcat(buf_send_server, "0300");	//length data
	strcat(buf_send_server, "111111");	//password

	sprintf(_check_sum, "%02X", caculate_checksum(buf_send_server));
	strcat(buf_send_server, _check_sum);
	strcat(buf_send_server, "16");


	trans_data_server_test();

}
//-------------------------------------------------------------------------------------------------
void get_command_from_server(char *data_process) {
	char *ptr_data;
	uint8_t uTemp;
	ptr_data = data_process;
	
	printf("ZO DAY");
	uTemp=check_data_rx_server(ptr_data);
	printf("return %u",uTemp);
//	//-------ptr_data is pointing the frame data (68xxxx16)--------------------------
//	if (check_data_rx_server(ptr_data) != ok_frame)
//		return;		//frame error
//	//---------frame rx from server OK------------------------------------------------

//	//process_data_rx_from_server(ptr_data, i);
	process_data_rx_from_server(buffer_rx.data_frame, buffer_rx.frame.command);

	uart1_rx.para_rx.state_uart = UART_IDLE;
	uart1_rx.para_rx.flag.bits.IDLE_RECEIVING = 0;
	uart1_rx.para_rx.uart_state = UART_STATE_NOTHING;
}
//---------------------------------------------------------------------------------------
flag_system check_data_rx_server(char *data_server) {
	WORD_UNSIGNED length_data;
	unsigned int i;
	char hexstring[7];
	//--------------------
	//--------------------
	//------check start_frame---------
	if (buffer_rx.frame.start_code != 0x68)
		return error_start_code;
	printf("qua 1");
	/*-------Check overflow frame----------------------------------------------------*/

	/*-------Check byte check_count----------------------------------------------------*/

	/*-------Check re_start code-------------------------------------------------------*/
	if (buffer_rx.frame.restart_code != 0x68)
		return error_restart_code;
	printf("qua 2");
	/*-------Check ID DCU-------------------------------------------------------*/
	StringToHex(hexstring, para_plc._ID);
	for (i = 0; i < 6; i++) {
		if (buffer_rx.frame.id_meter[i] != hexstring[5 - i])
			return error_id_dcu;
	}
	printf("qua 3");

	/*------------------------------------------------------------------------------*/
	/*-------Get Length of data & check----------------------------------------------*/
	length_data.byte.byte0 = buffer_rx.frame.length_data[0]; // byte LSB
	length_data.byte.byte1 = buffer_rx.frame.length_data[1]; // byte MSB
	printf("len=%4X",length_data.val);
	if (length_data.val > (NUMBER_COMMAND_MAX * 2 + 33))
		return commands_overload;
	/*-------check byte end_frame----------------------------------------------*/
//	i = length_data.val * 2 + LENG_POS + 4 + 2; //position of end frame
//	if ((*(ptr + i) != '1') || (*(ptr + i + 1) != '6'))
//		return error_end_frame;
	/*-------check byte checksum of frame----------------------------------------------*/
//	i = length_data.val * 2 + LENG_POS + 4; //position of checksum
//	temp_byte = 0;
//	for (j = 0; j < i; j += 2)
//		temp_byte += convert_string2hex(ptr + j);
//	if (temp_byte != convert_string2hex(ptr + i))
//		return error_check_sum;
	/*----------------FRAME OK - NO ERROR----------------------------------------------*/
	printf("qua 4");
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
		i = process_server_reading_direct(ptr,&len_buff);
		if (i == process_data_ok) {
			trans_data_server(len_buff);
		} else if (i == error_frame)
			uart1_rx.para_rx.unread++;
		else if (i == command_login_ok)
			system_flag.bits.LOGIN_OK = 1;
		else if (i == command_socket_ok)
			system_flag.bits.SOCKET_OK = 1;
		//delay_nsecond(2);
		break;
	case COMMAND_ACCEPT_LOGIN:
		system_flag.bits.LOGIN_OK = 1;
		break;
	case COMMAND_ACCEPT_SOCKET:
		system_flag.bits.SOCKET_OK = 1;
		break;
	case COMMAND_LOAD_CURVE:
		i = process_server_write_mode(ptr,&len_buff);
		if (i == process_data_ok) {
			trans_data_server(len_buff);
		} else if (i == error_frame)
			uart1_rx.para_rx.unread++;
		else if (i == command_login_ok)
			system_flag.bits.LOGIN_OK = 1;
		else if (i == command_socket_ok)
			system_flag.bits.SOCKET_OK = 1;
		//delay_nsecond(2);
		break;
	case COMMAND_READ_DATA_SAVED:
		process_server_reading_data_save(ptr,&len_buff);
		//answer_server();
		trans_data_server(len_buff);
		break;
	case COMMAND_SYN_TIME_MODULE:
		i = process_server_syntime_module(ptr,&len_buff);
		if (i)
			//answer_server();
			trans_data_server(len_buff);
		else
			uart1_rx.para_rx.unread++;
		break;
	case COMMAND_READ_TIME_MODULE:
		process_server_readtime_module(ptr,&len_buff);
		//answer_server();
		trans_data_server(len_buff);
		break;
	default:
		printf("UNKNOW COMMAND SERVER\r");
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
