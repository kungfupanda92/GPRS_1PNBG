#include "main.h"

//*****-----*****-----*****-----*****-----
extern _rtc_flag rtc_flag;
extern _system_flag system_flag;
extern PARA_PLC para_plc;
extern CONFIG_NEWWORK config_network;
extern _uart1_rx_frame uart1_rx;
extern __stateModule stateModule;
extern _uart_frame uart1_frame;
extern _uart_frame buffer_rx, buffer_tx;
//*****-----*****-----
int main(void) {
	/*config module hardware*/
	vSetupHardware();
	printf("hello baby\r");
	check_id();
	while (1) {
		//task1 communicate with server
		switch (stateModule) {
		case STATE_MODULE:
			checkPowerOnOffGPRS();
			break;
		case OFF_GPRS:
			offModuleGprs();
			break;
		case ON_GPRS:
			onOffModuleGprs();
			break;
		case READ_PARA_CONFIG:
			read_para();	//load para from metter: ID, IP, PORT, APN
			stateModule = INIT_GPRS;
			break;
		case INIT_GPRS:
			establishTCPClient();
			stateModule = CONNECTING_SERVER;
			break;
		case CONNECTING_SERVER:
			connectToServerNow();
			break;
		case LOGIN:
			send_command_to_server(COMMAND_LOGIN);
			stateModule = UPDATE_DATA_SERVER;
			break;
		case HOLD_SOCKET:
			send_command_to_server(COMMAND_SOCKET);
			stateModule = UPDATE_DATA_SERVER;
		case READY_CONNECT:
			if (uart1_rx.para_rx.state_buf_rx == BUF_RX_FULL) {
				uart1_rx.para_rx.state_buf_rx = BUF_RX_EMPTY;
				stateModule = PROCESS_DATA_SERVER;
			} else if (system_flag.bits.SEND_SOCKET) {
				system_flag.bits.SEND_SOCKET = 0;	//clear flag
				stateModule = HOLD_SOCKET;
			}

			break;
		case PROCESS_DATA_SERVER:
			get_command_from_server(buffer_rx.data_frame);
			break;
		case UPDATE_DATA_SERVER:
			trans_data_server(0);
			break;
		case LOST_CONNECT:
			vSaveTimeLostConnect();

			stateModule = CONNECTING_SERVER;
			break;
		default:
			break;
		}

		//Task2 Freeze data metter every interval
		check_freeze_data();

	}
}
void vSetupHardware(void) {
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

	//init_Watchdog();	//Init & Start WDT - TimeOut ~= 310 Second
	var_start();
	RTC_init();
	set_time_from_rtc();
	RTC_start();
	//---------------------
}
//----------------------------------------------------------------------------------------
void var_start(void) {
	stateModule = STATE_MODULE;
	rtc_flag.data_bits = 0x00;
	system_flag.data_bits = 0x0000;
	system_flag.bits.TIMEOUT_WAIT_LOGIN = 1;
	uart1_rx.para_rx.quick_connect = 0;
	uart1_rx.para_rx.uart_state = UART_STATE_NOTHING;
	rtc_flag.bits.have_time_300F = 1;
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
