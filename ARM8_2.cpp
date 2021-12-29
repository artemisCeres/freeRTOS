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
#include "Filter.h"

#include <cr_section_macros.h>
#include <cstdio>
#include <mutex>
#include <string>
#include <stdlib.h>

QueueHandle_t que;
QueueHandle_t setQue;
Filter *filter;
LpcUart *dbgu;

DigitalIoPin *SW1;
DigitalIoPin *SW2;
DigitalIoPin *SW3;

struct event {
	int tickcount;
	int sign;
} e;

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

static void vT1(void *pvParameters) {
	LpcPinMap none = { -1, -1 }; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8
			| UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin,
			none, none };

	dbgu = new LpcUart(cfg);

	SW1 = new DigitalIoPin(0, 17, true, true, true);
	SW2 = new DigitalIoPin(1, 11, true, true, true);
	SW3 = new DigitalIoPin(1, 9, true, true, true);

	filter = new Filter();

	//setQue = xQueueCreate(5, sizeof(int));
	que = xQueueCreate(20, sizeof(struct event));

	Chip_PININT_Init(LPC_GPIO_PIN_INT);

	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PININT);
	Chip_SYSCTL_PeriphReset(RESET_PININT);

	Chip_INMUX_PinIntSel(3, 1, 9);
	Chip_INMUX_PinIntSel(2, 1, 11);
	Chip_INMUX_PinIntSel(1, 0, 17);

	NVIC_SetPriority(PIN_INT1_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	NVIC_SetPriority(PIN_INT2_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	NVIC_SetPriority(PIN_INT3_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);

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
	char buffer[30];
	struct event rcv;
	int setparam;
	bool filterbool = false;

	dbgu->write("Started waiting:\r\n");

	while (1) {
		if (xQueueReceive(que, &(rcv), portMAX_DELAY) == pdTRUE) {
			filterbool = filter->stop(rcv.tickcount);
			if (filterbool) {
				snprintf(buffer, 30, "%d ms Button %d.\r\n",
						filter->elapsedTime, rcv.sign);
				dbgu->write(buffer);
			}
			filter->start(rcv.tickcount);
		}
		vTaskDelay(1);
	}
}

static void vT2(void *pvParameters) {
	vTaskDelay(100);

	char c;
	std::string set;
	std::string setparam;
	char buffer[50];
	while (1) {
		if (dbgu->read(c)) {
			dbgu->write(c);
			if (set == "FILTER") {
				if (c == '\r' || c == '\n') {
					int x = stoi(setparam);
					filter->set(x);
					set = "";
					setparam = "";
					snprintf(buffer, 50, "\r\nFilter is set to %d ms\r\n", x);
					dbgu->write(buffer);
				} else if (c == ' ') {
				} else {
					setparam += c;
				}
			} else {
				if(c=='\r' || c=='\n'){
					set="";
				}
				if(!isspace(c)){
				set += toupper(c);
				}
			}
		}
		vTaskDelay(1);
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

void PIN_INT1_IRQHandler(void) {
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH1);
	e = { xTaskGetTickCountFromISR(), 1 };

	xQueueSendFromISR(que, &e, &xTaskWokenByReceive);
}

void PIN_INT2_IRQHandler(void) {
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH2);
	e = { xTaskGetTickCountFromISR(), 2 };
	xQueueSendFromISR(que, &e, &xTaskWokenByReceive);
}

void PIN_INT3_IRQHandler(void) {
	BaseType_t xTaskWokenByReceive = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH3);
	e = { xTaskGetTickCountFromISR(), 3 };
	xQueueSendFromISR(que, &e, &xTaskWokenByReceive);
}

}
/* end runtime statictics collection */

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */
int main(void) {
	prvSetupHardware();

	heap_monitor_setup();



	xTaskCreate(vT1, "vPrint", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vT2, "vRead", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

