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
int get_active_dd_task_list(struct dd_task_list *list);
int get_complete_dd_task_list(struct dd_task_list *list);
int get_overdue_dd_task_list(struct dd_task_list *list);

// Helper prototypes
void printLinkedList(struct dd_task_list *list);
void insertIntoListByPriority(struct dd_task_list *active_head, struct dd_task user_task);
void genericInsertIntoList(struct dd_task_list *list_head, struct dd_task task);

// F Prototypes
void xDeadlineScheduler(void *pvParameters);
void xUserTasks(void *pvParameters);
void xDeadlineTask1Generator(void *pvParameters);
void xDeadlineTask2Generator(void *pvParameters);
void xDeadlineTask3Generator(void *pvParameters);
void xMonitorTask(void *pvParameters);

// Queue Definitions
xQueueHandle message_release_queue = 0;
xQueueHandle completed_message_queue = 0;
xQueueHandle active_task_queue = 0;
xQueueHandle completed_task_queue = 0;
xQueueHandle overdue_task_queue = 0;

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
	xTaskCreate(xMonitorTask, "MonitorTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	// Get size of struct
	int structSize = sizeof(struct dd_task);

	// Create all necessary queues
	message_release_queue = xQueueCreate(4, structSize);
	vQueueAddToRegistry(message_release_queue, "ReleaseMessage");
	completed_message_queue = xQueueCreate(4, sizeof(TaskHandle_t));
	vQueueAddToRegistry(completed_message_queue, "CompletedMessage");
	active_task_queue = xQueueCreate(1, structSize);
	vQueueAddToRegistry(active_task_queue, "ActiveTaskQueue");
	completed_task_queue = xQueueCreate(1, structSize);
	vQueueAddToRegistry(completed_task_queue, "CompletedTaskQueue");
	overdue_task_queue = xQueueCreate(1, structSize);
	vQueueAddToRegistry(overdue_task_queue, "OverdueTaskQueue");

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
	xQueueSend(completed_message_queue, &t_handle, 0);
	// Delete task
	vTaskDelete(NULL);
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
	xQueueSend(message_release_queue, &task, 0);
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

void insertIntoListByPriority(struct dd_task_list *active_head, struct dd_task user_task) {
	// Init user task node
	struct dd_task_list *new_user_task = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
	new_user_task->task = user_task;
	new_user_task->next_task = NULL;
	// Make copy of active_head
	struct dd_task_list *head = active_head;

	while(head->next_task != NULL) {
		// Check absolute deadline of task in active list against new task
		if(head->next_task->task.absolute_deadline > new_user_task->task.absolute_deadline) {
			new_user_task->next_task = head->next_task;
			head->next_task = new_user_task;
			return;
		} else {
			head = head->next_task;
		}
	}
	//new_user_task->next_task = active_head->next_task;
	head->next_task = new_user_task;
}

/*-----------------------------------------------------------*/

void genericInsertIntoList(struct dd_task_list *list_head, struct dd_task task) {
	// Init  task node
	struct dd_task_list *user_task = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
	user_task->task = task;
	user_task->next_task = NULL;
	// Make copy of list_head
	struct dd_task_list *head = list_head;
	// Iterate through list until at end
	while(head->next_task != NULL) head = head->next_task;
	// Add user_task to list
	head->next_task = user_task;
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

	while(1) {
		if(xQueueReceive(message_release_queue, &user_task, 0)){
			// Insert new list node
			insertIntoListByPriority(active_task_list, user_task);
		}
		// Next check message_release_queue again, if there's a value continue again to start of while loop
		// Otherwise start the task at the head of the active_list
		if(xQueuePeek(message_release_queue, &user_task, 0)) {
			continue;
		} else if (active_task_list->next_task != NULL) {
			// Print list
			// printLinkedList(active_task_list->next_task);

			// Get head of list
			struct dd_task task_to_run = active_task_list->next_task->task;
			// Create new task
			// Set release time for the task
			task_to_run.release_time = xTaskGetTickCount();
			xTaskCreate(xUserTasks, "UserDefinedTask", configMINIMAL_STACK_SIZE,(void *) task_to_run.execution_time, 1, &task_to_run.t_handle);

			// Wait to receive completed message
			TaskHandle_t completed_task_handle;
			while(1) if(xQueueReceive(completed_message_queue, &completed_task_handle, 1)) break;
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
				// Set completion_time
				finished_task->task.completion_time = (int) xTaskGetTickCount();
				// Check if task is overdue or not
				if(finished_task->task.completion_time > finished_task->task.absolute_deadline) {
					// TASK WAS OVERDUE ADD TO OVERDUE LIST
					genericInsertIntoList(overdue_task_list, finished_task->task);
//					printf("!! OVERDUE Completion with ID:%d, Deadline:%d, Execution:%d, Completion:%d\n", finished_task->task.task_id, finished_task->task.absolute_deadline, finished_task->task.execution_time, finished_task->task.completion_time);
				} else {
					// TASK WAS DONE ON TIME ADD TO COMPLETED LIST
					genericInsertIntoList(completed_task_list, finished_task->task);
//					printf("-- ONTIME Completion with ID:%d, Deadline:%d, Execution:%d, Completion:%d\n", finished_task->task.task_id, finished_task->task.absolute_deadline, finished_task->task.execution_time, finished_task->task.completion_time);
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

		// Send active list to active_task_queue
		if(active_task_list->next_task != NULL) xQueueOverwrite(active_task_queue, &active_task_list);
		// Send completed list to completed_task_queue
		if(completed_task_list->next_task != NULL) xQueueOverwrite(completed_task_queue, &completed_task_list);
		// Send overdue list to overdue_task_queue
		if(overdue_task_list->next_task != NULL) xQueueOverwrite(overdue_task_queue, &overdue_task_list);

		// Delay by 1 to allow other programs to pre-empt
		vTaskDelay(1);
	}


}

/*-----------------------------------------------------------*/

int get_active_dd_task_list(struct dd_task_list *list) {
	struct dd_task_list *head = list;
	int i = 0;
	while(head->next_task != NULL) {
		head = head->next_task;
		i++;
	}
	return i;
}

int get_complete_dd_task_list(struct dd_task_list *list) {
	struct dd_task_list *head = list;
	int i = 0;
	while(head->next_task != NULL) {
		head = head->next_task;
		i++;
	}
	return i;
}

int get_overdue_dd_task_list(struct dd_task_list *list) {
	struct dd_task_list *head = list;
	int i = 0;
	while(head->next_task != NULL) {
		head = head->next_task;
		i++;
	}
	return i;
}

/*-----------------------------------------------------------*/

void xMonitorTask(void *pvParameters) {
	while(1) {
		// We want Monitor Task to print results every hyper period so delay it by 1500ms each time
		vTaskDelay(pdMS_TO_TICKS(1500));

		printf("--- Monitor Task for HyperPeriod ---\n");
		// Get active list from queue and print results from get_active_dd_task_list
		struct dd_task_list *active_task_list = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
		if(xQueueReceive(active_task_queue, &active_task_list, 0)) {
			printf("  -- Number of Active Tasks: %d\n", get_active_dd_task_list(active_task_list));
		} else {
			// If nothing to receive from queue just print 0
			printf("  -- Number of Active Tasks: 0\n");
		}
		// Get completed list from queue and print results from get_complete_dd_task_list
		struct dd_task_list *complete_task_list = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
		if(xQueueReceive(completed_task_queue, &complete_task_list, 0)) {
			printf("  -- Number of Completed Tasks: %d\n", get_complete_dd_task_list(complete_task_list));
		} else {
			// If nothing to receive from queue just print 0
			printf("  -- Number of Completed Tasks: 0\n");
		}
		// Get overdue list from queue and print results from get_overdue_dd_task_list
		struct dd_task_list *overdue_task_list = (struct dd_task_list*) malloc(sizeof(struct dd_task_list));
		if(xQueueReceive(overdue_task_queue, &overdue_task_list, 0)) {
			printf("  -- Number of Overdue Tasks: %d\n", get_overdue_dd_task_list(overdue_task_list));
		} else {
			// If nothing to receive from queue just print 0
			printf("  -- Number of Overdue Tasks: 0\n");
		}
		printf("---------------------------------------\n");
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
