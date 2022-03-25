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
#define STATIC_TASK_STACK 200

StackType_t xStack [STATIC_TASK_STACK];

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
	struct dd_task task;
	struct dd_task_list *next_task;
};

// DD Prototypes
void release_dd_task(uint32_t type, uint32_t task_id, uint32_t execution_time, uint32_t absolute_deadline);
void complete_dd_task(TaskHandle_t t_handle);
void get_active_dd_task_list(void);
void get_complete_dd_task_list(void);
void get_overdue_dd_task_list(void);

// F Prototypes
void xDeadlineScheduler(void *pvParameters);
void xUserTasks(void *pvParameters);
void xDeadlineTask1Generator(void *pvParameters);
void xDeadlineTask2Generator(void *pvParameters);
void xDeadlineTask3Generator(void *pvParameters);
void xMonitorTask(void *pvParameters);

// Queue Definitions
xQueueHandle message_release_queue = 0;
xQueueHandle message_response_queue = 0;

xQueueHandle completed_message_queue = 0;
xQueueHandle completed_reply_queue = 0;

/*-----------------------------------------------------------*/

int main(void) {
	// Test bench variable
	int testBench = 1;

	// Initialize System
	SystemInit();

	// Create the tasks
	xTaskCreate(xDeadlineScheduler, "DeadlineScheduler", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
	xTaskCreate(xDeadlineTask1Generator, "DeadlineTask1Generator", configMINIMAL_STACK_SIZE, (void *) testBench, 3, NULL);
	xTaskCreate(xDeadlineTask2Generator, "DeadlineTask2Generator", configMINIMAL_STACK_SIZE, (void *) testBench, 3, NULL);
	xTaskCreate(xDeadlineTask3Generator, "DeadlineTask3Generator", configMINIMAL_STACK_SIZE, (void *) testBench, 3, NULL);
	// xTaskCreate(xMonitorTask, "MonitorTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	// Get size of struct
	int structSize = sizeof(struct dd_task);

	// Create message queue and message response queue
	message_release_queue = xQueueCreate(4,structSize);
	message_response_queue = xQueueCreate(4,sizeof(int));
	vQueueAddToRegistry(message_release_queue, "ReleaseMessage");
	vQueueAddToRegistry(message_response_queue, "ReponseMessage");

	completed_message_queue = xQueueCreate(4,sizeof(TaskHandle_t));
	completed_reply_queue = xQueueCreate(4,sizeof(int));
	vQueueAddToRegistry(completed_message_queue, "CompletedMessage");
	vQueueAddToRegistry(completed_reply_queue, "CompletedResponse");

	/* Start Scheduler */
	vTaskStartScheduler();

	return 0;
}

/*-----------------------------------------------------------*/

 void xDeadlineTask1Generator(void *pvParameters) {
	// Read test bench
	int bench = (int) pvParameters;

	uint32_t count = 0;
	uint32_t period;
	uint32_t execution;
	while(1){
		if(bench == 1){
			period = 500; execution = 95;
		} else if(bench == 2){
			period = 250; execution = 95;
		}else{
			period = 500; execution = 100;
		}
		// Increment count by period
		count = count + period;
		// Releasing task
		release_dd_task(0, 1, execution, count);
		// After task released delay by period
		vTaskDelay(pdMS_TO_TICKS(period));
	}
}

void xDeadlineTask2Generator(void *pvParameters) {
	// Read test bench
	int bench = (int) pvParameters;

	uint32_t count = 0;
	uint32_t period;
	uint32_t execution;
	while(1){
		if(bench == 1){
			period = 500; execution = 150;
		} else if(bench == 2){
			period = 500; execution = 150;
		}else{
			period = 500; execution = 200;
		}
		// Increment count by period
		count = count + period;
		// Releasing task
		release_dd_task(0, 2, execution, count);
		// After task released delay by period
		vTaskDelay(pdMS_TO_TICKS(period));
	}
}

void xDeadlineTask3Generator(void *pvParameters) {
	// Read test bench
	int bench = (int) pvParameters;

	uint32_t count = 0;
	uint32_t period;
	uint32_t execution;
	while(1){
		if(bench == 1){
			period = 750; execution = 250;
		} else if(bench == 2){
			period = 750; execution = 250;
		}else{
			period = 500; execution = 200;
		}
		// Increment count by period
		count = count + period;
		// Releasing task
		release_dd_task(0, 3, execution, count);
		// After task released delay by period
		vTaskDelay(pdMS_TO_TICKS(period));
	}
}

/*-----------------------------------------------------------*/

void complete_dd_task(TaskHandle_t t_handle) {
	// Send message that task was completed
	xQueueSend(completed_message_queue, &t_handle, 5);

	// Wait for response
	int response;
	while(1) if (xQueueReceive(completed_reply_queue, &response, 5)) break;
	vTaskDelete(NULL);
	//vTaskDelete(t_handle);
}

/*-----------------------------------------------------------*/

void release_dd_task(uint32_t type, uint32_t task_id, uint32_t execution_time, uint32_t absolute_deadline) {
	// Create new task struct with given parameters
	struct dd_task task;
	//task.t_handle = NULL;
	task.type = type;
	task.task_id = task_id;
	task.execution_time = execution_time;
	task.absolute_deadline = absolute_deadline;

	// Next send the struct via message queue
	xQueueSend(message_release_queue, &task, 5);

	// Lastly await response from xDeadlineScheduler
	int response;
	while(1) if (xQueueReceive(message_response_queue, &response, 5)) break;
}

/*-----------------------------------------------------------*/

void xUserTasks(void *pvParameters) {
	// Get times
	int executionTime = (int) pvParameters;
	int startTime = (int) xTaskGetTickCount();
	int endTime = startTime + executionTime;
	// Wait until endTime has been passed
	while(endTime > (int) xTaskGetTickCount());
	// Send completed task to completed_queue
	complete_dd_task(xTaskGetCurrentTaskHandle());
}

/*-----------------------------------------------------------*/

void xDeadlineScheduler(void *pvParameters) {
	// Create active_task_list
	struct dd_task_list *active_task_list = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
	// Set second value to NULL
	active_task_list->next_task = NULL;

	// Create completed_task_list
	struct dd_task_list *completed_task_list = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
	// Set second value to NULL
	completed_task_list->next_task = NULL;

	// Create overdue_task_list
	struct dd_task_list *overdue_task_list = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
	// Set second value to NULL
	overdue_task_list->next_task = NULL;

	struct dd_task user_task;

	while(1){
		if(xQueueReceive(message_release_queue, &user_task, 5)){
			// Init new list node
			struct dd_task_list *new_user_task = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
			new_user_task->task = user_task;
			// Create copy of head of active_task_list
			struct dd_task_list *active_head = active_task_list;
			// Iterate through active_task_list and new user_task to list based on absolute_deadline
			while(active_head->next_task != NULL) {
				// Check absolute deadline of task in active list against new task
				if(active_head->next_task->task.absolute_deadline > new_user_task->task.absolute_deadline) {
					new_user_task->next_task = active_head->next_task;
					active_head->next_task = new_user_task;
					break;
				}
				active_head = active_head->next_task;
			}
			new_user_task->next_task = active_head->next_task;
			active_head->next_task = new_user_task;


			// Send response to release_dd_task to allow it to continue
			int response = 1;
			xQueueSend(message_response_queue, &response, 5);
		}
		// Next check message_release_queue again, if there's a value continue again to start of while loop
		// Otherwise start the task at the head of the active_list
		if(xQueuePeek(message_release_queue, &user_task, 50)) {
			continue;
		} else if (active_task_list->next_task != NULL) {
			// Print list
			// printLinkedList(active_task_list->next_task);


			// Get head of list
			struct dd_task task_to_run = active_task_list->next_task->task;
			// Create new task
			printf("* Task Created with ID:%d, Deadline:%d, Execution:%d\n", task_to_run.task_id, task_to_run.absolute_deadline, task_to_run.execution_time);
			xTaskCreate(xUserTasks, "UserDefinedTask", configMINIMAL_STACK_SIZE,(void *) task_to_run.execution_time, 1, &task_to_run.t_handle);

			// Wait to receive completed message
			TaskHandle_t completed_task_handle;
			while(1) if(xQueueReceive(completed_message_queue, &completed_task_handle, 5)) break;
			// Set current_list variable to active_list_head
			struct dd_task_list *finished_task = active_task_list;
			// Find completed task in list of active tasks
			int foundCompletedTask = 1;
			while(finished_task->task.absolute_deadline != task_to_run.absolute_deadline && finished_task->task.execution_time != task_to_run.execution_time && finished_task->task.task_id != task_to_run.task_id) {
				if (finished_task->next_task == NULL) {
					printf("Error, could not find completed task\n");
					foundCompletedTask = 0;
					break;
				} else {
					finished_task = finished_task->next_task;
				}
			}
			if(foundCompletedTask) {
				// Send response to complete_dd_task
				int response = 1;
				xQueueSend(completed_reply_queue, &response, 5);

				// Set completion_time
				finished_task->task.completion_time = (int) xTaskGetTickCount();

				// Check if task is overdue or not
				if(finished_task->task.completion_time > finished_task->task.absolute_deadline) {
					// TASK WAS OVERDUE ADD TO OVERDUE LIST
					printf("!! OVERDUE Completion with ID:%d, Deadline:%d, Execution:%d, Completion:%d\n", finished_task->task.task_id, finished_task->task.absolute_deadline, finished_task->task.execution_time, finished_task->task.completion_time);
				} else {
					// TASK WAS DONE ON TIME ADD TO COMPLETED LIST
					printf("-- ONTIME Completion with ID:%d, Deadline:%d, Execution:%d, Completion:%d\n", finished_task->task.task_id, finished_task->task.absolute_deadline, finished_task->task.execution_time, finished_task->task.completion_time);
				}

				// Remove task from list
				// Set prev, and curr variables from active_list
				struct dd_task_list *curr = active_task_list;
				struct dd_task_list *prev = active_task_list;
				while(curr->task.task_id != finished_task->task.task_id) {
					if(curr->next_task == NULL) {
						printf("Error, completed task id not found in active task list\n");
						return;
					}
					prev = curr;
					curr = curr->next_task;
				}
				// After finished_task is equal to curr set prev to curr
				if(curr->next_task == NULL) {
					prev->next_task = NULL;
				} else {
					prev->next_task = curr->next_task;
				}
				// Finally free curr and finished_task
				free(finished_task);
				free(curr);
			}
		}




	}


}

/*-----------------------------------------------------------*/

void printLinkedList(struct dd_task_list *list) {
	printf("Printing linked list -----\n");
	struct dd_task_list *curr = list;
	while ( curr != NULL) {
		printf(" task_id:%d, execution_time:%d, absolute_deadline:%d\n", curr->task.task_id, curr->task.execution_time, curr->task.absolute_deadline);
		curr = curr->next_task;
	}
	printf("--------------\n");
}

/*-----------------------------------------------------------*/

void xMonitorTask(void *pvParameters) {
	while(1) {vTaskDelay(100);}
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
