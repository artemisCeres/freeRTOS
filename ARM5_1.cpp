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
	Board_LED_Set(1, false);

}

/* LED1 toggle thread */
static void vTask1(void *pvParameters) {


	DigitalIoPin step(0, 24, false);
	DigitalIoPin dir(1, 0, false);
	DigitalIoPin LimitSW1(0, 27, true, true, true);
	DigitalIoPin LimitSW2(0, 28, true, true, true);
	DigitalIoPin Button1(0, 17, true, true, true);
	DigitalIoPin Button2(1, 6, true, true, true);
	DigitalIoPin Button3(1, 9, true, true, true);



	while (1) {

		Board_LED_Set(0, false);
		Board_LED_Set(1, false);

		while (LimitSW1.read()) {
			Board_LED_Set(0, true);
		}
		while (LimitSW2.read()) {
			Board_LED_Set(1, true);
		}
		Board_LED_Set(0, false);
		Board_LED_Set(1, false);
	}
}

/* LED2 toggle thread */
static void vTask2(void *pvParameters) {
	DigitalIoPin step(0, 24, false);
	DigitalIoPin dir(1, 0, false);
	DigitalIoPin LimitSW1(0, 27, true, true, true);
	DigitalIoPin LimitSW2(0, 28, true, true, true);
	DigitalIoPin Button1(0, 17, true, true, true);
	DigitalIoPin Button2(1, 6, true, true, true);
	DigitalIoPin Button3(1, 9, true, true, true);


	while (1) {
		if (LimitSW1.read() || LimitSW2.read()) {
			step.write(false);
		}
		else if (Button1.read() && Button3.read()) {
			step.write(false);

		}else if (Button1.read()) {
			step.write(true);
			dir.write(false);
			vTaskDelay(10);
			step.write(false);


		}else if (Button3.read()) {
			step.write(true);
			dir.write(true);
			vTaskDelay(10);
			step.write(false);
		}

		vTaskDelay(1);

	}
}

/* UART (or output) thread */


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


	prvSetupHardware();
	heap_monitor_setup();

	guard = xSemaphoreCreateMutex();

	xTaskCreate(vTask1, "Task1",
			configMINIMAL_STACK_SIZE  + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask2, "Task2",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);



	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}
