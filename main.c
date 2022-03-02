/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wwrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
FreeRTOS is a market leading RTOS from Real Time Engineers Ltd. that supports
31 architectures and receives 77500 downloads a year. It is professionally
developed, strictly quality controlled, robust, supported, and free to use in
commercial products without any requirement to expose your proprietary source
code.

This simple FreeRTOS demo does not make use of any IO ports, so will execute on
any Cortex-M3 of Cortex-M4 hardware.  Look for TODO markers in the code for
locations that may require tailoring to, for example, include a manufacturer
specific header file.

This is a starter project, so only a subset of the RTOS features are
demonstrated.  Ample source comments are provided, along with web links to
relevant pages on the http://www.FreeRTOS.org site.

Here is a description of the project's functionality:

The main() Function:
main() creates the tasks and software timers described in this section, before
starting the scheduler.

The Queue Send Task:
The queue send task is implemented by the prvQueueSendTask() function.
The task uses the FreeRTOS vTaskDelayUntil() and xQueueSend() API functions to
periodically send the number 100 on a queue.  The period is set to 200ms.  See
the comments in the function for more details.
http://www.freertos.org/vtaskdelayuntil.html
http://www.freertos.org/a00117.html

The Queue Receive Task:
The queue receive task is implemented by the prvQueueReceiveTask() function.
The task uses the FreeRTOS xQueueReceive() API function to receive values from
a queue.  The values received are those sent by the queue send task.  The queue
receive task increments the ulCountOfItemsReceivedOnQueue variable each time it
receives the value 100.  Therefore, as values are sent to the queue every 200ms,
the value of ulCountOfItemsReceivedOnQueue will increase by 5 every second.
http://www.freertos.org/a00118.html

An example software timer:
A software timer is created with an auto reloading period of 1000ms.  The
timer's callback function increments the ulCountOfTimerCallbackExecutions
variable each time it is called.  Therefore the value of
ulCountOfTimerCallbackExecutions will count seconds.
http://www.freertos.org/RTOS-software-timer.html

The FreeRTOS RTOS tick hook (or callback) function:
The tick hook function executes in the context of the FreeRTOS tick interrupt.
The function 'gives' a semaphore every 500th time it executes.  The semaphore
is used to synchronise with the event semaphore task, which is described next.

The event semaphore task:
The event semaphore task uses the FreeRTOS xSemaphoreTake() API function to
wait for the semaphore that is given by the RTOS tick hook function.  The task
increments the ulCountOfReceivedSemaphores variable each time the semaphore is
received.  As the semaphore is given every 500ms (assuming a tick frequency of
1KHz), the value of ulCountOfReceivedSemaphores will increase by 2 each second.

The idle hook (or callback) function:
The idle hook function queries the amount of free FreeRTOS heap space available.
See vApplicationIdleHook().

The malloc failed and stack overflow hook (or callback) functions:
These two hook functions are provided as examples, but do not contain any
functionality.
*/

/* Standard includes. */
// stdlib.h needed for rand, time.h needed for time
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "stm32f4_discovery.h"
/* Kernel includes. */
#include "stm32f4xx.h"
#include "../FreeRTOS_Source/include/FreeRTOS.h"
#include "../FreeRTOS_Source/include/queue.h"
#include "../FreeRTOS_Source/include/semphr.h"
#include "../FreeRTOS_Source/include/task.h"
#include "../FreeRTOS_Source/include/timers.h"

/*-----------------------------------------------------------*/
#define mainQUEUE_LENGTH 100


// Global definitions for pins
#define INPUT_PORT GPIOC
#define traffic_light_red GPIO_Pin_0
#define traffic_light_yellow GPIO_Pin_1
#define traffic_light_green GPIO_Pin_2
#define shift_reset GPIO_Pin_8
#define shift_clock GPIO_Pin_7
#define shift_data GPIO_Pin_6

// Global variables for queues and timer
TimerHandle_t trafficTimer;
xQueueHandle flowQueue = 0;
xQueueHandle lightsQueue = 0;
xQueueHandle nextCarQueue = 0;

// Define light states
#define GREEN 1
#define YELLOW 2
#define RED 3

// Base flow to send if no potentiometer reading
#define BASE_FLOW 17
// Num positions defines the number of possible cars that can be on the board
#define NUM_POSITIONS 19

