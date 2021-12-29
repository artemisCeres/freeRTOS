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
#include <cr_section_macros.h>
#include <cstdio>
#include <mutex>
#include <string>
#include <stdlib.h>
#include <cstring>

SemaphoreHandle_t sem;

TimerHandle_t timer1;
TimerHandle_t timer2;

LpcUart *dbgu;
bool led = false;


static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

void vTimerCallbackTask1Expired( TimerHandle_t xTimer ){
	//inactivity monitoring. If no characters are received in 30 seconds all the
	//characters received so far are discarded and the program prints “[Inactive]”
	// semaphore -> task1
	xSemaphoreGive(sem);


}
void vTimerCallbackTask2Expired( TimerHandle_t xTimer ){
	//toggling the green led
	led = !led;
	Board_LED_Set(1, led);

}


static void vTask1(void *pvParameters) {
	xTimerStart(timer1, 0);
	xTimerStart(timer2, 0);

	LpcPinMap none = { -1, -1 }; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8
			| UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin,
			none, none };

	dbgu = new LpcUart(cfg);

	vTaskDelay(100);
	char c;
	char buffer[50];
	std::string set;
	std::string setparam;
	// a program that reads commands from the serial port.
	//When a character is received
	//the inactivity timer is started/reset. When enter is pressed the received character are processed in a
	//command interpreter. The commands are:
	//interval <number> - set the led toggle interval (default is 5 seconds)
	//If no valid command is found the program prints: “unknown command”.
	while (1) {
		if(xSemaphoreTake(sem, 1) == pdTRUE){
			set="";
			dbgu->write("\r\n[Inactive]\r\n");
		}
		if (dbgu->read(c)) {
			dbgu->write(c);

			xTimerReset(timer1, 0);

			if (set == "HELP") {
				if (c == '\r' || c == '\n') {
					set = "";

					dbgu->write("\r\nType:\r\n HELP to see instructions\r\n INTERVAL<number> to set the LED toggle interval\r\nTIME to see the time left until next toggle\r\n");
				} else if (c == ' ') {
				}
			}
			else if (set == "INTERVAL") {
				if (c == '\r' || c == '\n') {
					int num;
					num = stoi(setparam);
					xTimerChangePeriod( timer2, num, 0);
					snprintf(buffer, 50, "\r\n interval set to %d ms\r\n", num);
					dbgu->write(buffer);

					set = "";
					setparam = "";


					//get integer and set interval to it
				} else if (c == ' ') {
				} else {
					setparam += c;
				}
			}
			else if (set == "TIME") {
				if (c == '\r' || c == '\n') {
					//• time – prints the number of seconds with 0.1s accuracy since the last led toggle
					float left = xTimerGetExpiryTime( timer2 ) - xTaskGetTickCount();
					float period = xTimerGetPeriod(timer2);
					float out = (period-left)/1000;
					snprintf(buffer, 50, "\r\n  %0.1f seconds passed since last blink\r\n", out);
					dbgu->write(buffer );
					set = "";


				} else if (c == ' ') {

				} }
			else {
				if(c=='\r' || c=='\n'){
					set="";
					dbgu->write("\r\n unknown command\r\n");
				}
				if(!isspace(c)){
					set += toupper(c);
				}

			}

		}
		vTaskDelay(1);



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


	timer1 = xTimerCreate(
			"timer1", /* name */
			pdMS_TO_TICKS(30000), /* period/time */
			pdTRUE, /* auto reload */
			(void*)0, /* timer ID */
			vTimerCallbackTask1Expired); /* callback */
	if (timer1==NULL) {
		for(;;); /* failure! */

	}

	timer2 = xTimerCreate(
			"timer2", /* name */
			pdMS_TO_TICKS(5000), /* period/time */
			pdTRUE, /* auto reload */
			(void*)0, /* timer ID */
			vTimerCallbackTask2Expired); /* callback */
	if (timer2==NULL) {
		for(;;); /* failure! */

	}



	xTaskCreate(vTask1, "vTask1", 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t*) NULL);



	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;

}


