/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif


#include "DigitalIoPin.h"
#include "FreeRTOS.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "semphr.h"
#include "queue.h"

#include "Fmutex.h"
#include "LpcUart.h"

#include <cr_section_macros.h>
#include <cstdio>
#include <mutex>
#include <string>
#include <stdlib.h>

QueueHandle_t que;
QueueHandle_t textQue;

DigitalIoPin *SW1;
DigitalIoPin *SW2;
DigitalIoPin *SW3;

LpcPinMap none = { -1, -1 }; // unused pin has negative values in it
LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8
		| UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin, none,
		none };

LpcUart dbgu(cfg);

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

static void vTask1(void *pvParameters) {
	char buffer[30];

	int sign;

	int counter = 0;
	int pSign;

	while(1){

		if(xQueueReceive(que, &sign, portMAX_DELAY) == pdTRUE){
			counter++;
			if (sign != pSign){
				sprintf(buffer, "Button %d pressed %d times.\r\n", pSign, counter);
				dbgu.write(buffer);
				counter = 0;
			}
			pSign = sign;
		}
	}
}



/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats( void ) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

void PIN_INT1_IRQHandler(void){
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH1);
	int sign = 1;
	xQueueSendFromISR(que, &sign, &xTaskWokenByReceive);
}

void PIN_INT2_IRQHandler(void){
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH2);
	int sign = 2;
	xQueueSendFromISR(que, &sign, &xTaskWokenByReceive);
}

void PIN_INT3_IRQHandler(void){
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH3);
	int sign = 3;
	xQueueSendFromISR(que, &sign, &xTaskWokenByReceive);
}

}
/* end runtime statictics collection */

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */
int main(void)
{
	prvSetupHardware();

	heap_monitor_setup();

	SW1 = new DigitalIoPin(0, 17, true, true, true);
	SW2 = new DigitalIoPin(1, 11, true, true, true);
	SW3 = new DigitalIoPin(1, 9, true, true, true);


	que = xQueueCreate(5, sizeof(int));
	textQue = xQueueCreate(20, sizeof(struct Event*));

    Chip_PININT_Init(LPC_GPIO_PIN_INT);

    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PININT);
    Chip_SYSCTL_PeriphReset(RESET_PININT);

    Chip_INMUX_PinIntSel(3, 1, 9);
    Chip_INMUX_PinIntSel(2, 1, 11);
    Chip_INMUX_PinIntSel(1, 0, 17);

    NVIC_SetPriority(PIN_INT1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1 );
    NVIC_SetPriority(PIN_INT2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1 );
    NVIC_SetPriority(PIN_INT3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1 );

    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH1);
    Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH1);
    Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH1);

    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH2);
    Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH2);
    Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH2);

    Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH3);
    Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH3);
    Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH3);

    NVIC_ClearPendingIRQ(PIN_INT1_IRQn);
    NVIC_EnableIRQ(PIN_INT1_IRQn);

    NVIC_ClearPendingIRQ(PIN_INT2_IRQn);
    NVIC_EnableIRQ(PIN_INT2_IRQn);

    NVIC_ClearPendingIRQ(PIN_INT3_IRQn);
    NVIC_EnableIRQ(PIN_INT3_IRQn);

	xTaskCreate(vTask1, "vTask1",
				256, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);


	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