void hardwareInit() {

	/* Ensure all priority bits are assigned as preemption priority bits.
	http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	NVIC_SetPriorityGrouping( 0 );

	// Enable all GPIO clocks
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	// Traffic GPIO Structure INIT
	GPIO_InitTypeDef GPIO_Struct;
	GPIO_Struct.GPIO_Pin = traffic_light_red | traffic_light_yellow | traffic_light_green | shift_reset | shift_clock | shift_data;
	// Output mode
	GPIO_Struct.GPIO_Mode = GPIO_Mode_OUT;
	// Push-pull mode
	GPIO_Struct.GPIO_OType = GPIO_OType_PP;
	// Disable pull-ups/pull-downs
	GPIO_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	// Speed for refresh
	GPIO_Struct.GPIO_Speed = GPIO_Speed_50MHz;
	// GPIO INIT
	GPIO_Init(INPUT_PORT, &GPIO_Struct);

	// Enable GPIO clocks for ADC
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	// GPIO Init for Potentiometer
	GPIO_InitTypeDef GPIO_POT_Struct;
	// Set PIN for potentiometer
	GPIO_POT_Struct.GPIO_Pin = GPIO_Pin_3;
	// Set GPIO to analog
	GPIO_POT_Struct.GPIO_Mode = GPIO_Mode_AN;
	// Set no pull
	GPIO_POT_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_POT_Struct);

	// ADC INIT
	ADC_InitTypeDef ADC_Struct;
	// Continuous Conversion Mode
	ADC_Struct.ADC_ContinuousConvMode = DISABLE;
	// Data Alignment
	ADC_Struct.ADC_DataAlign = ADC_DataAlign_Right;
	// External Trigger Conversion
	ADC_Struct.ADC_ExternalTrigConv = DISABLE;
	ADC_Struct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_Struct.ADC_NbrOfConversion = 1;
	// Resolution (12 is highest)
	ADC_Struct.ADC_Resolution = ADC_Resolution_12b;
	// Scan Conv Mode for reading multiple channels
	ADC_Struct.ADC_ScanConvMode = DISABLE;
	ADC_Init(ADC1, &ADC_Struct);

	ADC_Cmd(ADC1, ENABLE);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 1, ADC_SampleTime_84Cycles);

	// Set and reset traffic bits
	GPIO_SetBits(GPIOC, shift_reset);
	GPIO_ResetBits(GPIOC, shift_reset);
	GPIO_SetBits(GPIOC, shift_reset);

}

/*-----------------------------------------------------------*/

int read_potentiometer() {
	// Start ADC
	ADC_SoftwareStartConv(ADC1);
	// Wait until flag status confirms completion
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
	// Return value
	return ADC_GetConversionValue(ADC1);
}

/*-----------------------------------------------------------*/

static void TrafficFlowAdjustmentTask(void *pvParameters) {
	int flow = BASE_FLOW;
	// Infinite while loop to continuously read potentiometer value
	// Ten flow stages to represent the different steps from 0-100 in increments of 10 of the potentiometer
	while(1) {
		// These values represent % chance from 0%-100% of a car being spawned
		if (read_potentiometer() <= 380 ) { flow = 17; }
	    else if (read_potentiometer() > 380 && read_potentiometer() <= 785) { flow = 20; }
	    else if (read_potentiometer() > 785 && read_potentiometer() <= 1195) { flow = 25; }
	    else if (read_potentiometer() > 1195 && read_potentiometer() <= 1605) { flow = 35; }
		else if (read_potentiometer() > 1605 && read_potentiometer() <= 2015) { flow = 40; }
		else if (read_potentiometer() > 2015 && read_potentiometer() <= 2425) { flow = 50; }
		else if (read_potentiometer() > 2425 && read_potentiometer() <= 2825) { flow = 60; }
		else if (read_potentiometer() > 2825 && read_potentiometer() <= 3245) { flow = 70; }
		else if (read_potentiometer() > 3245 && read_potentiometer() <= 3645) { flow = 80; }
		else if (read_potentiometer() > 3975){ flow = 90; }
		// Send flow value to flowQueue (wait 1000 ticks)
		if(!xQueueSend(flowQueue, &flow, 150)) printf("An error occurred sending the flow value to queue");

		// Delay the next task by 1000 ticks
		vTaskDelay(1000);
		}
}

static void TrafficGeneratorTask(void *pvParameters) {
	int flowVal;
	// If car is one then display a car, else display nothing
	int car = 0;
	// Time t and srand needed for randomness
	time_t t;
	srand((unsigned) time(&t));

	while(1) {
		// Get flow value generated from TrafficFlowAdjustmentTask
		if(xQueueReceive(flowQueue, &flowVal, 150)) {
			// Generate random number less than 100
			int random_value = rand() % 100;
			// Using random number determine if a car is to be sent or not
			if(random_value < flowVal){
				car = 1;
			} else {
				car = 0;
			}
			// Send value of car (0 if no car) to queue
			if(!xQueueSend(nextCarQueue, &car, 150)) printf("error sending car");

		}
		// Delay the next task by 1000 ticks
		vTaskDelay(1000);
	}
}

/*-----------------------------------------------------------*/

int determineGreenLight() {
	// Lab spec defined green light length to be proportional to flow
	// At max it should be ~ twice as long as when at min
	int flowVal;
	int time = 4500;
	if(xQueuePeek(flowQueue, &flowVal, 150)) {
		// Time here represents ticks
		if (flowVal == 17) { time = 3000; }
		else if (flowVal == 20) { time = 3333; }
		else if (flowVal == 25) { time = 3666; }
		else if (flowVal == 35) { time = 4000; }
		else if (flowVal == 40) { time = 4333; }
		else if (flowVal == 50) { time = 4666; }
		else if (flowVal == 60) { time = 5000; }
		else if (flowVal == 70) { time = 5333; }
		else if (flowVal == 80) { time = 5666; }
		else if (flowVal == 90) { time = 6000; }
	}
	return time;
}

static void TrafficLightStateTask(void *pvParameters) {
	int lightState;
	while(1){
		// First determine how long the green light is to stay on from the flowVal
		int greenLightLength = determineGreenLight();
		// After length of green light is determined check what state the light is in currently
		if(xQueuePeek(lightsQueue, &lightState, 150)) {
			// If light is currently green set timer to switch it to yellow
			if(lightState == GREEN) {
				xTimerStart(trafficTimer, pdMS_TO_TICKS(1000));
				// Delay it by a constant
				vTaskDelay(1000);
			// If light state is currently yellow set timer to switch it to red
			} else if (lightState == YELLOW) {
				xTimerStart(trafficTimer, pdMS_TO_TICKS(9000)-pdMS_TO_TICKS(greenLightLength));
				// For red light spec defines it to be inversely proportional to flow
				// So for delay of red light set constant then subtract by whatever greenLightLength is
//				vTaskDelay(1000);
				 vTaskDelay(pdMS_TO_TICKS(9000)-pdMS_TO_TICKS(greenLightLength));
			// If light state is currently red set timer to switch it to green
			} else if (lightState == RED) {
				xTimerStart(trafficTimer, pdMS_TO_TICKS(greenLightLength));
				// For green light simply delay by the ticks of greenLightLength
//				vTaskDelay(1000);
				vTaskDelay(pdMS_TO_TICKS(greenLightLength));
			}
		}
		// Since each light state in itself delys the next task we do not need another here at the end of while loop
		 vTaskDelay(1000);
	}
}

/*-----------------------------------------------------------*/

void add_car_and_shift() {
	GPIO_SetBits(GPIOC, shift_reset);
	GPIO_SetBits(GPIOC, shift_data);

	// Reset and set shift clock
	GPIO_ResetBits(GPIOC, shift_clock);
	GPIO_SetBits(GPIOC, shift_clock);
}


void no_car_and_shift() {
	GPIO_SetBits(GPIOC, shift_reset);
	GPIO_ResetBits(GPIOC, shift_data);

	// Reset and set shift clock
	GPIO_ResetBits(GPIOC, shift_clock);
	GPIO_SetBits(GPIOC, shift_clock);
}

static void SystemDisplayTask(void *pvParameters) {
	int lightState, carState;
	// This array exists to keep track of the position of all the cars for the 19 possible positions, need to init to 0
	int car_arr[NUM_POSITIONS] = {0};
	while(1){
		// Get values of lightState and carState
		if(xQueuePeek(lightsQueue, &lightState, 150) && xQueueReceive(nextCarQueue, &carState, 150)) {
			if(lightState == GREEN) {
				// Turn off red light and turn on green light
				GPIO_ResetBits(INPUT_PORT, traffic_light_red);
				GPIO_SetBits(INPUT_PORT, traffic_light_green);
				// Shift all cars in all positions starting from end of array
				for (int i=NUM_POSITIONS-1; i>0; i--) {
					car_arr[i] = car_arr[i-1];
					car_arr[i-1] = 0;
				}
				// Set the first car to that of the carState
				car_arr[0] = carState;
			} else {
				// For all intents and purposes a yellow acts like a red, in that cars will also stop at a yellow
				if(lightState == YELLOW) {
					// Turn off green light and turn on yellow light
					GPIO_ResetBits(INPUT_PORT, traffic_light_green);
					GPIO_SetBits(INPUT_PORT, traffic_light_yellow);
				}
				if(lightState == RED) {
					// Turn off yellow light and turn on red light
					GPIO_ResetBits(INPUT_PORT, traffic_light_yellow);
					GPIO_SetBits(INPUT_PORT, traffic_light_red);
				}
				// Shift cars before red/yellow light until they hit another car or light
				// Otherwise shift all cars past the red/yellow light
				// For all cars past the 8th position shift them
				for(int i= NUM_POSITIONS-1; i>8; i--) {
					car_arr[i] = car_arr[i-1];
					car_arr[i-1] = 0;
				}
				// Set the value of the 8th position to be 0 (as it was shifted in the above for loop) but do not change the value of the car before the stop line
				car_arr[8] = 0;
				// Next deal with cars in front of the light
				for(int i=7; i>0; i--) {
					// If the position is empty, shift the position behind it
					if (car_arr[i] == 0) {
						car_arr[i] = car_arr[i-1];
						car_arr[i-1] = 0;
					}
				}
				// Lastly if the first position does not hold a car insert the value of carState
				if (!car_arr[0]) car_arr[0] = carState;
			}
			// Finally iterate through car array and set bits according to the value of the car in each position
			for (int i=NUM_POSITIONS-1; i>-1; i--) {
				if(car_arr[i]) {
					add_car_and_shift();
				} else {
					no_car_and_shift();
				}
			}
		}
		// Delay the next task by 1000 ticks
		vTaskDelay(1000);
	}
}

/*-----------------------------------------------------------*/

void TimerCallback(TimerHandle_t t) {
	int lightState, nextLightState;
	if(xQueueReceive(lightsQueue, &lightState, 150)) {
		// Handle case when current lightState is green
		if(lightState == GREEN) {
			// When green pass yellow as nextLightState
			nextLightState = YELLOW;
			if(!xQueueSend(lightsQueue, &nextLightState, 150)) printf("error sending light");
		// Handle case when current lightState is yellow
		} else if (lightState == YELLOW) {
			// When yellow pass red as nextLightState
			nextLightState = RED;
			if(!xQueueSend(lightsQueue, &nextLightState, 150)) printf("error sending light");
		// Handle case when current lightState is red
		} else if (lightState == RED) {
			// When red pass green as nextLightState
			nextLightState = GREEN;
			if(!xQueueSend(lightsQueue, &nextLightState, 150)) printf("error sending light");
		}
	}
}

/*-----------------------------------------------------------*/

int main(void)
{

	// This function is responsible for initializing the GPIO and ADC

	hardwareInit();

	// Create the queues for the flow of traffic and which lights are on
	flowQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(int));
	nextCarQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(int));
	lightsQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(int));
	// Register Queues
	vQueueAddToRegistry(flowQueue, "TrafficFlowQueue");
	vQueueAddToRegistry(nextCarQueue, "FutureCarQueue");
	vQueueAddToRegistry(lightsQueue, "TrafficLightQueue");
	// Create the tasks
	xTaskCreate(TrafficFlowAdjustmentTask, "TrafficFlowTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate(TrafficGeneratorTask, "TrafficGeneratorTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(TrafficLightStateTask, "TrafficLightTask", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
	xTaskCreate(SystemDisplayTask, "SystemDisplayTask", configMINIMAL_STACK_SIZE, NULL, 4, NULL);

	// Initialize timer used to determine how long lights stay on
	trafficTimer = xTimerCreate("TrafficTimer", 1000, pdFALSE, (void *)0, TimerCallback);

	// Send GREEN light to queue to initialize light
	int initLightState = GREEN;
	xQueueSend(lightsQueue, &initLightState, 150);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	return 0;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* The malloc failed hook is enabled by setting
	configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

	Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software 
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  pxCurrentTCB can be
	inspected in the debugger if the task name passed into this function is
	corrupt. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
	FreeRTOSConfig.h.

	This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amount of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}
