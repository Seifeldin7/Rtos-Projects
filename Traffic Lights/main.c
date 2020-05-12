  
#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

void initTask(void *);
void BTN1_CHK_TASK(void *);
void BTN2_CHK_TASK(void *);
void UART_TASK(void *);

int counter = 0;

xQueueHandle xQueue;

void vApplicationIdleHook(void){}
	
int main(){
	
	xQueue = xQueueCreate(1, sizeof( int ));
	if( xQueue != NULL ){
		xTaskCreate(BTN1_CHK_TASK, "Btn1", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
		xTaskCreate(BTN2_CHK_TASK, "Btn2", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
		xTaskCreate(UART_TASK, "UART", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
		xTaskCreate(initTask, "Init", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
	}
	
	vTaskStartScheduler();
	for(;;);
	return 0;
}


void initTask(void *pvParameters){

for( ; ; ){
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	
	
GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);

GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0);
GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	
HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK) = GPIO_LOCK_KEY;
HWREG(GPIO_PORTF_BASE+GPIO_O_CR) |= 0x01;

GPIOPadConfigSet(GPIO_PORTF_BASE , GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

GPIOPadConfigSet(GPIO_PORTF_BASE , GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	

UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
UART_CONFIG_PAR_NONE));

vTaskSuspend(NULL);

	}
}

void BTN1_CHK_TASK(void *pvParameters){

for( ; ; ){
if((GPIO_PORTF_DATA_R & GPIO_PIN_4) != GPIO_PIN_4){
	counter++;
		}
	}
	
}

void BTN2_CHK_TASK(void *pvParameters){

for( ; ; ){
if((GPIO_PORTF_DATA_R & GPIO_PIN_0) != GPIO_PIN_0){
	xQueueSendToBack( xQueue, &counter, 0);
	counter = 0;
		}
	}	
}

void UART_TASK(void *pvParameters){

portBASE_TYPE xStatus;
int receivedValue;
const portTickType xTicksToWait = 100 / portTICK_RATE_MS;
char c;
	
for( ; ; ){

xStatus = xQueueReceive(xQueue, &receivedValue, xTicksToWait);
if(xStatus == pdPASS){
		c = (char)(receivedValue);
		UARTCharPut(UART0_BASE, c);
		}

	}

}