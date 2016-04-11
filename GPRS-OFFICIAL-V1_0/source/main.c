#include "main.h"

//*****-----*****-----*****-----*****-----
extern _rtc_flag rtc_flag;
extern _system_flag system_flag;
extern PARA_PLC para_plc;
extern CONFIG_NEWWORK config_network;
extern _uart1_rx_frame uart1_rx;

extern _uart_frame uart1_frame;
extern _uart_frame buffer_rx,buffer_tx;
//*****-----*****-----
int main(void) {
	int temp_var;
	char time_current[11];
	unsigned char counter_reset_gprs;
	//*****-----*****-----*****-----*****-----
	init_VIC();
	init_PLL(); 		//Initialize CPU and Peripheral Clocks @ 60Mhz
	initTimer0(); 		//Initialize Timer0
	//-----------------------------------------
	gpio_config();
	initUart0(1200); 	//Initialize Uart0
	initUart1(9600);	//Initialize Uart1
	//--------------------------------
#ifdef EXTERNAL_EEPROM
	/*config external eeprom*/
	IODIR0 |= (1 << 7);
	IODIR1 |= (1 << 24) | (1 << 25);
	IOSET1 |= (1 << 24) | (1 << 25);
	//IOCLR0 |= (1 << 7);
	m25lc512_ON();
	spiInit();
#endif
	
//	//test_fram();
//	while(1);
	//init_Watchdog();	//Init & Start WDT - TimeOut ~= 310 Second
	//--------------------------------
	var_start();
	//--------------------------------
	printf("hello Tai Cho Dien\r");
	if (ON_OFF_mudule_GPRS()) {
		printf("Welcome module SIM800C\r");
	}
	read_para();		//load para from metter: ID, IP, PORT, APN
	prepare_config_mudule();
	//clear_watchdog();
	//check_id();
	clear_watchdog();
	//enable_ext_wdt();
	//---------------------
	RTC_init();
	set_time_from_rtc();
	RTC_start();
	//---------------------
	printf("hello baby\r");
	rtc_flag.bits.have_time_300F = 1;
	/*
	 while (1) {
	 send_data_to_server();
	 if (uart1_rx.para_rx.state_buf_rx == BUF_RX_FULL) {
	 get_command_from_server(uart1_rx.buffer_rx.buf_rx_server);
	 //printf("string data rx: %s\r", uart1_rx.buffer_rx.buf_rx_server);
	 uart1_rx.para_rx.state_buf_rx = BUF_RX_EMPTY;
	 }
	 }
	 */
	while (1) {
		temp_var = 0;
		counter_reset_gprs = 0;			//clear counter in the main loop
		//----------------Check & Save the Offline time ------------------------------------
		if (system_flag.bits.SEND_ERROR) {
			sprintf(time_current, "%02u%02u%02u%02u%02u",
					(uint8_t) (YEAR - 2000), MONTH, DOM, HOUR, MIN);
			save_time_offline(time_current);
		}
		//----------------------------------------------------------------------------------
		/*----Process when can not send data to server--------------------------------------
		 ----------------------------------------------------------------------------------*/
		while (system_flag.bits.SEND_ERROR || system_flag.bits.RESET_CONFIG) {
			if (system_flag.bits.RESET_CONFIG)
				system_flag.bits.RESET_CONFIG = 0;
			read_para();	//load para from metter: ID, IP, PORT, APN
			clear_watchdog();
			prepare_config_mudule();
			clear_watchdog();
			check_freeze_data();
			if (system_flag.bits.CONNECT_OK) {
				send_command_to_server(COMMAND_LOGIN);
				//delay_nsecond(3);		//wait 3s after send command login
			} else {
				delay_nsecond(5);		//delay 5s
				temp_var++;
			}
			//*****-----
			if ((system_flag.bits.SEND_ERROR) && (temp_var >= 2)) {
				//Need reset Module GPRS
				temp_var = 0;
				ON_OFF_mudule_GPRS();
				clear_watchdog();
				check_freeze_data();
				//------------------------------------
				counter_reset_gprs++;//increase counter after reset module gprs
				//------------------------------------
				if (counter_reset_gprs == 3) { 	//module gprs was reset 3 time
					system_flag.bits.IP_CONNECT = 1;	//Switch to connect IP2
				} else if (counter_reset_gprs == 4) {
					system_flag.bits.IP_CONNECT = 0;	//Switch to connect IP1
				} else
					system_flag.bits.IP_CONNECT = 0;//Switch to connect IP1 - default
				//------------------------------------
				if (counter_reset_gprs >= 5)
					counter_reset_gprs = 0;				//reset counter
				//------------------------------------
				check_freeze_data();
			}
		}
		/*--------------------------------------------------------------------------------
		 ---------------------------------------------------------------------------------*/
		send_data_to_server();
		//clear_watchdog();
		if (uart1_rx.para_rx.state_buf_rx == BUF_RX_FULL) {
			get_command_from_server(buffer_rx.data_frame);
			//printf("string data rx: %s\r", uart1_rx.buffer_rx.buf_rx_server);
			uart1_rx.para_rx.state_buf_rx = BUF_RX_EMPTY;
		}

		check_freeze_data();
		clear_watchdog();
		get_RSSI_signal();
	}
}
//----------------------------------------------------------------------------------------
void var_start(void) {
	rtc_flag.data_bits = 0x00;
	system_flag.data_bits = 0x0000;
	system_flag.bits.TIMEOUT_WAIT_LOGIN = 1;
	uart1_rx.para_rx.quick_connect = 0;
	uart1_rx.para_rx.uart_state = UART_STATE_NOTHING;
}
//----------------------------------------------------------------------------------------
void gpio_config(void) {
	//IO0DIR = 0xFFFFFFFF; //Configure all pins as outputs

	//PINSEL1 &= 0xCFFFFFFF; //select P0.30 is GPIO (dissable EXT_INT3)
	//GPIO_Input(GPIO_P0, GPIO_PIN_30);				//select pin is INPUT

	PINSEL1 &= 0xFFFFFFFC; 							//select P0.16 is GPIO
	GPIO_Output(GPIO_P0, GPIO_PIN_23); //Port Output, used for ON/OFF mudule GPRS

	//GPIO_Output(GPIO_P0, GPIO_PIN_0);				//Port Output, TX0
	//GPIO_Output(GPIO_P0, GPIO_PIN_8);				//Port Output, TX1

	PINSEL0 &= 0xCFFFFFFF; 							//select P0.14 is GPIO
	GPIO_Output(GPIO_P0, GPIO_PIN_14); //Port Output, --> control Power for MAX705
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_14, 0); 		//--> Power_Off for MAX705

	PINSEL1 &= 0xFFFFFFCF; 							//select P0.18 is GPIO
	GPIO_Output(GPIO_P0, GPIO_PIN_18);		//Port Output, --> Toggled MAX705

	PINSEL0 &= 0xFFFFFF3F;
	GPIO_Output(GPIO_P0, GPIO_PIN_3);	//Port Output, --> Toggled PowerON GPRS
}
//----------------------------------------------------------------------------------------
void init_VIC(void) {
	/* initialize VIC*/
	VICIntEnClr = 0xffffffff;
	VICVectAddr = 0;
	VICIntSelect = 0;

	/* Install the default VIC handler here */
	VICDefVectAddr = (int) DefaultVICHandler;
	return;
}
//----------------------------------------------------------------------------------------
void enable_ext_wdt(void) {
	GPIO_WriteBit(GPIO_P0, GPIO_PIN_14, 1); //--> Power_On for MAX705 - EXT WDT
}
//----------------------------------------------------------------------------------------
