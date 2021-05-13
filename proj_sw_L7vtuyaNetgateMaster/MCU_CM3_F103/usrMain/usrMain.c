/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include "delay.h"

#include "UART_dataTransfer.h"
#include "bspDrv_iptDevice.h"
#include "bussiness_timerHard.h"
#include "bussiness_timerSoft.h"
#include "timer_Activing.h"

/*
 * main: initialize and start the system
 */
int main (void) {
	
  osKernelInitialize ();                    // initialize CMSIS-RTOS

  // initialize peripherals here

  // create 'thread' functions that start executing,
  // example: tid_name = osThreadCreate (osThread(name), NULL);
	
	bsp_delayinit();
	bsp_dataManageInit();
	
	osMemoryInit();
	msgQueueInit();
	
	communicationActive(comObj_DbugP1, ftOBJ_DEBUG);
	communicationActive(comObj_DataTransP1, ftOBJ_ZIGB);
	communicationActive(comObj_DataTransP2, ftOBJ_WIFI);
	
	bsp_hwTimerInit();
	usrAppNormalBussiness_softTimerInit();
	usrAppTimActiving_processInit();
	devDrvIpt_threadProcessActive();

  osKernelStart ();                         // start thread execution 
}
