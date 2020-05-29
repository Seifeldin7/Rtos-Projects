#include <stdio.h>
#include <stdlib.h>
#include "RTE_Components.h"             // Component selection
#include CMSIS_device_header

#include "FreeRTOS.h"                   // Keil::RTOS:FreeRTOS:Core
#include "task.h"                       // Keil::RTOS:FreeRTOS:Core
#include "basic_io.h"
#include "semphr.h"
#include "bool.h"
#include "vector.h"

/* Dimensions the buffer into which messages destined for stdout are placed. */
#define mainMAX_MSG_LEN	( 80 )
# define N  5
# define tst  5
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
	TaskHandle_t  xTaskHandler;
}xTask;
typedef struct 
{
	unsigned int op;
	unsigned int new_task;
}send_task;

vector currentTasks;
vector currentTasksId;
//TaskHandle_t tid_phaseA;                /* Thread id of thread: phase_a      */
xQueueHandle xTaskQueue;
xSemaphoreHandle mutex ; 


static xTask* generate_task(void);
static void static_task_admittance(void);
static void dynamic_task_admittance( void *pvParameters );

static void SchedulerGatekeeperTask( void *pvParameters );
static int isSchedulable(float addedUcpu);
static void swap(int xp, int yp);
static void sortTask(int arr[], int n);
static void set_priority(void);

static void vTaskCreate( void *pvParameters );
void app_main (void *argument);


/*----------------------------------------------------------------------------
 *      Initialize and start threads
 *---------------------------------------------------------------------------*/

void app_main (void *argument) {
	vTaskSuspendAll();
	xTaskCreate( dynamic_task_admittance, "Print1", 240, ( void * ) 0, 0, NULL );
	xTaskCreate( SchedulerGatekeeperTask, "Gatekeeper", 240, NULL, 0, NULL );
	xTaskResumeAll();
  vTaskDelay (portMAX_DELAY);
  while(1);
}

int main (void) {
	printf("starting program ................\n");
	printf("N: %d,\n tst: %d,\n latest_arrival_time: %d,\n maximum_computation_time: %d,\n maximum_period_multipler: %d,\n safe_mode: %d\n",
					N, tst, latest_arrival_time, maximum_computation_time, maximum_period_multipler, safe_mode);
	xTaskQueue = xQueueCreate( 5, sizeof( send_task ) );
	mutex = xSemaphoreCreateMutex();
	vector_init(&currentTasks);
	vector_init(&currentTasksId);
	if( xTaskQueue != NULL  && mutex != NULL)
	{

		srand( 567 );
		SystemCoreClockUpdate();
		//EvrFreeRTOSSetup(0);
		static_task_admittance();
		xTaskCreate (app_main, "app_main", 64, NULL, tskIDLE_PRIORITY+1, NULL);

		vTaskStartScheduler();
	}
  while(1);
}


static int task_index=0;
static xTask* generate_task()
	{
	xTask* temp = malloc(sizeof(xTask));
	temp->index = task_index;
	temp->Ta = (rand() % (latest_arrival_time - 0 + 1)) + 0;
	temp->Tc = (rand() % (maximum_computation_time - 1 + 1)) + 1;
	temp->priority = 2;
	TaskHandle_t  xtempTaskHandler = NULL;
	temp->xTaskHandler = xtempTaskHandler;
	if(safe_mode){
		temp->Tp = (rand() % ((maximum_period_multipler * temp->Tc) - (3*temp->Tc) + 1)) + (3*temp->Tc);
		//tasks[i].Tp += tasks[i].Tp%tst;
	}
	else{
		temp->Tp = (rand() % ((10*temp->Tc) - (3*temp->Tc) + 1)) + (3*temp->Tc);
		//tasks[i].Tp += tasks[i].Tp%tst;
	}
	temp->cpu_util =  temp->Tc * 1.0 / temp->Tp * 1.0;
	printf("generating new task:-\n index: %d\t Ta: %lu\t Tc: %lu\t Tp: %lu\t cpu_util: %f\t pri: %d\n", 
					temp->index, temp->Ta, temp->Tc, temp->Tp, temp->cpu_util, temp->priority);
	task_index++;
	return temp;
}	


static void static_task_admittance()
{
	printf("\n_________________________________________________________________________________________________\n");
	int n = (rand() % (N - 2 + 1)) + 2;
	printf("entering static_task_admittance()...........\n generating static random n  ......\n n: %d\n", n);
	for(int i = 0; i < n; i++){
		xTask* tempTask =  generate_task();
		if(isSchedulable(tempTask->cpu_util)){
			VECTOR_ADD(currentTasks, tempTask);
		//VECTOR_ADD(currentTasksId, tempTask->index);
			send_task temp; 
			temp.op=1,temp.new_task = tempTask->index;
			xQueueSendToBack( xTaskQueue, &temp, 0  );
			printf( "Sending new task to back of queue with opcode: %d............\n", 1 );
			}else{ printf("Respond: failed\n");}
			printf("-----------------------------------------------------------------------------------------------\n");
	}
}


