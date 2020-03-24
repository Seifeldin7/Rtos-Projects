#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "stdio.h"
#include "new.h"

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
		if(taskData.taskNo == 0)
		{
			// NS
			GPIO_PORTF_DATA_R = 0x04;       // LED is blue
			GPIO_PORTE_DATA_R = (1<<1) | (1<<2);       
			vTaskDelay(taskData.taskPeriod);
		}
		else if(taskData.taskNo == 1)
		{
			// EW
			GPIO_PORTF_DATA_R = 0x02;       // LED is red
			GPIO_PORTE_DATA_R = (1<<0) | (1<<3);       
			vTaskDelay(taskData.taskPeriod);
		}
    else if(taskData.taskNo == 2)
		{
			//PD
			GPIO_PORTF_DATA_R = 0x08;       
			GPIO_PORTE_DATA_R = (1<<0) | (1<<2);     
			vTaskDelay(taskData.taskPeriod);
		}
	
	}
}


/* ............................................................*/

static void vEWTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	xData dataToSend = { 1 , 2500 };
	
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
	xData dataToSend = { 0 , 5000 };
	
	for( ;; )
	{
		xQueueSendToBack(xQueue,&dataToSend,2);
		vTaskPrioritySet(NULL,2);
		taskYIELD();
	}
}

volatile unsigned long int data = 0;
static void vPDTask( void *pvParameters ){
	xData taskData;
	xData dataToSend = { 2 , 10000 };
	unsigned char flag_low = 0x01 ;
	for( ;; )
	{
		//input to predstrian port B 0
		data = GPIO_PORTF_DATA_R & 0x01;
		if((!(data)) && (flag_low == 0x01)){
			xQueueReceive(xQueue,&taskData,2);
			xQueueReset(xQueue );
			xQueueSendToBack(xQueue,&dataToSend,2);
			xQueueSendToBack(xQueue,&taskData,2);
		}
		else{ vTaskDelay(10); }
		flag_low = data;
		vTaskPrioritySet(NULL,2);
		taskYIELD();
	}
}

volatile unsigned long int data1 = 0;
volatile unsigned long int data2 = 0;

static void vTRTask( void *pvParameters ) {
	unsigned char flag_low1 = 0x10 ;	
	unsigned char flag_low2 = 0x10 ;
	for( ;; )
	{
		data1 = GPIO_PORTF_DATA_R & 0x10;
		data2 = (GPIO_PORTE_DATA_R & (1<<4));
		if((!(GPIO_PORTF_DATA_R & 0x10)) ){
			vTaskSuspend(CR_handle);
			for(int i = 0; i < 5; i++){
				GPIO_PORTF_DATA_R = 0xFF;
				GPIO_PORTE_DATA_R = (1<<0) | (1<<2); 
				vTaskDelay(1000);
				GPIO_PORTF_DATA_R = 0x00;
				GPIO_PORTE_DATA_R = 0x00; 
				vTaskDelay(1000);
			}
			while(!(GPIO_PORTE_DATA_R & (1<<4))){}
				vTaskDelay(200);
			vTaskResume(CR_handle);
		}
		else if(((GPIO_PORTE_DATA_R & (1<<4))) ){
			vTaskSuspend(CR_handle);
			for(int i = 0; i < 5; i++){
				GPIO_PORTF_DATA_R = 0x04;
				GPIO_PORTE_DATA_R = (1<<0) | (1<<2); 
				vTaskDelay(1000);
				GPIO_PORTF_DATA_R = 0x00;
				GPIO_PORTE_DATA_R = 0x00; 
				vTaskDelay(1000);
			}
			while((GPIO_PORTF_DATA_R & (1<<4))){}
				vTaskDelay(200);
			vTaskResume(CR_handle);
		}
		else{ vTaskDelay(10); }
		flag_low1 = data1;
		flag_low2 = data2;
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

