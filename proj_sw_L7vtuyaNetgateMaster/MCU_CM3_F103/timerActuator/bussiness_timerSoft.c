#include "bussiness_timerSoft.h"

#include "datsProcess_uartZigbee.h"

extern u8 dbSysLogLoopCounter;

static void usrAppNormalbussiness_timerCb(void const *arg);

osTimerDef(timerUsrApp_nromalBussiness, usrAppNormalbussiness_timerCb);

static void usrAppNormalbussiness_timerCb(void const *arg){
	
	const u8 period_nodeSystimeSynchronous = 10; //
	static u8 counter_nodeSystimeSynchronous = 0;
	
	const u8 period_rootSystimeReales = 15; //
	static u8 counter_rootSystimeReales = 0;
	
	const u8 period_localSystimeZigbAdjust = 20; //
	static u8 counter_localSystimeZigbAdjust = 0;
	
	{/*秒周期业务*/
		
		const u16 period_1second = 1000;
		static u16 counter_1second = 0;
		
		if(counter_1second < period_1second)counter_1second ++;
		else{

			counter_1second = 0;
			
			if(counter_nodeSystimeSynchronous < period_nodeSystimeSynchronous)counter_nodeSystimeSynchronous ++;
			else{

				counter_nodeSystimeSynchronous = 0;

				zigbFunction_callFromThread(msgFun_nodeSystimeSynchronous);
			}
			
			if(counter_rootSystimeReales < period_rootSystimeReales)counter_rootSystimeReales ++;
			else{
			
				counter_rootSystimeReales = 0;
				
				zigbFunction_callFromThread(msgFun_rootSystimeReales);
			}
			
			if(counter_localSystimeZigbAdjust < period_localSystimeZigbAdjust)counter_localSystimeZigbAdjust ++;
			else{
			
				counter_localSystimeZigbAdjust = 0;
				
				zigbFunction_callFromThread(msgFun_localSystimeZigbAdjust);
			}
			
			if(dbSysLogLoopCounter)dbSysLogLoopCounter --;
		}
	}
}

void usrAppNormalBussiness_softTimerInit(void){

	osTimerId tid_usrAppTimer_normalBussiness = NULL;
	
	tid_usrAppTimer_normalBussiness = osTimerCreate(osTimer(timerUsrApp_nromalBussiness), osTimerPeriodic, NULL);
	osTimerStart(tid_usrAppTimer_normalBussiness, 1UL);
}

