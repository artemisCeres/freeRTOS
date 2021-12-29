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
#include "heap_lock_monitor.h"
#include "DigitalIoPin.h"


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

/* LED1 toggle thread */
static void vLEDTask1(void *pvParameters) {
	bool LedState = false;


	while (1) {
		int counter = 1;
		while(counter <= 3){
			LedState = true;
			Board_LED_Set(0, LedState);

			vTaskDelay(100);
			LedState = false;
			Board_LED_Set(0, LedState);
			vTaskDelay(200);
			counter++;
		}
		while(3 < counter && counter <= 6){
					LedState = true;
					Board_LED_Set(0, LedState);
					vTaskDelay(300);
					LedState = false;
					Board_LED_Set(0, LedState);
					vTaskDelay(200);
					counter++;
		}
		while(6 < counter && counter <= 9){
					LedState = true;
					Board_LED_Set(0, LedState);
					vTaskDelay(100);
					LedState = false;
					Board_LED_Set(0, LedState);
					vTaskDelay(200);
					counter++;

		}
		vTaskDelay(200);

	}
}

/* LED2 toggle thread */
static void vLEDTask2(void *pvParameters) {
	while (1) {
		bool LedState = false;
		Board_LED_Set(1, LedState);
		vTaskDelay(3500);
		LedState = true;
		Board_LED_Set(1, LedState);
		vTaskDelay(3500);


	}
}

/* UART (or output) thread */
static void vUARTTask1(void *pvParameters) {
	int tickMin = 0;
	int tickSec = 0;
	int tickHour = 0;

	while (1) {
		DEBUGOUT("Time: %02d : %02d\r\n", tickMin, tickSec);
		tickSec++;

		if(tickSec == 60){
			tickSec = 0;
			tickMin++;
			if(tickMin == 60){
				tickMin = 0;
				tickHour++;
			}
		}

		/* About a 1s delay here */
		vTaskDelay(configTICK_RATE_HZ);
	}
}
static void vUARTTask2(void *pvParameters) {
	int tickCount = 0;

	while (1) {
	    DigitalIoPin sw1(0,17,true, true, true);
		while(sw1.read()){
		DEBUGOUT("Tick: %d \r\n", tickCount);
		vTaskDelay(configTICK_RATE_HZ/10);
		tickCount++;
		}
		DEBUGOUT("Tick: %d \r\n", tickCount);
		vTaskDelay(configTICK_RATE_HZ);
		tickCount++;
		}


		/* About a 1s delay here */
		vTaskDelay(configTICK_RATE_HZ);
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

	/* LED1 toggle thread */
	xTaskCreate(vLEDTask1, "vTaskLed1",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* LED2 toggle thread */
	xTaskCreate(vLEDTask2, "vTaskLed2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(vUARTTask1, "vTaskUart1",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);
	xTaskCreate(vUARTTask2, "vTaskUart2",
				configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

