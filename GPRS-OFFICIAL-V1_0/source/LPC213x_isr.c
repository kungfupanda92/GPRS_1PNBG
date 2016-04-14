#include "LPC213x_isr.h"
#include <LPC213x.H>
#include <string.h>
#include "main.h"
#include "wdt.h"

extern _program_counter program_counter;
extern _system_flag system_flag;
extern _uart1_rx_frame uart1_rx;

extern _uart_frame uart1_frame;
extern _uart_frame buffer_rx, buffer_tx;

extern unsigned char buffer_PLC[];
extern unsigned char check_frame;
extern unsigned char start_frame;
extern unsigned char stop_frame;
extern unsigned char stop_frame_immediately;
/******************************************************************************/
/*            LPC213x Peripherals Interrupt Handlers                        */
/******************************************************************************/
/**
 * @brief  This function handles TIM0 global interrupt request.
 * @param  None
 * @retval None
 */
__irq void myTimer0_ISR(void) {
	long int regVal;
	//volatile static uint16_t timer_read_command,timer_send_socket;
	regVal = T0IR; // read the current value in T0's Interrupt Register

	//Clear_Watchdog();
	program_counter.timer_send_socket++;
	program_counter.timer_delay++;
	program_counter.timer_wait_response_login++;
	program_counter.timer_get_rssi++;

	if (program_counter.timer_get_rssi >= 400) { //50ms ngat 1 lan
		program_counter.timer_get_rssi = 0;
		system_flag.bits.GET_RSSI = 1;
	}

	if (program_counter.timer_send_socket >= 20 * TIME_SEND_SOCKET) {
		program_counter.timer_send_socket = 0;
		system_flag.bits.SEND_SOCKET = 1;
	}
	//----------------
	if (program_counter.timer_wait_response_login
			>= 20 * TIME_WAIT_RESPONSE_LOGIN) {
		program_counter.timer_wait_response_login = 0;
		system_flag.bits.TIMEOUT_WAIT_LOGIN = 1;
	}
	GPIO_Toggled(GPIO_P0, GPIO_PIN_18);
	//----------------
	T0IR = regVal; // write back to clear the interrupt flag
	VICVectAddr = 0x0; // Acknowledge that ISR has finished execution
}
//-------------------------------------------------------------------------------------------
__irq void myTimer1_ISR(void) {
	long int regVal;
	regVal = T1IR; // read the current value in T0's Interrupt Register

	T1IR = regVal; // write back to clear the interrupt flag
	VICVectAddr = 0x0; // Acknowledge that ISR has finished execution
}

/******************************************************************************/
/*            LPC213x Peripherals Interrupt Handlers                        */
/******************************************************************************/
/**
 * @brief  This function handles UART0 global interrupt request.
 */
__irq void myUart0_ISR(void) {
	uint8_t in_data, LSRValue;
	static bool start_OK = false;
	static uint32_t index = 0;
	LSRValue = U0LSR;
	in_data = U0RBR; // dummy read
	/* Receive Line Status */
	if (LSRValue & (LSR_OE | LSR_PE | LSR_FE | LSR_RXFE | LSR_BI)) {
		/* There are errors or break interrupt */
		/* Read LSR will clear the interrupt */
		VICVectAddr = 0; /* Acknowledge Interrupt */
		return;
	}
	if (LSRValue & LSR_RDR) /* Receive Data Ready */
	{
		/* If no error on RLS, normal ready, save into the data buffer. */
		/* Note: read RBR will clear the interrupt */
		//Recieve Data Available Interrupt has occured
		if (!start_OK) {
			if (in_data == start_frame) {
				if (stop_frame_immediately == true) {
					check_frame = true;
				} else {
					start_OK = true;
					index = 0;
				}
			}
		} else {
			if (in_data != stop_frame) {
				buffer_PLC[index++] = in_data;
				if (index >= MAX_BUFFER)
					index = 0;
			} else {
				buffer_PLC[index++] = 0;
				check_frame = true;
				start_OK = false;
			}
		}
	}

	VICVectAddr = 0x0; // Acknowledge that ISR has finished execution
}
/******************************************************************************/
/*            LPC213x Peripherals Interrupt Handlers                        */
/******************************************************************************/
/**
 * @brief  This function handles UART1 global interrupt request.
 * @param  None
 * @retval None
 */
