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
#include "semphr.h"
#include "heap_lock_monitor.h"
#include "DigitalIoPin.h"
#include "time.h"
SemaphoreHandle_t sem, guard;

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Sets up system hardware */
static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

static void vUARTTask1(void *pvParameters) {
	int c;
	while (1) {
		c = Board_UARTGetChar();

		if (c != EOF) {
			Board_UARTPutChar(c);
			xSemaphoreGive(guard);
		}
	}
}

static void vUARTTask2(void *pvParameters) {
	while (1) {
		if (xSemaphoreTake(guard, portMAX_DELAY) == pdTRUE) {
			Board_LED_Set(0, true);
			vTaskDelay(configTICK_RATE_HZ / 10);
			Board_LED_Set(0, false);
			vTaskDelay(configTICK_RATE_HZ / 10);
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
/* end runtime statictics collection */

/**
 * @brief	main routine for FreeRTOS blinky example
 * @return	Nothing, function should not exit
 */
int main(void)
{

	prvSetupHardware();

	heap_monitor_setup();

	sem = xSemaphoreCreateCounting(3, 0);
	guard = xSemaphoreCreateMutex();

	/* LED1 toggle thread */
	xTaskCreate(vUARTTask1, "vUARTTask1",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* LED2 toggle thread */
	xTaskCreate(vUARTTask2, "vUARTTask2",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);



	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