static void dynamic_task_admittance( void *pvParameters )
{
	int j=0,k=0;
	for( ;; )
	{
		j++;
		if(j>100){
			printf("\n_________________________________________________________________________________________________\n");
			printf("entering dynamic_task_admittance()...........\n\n");
			if(k % 3 != 0){
				printf( "Trying to create a new task...........\n" );
				xTask* tempTask =  generate_task();
				if(isSchedulable(tempTask->cpu_util)){
					VECTOR_ADD(currentTasks, tempTask);
					send_task temp; 
					temp.op=1,temp.new_task = VECTOR_TOTAL(currentTasks)-1;
					xQueueSendToBack( xTaskQueue, &temp , 0 );
					printf( "Sending new task to back of queue with opcode: %d............\n", 1 );
				}else{ printf("Respond: failed\n");}
			}
			else{
				printf( "Trying to delete an old task...........\n" );
				int no = rand() % VECTOR_TOTAL(currentTasks);
				if( isSchedulable( (( VECTOR_GET(currentTasks, xTask*, no) )->cpu_util) * -1) ){
					send_task temp; 
					temp.op=2,temp.new_task = no;
					xQueueSendToBack( xTaskQueue, &temp , 0 );
					printf( "Sending new task to back of queue with opcode: %d............\n", 2 );
				}else{ printf("Respond: failed\n");}
			}
			j=0,k++;
		}
		vTaskDelay( ( rand() & 0x1FF ) );	
	}
}




//_________________________________________________________________________//

static void SchedulerGatekeeperTask( void *pvParameters )
{
	send_task sendTask;
	for( ;; )
	{
		 if(xSemaphoreTake( mutex, portMAX_DELAY ))
		 {
			 printf("\n_________________________________________________________________________________________________\n");
			 printf("entering SchedulerGatekeeperTask()...........\n  Suspending All Tasks.............\n checking opcode from queue.......\n" );
				xQueueReceive( xTaskQueue, &sendTask, portMAX_DELAY );
				if ( sendTask.op == 2){
					//delete
					printf( "opcode: %d  task deletion order ............\n",  sendTask.op );
					vTaskDelete( ( (xTask*) vector_get(&currentTasks, sendTask.new_task) )->xTaskHandler );
					VECTOR_DELETE(currentTasks, sendTask.new_task);
					printf("deleting task:-\n index: %d.\n", sendTask.new_task);
					
				}
				else if ( sendTask.op == 1 ){
					// add new task
					printf( "opcode: %d  new task creation order ............\n",  sendTask.op);
					xTaskCreate( vTaskCreate, "Task", 120,
											( void * )sendTask.new_task,
												( VECTOR_GET(currentTasks, xTask*, sendTask.new_task) )->priority,
												&( (xTask*) vector_get(&currentTasks, sendTask.new_task) )->xTaskHandler);
				}
				printf(" currently tasks handled == %d\n",((int) uxTaskGetNumberOfTasks()) - 4);
				printf("---------------------------------------------------------------------------------------------\n");
				for (int i = 0; i < VECTOR_TOTAL(currentTasks); i++){
					xTask* temp = (xTask*) vector_get(&currentTasks, i);
					printf("entering task  %d::::: task:-\n index: %d\t Ta: %lu\t Tc: %lu\t Tp: %lu\t cpu_util: %f\t pri: %d\n", 
								i,temp->index, temp->Ta, temp->Tc, temp->Tp, temp->cpu_util, temp->priority);
				}
				printf("-------------------------------------------------------------------------------------------------\n");
				set_priority();
		  	printf("_________________________________________________________________________________________________\n");
		 }
		 xSemaphoreGive( mutex );
	}
}


float Ucpu = 0;
static int isSchedulable(float addedUcpu){
	printf("Checking Schedulability...........\n" );
	printf(" Old Ucpu: %f\n", Ucpu );
	Ucpu += addedUcpu;
	printf(" New Ucpu: %f\n", Ucpu );
	if(Ucpu < 0.7){
		printf("Scheduable\n" );
		return TRUE;
	}
	printf("Not Scheduable\n" );
	Ucpu -= addedUcpu;
	return FALSE;
}

static void swap(int xp, int yp) { 
    int temp = xp; 
    xp = yp; 
    yp = temp; 
}
static void sortTask(int arr[], int n){
	int i, key, j; 
    for (i = 1; i < n; i++) { 
        key = arr[i]; 
        j = i - 1; 
        while (j >= 0 && ( ( (xTask*) vector_get(&currentTasks, arr[j]) )->Tp ) > ( ( (xTask*) vector_get(&currentTasks, key) )->Tp )) { 
            arr[j + 1] = arr[j]; 
            j = j - 1; 
        } 
        arr[j + 1] = key; 
    }  	
}
static void set_priority(){
	printf( "Setting priority for all tasks\n" );
	int sorted_id[currentTasks.total];
	for(int i = 0; i < currentTasks.total; i++){
		sorted_id[i] = i;
	}	
	int n = sizeof(sorted_id) / sizeof(sorted_id[0]); 
	sortTask(sorted_id,n);
	for (int i = 0; i < currentTasks.total; i++){
		xTask* tempTask = ((xTask*) vector_get(&currentTasks, sorted_id[i]));
		tempTask->priority = currentTasks.total - i +1;
		vTaskPrioritySet( tempTask->xTaskHandler , currentTasks.total - i +1);
		printf( "task %d, Tp: %lu, Priority: %d\n", tempTask->index, tempTask->Tp, tempTask->priority);
	}
}
//_________________________________________________________________________//

static void vTaskCreate( void *pvParameters )
{
	int sendTask = ( int ) pvParameters;
	int index = ( VECTOR_GET(currentTasks, xTask*, sendTask) )->index;
	unsigned long TC = ( VECTOR_GET(currentTasks, xTask*, sendTask) )->Tc;
	unsigned long TP = ( VECTOR_GET(currentTasks, xTask*, sendTask) )->Tp;
	printf( "Task %d created\n", index);
	for( ;; )
  {
			{
				printf( "%d:Handler task - Processing event %d\t %lu \t %lu\t priority: %lu\n",sendTask,index,TC,TP, uxTaskPriorityGet( NULL ));
				//printf( "Handler task - Processing event %d\n",index);
				vTaskDelay( TC * tst );
			}

			vTaskDelay( TP * tst );
			
	}
}
	
//_________________________________________________________________________//



