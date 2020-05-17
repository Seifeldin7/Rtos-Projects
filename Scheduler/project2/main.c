/* Library includes. */
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "vector.h"
#include "bool.h"

# define N  5
# define tst  1
# define latest_arrival_time 15
# define maximum_computation_time 8
# define maximum_period_multipler  17
# define safe_mode TRUE
typedef struct 
{
	unsigned int index;
	unsigned long Ta;
	unsigned long Tc;
	unsigned long Tp;
	float cpu_util;
	unsigned int priority;
	xTaskHandle xTaskHandler;
}xTask;
typedef struct 
{
	unsigned int op;
	xTask new_task;
}send_task;

/* Dimensions the buffer into which messages destined for stdout are placed. */
#define mainMAX_MSG_LEN	( 80 )

static xTask generate_task(void);
static void static_task_admittance(void);
static void dynamic_task_admittance( void *pvParameters );

static void SchedulerGatekeeperTask( void *pvParameters );
static int isSchedulable(void);
static void cpu_util(void);
static void swap(int xp, int yp);
static int* sortTask(int* sorted_index);
static void set_priority(void);

static void vTaskCreate( void *pvParameters );

/*-----------------------------------------------------------*/

xQueueHandle xTaskQueue;
xTaskHandle  task_handle = NULL;
xSemaphoreHandle mutex ; 

vector currentTasks;
vector currentTasksId;
/////////////////////////////////////////////////////////////////

int main( void )
{
	printf("starting program ................\n");
	printf("N: %d,\n tst: %d,\n latest_arrival_time: %d,\n maximum_computation_time: %d,\n maximum_period_multipler: %d,\n safe_mode: %d\n",
					N, tst, latest_arrival_time, maximum_computation_time, maximum_period_multipler, safe_mode);
	mutex = xSemaphoreCreateMutex();
	xTaskQueue = xQueueCreate( 5, sizeof( send_task ) );
	vector_init(&currentTasks);
	vector_init(&currentTasksId);
	srand( 567 );

	if( (xTaskQueue != NULL) && ( mutex != NULL ) )
	{

		static_task_admittance();
		xTaskCreate( dynamic_task_admittance, "Print1", 240, 0, 1, NULL );
		xTaskCreate( SchedulerGatekeeperTask, "Gatekeeper", 240, NULL, 0, NULL );

		
		vTaskStartScheduler();
	}
	for( ;; );
}
/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/
static int task_index=0;
static xTask generate_task(){
	xTask temp;
	temp.index = task_index;
	temp.Ta = (rand() % (latest_arrival_time - 0 + 1)) + 0;
	temp.Tc = (rand() % (maximum_computation_time - 1 + 1)) + 1;
	temp.cpu_util = NULL;
	temp.priority = NULL;
	if(safe_mode){
		temp.Tp = (rand() % (maximum_period_multipler - (3*temp.Tc) + 1)) + (3*temp.Tc);
		//tasks[i].Tp += tasks[i].Tp%tst;
	}
	else{
		temp.Tp = (rand() % ((10*temp.Tc) - (3*temp.Tc) + 1)) + (3*temp.Tc);
		//tasks[i].Tp += tasks[i].Tp%tst;
	}
	printf("generating new task:-\n index: %d\t Ta: %lo\t Tc: %lo\t Tp: %lo\t cpu_util: %f\t pri: %d\n", 
					temp.index, temp.Ta, temp.Tc, temp.Tp, temp.cpu_util, temp.priority);
	task_index++;
	return temp;
}	

static void static_task_admittance(){
	int n = (rand() % (N - 2 + 1)) + 2;
	printf("entering static_task_admittance()...........\n generating static random n  ......\n n: %d\n", n);
	for(int i = 0; i < n; i++){
		xTask tempTask =  generate_task();
		send_task temp = { 1 , tempTask};
		xQueueSendToBack( xTaskQueue, &( temp ), 0 );
		printf( "Sending new task to back of queue with opcode: %d............\n", temp.op );
	}
}


