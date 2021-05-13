#include "UART_dataTransfer.h"

#include "datsProcess_uartZigbee.h"
#include "datsProcess_uartWifi.h"

osThreadId tid_com1DbugP1_Thread;
osThreadId tid_com2DataTransP1_Thread;
osThreadId tid_com5DataTransP2_Thread;

osThreadDef(com1DbugP1_Thread,osPriorityNormal,	1,	512);
osThreadDef(com2DataTransP1_Thread,osPriorityNormal,	1,	1024 * 8);
osThreadDef(com5DataTransP2_Thread,osPriorityNormal,	1,	1024 * 2);

osPoolId  (dttAct_poolAttr_id); // Memory pool ID
osPoolDef (dtThreadActInitParm_pool, 5, type_ftOBJ); // Declare memory pool	//数据传输进程初始化信息 数据结构池

osPoolId  (threadDP_poolAttr_id); 
osPoolDef (threadDatsPass_pool, 30, stt_threadDatsPass); // Declare memory pool	//数据传输进程间消息传递 数据结构池 

static void (*usartEventCb[5])(uint32_t event);

ARM_DRIVER_USART *uartDrvObjGet(type_uart_OBJ uartObj){

	ARM_DRIVER_USART *uartDrvObj = NULL;
	
	switch(uartObj){
	
		case comObj_DbugP1:			{uartDrvObj = &DbugP1;}break;		
		
		case comObj_DataTransP1:	{uartDrvObj = &DataTransP1;}break;		
		
		case comObj_DataTransP2:	{uartDrvObj = &DataTransP2;}break;	

		default:break;
	}
	
	return uartDrvObj;
}

uint32_t uartRxTimeout_signalDefGet(type_uart_OBJ uartObj){

	uint32_t sig = 0;
	
	switch(uartObj){
		
		case comObj_DbugP1:{
		
			sig = USRAPP_OSSIGNAL_DEF_UART1_RX_TOUT;
		
		}break;
		
		case comObj_DataTransP1:{
			
			sig = USRAPP_OSSIGNAL_DEF_UART2_RX_TOUT;
			
		}break;
		
		case comObj_DataTransP2:{
					
			sig = USRAPP_OSSIGNAL_DEF_UART5_RX_TOUT;
			
		}break;
			
		default:{}break;
	}
	
	return sig;
}

static void USART1_DbugP1_Init(void){

    /*Initialize the USART driver */
	Driver_USART1.Uninitialize();
    Driver_USART1.Initialize(usartEventCb[0]);
    /*Power up the USART peripheral */
    Driver_USART1.PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 115200 Bits/sec */
    Driver_USART1.Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 115200);

    /* Enable Receiver and Transmitter lines */
    Driver_USART1.Control (ARM_USART_CONTROL_TX, 1);
    Driver_USART1.Control (ARM_USART_CONTROL_RX, 1);
	
    Driver_USART1.Send("i'm usart1 for debugP1\r\n", 24);
}

static void USART2_DataTransP1_Init(void){
	
	GPIO_InitTypeDef GPIO_InitStructure;

    /*Initialize the USART driver */
	Driver_USART2.Uninitialize();
    Driver_USART2.Initialize(usartEventCb[1]);
    /*Power up the USART peripheral */
    Driver_USART2.PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 115200 Bits/sec */
    Driver_USART2.Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 115200);

    /* Enable Receiver and Transmitter lines */
    Driver_USART2.Control (ARM_USART_CONTROL_TX, 1);
    Driver_USART2.Control (ARM_USART_CONTROL_RX, 1);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    Driver_USART2.Send("i'm usart2 for dataTransP1\r\n", 28);
}

