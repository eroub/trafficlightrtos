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

enum task_type {PERIODIC,APERIODIC};

// Struct Inits
struct dd_task {
	TaskHandle_t t_handle;
	uint32_t type;
	uint32_t task_id;
	uint32_t release_time;
	uint32_t execution_time;
	uint32_t absolute_deadline;
	uint32_t completion_time;
};

struct dd_task_list {
	dd_task task;
	struct dd_task_list *next_task;
};

// DD Prototypes
void create_dd_task(TaskHandle_t t_handle, uint32_t type, uint32_t task_id, uint32_t absolute_deadline);
void delete_dd_task(uint32_t task_id);
void get_active_dd_task_list(void);
void get_complete_dd_task_list(void);
void get_overdue_dd_task_list(void);

// F Prototypes
void xDeadlineScheduler(void *pvParameters);
void xUserTasks(void *pvParameters);
void xDeadlineTaskGenerator1(void *pvParameters);
void xDeadlineTaskGenerator2(void *pvParameters);
void xDeadlineTaskGenerator3(void *pvParameters);
void xMonitorTask(void *pvParameters);

// Queue Definitions
xQueueHandle message_release_queue = 0;
xQueueHandle message_response_queue = 0;

/*-----------------------------------------------------------*/

int main(void)
{
	// Initialize System
	SystemInit();

	// Create the tasks
	xTaskCreate(xDeadlineScheduler, "DeadlineScheduler", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
	xTaskCreate(xUserTasks, "UserTasks", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
	xTaskCreate(xDeadlineTaskGenerator1, "DeadlineTaskGenerator", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(xDeadlineTaskGenerator2, "DeadlineTaskGenerator", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(xDeadlineTaskGenerator3, "DeadlineTaskGenerator", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(xMonitorTask, "MonitorTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	// Get size of struct
	int structSize = sizeof(struct dd_task);

	// Create message queue and message response queue
	message_release_queue = xQueueCreate(1,structSize);
	message_response_queue = xQueueCreate(1,structSize);
	vQueueAddToRegistry(message_release_queue, "ReleaseMessage");
	vQueueAddToRegistry(message_response_queue, "ReponseMessage");

	/* Start Scheduler */
	vTaskStartScheduler();

	return 0;
}

/*-----------------------------------------------------------*/

static void xDeadlineTaskGenerator1(void *pvParameters) {
	while(1){

	}
	// Delay the next task by 1000 ticks
	vTaskDelay(1000);
}

static void xDeadlineTaskGenerator2(void *pvParameters) {
	while(1){

	}
	// Delay the next task by 1000 ticks
	vTaskDelay(1000);
}

static void xDeadlineTaskGenerator3(void *pvParameters) {
	while(1){

	}
	// Delay the next task by 1000 ticks
	vTaskDelay(1000);
}

/*-----------------------------------------------------------*/

void release_dd_task(uint32_t type, uint32_t task_id, uint32_t execution_time, uint32_t absolute_deadline){
	// Create new task struct with given parameters
	struct dd_task task;
	task.type = type;
	task.task_id = task_id;
	task.execution_time = execution_time;
	task.absolute_deadline = absolute_deadline;
	
	// Call xTaskCreate to create a xUserTask for the new task struct
	xTaskCreate(xUserTasks, "UserDefinedTask", configMINIMAL_STACK_SIZE, &(task.execution_time), 1, &(task.t_handle));

	// Next send the struct via message queue
	xQueueSend(message_release_queue, &task, 50);

	// Lastly await response from xDeadlineScheduler
	int reponse;
	while(1) if (xQueueReceive(message_response_queue, &reponse, 50)) break;
}

static void xDeadlineScheduler(void *pvParameters) {
	// Delay the next task by 1000 ticks
	vTaskDelay(1000);
}

/*-----------------------------------------------------------*/

static void xUserTasks(void *pvParameters) {
	while(xGet)
	vTaskDelay(1000);
}

/*-----------------------------------------------------------*/

static void xMonitorTask(void *pvParameters) {
	// Delay the next task by 1000 ticks
	vTaskDelay(1000);
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
