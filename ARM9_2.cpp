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

LpcUart *dbgu;

SemaphoreHandle_t sem;

EventGroupHandle_t xCreatedEventGroup;

const uint32_t task1_bit = (1 << 0); //bit 0 (001)
const uint32_t task2_bit = (1 << 1); //bit 1 (010)
const uint32_t task3_bit = (1 << 2); //bit 2 (100)
const uint32_t task4_bit = (1 << 3); //bit 3 (1000)
const uint32_t watchdog_bit = (task1_bit | task2_bit | task3_bit);//bit 2, 1, 0 (111)
const uint32_t all_bits = (watchdog_bit | task4_bit); //bit 3, 2, 1, 0 (1111)


DigitalIoPin *SW1;
DigitalIoPin *SW2;
DigitalIoPin *SW3;

static int count1 = 0;
static int count2 = 0;
static int count3 = 0;


// TODO: insert other include files here

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

static void vTask1(void *pvParameters) {
	//Tasks 1 – 3 implement a loop that runs when a button is pressed and
	//released and sets a bit the event group on each loop round. The task must not run if button is pressed
	//constantly without releasing it. Each task monitors one button.

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

	xEventGroupSync( xCreatedEventGroup,
			task1_bit,
			all_bits,
			portMAX_DELAY);


	while (1) {
		if (SW1->read()) {
			while(SW1->read()){
				vTaskDelay(10);
			}
			xEventGroupSetBits(xCreatedEventGroup, task1_bit);
			count1 = xTaskGetTickCount();
		}
		vTaskDelay(10);
	}
}

static void vTask2(void *pvParameters) {

	//Tasks 1 – 3 implement a loop that runs when a button is pressed and
	//released and sets a bit the event group on each loop round. The task must not run if button is pressed
	//constantly without releasing it. Each task monitors one button.
	char buffer[50];
	xEventGroupSync( xCreatedEventGroup,
			task2_bit,
			all_bits,
			portMAX_DELAY);
	while (1) {
		if (SW2->read()) {
			while(SW2->read()){
				vTaskDelay(10);
			}
			xEventGroupSetBits(xCreatedEventGroup, task2_bit);
			count2 = xTaskGetTickCount();
		}
		vTaskDelay(10);
	}
}

static void vTask3(void *pvParameters) {

	//Tasks 1 – 3 implement a loop that runs when a button is pressed and
	//released and sets a bit the event group on each loop round. The task must not run if button is pressed
	//constantly without releasing it. Each task monitors one button.
	xEventGroupSync( xCreatedEventGroup,
			task3_bit,
			all_bits,
			portMAX_DELAY);
	while (1) {
		if (SW3->read()) {
			while(SW3->read()){
				vTaskDelay(10);
			}
			xEventGroupSetBits(xCreatedEventGroup, task3_bit);
			count3 = xTaskGetTickCount();
		}
		vTaskDelay(10);
	}
}


static void vTaskWatchDog(void *pvParameters) {

	//Task 4 is a watchdog task that monitors
	//that tasks 1 – 3 run at least once every 30 seconds.
	//Task 4 prints “OK” and number of elapsed ticks from last “OK” when all (other) tasks have notified that they
	//have run the loop. If some of the tasks does not run within 30 seconds Task 4 prints “Fail” and the number
	//of the task plus the number of elapsed ticks for each task that did not meet the deadline and then Task 4
	//suspends itself.

	int current_tick, OK_tick = 0, num1, num2, num3;
	char buffer[50];
	xEventGroupSync( xCreatedEventGroup,
			task4_bit,
			all_bits,
			portMAX_DELAY);
	while (1) {
		if((xEventGroupWaitBits(xCreatedEventGroup, watchdog_bit, pdTRUE, pdTRUE, 30000)) == watchdog_bit){ // 111
			current_tick = xTaskGetTickCount();
			int num = current_tick - OK_tick;
			snprintf(buffer, 50, "\r\n OK: %d ms\r\n", num);
			dbgu->write(buffer);
			OK_tick = current_tick;

		}
		else{
			int bits = xEventGroupGetBits(xCreatedEventGroup);
			dbgu->write("\r\n FAIL:  ");
			current_tick = xTaskGetTickCount();
			num1 = current_tick - count1;
			num2 = current_tick - count2;
			num3 = current_tick - count3;

			switch (bits) {

			case 6: //110
				snprintf(buffer, 50, "\r\n Task 1: %d ms\r\n", num2);
				dbgu->write(buffer);

				break;
			case 5: //101
				snprintf(buffer, 50, "\r\n Task 2: %d ms\r\n", num2);
				dbgu->write(buffer);

				break;
			case 4: //100
				snprintf(buffer, 50, "\r\n Task 1: %d ms\r\n Task 2: %d ms\r\n ", num1, num2);
				dbgu->write(buffer);

				break;
			case 3://011
				snprintf(buffer, 50, "\r\n Task 3: %d ms\r\n", num3);
				dbgu->write(buffer);

				break;
			case 2://010
				snprintf(buffer, 50, "\r\n Task 1: %d ms\r\n Task 3: %d ms\r\n ", num1, num3);
				dbgu->write(buffer);

				break;
			case 1://001
				snprintf(buffer, 50, "\r\n Task 2: %d ms\r\n Task 3: %d ms\r\n ", num2, num3);
				dbgu->write(buffer);

				break;

			case 0://000
				snprintf(buffer, 50, "\r\n Task 1: %d ms\r\n Task 2: %d ms\r\n Task 3: %d ms\r\n ", num1, num2, num3);
				dbgu->write(buffer);

				break;
			}
			vTaskSuspend(NULL);
		}
		vTaskDelay(10);
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


	xCreatedEventGroup = xEventGroupCreate();


	xTaskCreate(vTask1, "vTask1", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vTask2, "vTask2", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vTask3, "vTask3", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	xTaskCreate(vTaskWatchDog, "vTaskWatchDog", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);

	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;

}
