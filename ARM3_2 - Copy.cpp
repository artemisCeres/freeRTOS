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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "heap_lock_monitor.h"
#include "time.h"

#include "DigitalIoPin.h"
#include <string>

#define QUEUE_SIZE (20)
/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

QueueHandle_t xQueue;
SemaphoreHandle_t guard;
const TickType_t xTicksToWait = pdMS_TO_TICKS(100);

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Print to UART with guard*/
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

	Board_LED_Set(0, false);
}

static void vTask1(void *pvParameters) {
	int i, interval = rand() % 400 + 100;

	while (1) {
		i = rand() % 100;
		if(xQueueSendToBack(xQueue, &i, xTicksToWait) != pdPASS ) {
			sem_print( "Unable to send to que.\r\n" );
		}
		vTaskDelay(interval);
	}
}

static void vTask2(void *pvParameters) {
	DigitalIoPin sw1(0, 17, DigitalIoPin::pullup, true);
	const int to_send = 112;

	while (1) {
		if (sw1.read()) {
			if(xQueueSendToFront( xQueue, &to_send, xTicksToWait) != pdPASS) {
				sem_print("Unable to send to beginning of que.\r\n" );
			}
			vTaskDelay(1000);
		}
	}
}

static void vTask3(void *pvParameters) {
	int lReceivedValue;
	char buffer[50];

	while(1) {
		if(xQueueReceive(xQueue, &lReceivedValue, xTicksToWait) == pdPASS){ // Read first element "FIFO"
			if(lReceivedValue == 112) {
				sem_print("Help me\r\n" );
				vTaskDelay(800);
			}
			else {
				sprintf(buffer, "Generated value is %d \r\n", lReceivedValue);
				sem_print(buffer);
				buffer[0] = '\0';

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
	//Do stack size + 128 if you need to print in UART

	xQueue = xQueueCreate(QUEUE_SIZE, sizeof(int));
	guard = xSemaphoreCreateMutex();

	srand(time(NULL));

	prvSetupHardware();
	heap_monitor_setup();

	xTaskCreate(vTask1, "Task1",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask2, "Task2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask3, "Task3",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
