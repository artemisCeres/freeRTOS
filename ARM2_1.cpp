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
#include <mutex>
#include "Fmutex.h"
Fmutex guard;

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
    DigitalIoPin sw1(0,17,true, true, true);
    while (1) {
    if(sw1.read()) {
    	std::lock_guard<Fmutex> lock(guard);
    	Board_UARTPutSTR("SW1 was pressed\r\n");


    	}
    }
}

static void vUARTTask2(void *pvParameters) {
    DigitalIoPin sw2(1,11,true, true, true);
    while (1) {
    if(sw2.read()) {
    	std::lock_guard<Fmutex> lock(guard);
    	Board_UARTPutSTR("SW2 was pressed\r\n");

       }
    }
}

static void vUARTTask3(void *pvParameters) {
    DigitalIoPin sw3(1,9,true, true, true);
    while (1) {
    if(sw3.read()) {
    	std::lock_guard<Fmutex> lock(guard);
    	Board_UARTPutSTR("SW3 was pressed\r\n");

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

	/* LED1 toggle thread */
	xTaskCreate(vUARTTask1, "vUARTTask1",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* LED2 toggle thread */
	xTaskCreate(vUARTTask2, "vUARTTask2",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	/* UART output thread, simply counts seconds */
	xTaskCreate(vUARTTask3, "vUARTTask3",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);


	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

