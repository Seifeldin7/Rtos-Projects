#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
//#include "timers.h"
#include "queue.h"
#include <stdlib.h>
#include "main.h"
#include <stdio.h> 
#include "semphr.h"

# define N  5
# define tst  1
# define latest_arrival_time 15
# define maximum_computation_time 8
# define maximum_period_multipler  17
# define safe_mode TRUE

xSemaphoreHandle mutex ; 



#ifdef dynamic
	void createList()
	{
		xTask * task1 = (xTask *) malloc(sizeof(xTask));		
	}
#else
	xTask tasks[50];
#endif

//int addr = 0;
xTaskHandle  task_handle = NULL;

//xQueueHandle xQueue = NULL;


void vApplicationIdleHook(void){}


static void initializeRandomVariables(int *n, xTask tasks[])
{
	int i;
	//*n = (rand() % (2*N - 2 + 1)) + 2;
	*n=5;
	tasks[0].Tp=11;
	tasks[1].Tp=20;
	tasks[2].Tp=32;
	tasks[3].Tp=27;
	tasks[4].Tp=13;
	for(i = 0; i<*n; i++) {
		tasks[i].Ta = rand()%latest_arrival_time;
		tasks[i].Tc = (rand() % (2*N)) + 1;
		/*if(safe_mode){
			tasks[i].Tp = (rand() % (maximum_period_multipler*tasks[i].Tc - 3*tasks[i].Tc + 1)) + 3*tasks[i].Tc;
			tasks[i].Tp += tasks[i].Tp%tst;
		}else{
			tasks[i].Tp = (rand() % (10*tasks[i].Tc - 3*tasks[i].Tc + 1)) + 3*tasks[i].Tc;
			tasks[i].Tp += tasks[i].Tp%tst;
		}*/
	}
         
}
static bool checkSchedulablity(int n, xTask tasks[])
{
	int i;
	float Ucpu=0;
	for(i = 0; i<n; i++){
		Ucpu+=(float)tasks[i].Tc/tasks[i].Tp;
	}
	if(Ucpu < 0.7)
		return TRUE;
	return FALSE;
	
}
static void initializeDataStructure(void)
{
	
}
int task_sent[100] = {0,1,2,3,4};
static void createTasks(int n, xTask tasks[])
{
	int i;
	for(i = 0; i < n; i++)
	{
		xTaskCreate( vTask, "Task", 120, &task_sent[i], tasks[i].priority, NULL );
	}
}

static void sortList(void)
{
	
}

void swap(xTask *xp, xTask *yp) 
{ 
    xTask temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 

static void sortArr(int n, xTask tasks[])
{
	int i, j, max_idx; 
  
    for (i = 0; i < n-1; i++) 
    { 
        max_idx = i; 
        for (j = i+1; j < n; j++) 
          if (tasks[j].Tp > tasks[max_idx].Tp) 
            max_idx = j; 
  
        swap(&tasks[max_idx], &tasks[i]); 
    } 
}
static void sortTasks(int n, xTask tasks[])
{
	#ifdef dynamic
	sortList();
	#else
	sortArr(n,tasks);
	#endif
}


static void vTask( int *pvParameters )
{
	int x = *pvParameters;
	vPrintStringAndNumber( "Task created", x);
	for( ;; )
  {
		  if(xSemaphoreTake(mutex, 10000))
			{
				vPrintStringAndNumber( "Handler task - Processing event",x);
				vTaskDelay(tasks[x].Tc);
			}
			xSemaphoreGive(mutex);
			vTaskDelay(tasks[x].Tp);
			
	}
}


int main( void )
{
				int n,i;
				bool schedulable = FALSE;
				mutex = xSemaphoreCreateMutex();
				/* Initialize random variables N, tst,...etc */ 
				initializeRandomVariables(&n, tasks);
				vPrintStringAndNumber( "n= ", n);
				for(i=0;i<n;i++)
				{
					vPrintStringAndNumber( "Tp", tasks[i].Tp);
				}
				createTasks(n, tasks);
				vTaskStartScheduler();
				/* Initialize array(static) or list(dynamic) with tasks and add Tp, Tc,Ta to each task */
		
				//initializeDataStructure(&n, arrival_time, computation_time, period_multipler);
				/* Calculate total CPU utilization, and perform a Schedulablity test */ 
				schedulable = checkSchedulablity(n, tasks);
				if(schedulable)
				{
					/* Sort the tasks based on their periods and assign their priorities */
					sortTasks(n, tasks);
					/* Loop over all tasks in the DS and create them */
					createTasks(n, tasks);
					/* Start the scheduler so the created tasks start executing. */
					vTaskStartScheduler();
				}
				else
				{
					vPrintString( "Tasks are not schedulable");
				}
    
				for( ;; );
}
/*-----------------------------------------------------------*/