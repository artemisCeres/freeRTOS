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

#include <cr_section_macros.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

/* FREERTOS INCLUDES */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "heap_lock_monitor.h"
#include "DigitalIoPin.h"
#include <string>

#define QUEUESIZE (5)

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

QueueHandle_t xQueue;
SemaphoreHandle_t guard;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Print to UART with guard. Stands for Semaphore Print*/
void sem_print(const char *str){
	xSemaphoreTake(guard,portMAX_DELAY);
	Board_UARTPutSTR(str);
	xSemaphoreGive(guard);
}


/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

/* LED1 toggle thread */
static void vTask1(void *pvParameters) {
	int c;
	int counter = 0;
	const TickType_t xTicksToWait = pdMS_TO_TICKS(100);

	while (1) {

		xSemaphoreTake(guard,portMAX_DELAY);
		c = Board_UARTGetChar();
		xSemaphoreGive(guard);

		if (c != EOF) {

			xSemaphoreTake(guard,portMAX_DELAY);
			Board_UARTPutChar(c);
			xSemaphoreGive(guard);

			if (c == '\n' || c == '\r' ) {
				if( xQueueSendToBack( xQueue, &counter, xTicksToWait) != pdPASS ) {
					sem_print( "Unable to send to que.\r\n" );
				}
				counter = 0;
			}
			else {
				counter++;
			}
		}
	}
}

/* LED2 toggle thread */
static void vTask2(void *pvParameters) {
	DigitalIoPin sw1(0, 17, DigitalIoPin::pullup, true);
	const int trigger = -1;
	const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );
	while (1) {
		if (sw1.read()) {
			if( xQueueSendToBack( xQueue, &trigger, xTicksToWait) != pdPASS ) {
				sem_print( "Could not send to the queue.\r\n" );
			}
			vTaskDelay(100);
		}
	}
}

/* UART (or output) thread */
static void vTask3(void *pvParameters) {
	int lReceivedValue;
	const TickType_t xTicksToWait = pdMS_TO_TICKS( 100 );
	int sum = 0;
	char buffer[50];

	while (1) {
		if(xQueueReceive(xQueue, &lReceivedValue, xTicksToWait) == pdPASS){
			if(lReceivedValue != -1){
				sum += lReceivedValue;
			} else {

				sprintf(buffer, "You have typed: %d characters \r\n",sum);
				sem_print(buffer);
				buffer[0] = '\0';

				sum = 0;
				lReceivedValue = 0;
			}
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

}
/* end runtime statistics collection */

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */


int main(void)
{
	//NOTE: configTICK_RATE_HZ = 1000

	xQueue = xQueueCreate( QUEUESIZE, sizeof( int ) );
	guard = xSemaphoreCreateMutex();

	prvSetupHardware();
	heap_monitor_setup();

	xTaskCreate(vTask1, "Task1",
			configMINIMAL_STACK_SIZE  + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask2, "Task2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask3, "Task3",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
