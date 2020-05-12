#ifndef MAIN_H_
#define MAIN_H_

#include "bool.h"

//#define dynamic

typedef struct 
{
	unsigned long Ta;
	unsigned long Tc;
	unsigned long Tp;
	unsigned int priority;
}xTask;

static void initializeRandomVariablesint (int *n, xTask task[]);
static bool checkSchedulablity(int n, xTask tasks[]);
static void initializeDataStructure(void);
static void createTasks(int n,xTask tasks[]);
static void sortTasks(int n, xTask tasks[]);

static void vTask( int *pvParameters );



#endif /* MAIN_H_ */