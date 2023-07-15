/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

#define TSK1_PERIOD		50
#define TSK2_PERIOD		50
#define TSK3_PERIOD		100
#define TSK4_PERIOD		20
#define TSK5_PERIOD		10
#define TSK6_PERIOD		100

#define TICK_GPIO		PORT_0, PIN0
#define IDLE_GPIO		PORT_0, PIN7

#define TASK_GPIO(TASK_NUM)	PORT_0, (PIN0+TASK_NUM)

#define BUTTON_1		PORT_1, PIN0
#define BUTTON_2		PORT_1, PIN1
/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
char runTimeStatsBuff[190];
/*-----------------------------------------------------------*/
void vApplicationTickHook( void )
{
	 GPIO_write(TICK_GPIO, PIN_IS_HIGH);
	 GPIO_write(TICK_GPIO, PIN_IS_LOW);
}

void vApplicationIdleHook( void )
{
		uint8_t i;
		for (i = 1; i < 7; i++)
		{	
			GPIO_write(TASK_GPIO(i), PIN_IS_LOW);
		}
		GPIO_write(IDLE_GPIO, PIN_IS_HIGH);
}

volatile unsigned int misses = 0;
unsigned int idleTime = 0, idleStart = 0;
unsigned int systemTime = 0, CPU_LOAD = 0;

TaskHandle_t TASK_1_HANDLE;
TaskHandle_t TASK_2_HANDLE;
TaskHandle_t TASK_3_HANDLE;
TaskHandle_t TASK_4_HANDLE;
TaskHandle_t TASK_5_HANDLE;
TaskHandle_t TASK_6_HANDLE;

/* TASK 1 */
void Button_1_Monitor(char ** messageToSend)
{
	GPIO_write(TASK_GPIO(1), PIN_IS_HIGH);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	vTaskSetApplicationTaskTag(NULL, (void *) 1);

	pinState_t buttonState;
	static pinState_t buttonOldState = PIN_IS_LOW;
	
	for ( ; ; )
	{
		buttonState = GPIO_read(BUTTON_1);
		
		if (buttonState != buttonOldState)
		{
			if (buttonOldState == PIN_IS_LOW)
					*messageToSend = "Button 1 RISING\n";
			else
					*messageToSend = "Button 1 FALLING\n";
			
			buttonOldState = buttonState;
		}

		vTaskDelayUntil(&xLastWakeTime, TSK1_PERIOD);
		xLastWakeTime = xTaskGetTickCount();
	}
}

void Button_2_Monitor(char ** messageToSend)
{
	GPIO_write(TASK_GPIO(2), PIN_IS_HIGH);
	vTaskSetApplicationTaskTag(NULL, (void *) 2);
	TickType_t xLastWakeTime = xTaskGetTickCount();

	pinState_t buttonState;
	static pinState_t buttonOldState = PIN_IS_LOW;
	
	for ( ; ; )
	{
		buttonState = GPIO_read(BUTTON_2);
		
		if (buttonState != buttonOldState)
		{
			if (buttonOldState == PIN_IS_LOW)
					*messageToSend = "Button 2 RISING\n";
			else
					*messageToSend = "Button 2 FALLING\n";
			
			buttonOldState = buttonState;
		}
		
			vTaskDelayUntil(&xLastWakeTime, TSK2_PERIOD);
			xLastWakeTime = xTaskGetTickCount();
	}
}

void Periodic_Transmitter(char ** messageToSend)
{
	GPIO_write(TASK_GPIO(3), PIN_IS_HIGH);
	vTaskSetApplicationTaskTag(NULL, (void *) 3);
	TickType_t xLastWakeTime = xTaskGetTickCount();

	for ( ; ; )
	{
			*messageToSend = "TRANSMITTER\n";
			vTaskDelayUntil(&xLastWakeTime, TSK3_PERIOD);
			xLastWakeTime = xTaskGetTickCount();
	}
}

void UART_Receiver(char ** messageReceived)
{
	GPIO_write(TASK_GPIO(4), PIN_IS_HIGH);
	vTaskSetApplicationTaskTag(NULL, (void *) 4);
	TickType_t xLastWakeTime = xTaskGetTickCount();

	for ( ; ; )
	{
			while(vSerialPutString(*messageReceived, 150) == pdFALSE)
			vTaskDelayUntil(&xLastWakeTime, TSK4_PERIOD);
			xLastWakeTime = xTaskGetTickCount();
	}
}

void Load_1_Simulation(void * pvParameters)
{
	GPIO_write(TASK_GPIO(5), PIN_IS_HIGH);
	vTaskSetApplicationTaskTag(NULL, (void *) 5);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	int i;
	
	for ( ; ; )
	{
			for (i = 0; i < 36500; i++);
			vTaskDelayUntil(&xLastWakeTime, TSK5_PERIOD);
			xLastWakeTime = xTaskGetTickCount();
	}
}

void Load_2_Simulation(void * pvParameters)
{
	GPIO_write(TASK_GPIO(6), PIN_IS_HIGH);
	TickType_t xLastWakeTime = xTaskGetTickCount();
	vTaskSetApplicationTaskTag(NULL, (void *) 6);

	int i;
	
	for ( ; ; )
	{
			for (i = 0; i < 90000; i++);
			vTaskDelayUntil(&xLastWakeTime, TSK6_PERIOD);
			xLastWakeTime = xTaskGetTickCount();
	}
}

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */


int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	int i;
	
	char ** messages = NULL;
	
	GPIO_write(TICK_GPIO, PIN_IS_LOW);

	for (i = 1; i < 7; i++)
	{	
		GPIO_write(TASK_GPIO(i), PIN_IS_LOW);
	}
	
	
	
	#if ( configUSE_EDF_SCHEDULER == 1 )
			xTaskCreatePeriodic( Button_1_Monitor, 
														"BTN1", 
														configMINIMAL_STACK_SIZE, 
														(char *)&messages, 
														1, 
														TASK_1_HANDLE,
														TSK1_PERIOD );
														
			xTaskCreatePeriodic( Button_2_Monitor, 
														"BTN2", 
														configMINIMAL_STACK_SIZE, 
														(char *)&messages, 
														1, 
														TASK_2_HANDLE,
														TSK2_PERIOD );	
														
			xTaskCreatePeriodic( Periodic_Transmitter, 
														"TRANS", 
														configMINIMAL_STACK_SIZE, 
														(char *)&messages, 
														1, 
														TASK_3_HANDLE,
														TSK3_PERIOD );	
														
			xTaskCreatePeriodic( UART_Receiver, 
														"REC", 
														configMINIMAL_STACK_SIZE, 
														(char *)&messages, 
														1, 
														TASK_4_HANDLE,
														TSK4_PERIOD );	
														
			xTaskCreatePeriodic( Load_1_Simulation, 
														"LOAD1", 
														configMINIMAL_STACK_SIZE, 
														(	void * ) NULL, 
														1, 
														TASK_5_HANDLE,
														TSK5_PERIOD );		
														
			xTaskCreatePeriodic( Load_2_Simulation, 
														"LOAD2", 
														configMINIMAL_STACK_SIZE, 
														(	void * ) NULL, 
														1, 
														TASK_6_HANDLE,
														TSK6_PERIOD );	
																																	
	#else
	#endif 

	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


