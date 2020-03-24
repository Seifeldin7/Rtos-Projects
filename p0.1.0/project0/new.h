#ifndef NEW_H_
#define NEW_H_

#include "tm4c123gh6pm.h"

void PortF_Init(void);
void PortE_Init(void);


void vApplicationIdleHook(void);

static void vEWTask( void *pvParameters );
static void vNSTask( void *pvParameters );
static void vPDTask( void *pvParameters );
static void vTRTask( void *pvParameters );
static void vControllerTask( void *pvParameters );


#endif