static void dynamic_task_admittance( void *pvParameters )
	{
	//int iIndexToString = ( int ) pvParameters;
	printf( "creating dynamic_task_admittance Task...........\n" );
	for( ;; )
	{
		printf("entering dynamic_task_admittance()...........\n" );
		if(rand() % 2 == 0){
			xTask tempTask =  generate_task();
			send_task temp = { 1 , tempTask};
			xQueueSendToBack( xTaskQueue, &( temp ), 0 );
			printf( "Sending new task to back of queue with opcode: %d............\n", temp.op );
		}
		else{
			xTask tempTask =  generate_task();
			int no = rand()% currentTasksId.total;
			tempTask.index = (( unsigned int )vector_get(&currentTasksId,no)); 
			send_task temp = { 2 , tempTask};
			xQueueSendToBack( xTaskQueue, &( temp ), 0 );
			printf( "Sending task deletion order for task %d to back of queue with opcode: %d............\n", tempTask.index, temp.op );
		}
		vTaskDelay( ( rand() & 0x1FF ) );
	}
}
void vApplicationTickHook( void ){
	static int iCount = 0;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Print out a message every 200 ticks.  The message is not written out
	directly, but sent to the gatekeeper task. */
	iCount++;
	if( iCount >= 200 )
	{
		printf("entering vApplicationTickHook() After icount >= 200 ...........\n" );
		send_task temp = { 3 , NULL};
		xQueueSendToBack( xTaskQueue, &( temp ), 0 );
		/* In this case the last parameter (xHigherPriorityTaskWoken) is not
		actually used but must still be supplied. */
		xQueueSendToFrontFromISR( xTaskQueue, &( temp ), &xHigherPriorityTaskWoken );
		printf( "Sending Schedule Refreshing order for schedule to the fornt of queue with opcode: %d............\n",  temp.op );
		iCount = 0;
	}
}
/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/
static void SchedulerGatekeeperTask( void *pvParameters )
	{
	send_task sendTask;
	printf( "creating SchedulerGatekeeperTask...........\n" );
	
	for( ;; )
	{
		printf("entering SchedulerGatekeeperTask()...........\n  Suspending All Tasks.............\n checking opcode from queue.......\n" );
		vTaskSuspendAll();
		/* Wait for a message to arrive. */
		xQueueReceive( xTaskQueue, &sendTask, portMAX_DELAY );
		unsigned int op = sendTask.op;
		xTask newTask = sendTask.new_task;
		if( op == 3){
			printf( "opcode: %d  Schedule Refreshing order ............\n",  sendTask.op );
			//run sch. again while removing finished
			if ( isSchedulable() ){
				set_priority();
			}
		}
		else if ( op == 2){
			//delete
			printf( "opcode: %d  task deletion order ............\n",  sendTask.op );
			vTaskDelete(newTask.xTaskHandler);
			vector_delete(&currentTasks, newTask.index);
			vector_delete(&currentTasksId, newTask.index);
			printf("deleting task:-\n index: %d.", newTask.index);
		}
		else if ( op == 1 ){
			// add new task
			printf( "opcode: %d  new task creation order ............\n",  sendTask.op );
			if ( isSchedulable() ){
				set_priority();
				xTaskHandle xtempTaskHandler;
				newTask.xTaskHandler = xtempTaskHandler;
				vector_add(&currentTasks, &newTask);
				vector_add(&currentTasksId, &newTask.index);
				xTaskCreate( vTaskCreate, "Task", 120, newTask.index, newTask.priority , newTask.xTaskHandler);
				printf("creating new task:-\n index: %d\t Ta: %lo\t Tc: %lo\t Tp: %lo\t cpu_util: %f\t pri: %d\n", 
					newTask.index, newTask.Ta, newTask.Tc, newTask.Tp, newTask.cpu_util, newTask.priority);
			}
			else{ printf("not Scheduable.... \n continue woth prev Tasks\n"); }
		}
		printf( "Done Updating.................\n" );
		printf("leaving SchedulerGatekeeperTask()...........\n  Resuming All Tasks.............\n" );
		xTaskResumeAll();
		/* Now simply go back to wait for the next message. */
	}
}