//Communicate with module SIM800C
__irq void myUart1_ISR(void) {
	uint8_t regVal, LSRValue;
	//static unsigned char counter_rx;
	WORD_UNSIGNED data_check;

	LSRValue = U1LSR;
	regVal = U1RBR; // dummy read
	/* Receive Line Status */
	if (LSRValue & (LSR_OE | LSR_PE | LSR_FE | LSR_RXFE | LSR_BI)) {
		/* There are errors or break interrupt */
		/* Read LSR will clear the interrupt */
		VICVectAddr = 0; /* Acknowledge Interrupt */
		return;
	}
	if (LSRValue & LSR_RDR) { /* Receive Data Ready */
		/* If no error on RLS, normal ready, save into the data buffer. */
		/* Note: read RBR will clear the interrupt */
		//Recieve Data Available Interrupt has occured
		//regVal = U1RBR; // dummy read
		//process receive data frame from server (hex)
		if (uart1_rx.para_rx.uart_state == UART_STATE_RECEIVING) {
			buffer_rx.data_frame[uart1_rx.para_rx.counter_rx] = regVal; //contain data to buffer
			uart1_rx.para_rx.counter_rx++; //increase counter
			if ((regVal == 0x16) && (uart1_rx.para_rx.counter_rx >= 13)) {
				data_check.byte.byte0 = buffer_rx.frame.length_data[0];
				data_check.byte.byte1 = buffer_rx.frame.length_data[1];
				if ((data_check.val + 13 == uart1_rx.para_rx.counter_rx)) {
					//received enough frame_data
//					convert_array_hex2string(uart1_frame.data_frame,
//							uart1_rx.buffer_rx.buf_rx_server,
//							uart1_rx.para_rx.counter_rx);
					uart1_rx.para_rx.state_buf_rx = BUF_RX_FULL;
					uart1_rx.para_rx.uart_state = UART_STATE_BLOCK;
				}
			} else if (uart1_rx.para_rx.counter_rx >= 500)
				uart1_rx.para_rx.uart_state = UART_STATE_NOTHING;

			VICVectAddr = 0x0; // Acknowledge that ISR has finished execution
			return;
		} else if (uart1_rx.para_rx.uart_state == UART_STATE_NOTHING) {
			if (regVal == 0x68) {
				uart1_rx.para_rx.uart_state = UART_STATE_RECEIVING; //Bao hieu dang thu DATA
				buffer_rx.data_frame[0] = regVal;
				uart1_rx.para_rx.counter_rx = 1;          //Bien dem Rx = 1

				VICVectAddr = 0x0; // Acknowledge that ISR has finished execution
				return;
			}
		}
		//-------------------------------------------------------------------
		switch (uart1_rx.para_rx.state_uart) {

		case UART_WAIT_RESPONDE:
			if (regVal == 0x20) { //0x20: '>'
				uart1_rx.para_rx.flag.bits.PREPARE_SEND_OK = 1;
			}
			if (regVal != 0 && regVal != 0x0D && regVal != 0x0A) {
				uart1_rx.buffer_rx.buf_response_command[uart1_rx.para_rx.counter_rx_command] =
						regVal; //contain data to buffer
				uart1_rx.para_rx.counter_rx_command++; //increase counter
			}

			if (uart1_rx.para_rx.counter_rx_command >= 70) {
				uart1_rx.para_rx.counter_rx_command = 70;
			}
		default:
			break;
		}
	}
	VICVectAddr = 0x0; // Acknowledge that ISR has finished execution
}
//-----------------------------------------------------------------------------------------------------
void DefaultVICHandler(void)
__irq
{
	VICVectAddr = 0; /* Acknowledge Interrupt */
}

