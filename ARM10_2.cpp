
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
#include "timers.h"
#include <cr_section_macros.h>
#include <cstdio>
#include <mutex>
#include <string>
#include <stdlib.h>

QueueHandle_t que;

SemaphoreHandle_t sem;

TimerHandle_t timer1;

LpcUart *dbgu;

DigitalIoPin *SW1;
DigitalIoPin *SW2;
DigitalIoPin *SW3;

// TODO: insert other include files here

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

void vTimerCallbackTask1Expired( TimerHandle_t xTimer ){
	//One shot timer callback gives the binary semaphore
	Board_LED_Set(1, false);


}



static void vTask1(void *pvParameters) {
	SW1 = new DigitalIoPin(0, 17, true, true, true);
	SW2 = new DigitalIoPin(1, 11, true, true, true);
	SW3 = new DigitalIoPin(1, 9, true, true, true);

	LpcPinMap none = { -1, -1 }; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8
			| UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin,
			none, none };

	dbgu = new LpcUart(cfg);

	vTaskDelay(100);

	// Printing task, waits on queue to print
	while (1) {
	if (SW1->read() || SW2->read() || SW3->read()) {
		xTimerStart(timer1, 0);
		Board_LED_Set(1, true);

	}

	}

}

// TODO: insert other definitions and declarations here
extern "C" {

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
}
int main(void) {
	prvSetupHardware();

	heap_monitor_setup();
	timer1 = xTimerCreate(
			"timer1", /* name */
			pdMS_TO_TICKS(5000), /* period/time */
			pdTRUE, /* auto reload */
			(void*)0, /* timer ID */
			vTimerCallbackTask1Expired); /* callback */
	if (timer1==NULL) {
		for(;;); /* failure! */

	}




	xTaskCreate(vTask1, "vTask1", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);


	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;

}
