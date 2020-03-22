#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
void PortF_Init(void);

static void vEWTask( void *pvParameters );
static void vNSTask( void *pvParameters );
static void vTRTask( void *pvParameters );
static void vPDTask( void *pvParameters );


TaskHandle_t  first_handle = NULL;
TaskHandle_t  second_handle = NULL;
TaskHandle_t  third_handle = NULL;
TaskHandle_t  fourth_handle = NULL;

unsigned char pd_flag = 0;
unsigned char tr_flag = 0;
xQueueHandle xQueue = NULL;

void Btn_Interrupt_Init(void){                          
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  GPIO_PORTF_DIR_R &= ~0x10;    // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;  //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F0000; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;     //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
  __asm("CPSIE I\n");           // (i) Clears the I bit
}
void GPIOF_Handler(void){
  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag4


  pd_flag = 1;
}

void vApplicationIdleHook(void){
}


int main(void){   
	xQueue = xQueueCreate(1,sizeof(int));
  PortF_Init();  	// Call initialization of port PF3, PF2, PF1    
	Btn_Interrupt_Init();
	xTaskCreate( vEWTask, (const portCHAR *)"East West", configMINIMAL_STACK_SIZE, NULL, 2, &first_handle );
	xTaskCreate( vNSTask, (const portCHAR *)"North South", configMINIMAL_STACK_SIZE, NULL, 3, &second_handle );
	xTaskCreate( vTRTask, (const portCHAR *)"North South", configMINIMAL_STACK_SIZE, NULL, 1, &third_handle );
	xTaskCreate( vPDTask, (const portCHAR *)"North South", configMINIMAL_STACK_SIZE, NULL, 4, &fourth_handle );
		/* Start the scheduler. */
	vTaskStartScheduler();
}

static void vEWTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	int current_task = 1;
	for( ;; )
	{

    GPIO_PORTF_DATA_R = 0x04;       // LED is blue
		vTaskSuspend(second_handle);
		xQueueReset(xQueue);
		xQueueSendToBack(xQueue,&current_task,0);
		vTaskDelay(2500);
		if(pd_flag)
		{
			vTaskResume(fourth_handle);
			vTaskDelay(10000);
			pd_flag = 0;
		}
		vTaskResume(second_handle);
		vTaskPrioritySet(second_handle,3);
		
	}
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

static void vNSTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	int current_task = 0;
	for( ;; )
	{
		//Turn red LED on
    GPIO_PORTF_DATA_R = 0x02;     // LED is red
		//Suspend first task to keep the red light
		vTaskSuspend(first_handle);
		xQueueReset(xQueue);
		xQueueSendToBack(xQueue,&current_task,0);
		//delay for 5 seconds (as if the higher priority road is running)
		vTaskDelay(5000);
		//resume the first task and decrease the curret task priority to allow the first task to run
		if(pd_flag)
		{
			//GPIO_PORTF_DATA_R = 0x08;
			vTaskResume(fourth_handle);
			vTaskDelay(10000);
			pd_flag = 0;
		}
		vTaskResume(first_handle);
		vTaskPrioritySet(NULL,1);
	}
}
/*-----------------------------------------------------------*/
static void vPDTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	int current_task = 0;
	for( ;; )
	{
		//Turn red LED on
    GPIO_PORTF_DATA_R = 0x08;     // LED is red
		vTaskSuspend(NULL);
	}
}
static void vTRTask( void *pvParameters )
{

	/* Continuously perform a calculation.  If the calculation result is ever
	incorrect turn the LED on. */
	int current_task = 0;
	for( ;; )
	{
		if(!(GPIO_PORTF_DATA_R & (1<<0)))
		{
		xQueueReceive(xQueue,&current_task,2);
		vTaskSuspend(first_handle);
		vTaskSuspend(second_handle);
    GPIO_PORTF_DATA_R = 0xFF; 
		
		vTaskDelay(5000);
		if(current_task == 0)
		{
			 GPIO_PORTF_DATA_R = 0x02;
			vTaskResume(second_handle);
		}
		else
		{
			 GPIO_PORTF_DATA_R = 0x04;
			vTaskResume(first_handle);
		}
		
		}
		
	}
}


void PortF_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x00000020; // activate clock for port F
  GPIO_PORTF_DIR_R |= 0x0E;  
  GPIO_PORTF_DEN_R |= 0x0E;     // enable digital I/O on PF3-PF1 
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
  GPIO_PORTF_DEN_R = 0x0E;          // 7) enable digital pins PF3-PF1        
	
	GPIO_PORTF_LOCK_R = 0x4c4f434b;
  GPIO_PORTF_CR_R = 0x01f;
	GPIO_PORTF_AMSEL_R &= ~0x11;
	GPIO_PORTF_PCTL_R &= ~0x11; 
	GPIO_PORTF_AFSEL_R &= ~0x11;
  GPIO_PORTF_DIR_R   &= ~(1<<0);
  GPIO_PORTF_PUR_R |=  (1<<0);
  GPIO_PORTF_DEN_R   |=   (1<<0) ;
}
