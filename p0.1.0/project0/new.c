#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "stdio.h"
#include "new.h"
#include "config.h"

#define TASK_NO_NS 0
#define TASK_NO_EW 1
#define TASK_NO_PD 2

void BTN_init(void)
{
  SYSCTL_RCGCGPIO_R  |=  (1<<5);
  GPIO_PORTF_LOCK_R = 0x4c4f434b;
  GPIO_PORTF_CR_R = 0x01f;
	GPIO_PORTF_AMSEL_R &= ~0x11;
	GPIO_PORTF_PCTL_R &= ~0x11; 
	GPIO_PORTF_AFSEL_R &= ~0x11;
  GPIO_PORTF_DIR_R   &= ~(1<<4);
  GPIO_PORTF_DIR_R   &= ~(1<<0);
  GPIO_PORTF_PUR_R |= (1<<4) | (1<<0);
  GPIO_PORTF_DEN_R   |=   (1<<0) | (1<<4);
}

/////////////////////////////////////////////////////////////////////////////



typedef struct 
{
	unsigned char taskNo;
	unsigned int taskPeriod;
}xData;


TaskHandle_t  EW_handle = NULL;
TaskHandle_t  NS_handle = NULL;
TaskHandle_t  PD_handle = NULL;
TaskHandle_t  TR_handle = NULL;
TaskHandle_t  CR_handle = NULL;
xQueueHandle xQueue = NULL;

int main(void){   
	xQueue = xQueueCreate(2,sizeof(xData));	
  PortF_Init();     
	PortE_Init();
	BTN_init();
	xTaskCreate( vNSTask, (const portCHAR *)"North South", configMINIMAL_STACK_SIZE, NULL, 2, &NS_handle );
	xTaskCreate( vEWTask, (const portCHAR *)"East West", configMINIMAL_STACK_SIZE, NULL, 2, &EW_handle );
	xTaskCreate( vPDTask, (const portCHAR *)"Pedestrian", configMINIMAL_STACK_SIZE, NULL, 3, &PD_handle );
	xTaskCreate( vTRTask, (const portCHAR *)"Train", configMINIMAL_STACK_SIZE, NULL, 4, &TR_handle );
	xTaskCreate( vControllerTask, (const portCHAR *)"Controller", configMINIMAL_STACK_SIZE, NULL, 1, &CR_handle );
		/* Start the scheduler. */
	vTaskStartScheduler();
}
static void vControllerTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	xData taskData;
	for( ;; )
	{
		xQueueReceive(xQueue,&taskData,2);
		if(taskData.taskNo == TASK_NO_NS)
		{
			BOARD_LED = BOARD_LED_BLUE;
			TRAFFIC_LIGHT = TRAFFIC_GREEN_NS | TRAFFIC_RED_EW;
			vTaskDelay(taskData.taskPeriod);
		}
		else if(taskData.taskNo == TASK_NO_EW)
		{
			BOARD_LED = BOARD_LED_RED;
			TRAFFIC_LIGHT = TRAFFIC_RED_NS | TRAFFIC_GREEN_EW;
			vTaskDelay(taskData.taskPeriod);
		}
    else if(taskData.taskNo == TASK_NO_PD)
		{
			BOARD_LED = BOARD_LED_GREEN;
			TRAFFIC_LIGHT = TRAFFIC_RED_NS | TRAFFIC_RED_EW;     
			vTaskDelay(taskData.taskPeriod);
		}
	
	}
}


/* ............................................................*/

static void vEWTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	xData dataToSend = { TASK_NO_EW , 2500 };
	
	for( ;; )
	{
		xQueueSendToBack(xQueue,&dataToSend,2);	
		vTaskPrioritySet(NULL,2);
		taskYIELD();
	}
}


static void vNSTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	xData dataToSend = { TASK_NO_NS , 5000 };
	
	for( ;; )
	{
		xQueueSendToBack(xQueue,&dataToSend,2);
		vTaskPrioritySet(NULL,2);
		taskYIELD();
	}
}

static void vPDTask( void *pvParameters ){
	volatile unsigned long int pd_sw_value = 0;
	xData taskData;
	xData dataToSend = { TASK_NO_PD , 10000 };
	unsigned char previous_pd_sw_value = 0x01 ;
	for( ;; )
	{
		pd_sw_value = PEDSTARIAN_SW;
		if(!pd_sw_value && previous_pd_sw_value){
			xQueueReceive(xQueue,&taskData,2);
			xQueueReset(xQueue );
			xQueueSendToBack(xQueue,&dataToSend,2);
			xQueueSendToBack(xQueue,&taskData,2);
		}
		else{ vTaskDelay(10); }
		previous_pd_sw_value = pd_sw_value;
		vTaskPrioritySet(NULL,2);
		taskYIELD();
	}
}


static void vTRTask( void *pvParameters ) {
	for( ;; )
	{
		if( !RTL_TRAIN_SW ){
			vTaskSuspend(CR_handle);
			for(int i = 0; i < 5; i++){
				BOARD_LED = BOARD_LED_WHITE;
				TRAFFIC_LIGHT = TRAFFIC_RED_EW | TRAFFIC_RED_NS; 
				vTaskDelay(1000);
				BOARD_LED = BOARD_LED_OFF;
				TRAFFIC_LIGHT = TRAFFIC_OFF; 
				vTaskDelay(1000);
			}
			while(!LTR_TRAIN_SW){}
				vTaskDelay(200);
			vTaskResume(CR_handle);
		}
		else if( LTR_TRAIN_SW ){
			vTaskSuspend(CR_handle);
			for(int i = 0; i < 5; i++){
				BOARD_LED = BOARD_LED_RED;
				TRAFFIC_LIGHT = TRAFFIC_RED_EW | TRAFFIC_RED_NS; 
				vTaskDelay(1000);
				BOARD_LED = BOARD_LED_OFF;
				TRAFFIC_LIGHT = TRAFFIC_OFF; 
				vTaskDelay(1000);
			}
			while( RTL_TRAIN_SW ){}
				vTaskDelay(200);
			vTaskResume(CR_handle);
		}
		else{ vTaskDelay(10); }
		vTaskPrioritySet(NULL,2);
		taskYIELD();
	}
	
}

/*-----------------------------------------------------------*/


void PortF_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x00000020; // activate clock for port F
  GPIO_PORTF_DIR_R |= 0x0E;  
  GPIO_PORTF_DEN_R |= 0x0E;     // enable digital I/O on PF3-PF1 
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
  GPIO_PORTF_DEN_R = 0x0E;          // 7) enable digital pins PF3-PF1        
}

// port 0 : red   NS
// port 1 : green NS
// port 2 : red   EW
// port 3 : green EW
void PortE_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x00000010; // activate clock for port E
  GPIO_PORTE_DIR_R |= 0x0F;  
  GPIO_PORTE_DEN_R |= 0x1F;     // enable digital I/O on PE4-PE0 
	GPIO_PORTE_DIR_R &= ~(1<<4);
}






void vApplicationIdleHook(void){
}