static void USART5_DataTransP2_Init(void){
	
    GPIO_InitTypeDef GPIO_InitStructure;

    /*Initialize the USART driver */
	Driver_USART5.Uninitialize();
    Driver_USART5.Initialize(usartEventCb[4]);
    /*Power up the USART peripheral */
    Driver_USART5.PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 115200 Bits/sec */
    Driver_USART5.Control(ARM_USART_MODE_ASYNCHRONOUS |
                          ARM_USART_DATA_BITS_8 |
                          ARM_USART_PARITY_NONE |
                          ARM_USART_STOP_BITS_1 |
                          ARM_USART_FLOW_CONTROL_NONE, 115200);

    /* Enable Receiver and Transmitter lines */
    Driver_USART5.Control (ARM_USART_CONTROL_TX, 1);
    Driver_USART5.Control (ARM_USART_CONTROL_RX, 1);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	PAout(7) = 1;
	
    Driver_USART5.Send("i'm usart5 for dataTransP2\r\n", 28);
}

void usrApp_usartEventCb_register(type_uart_OBJ uartObj, void (*funCb)(uint32_t event)){
	
	switch(uartObj){
		
		case comObj_DbugP1:{
		
			usartEventCb[0] = funCb;
			USART1_DbugP1_Init();
		
		}break;
		
		case comObj_DataTransP1:{
			
			usartEventCb[1] = funCb;
			USART2_DataTransP1_Init();
			
		}break;
		
		case comObj_DataTransP2:{
					
			usartEventCb[4] = funCb;
			USART5_DataTransP2_Init();
			
		}break;
			
		default:{}break;
	}
	
	{
		memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
		snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "com evtCb[%02X] reg.\n", uartObj);
		osDelay(20);
		DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
	}
}

/*通讯主线程激活*/
void dataTransThread_Active(const void *argument){

	paramLaunch_OBJ *datsTransActOBJ = (paramLaunch_OBJ *)argument;
	
	switch(datsTransActOBJ->funTrans_OBJ){
	
		case ftOBJ_ZIGB:{
		
			ZigB_mainThread(datsTransActOBJ->uart_OBJ);
		}break;
		
		case ftOBJ_WIFI:{
		
			WIFI_mainThread(datsTransActOBJ->uart_OBJ);
		}break;
		
		case ftOBJ_DEBUG:{
	
			usrApp_usartEventCb_register(datsTransActOBJ->uart_OBJ, NULL);
			
		}break;
		
		default:break;
	}
	
	osPoolFree(dttAct_poolAttr_id,datsTransActOBJ);
}

void com1DbugP1_Thread(const void *argument){
	
	dataTransThread_Active(argument);
	
	for(;;){
	
		osDelay(10);
	}
}

void com2DataTransP1_Thread(const void *argument){
	
	dataTransThread_Active(argument);
}

void com5DataTransP2_Thread(const void *argument){

	dataTransThread_Active(argument);
}

void osMemoryInit(void){

	dttAct_poolAttr_id = osPoolCreate(osPool(dtThreadActInitParm_pool)); //数据传输进程初始化信息 数据结构堆内存池初始化
}

void msgQueueInit(void){

	threadDP_poolAttr_id = osPoolCreate(osPool(threadDatsPass_pool)); //zigbee与wifi主线程数据传输消息队列 数据结构堆内存池初始化
}

void communicationActive(type_uart_OBJ comObj,type_ftOBJ ftTransObj){
	
	paramLaunch_OBJ *param_DatsTransAct = (paramLaunch_OBJ *)osPoolAlloc(dttAct_poolAttr_id);
	
	param_DatsTransAct->uart_OBJ 		= comObj;
	param_DatsTransAct->funTrans_OBJ 	= ftTransObj;

	switch(param_DatsTransAct->uart_OBJ){
	
		case comObj_DbugP1:		tid_com1DbugP1_Thread = osThreadCreate(osThread(com1DbugP1_Thread),param_DatsTransAct);
								break;
				
		case comObj_DataTransP1:tid_com2DataTransP1_Thread = osThreadCreate(osThread(com2DataTransP1_Thread),param_DatsTransAct);
								break;
				
		case comObj_DataTransP2:tid_com5DataTransP2_Thread = osThreadCreate(osThread(com5DataTransP2_Thread),param_DatsTransAct);
								break;
		
		default:				break;
	}	
}
