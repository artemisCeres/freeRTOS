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
#include "timers.h"
#include "event_groups.h"
#include <cr_section_macros.h>
#include <cstdio>
#include <mutex>
#include <string>
#include <stdlib.h>
#include <ctime>
#include <cstdlib>
using namespace std;

QueueHandle_t que;

SemaphoreHandle_t sem;

LpcUart *dbgu;


EventGroupHandle_t xCreatedEventGroup;
const uint32_t task_bit = (1 << 0);


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




static void vTask1(void *pvParameters) {
	//Task 1 waits for user to press a button. When the
	//button is pressed task 1 sets bit 0 of the event group
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
	while (1) {
		if (SW1->read() || SW2->read() || SW3->read()) {
			xEventGroupSetBits(xCreatedEventGroup, task_bit);
		}
	}

}
static void vTask2(void *pvParameters) {
	//Tasks 2 – 4 wait on the event bit with an infinite
	//timeout. When the bit is set the tasks start running. Each task prints task number and number of elapsed
	//ticks at random interval that is between 1 – 2 seconds. Remember to protect access to the serial port with a
	//semaphore
	char buffer[50];
	srand (time(NULL)+1);

	xEventGroupWaitBits(xCreatedEventGroup, task_bit, pdFALSE, pdTRUE, portMAX_DELAY);
	while (1) {
		int delay = rand() % 2000 + 1000;
		int num =0;
		num = xTaskGetTickCount();
		snprintf(buffer, 50, "\r\n Task 2: %d ms\r\n", num);
		dbgu->write(buffer);
		vTaskDelay(delay);
	}
}
static void vTask3(void *pvParameters) {
	//Tasks 2 – 4 wait on the event bit with an infinite
	//timeout. When the bit is set the tasks start running. Each task prints task number and number of elapsed
	//ticks at random interval that is between 1 – 2 seconds. Remember to protect access to the serial port with a
	//semaphore
	char buffer[50];
	srand (time(NULL)+2);

	xEventGroupWaitBits(xCreatedEventGroup, task_bit, pdFALSE, pdTRUE, portMAX_DELAY);
	while (1) {
		int delay = rand() % 2000 + 1000;
		int num =0;
		num = xTaskGetTickCount();
		snprintf(buffer, 50, "\r\n Task 3: %d ms\r\n", num);
		dbgu->write(buffer);
		vTaskDelay(delay);

	}
}
static void vTask4(void *pvParameters) {
	//Tasks 2 – 4 wait on the event bit with an infinite
	//timeout. When the bit is set the tasks start running. Each task prints task number and number of elapsed
	//ticks at random interval that is between 1 – 2 seconds. Remember to protect access to the serial port with a
	//semaphore
	char buffer[50];
	srand (time(NULL)+3);

	xEventGroupWaitBits(xCreatedEventGroup, task_bit, pdFALSE, pdTRUE, portMAX_DELAY);
	while (1) {
		int delay = rand() % 2000 + 1000;
		int num =0;
		num = xTaskGetTickCount();
		snprintf(buffer, 50, "\r\n Task 4: %d ms\r\n", num);
		dbgu->write(buffer);
		vTaskDelay(delay);
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

	sem = xSemaphoreCreateBinary();

	que = xQueueCreate(20, sizeof(int));

	xCreatedEventGroup = xEventGroupCreate();



	xTaskCreate(vTask1, "vTask1", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vTask2, "vTask2", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vTask3, "vTask3", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vTask4, "vTask4", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;

}

