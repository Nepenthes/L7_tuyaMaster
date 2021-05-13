#include "datsProcess_uartWifi.h"

#include "wifi.h"

extern osPoolId (threadDP_poolAttr_id);

osPoolId  (WIFITrans_poolAttr_id); // Memory pool ID           
osPoolDef (datsWIFITrans_pool, 5, datsAttr_WIFITrans); // Declare memory pool //WIFI进程内数据传输 数据结构池  

osMessageQId  mqID_threadDP_W2Z; // messageQ ID
osMessageQDef(MsgBox_threadDP_W2Z, 20, &stt_threadDatsPass); // Define message queue	//WIFI到ZigB消息队列

static osThreadId tid_wifiThread = NULL;
static type_uart_OBJ uartObj_current = comObj_DbugP1;
static ARM_DRIVER_USART *uartDrvObj_local = NULL;
static uint32_t localSigVal_uartWifi_rxTout = 0;

void uartConWifi_eventFunCb(uint32_t event){

	if(event & ARM_USART_EVENT_RX_TIMEOUT){
	
		osSignalSet(tid_wifiThread, localSigVal_uartWifi_rxTout);
	}
}

void uartConWifi_dataTxPort(unsigned char *data, unsigned short dataLen){

	uartDrvObj_local->Send(data, dataLen);
}

/*WIFI端数据自定义校验*/
u8 frame_Check(u8 frame_temp[], u8 check_num){
  
	u8 loop 	 = 0;
	u8 val_Check = 0;
	
	for(loop = 0; loop < check_num; loop ++){
	
		val_Check += frame_temp[loop];
	}
	
	val_Check  = ~val_Check;
	val_Check ^= 0xa7;
	
	return val_Check;
}

/*WIFI数据帧加载*///报废
u8 WIFI_TXFrameLoad(u8 frame[],u8 cmd[],u8 cmdLen,u8 dats[],u8 datsLen){

	const u8 frameHead = WIFI_FRAME_HEAD;	//SOF帧头
	u8 xor_check = datsLen;					//异或校验，帧尾
	u8 loop = 0;
	u8 ptr = 0;
	
	frame[ptr ++] = frameHead;
	frame[ptr ++] = datsLen;
	
	memcpy(&frame[ptr],cmd,cmdLen);
	ptr += cmdLen;
	for(loop = 0;loop < cmdLen;loop ++)xor_check ^= cmd[loop];

	memcpy(&frame[ptr],dats,datsLen);
	ptr += datsLen;
	for(loop = 0;loop < datsLen;loop ++)xor_check ^= dats[loop];	
	
	frame[ptr ++] = xor_check;
	
	return ptr;
}

/*WIFI单指令下发及响应验证*/
bool wifi_ATCMD_INPUT(type_uart_OBJ UART_OBJ,char *CMD,char *REPLY[2],u8 REPLY_LEN[2],u8 times,u16 timeDelay){
	
	const u8 dataLen = 100;
	u8 dataRXBUF[dataLen] = {0};
	u8 loop = 0;
	u8 loopa = 0;
	
	static ARM_DRIVER_USART *USARTdrv;
	
	switch(UART_OBJ){
	
		case comObj_DbugP1:			{USARTdrv = &DbugP1;}break;		
		
		case comObj_DataTransP1:	{USARTdrv = &DataTransP1;}break;		
		
		case comObj_DataTransP2:	{USARTdrv = &DataTransP2;}break;	

		default:break;
	}
	
	for(loop = 0;loop < times;loop ++){
	
		USARTdrv->Send(CMD,strlen(CMD));
		USARTdrv->Receive(dataRXBUF,dataLen);
		osDelay(timeDelay);
		osDelay(20);
		USARTdrv->Control(ARM_USART_ABORT_RECEIVE,0);
		
		for(loopa = 0;loopa < 2;loopa ++)if(memmem(dataRXBUF,dataLen,REPLY[loopa],REPLY_LEN[loopa]))return true;
	}return false;
}

static void threadProcessWifi_initialization(type_uart_OBJ UART_OBJ){

	WIFITrans_poolAttr_id = osPoolCreate(osPool(datsWIFITrans_pool)); //wifi数据传输 数据结构堆内存池初始化
	
	mqID_threadDP_W2Z = osMessageCreate(osMessageQ(MsgBox_threadDP_W2Z), NULL);	//wifi到zigbee消息队列初始化
	
	wifi_protocol_init(); //tuya
	
	usrApp_usartEventCb_register(UART_OBJ, uartConWifi_eventFunCb); //串口回调注册
	
	tid_wifiThread = osThreadGetId();
	uartObj_current = UART_OBJ;
	localSigVal_uartWifi_rxTout = uartRxTimeout_signalDefGet(UART_OBJ);
	uartDrvObj_local = uartDrvObjGet(UART_OBJ);
	
	osDelay(10);
	DbugP1TX("uartWifi Initialize.\n", 40);
}

static void wifiThreadPrt_uartDataRcv_process(void){
	
	osEvent evt;
	static uint32_t uartDataRcv_len = 0;
	static bool uartRcv_startFlg = false;
	static uint8_t uartDataRcv_temp[UART_DATA_RCV_TEMP_LENGTH] = {0};
	
	if(!uartRcv_startFlg){
	
		uartDrvObj_local->Receive(uartDataRcv_temp, UART_DATA_RCV_TEMP_LENGTH);
		uartRcv_startFlg = true;
		
//		osDelay(20);
//		DbugP1TX("uartWifi dataRcv start.\n", 40);
	}
	
	evt = osSignalWait(localSigVal_uartWifi_rxTout, 0);
	if(osEventSignal == evt.status){
		
		uint8_t loop = 0;
		
		uartDrvObj_local->Control(ARM_USART_ABORT_RECEIVE, 0);

		for(loop = 0; loop < (uint8_t)(uartDrvObj_local->GetRxCount()); loop ++){
		
			uart_receive_input(uartDataRcv_temp[loop]); //tuya
		}
		
		uartRcv_startFlg = false;
		
//		{
//			memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
//			snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "uartWifi dataRcv[len:%d]:%02X, %02X, %02X.\n", uartDrvObj_local->GetRxCount(),
//																										   uartDataRcv_temp[0],
//																										   uartDataRcv_temp[1],
//																										   uartDataRcv_temp[2]);
//			osDelay(20);
//			DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
//		}
	}
}

/*WIFI主线程*/
void WIFI_mainThread(type_uart_OBJ UART_OBJ){
	
	threadProcessWifi_initialization(UART_OBJ);
	
	for(;;){
	
		wifiThreadPrt_uartDataRcv_process();
		wifi_uart_service(); //tuya
	}
}
