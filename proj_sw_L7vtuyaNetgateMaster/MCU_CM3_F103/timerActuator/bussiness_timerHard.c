#include "bussiness_timerHard.h"

#include "stm32f10x_tim.h"

#include "UART_dataTransfer.h"
#include "bspDrv_iptDevice.h"

extern uint8_t zigbee_currentPanID_reslesCounter;

void drvApplication_hwTimer_initialization(uint16_t arr, uint16_t psc){
	
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); 			

	TIM_TimeBaseStructure.TIM_Period = arr;		 				
	TIM_TimeBaseStructure.TIM_Prescaler = psc; 						
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; 		
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  	
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); 			
 
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);		

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn; 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
	NVIC_Init(&NVIC_InitStructure); 
	
	TIM_Cmd(TIM3, ENABLE);  									
}

void TIM3_IRQHandler(void){

	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
	
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		
		{ //1ms定时专用

			const u16 period_1ms = 20;
			static u16 counter_1ms = 0;

			if(counter_1ms < period_1ms)counter_1ms ++;
			else{

				counter_1ms = 0;
				
				//触摸按键计时逻辑业务
				if(touchPadActCounter)touchPadActCounter --; //
				if(touchPadContinueCnt)touchPadContinueCnt --; //
				
				//轻触按键计时逻辑业务
				if(usrKeyCount_EN)usrKeyCount ++;
				else usrKeyCount = 0;
				
				if(combinationFunFLG_3S5S_cancel_counter)combinationFunFLG_3S5S_cancel_counter --;
			}
		}
		
		{ //1s定时专用

			const u16 period_1second = 20000;
			static u16 counter_1second = 0;

			if(counter_1second < period_1second)counter_1second ++;
			else{

				counter_1second = 0;
				
				if(zigbee_currentPanID_reslesCounter)zigbee_currentPanID_reslesCounter --;
				
				/*UTC时间本地保持计数*/
				sysTimeKeep_counter ++;
				
				/*smartconfig开启时间倒计时*/
				if(smartconfigOpen_flg){
				
					if(timeCounter_smartConfig_start)timeCounter_smartConfig_start --; 
					else{

						
					}
				}
			}
		}
	}	
}

void bsp_hwTimerInit(void){

	drvApplication_hwTimer_initialization(50 - 1, 72 - 1);
	
	{
		memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
		snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "hwTim3 init.\n");
		osDelay(20);
		DbugP1TX(printfLog_bufTemp, strlen((char *)printfLog_bufTemp));	
	}
}