static int isSchedulable(){
	printf("Checking Schedulability...........\n" );
	cpu_util();
	float Ucpu = 0;
	for (int i = 0; i < currentTasks.total; i++){
			Ucpu += ((xTask*) vector_get(&currentTasks, i))->cpu_util;
	}
	printf("Ucpu: %f\n", Ucpu );
	if(Ucpu < 0.7){
		printf("Scheduable\n" );
		return TRUE;
	}
	return FALSE;
}
static void cpu_util(){
	printf("calc cpu_util for all running tasks ...........\n" );
	for (int i = 0; i < VECTOR_TOTAL(currentTasks); i++){
		xTask* tempTask;
		tempTask = (xTask*) vector_get(&currentTasks, i);
		tempTask->cpu_util =  tempTask->Tc * 1.0 / tempTask->Tp * 1.0;
	}
}
static void swap(int xp, int yp) { 
    int temp = xp; 
    xp = yp; 
    yp = temp; 
} 


static int* sortTask(int* sorted_index){
	int max_idx; 
  
	for(int i = 0; i < currentTasks.total; i++){
		sorted_index[i] = ((xTask*) vector_get(&currentTasks, i))->index;
	}
	for (int i = 0; i < currentTasks.total-1; i++) 
	{
			max_idx = i; 
			for (int j = i+1; j < currentTasks.total; j++) 
				if (((xTask*) vector_get(&currentTasks, j))->Tp > ((xTask*) vector_get(&currentTasks, max_idx))->Tp) 
					max_idx = j; 

			swap( sorted_index[max_idx] , sorted_index[i] ); 
	} 
	return sorted_index;
		
}


static void set_priority(){
	printf( "Setting priority for all tasks\n" );
	int unsorted_id[currentTasks.total];
	int *sorted_id = sortTask(unsorted_id);
	
	for (int i = 0; i < currentTasks.total; i++){
		xTask* tempTask = ((xTask*) vector_get(&currentTasks, sorted_id[i]));
		tempTask->priority = (currentTasks.total - i); 
		printf( "task %d, Tp: %lo, Priority: %d\n", tempTask->index, tempTask->Tp, tempTask->priority);
	}
	printf( "...........................................................\n" );
}


/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/
static void vTaskCreate( void *pvParameters )
	{
	int x = ( int ) pvParameters;
	printf( "Task %d created\n", x);
	for( ;; )
  {
		  if(xSemaphoreTake( mutex, portMAX_DELAY ))
			{
				printf( "Handler task - Processing event %d\n",x);
				vTaskDelay(((xTask*) vector_get(&currentTasks, x))->Tc);
			}
			xSemaphoreGive( mutex );
			vTaskDelay(((xTask*) vector_get(&currentTasks, x))->Tp);
			
	}
}



		

		
/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook( void ){
	/* This function will only be called if an API call to create a task, queue
	or semaphore fails because there is too little heap RAM remaining - and
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. */
	for( ;; );
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName ){
	/* This function will only be called if a task overflows its stack.  Note
	that stack overflow checking does slow down the context switch
	implementation and will only be performed if configCHECK_FOR_STACK_OVERFLOW
	is set to either 1 or 2 in FreeRTOSConfig.h. */
	for( ;; );
}
void vApplicationIdleHook( void ){
	/* This example does not use the idle hook to perform any processing.  The
	idle hook will only be called if configUSE_IDLE_HOOK is set to 1 in 
	FreeRTOSConfig.h. */
}

/*-----------------------------------------------------------*/
