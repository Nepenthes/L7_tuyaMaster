#include "datsProcess_uartZigbee.h"

#include "datsProcess_uartWifi.h"

extern osPoolId (threadDP_poolAttr_id);
extern osMessageQId  mqID_threadDP_W2Z;

static void uartZigbDataRcvHandle_Thread(const void *argument);
osThreadId tid_uartZigbDataRcvHandle_Thread;
osThreadDef(uartZigbDataRcvHandle_Thread, osPriorityNormal,	1, 512);

osTimerDef(TimerZigbDevManage, TimerZigbDevManage_Callback);

osPoolId  (ZigbnwkState_poolAttr_id); // Memory pool ID              
osPoolDef (nwkZigbDevStateAttr_pool, 72, nwkStateAttr_Zigb); // Declare memory pool	//ZIGB网络节点信息 数据结构池

osPoolId  (ZigbUartMsgDataSysRespond_poolAttr_id); // Memory pool ID              
osPoolDef (ZigbUartMsgDataSysRespond_pool, 30, sttUartZigbRcv_sysDat); // 
osPoolId  (ZigbUartMsgDataRemoteComing_poolAttr_id); // Memory pool ID              
osPoolDef (ZigbUartMsgDataRemoteComing_pool, 40, sttUartZigbRcv_rmoteDatComming); // 
osPoolId  (ZigbUartMsgRmDataReqResp_poolAttr_id); // Memory pool ID              
osPoolDef (ZigbUartMsgRmDataReqResp_pool, 30, sttUartZigbRcv_rmDatsReqResp); // 

osPoolId  (ZigbFuncRemind_poolAttr_id); // Memory pool ID              
osPoolDef (ZigbFuncRemind_pool, 5, stt_zigbFunMsg); // 

osMessageQId  mqID_threadDP_Z2W; // messageQ ID              	
osMessageQDef(MsgBox_threadDP_Z2W, 20, &stt_threadDatsPass); // Define message queue	//ZigB功能触发消息 内存池

osMessageQId  mqID_uartZigbToutDats_dataSysRespond; // messageQ ID              	
osMessageQDef(MsgBox_uartZigbToutDats_dataSysRespond, 30, &sttUartZigbRcv_sysDat); // Define message queue	//ZigB模块串口数据归类队列：常规系统响应
osMessageQId  mqID_uartZigbToutDats_dataRemoteComing; // messageQ ID              	
osMessageQDef(MsgBox_uartZigbToutDats_dataRemoteComing, 40, &sttUartZigbRcv_rmoteDatComming); // Define message queue	//ZigB模块串口数据归类队列：远端数据请求（子节点数据来了）
osMessageQId  mqID_uartZigbToutDats_rmDataReqResp; // messageQ ID              	
osMessageQDef(MsgBox_uartZigbToutDats_rmDataReqResp, 30, &sttUartZigbRcv_rmDatsReqResp); // Define message queue	//ZigB模块串口数据归类队列：远端数据请求结果系统响应回复

osMessageQId  mqID_zigbFuncRemind; // messageQ ID              	
osMessageQDef(MsgBox_zigbFuncRemind, 3, &stt_zigbFunMsg);

uint8_t zigbee_currentPanID_reslesCounter = ZIGBPANID_CURRENT_REALESPERIOD;

LOCAL bool zigbNodeDevDetectManage_runningEN = false; //
      bool zigbNodeDevDetectManage_runningFLG = false; //

static osThreadId tid_zigbThread = NULL;
static osThreadId tid_zigbUartDatsRcvThread = NULL;
static type_uart_OBJ uartObj_current = comObj_DbugP1;
static ARM_DRIVER_USART *uartDrvObj_local = NULL;
static uint32_t localSigVal_uartWifi_rxTout = 0;

static bool nwkZigbOnline_IF = false;
static uint16_t nwkZigb_currentPANID = 0;
static uint16_t sysZigb_random = 0x1234;

static uint8_t uartDataZigbRcv_temp[UARTCON_ZIGB_DATA_RCV_TEMP_LENGTH] = {0};

static stt_dataRemoteReq localZigbASYDT_bufQueueRemoteReq[zigB_remoteDataTransASY_QbuffLen] = {0};
static uint8_t dataRemoteRequest_failCount = 0;

LOCAL uint16_t uartBuff_baseInsert = 0; //
LOCAL uint16_t uartBuff_baseInsertStart = 0;

void uartConZigb_eventFunCb(uint32_t event){

	if(event & ARM_USART_EVENT_RX_TIMEOUT){
	
		osSignalSet(tid_zigbUartDatsRcvThread, localSigVal_uartWifi_rxTout);
	}
}

/*zigbee重复节点信息优化去重*/ 
static void zigbDev_delSame(nwkStateAttr_Zigb *head)	
{
    nwkStateAttr_Zigb *p,*q,*r;
    p = head->next; 
    while(p != NULL)    
    {
        q = p;
        while(q->next != NULL) 
        {
            if(q->next->nwkAddr == p->nwkAddr || (!memcmp(q->next->macAddr, p->macAddr, DEVMAC_LEN) && (q->next->devType == p->devType))) 
            {
                r = q->next; 
                q->next = r->next;   
                osPoolFree(ZigbnwkState_poolAttr_id,r);
				
				memcpy(q->macAddr, r->macAddr, DEVMAC_LEN * sizeof(u8)); //
				q->nwkAddr = r->nwkAddr;
				q->onlineDectect_LCount = r->onlineDectect_LCount;
            }
            else
                q = q->next;
        }

        p = p->next;
    }
}

/*zigbee设备信息节点新增节点*/ 
static u8 zigbDev_eptCreat(nwkStateAttr_Zigb *pHead,nwkStateAttr_Zigb pNew){
	
	nwkStateAttr_Zigb *pAbove = pHead;
	nwkStateAttr_Zigb *pFollow = NULL;
	u8 nCount = 0;
	
	nwkStateAttr_Zigb *pNew_temp = (nwkStateAttr_Zigb *) osPoolAlloc(ZigbnwkState_poolAttr_id);
	pNew_temp->nwkAddr 				= pNew.nwkAddr;
	memcpy(pNew_temp->macAddr, pNew.macAddr, DEVMAC_LEN);
	pNew_temp->devType 				= pNew.devType;
	pNew_temp->onlineDectect_LCount = pNew.onlineDectect_LCount;
	pNew_temp->next	   				= NULL;
	
	while(pAbove->next != NULL){
		
		nCount ++;
		pFollow = pAbove;
		pAbove	= pFollow->next;
	}
	
	pAbove->next = pNew_temp;
	return ++nCount;
}

/*zigbee设备信息链表节点输出，根据网络地址		！！！谨记使用完取出的节点信息内存后要释放！！！*/ 
static nwkStateAttr_Zigb *zigbDev_eptPutout_BYnwk(nwkStateAttr_Zigb *pHead,u16 nwkAddr,bool method){	//method = 1,源节点地址返回；method = 0,源节点信息映射地址返回，地址操作不会影响源节点
	
	nwkStateAttr_Zigb *pAbove = pHead;
	nwkStateAttr_Zigb *pFollow = NULL;
	
	nwkStateAttr_Zigb *pTemp = (nwkStateAttr_Zigb *)osPoolAlloc(ZigbnwkState_poolAttr_id);
	pTemp->next = NULL;
	
	while(!(pAbove->nwkAddr == nwkAddr) && pAbove->next != NULL){
		
		pFollow = pAbove;
		pAbove	= pFollow->next;
	}
	
	if(pAbove->nwkAddr == nwkAddr){
		
		if(!method){
			
			pTemp->nwkAddr 				= pAbove->nwkAddr;
			memcpy(pTemp->macAddr, pAbove->macAddr, DEVMAC_LEN);
			pTemp->devType 				= pAbove->devType;
			pTemp->onlineDectect_LCount = pAbove->onlineDectect_LCount;
		}else{
			
			osPoolFree(ZigbnwkState_poolAttr_id, pTemp);
			pTemp = pAbove;	
		}
		
		return pTemp;
	}else{
		
		osPoolFree(ZigbnwkState_poolAttr_id, pTemp);
		return NULL;
	}	
} 

/*zigbee设备信息链表节点输出，根据设备MAC地址及设备类型		！！！谨记使用完取出的节点信息内存后要释放！！！*/ 
static nwkStateAttr_Zigb *zigbDev_eptPutout_BYpsy(nwkStateAttr_Zigb *pHead,u8 macAddr[DEVMAC_LEN],u8 devType,bool method){		//method = 1,源节点地址返回；method = 0,源节点信息映射地址返回，地址操作不会影响源节点
	
	nwkStateAttr_Zigb *pAbove = pHead;
	nwkStateAttr_Zigb *pFollow = NULL;
	
	nwkStateAttr_Zigb *pTemp = (nwkStateAttr_Zigb *) osPoolAlloc(ZigbnwkState_poolAttr_id);
	pTemp->next = NULL;
	
	while(!(!memcmp(pAbove->macAddr, macAddr, DEVMAC_LEN) && pAbove->devType == devType) && pAbove->next != NULL){
		
		pFollow = pAbove;
		pAbove	= pFollow->next;
	}
	
	if(!memcmp(pAbove->macAddr, macAddr, DEVMAC_LEN) && pAbove->devType == devType){
		
		if(!method){
			
			pTemp->nwkAddr 				= pAbove->nwkAddr;
			memcpy(pTemp->macAddr, pAbove->macAddr, DEVMAC_LEN);
			pTemp->devType 				= pAbove->devType;
			pTemp->onlineDectect_LCount = pAbove->onlineDectect_LCount;
		}else{
			
			osPoolFree(ZigbnwkState_poolAttr_id, pTemp);
			pTemp = pAbove;	
		}
		
		return pTemp;
	}else{
		
		osPoolFree(ZigbnwkState_poolAttr_id, pTemp);
		return NULL;
	}	
}

/*zigbee设备信息链表节点删除，根据网络地址*/ 
static bool zigbDev_eptRemove_BYnwk(nwkStateAttr_Zigb *pHead,u16 nwkAddr){
	
	nwkStateAttr_Zigb *pAbove = pHead;
	nwkStateAttr_Zigb *pFollow = NULL;
	
	nwkStateAttr_Zigb *pTemp = NULL;
	
	while(!(pAbove->nwkAddr == nwkAddr) && pAbove->next != NULL){
		
		pFollow = pAbove;
		pAbove	= pFollow->next;
	}
	
	if(pAbove->nwkAddr == nwkAddr){
		
		pTemp = pAbove;
		pFollow->next = pAbove->next;
		if(pTemp != NULL){

			osPoolFree(ZigbnwkState_poolAttr_id,pTemp);
			pTemp = NULL;
		}
		
		return true;
		
	}else{
		
		return false;
	}
}

/*zigbee设备信息链表节点删除，根据设备MAC地址及设备类型*/ 
static bool zigbDev_eptRemove_BYpsy(nwkStateAttr_Zigb *pHead,u8 macAddr[DEVMAC_LEN],u8 devType){
	
	nwkStateAttr_Zigb *pAbove = pHead;
	nwkStateAttr_Zigb *pFollow = NULL;
	
	nwkStateAttr_Zigb *pTemp = NULL;
	
	while(!(!memcmp(pAbove->macAddr, macAddr, DEVMAC_LEN) && pAbove->devType == devType) && pAbove->next != NULL){
		
		pFollow = pAbove;
		pAbove	= pFollow->next;
	}
	
	if(!memcmp(pAbove->macAddr, macAddr, DEVMAC_LEN) && pAbove->devType == devType){
		
		 pTemp = pAbove;
		 pFollow->next = pAbove->next;
		 osPoolFree(ZigbnwkState_poolAttr_id,pTemp);
		 return true;
	}else{
		
		return false;
	}
}

/*zigbee设备信息链表长度测量*/
static u8 zigbDev_chatLenDectect(nwkStateAttr_Zigb *pHead){
	
	nwkStateAttr_Zigb *pAbove = pHead;
	nwkStateAttr_Zigb *pFollow = NULL;
	u8 loop = 0;
	
	while(pAbove->next != NULL){
		
		loop ++;
		pFollow = pAbove;
		pAbove	= pFollow->next;
	}

	return loop;
}

/*zigbee设备信息链表遍历  遍历 设备MAC地址 及 设备类型 将其打包输出*/
static u8 ZigBdevDispList(nwkStateAttr_Zigb *pHead,u8 DevInform[]){
	
	nwkStateAttr_Zigb *Disp = pHead;
	u8 loop = 0;
	
	if(0 == zigbDev_chatLenDectect(pHead)){
		
		return 0;
	}

	while(Disp->next != NULL){
	
		Disp = Disp->next;
		
		memcpy(&DevInform[loop * (DEVMAC_LEN + 1)], Disp->macAddr, DEVMAC_LEN);
		DevInform[loop * (DEVMAC_LEN + 1) + 5] = Disp->devType;
		loop ++;
	}
	
	return loop;
}

/*zigbee端数据异或校验*/
static u8 XOR_CHECK(u8 buf[],u8 length){

	u8 loop = 0;
	u8 valXOR = buf[0];
	
	for(loop = 1;loop < length;loop ++)valXOR ^= buf[loop];
	
	return valXOR;
}

/*zigbee数据帧加载*/
static u8 ZigB_TXFrameLoad(u8 frame[],u8 cmd[],u8 cmdLen,u8 dats[],u8 datsLen){		

	const u8 frameHead = ZIGB_FRAME_HEAD;	//ZNP,SOF帧头
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

LOCAL void ZigB_sysCtrlFrameLoad(u8 datsTemp[], frame_zigbSysCtrl dats){

	datsTemp[0] = dats.command;
	datsTemp[1] = dats.datsLen;
	memcpy((char *)&datsTemp[2], (char *)dats.dats, dats.datsLen);
}

static bool zigb_datsRequest(u8 frameREQ[],	//
							 u8 frameREQ_Len,	//
							 u8 resp_cmd[2],	//
							 datsZigb_reqGet *datsRX,	//
							 u16 timeWait, //
							 remoteDataReq_method method){ //

	osEvent evt;
	sttUartRcv_sysDat *rptr_uartDatsRcv = NULL;
	uint16_t datsRcv_tout = timeWait;

	if(!method.keepTxUntilCmp_IF)uartDrvObj_local->Send(frameREQ, frameREQ_Len); //
	
	while(datsRcv_tout --){

		osDelay(1);

		if(method.keepTxUntilCmp_IF){ //

			if((datsRcv_tout % method.datsTxKeep_Period) == 0)uartDrvObj_local->Send(frameREQ, frameREQ_Len);
		}
	
		evt = osMessageGet(mqID_uartZigbToutDats_dataSysRespond, 0);
		while(evt.status == osEventMessage){
		
			rptr_uartDatsRcv = evt.value.p;
			
			if((rptr_uartDatsRcv->dats[0] == ZIGB_FRAME_HEAD) &&
			   (rptr_uartDatsRcv->dats[rptr_uartDatsRcv->datsLen - 1] == XOR_CHECK(&(rptr_uartDatsRcv->dats[1]), rptr_uartDatsRcv->datsLen - 2)) &&
			   (!memcmp(&(rptr_uartDatsRcv->dats[2]), resp_cmd, 2))
			   ){

					memcpy(datsRX->cmdResp, &rptr_uartDatsRcv->dats[2], 2);
					memcpy(datsRX->frameResp, rptr_uartDatsRcv->dats, rptr_uartDatsRcv->datsLen);
					datsRX->frameRespLen = rptr_uartDatsRcv->datsLen;
				   
					osPoolFree(ZigbUartMsgDataSysRespond_poolAttr_id, rptr_uartDatsRcv);
					rptr_uartDatsRcv = NULL;
				   
					return true;
			}
			else
			{
				osPoolFree(ZigbUartMsgDataSysRespond_poolAttr_id, rptr_uartDatsRcv);
				rptr_uartDatsRcv = NULL;
			}
			
			evt = osMessageGet(mqID_uartZigbToutDats_dataSysRespond, 0);
		}
	}

	osDelay(1);

	return false;
}
					
bool  
zigb_VALIDA_INPUT(uint8_t REQ_CMD[2],			//
				  uint8_t REQ_DATS[],	//
				  uint8_t REQdatsLen,	//
				  uint8_t ANSR_frame[],	//
				  uint8_t ANSRdatsLen,	//
				  uint8_t times,uint16_t timeDelay){//
					   
#define zigbDatsTransLen 128

//	uint8_t 	dataTXBUF[zigbDatsTransLen] = {0};
	uint8_t 	*dataTXBUF = (uint8_t *)osPoolAlloc(normalMemUint8_t_poolAttr_id);
	uint8_t 	loop = 0;
	uint8_t 	datsTX_Len;
	sttUartRcv_sysDat *rptr_uartDatsRcv = NULL;
	osEvent 	evt;

	bool result_REQ = false;
	
	datsTX_Len = ZigB_TXFrameLoad(dataTXBUF, REQ_CMD, 2, REQ_DATS, REQdatsLen);

	for(loop = 0;loop < times;loop ++){

		u16 datsRcv_tout = timeDelay;

		uartDrvObj_local->Send(dataTXBUF, datsTX_Len);
		
		while(datsRcv_tout --){
		
			osDelay(1);
			evt = osMessageGet(mqID_uartZigbToutDats_dataSysRespond, 0);
			while(evt.status == osEventMessage){
			
				rptr_uartDatsRcv = evt.value.p;
				
				if(memmem(rptr_uartDatsRcv->dats, rptr_uartDatsRcv->datsLen, ANSR_frame, ANSRdatsLen)){

					result_REQ = true;
					osPoolFree(ZigbUartMsgDataSysRespond_poolAttr_id, rptr_uartDatsRcv);
					break;
				}
				
				osPoolFree(ZigbUartMsgDataSysRespond_poolAttr_id, rptr_uartDatsRcv);
				evt = osMessageGet(mqID_uartZigbToutDats_dataSysRespond, 0);
			}if(true == result_REQ)break;
		}if(true == result_REQ)break;
	}

	osPoolFree(normalMemUint8_t_poolAttr_id, dataTXBUF);
	
	return result_REQ;
}
				  
LOCAL bool zigb_clusterSet(u16 deviveID, u8 endPoint){

	const datsAttr_ZigbInit default_param = {{0x24,0x00},{0x0E,0x0D,0x00,0x0D,0x00,0x0D,0x00,0x01,0x00,0x00,0x01,0x00,0x00},0x0D,{0xFE,0x01,0x64,0x00,0x00,0x65},0x06,20}; //?????,????
	const u8 frameResponse_Subs[6] = {0xFE,0x01,0x64,0x00,0xB8,0xDD}; //
	
#define paramLen_clusterSet 100

	u8 paramTX_temp[paramLen_clusterSet] = {0};

	bool resultSet = false;
	
	memcpy(paramTX_temp, default_param.zigbInit_reqDAT, default_param.reqDAT_num);
	paramTX_temp[0] = endPoint;
	paramTX_temp[3] = (uint8_t)((deviveID & 0x00ff) >> 0);
	paramTX_temp[4] = (uint8_t)((deviveID & 0xff00) >> 8);
	
	resultSet = zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								  (u8 *)paramTX_temp,
								  default_param.reqDAT_num,
								  (u8 *)default_param.zigbInit_REPLY,
								  default_param.REPLY_num,
								  2, 	//
								  default_param.timeTab_waitAnsr);

	if(resultSet)return resultSet;
	else{

		return zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								 (u8 *)paramTX_temp,
								 default_param.reqDAT_num,
								 (u8 *)frameResponse_Subs,
								 6,
								 2, 	//
								 default_param.timeTab_waitAnsr);

	}
}

LOCAL bool zigb_clusterMultiSet(devDatsTrans_portAttr devPort[], u8 Len){

	u8 loop = 0;

	for(loop = 0; loop < Len; loop ++){

		if(!zigb_clusterSet(devPort[loop].deviveID, devPort[loop].endPoint))return false;
	}

	return true;
}

LOCAL bool zigbNetwork_OpenIF(bool opreat_Act, u8 keepTime){

	const datsAttr_ZigbInit default_param = {{0x26,0x08}, {0xFC,0xFF,0x00}, 0x03, {0xFE,0x01,0x66,0x08,0x00,0x6F}, 0x06, 500};	//
#define nwOpenIF_paramLen 64

	bool result_Set = false;

	u8 paramTX_temp[nwOpenIF_paramLen] = {0};
	
	memcpy(paramTX_temp,default_param.zigbInit_reqDAT,default_param.reqDAT_num);
	if(true == opreat_Act)paramTX_temp[2] = keepTime;
	else paramTX_temp[2] = 0x00;
	
	result_Set = zigb_VALIDA_INPUT(	(u8 *)default_param.zigbInit_reqCMD,
									(u8 *)paramTX_temp,
									default_param.reqDAT_num,
									(u8 *)default_param.zigbInit_REPLY,
									default_param.REPLY_num,
									3,		//
									default_param.timeTab_waitAnsr);	

//	if(result_Set)os_printf("[Tips_uartZigb]: zigb nwkOpen success.\n");
//	else os_printf("[Tips_uartZigb]: zigb nwkOpen fail.\n");

	return result_Set;
}

LOCAL bool 
zigB_sysTimeSet(uint32_t timeStamp){

	const datsAttr_ZigbInit default_param = {{0x21,0x10},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x0B,{0xFE,0x01,0x61,0x10,0x00},0x05,30}; //
	u8 timeStampArray[11] = {0};
	bool resultSet = false;
	uint32_t timeStamp_temp = timeStamp;

	if(sysTimeZone_H <= 12){
	
		timeStamp_temp += (3600UL * (long)sysTimeZone_H + 60UL * (long)sysTimeZone_M); //
		
	}else
	if(sysTimeZone_H > 12 && sysTimeZone_H <= 24){
	
		timeStamp_temp -= (3600UL * (long)(sysTimeZone_H - 12) + 60UL * (long)sysTimeZone_M); //
		
	}else
	if(sysTimeZone_H == 30 || sysTimeZone_H == 31){ 
		
		timeStamp_temp += (3600UL * (long)(sysTimeZone_H - 17) + 60UL * (long)sysTimeZone_M); //
	}
	
	timeStampArray[0] = (u8)((timeStamp_temp & 0x000000ff) >> 0);
	timeStampArray[1] = (u8)((timeStamp_temp & 0x0000ff00) >> 8);
	timeStampArray[2] = (u8)((timeStamp_temp & 0x00ff0000) >> 16);
	timeStampArray[3] = (u8)((timeStamp_temp & 0xff000000) >> 24);
	
	resultSet = zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								  (u8 *)timeStampArray,
								  11,
								  (u8 *)default_param.zigbInit_REPLY,
								  default_param.REPLY_num,
								  2,	//
								  default_param.timeTab_waitAnsr);
	
//	os_printf("[Tips_uartZigb]: zigbee sysTime set result: %d.\n", resultSet);
//	if(resultSet){

//		os_printf("[Tips_uartZigb]: zigbSystime set success.\n");
//	}
//	else os_printf("[Tips_uartZigb]: zigbSystime set fail.\n");

	return resultSet;
}

bool zigB_sysTimeSet_detailedFormatTuya(unsigned char timeParam[7]){

	const datsAttr_ZigbInit default_param = {{0x21,0x10},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x0B,{0xFE,0x01,0x61,0x10,0x00},0x05,100}; //
	u8 timeStampArray[11] = {0};
	bool resultSet = false;
	uint16_t temp_Y = 2000 + (uint16_t)timeParam[0]; //年缓
	
	timeStampArray[4]  = timeParam[3]; //时
	timeStampArray[5]  = timeParam[4]; //分
	timeStampArray[6]  = timeParam[5]; //秒
	timeStampArray[7]  = timeParam[1]; //月
	timeStampArray[8]  = timeParam[2]; //日
	timeStampArray[9]  = (uint8_t)((temp_Y >> 0) & 0x00ff); //年低
	timeStampArray[10] = (uint8_t)((temp_Y >> 8) & 0x00ff); //年高
	
	resultSet = zigb_VALIDA_INPUT((u8 *)default_param.zigbInit_reqCMD,
								  (u8 *)timeStampArray,
								  11,
								  (u8 *)default_param.zigbInit_REPLY,
								  default_param.REPLY_num,
								  2,	//
								  default_param.timeTab_waitAnsr);
	
//	os_printf("[Tips_uartZigb]: zigbee sysTime set result: %d.\n", resultSet);
//	if(resultSet){

//		os_printf("[Tips_uartZigb]: zigbSystime set success.\n");
//	}
//	else os_printf("[Tips_uartZigb]: zigbSystime set fail.\n");

	return resultSet;
}

LOCAL bool 
zigB_sysTimeGetRealesWithLocal(void){

	uint32_t timeStamp_temp = 0;
	datsZigb_reqGet local_datsParam = {0};
	const u8 frameREQ_zigbSysTimeGet[5] = {0xFE, 0x00, 0x21, 0x11, 0x30};	//
	const u8 cmdResp_zigbSysTimeGet[2] 	= {0x61, 0x11};	//
	bool resultREQ = false;
	const remoteDataReq_method datsReq_method = {0};

	resultREQ = zigb_datsRequest((u8 *)frameREQ_zigbSysTimeGet,
								 5,
								 (u8 *)cmdResp_zigbSysTimeGet,
								 &local_datsParam,
								 30,
								 datsReq_method);

	if(true == resultREQ){

		u16 Y_temp16 = 0;
		u8  Y_temp8 = 0;
		u8  M_temp8 = 0;
		
		u8 Y = 0;
		u8 M = 0;
		u8 D = 0;
		u8 W = 0;
		
		timeStamp_temp |= (((uint32_t)(local_datsParam.frameResp[4]) << 0)  & 0x000000FF);
		timeStamp_temp |= (((uint32_t)(local_datsParam.frameResp[5]) << 8)  & 0x0000FF00);
		timeStamp_temp |= (((uint32_t)(local_datsParam.frameResp[6]) << 16) & 0x00FF0000);
		timeStamp_temp |= (((uint32_t)(local_datsParam.frameResp[7]) << 24) & 0xFF000000);
		systemUTC_current = timeStamp_temp + ZIGB_UTCTIME_START;

		Y_temp16 = ((u16)local_datsParam.frameResp[13] << 0) | ((u16)local_datsParam.frameResp[14] << 8);
		Y_temp8 = 0;
		M_temp8 = 0;
		
		Y = (u8)(Y_temp16 % 2000);
		M = local_datsParam.frameResp[11];
		D = local_datsParam.frameResp[12];
		W = 0;
		
		Y_temp8 = Y;
		if(M == 1 || M == 2){ //
		
			M_temp8 = M + 12;
			Y_temp8 --;
		}
		else M_temp8 = M;
		
		W =	 Y_temp8 + (Y_temp8 / 4) + 5 - 40 + (26 * (M_temp8 + 1) / 10) + D - 1;	//
		W %= 7; 
		
		W?(systemTime_current.time_Week = W):(systemTime_current.time_Week = 7);
		
		systemTime_current.time_Month = 	M;
		systemTime_current.time_Day = 		D;
		systemTime_current.time_Year = 		Y;
		
		systemTime_current.time_Hour = 		local_datsParam.frameResp[8];
		systemTime_current.time_Minute =	local_datsParam.frameResp[9];
		systemTime_current.time_Second = 	local_datsParam.frameResp[10];

		sysTimeKeep_counter = systemTime_current.time_Minute * 60 + systemTime_current.time_Second; //

//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam.frameResp, local_datsParam.frameRespLen);
	}
	
//	return timeStamp_temp;
	return resultREQ;
}

LOCAL bool ZigB_resetInit(void){

	#define zigbInit_loopTry 	3
	#define zigbInit_onceWait 	500

	const u8 initCmp_Frame[11] = {0xFE, 0x06, 0x41, 0x80, 0x01, 0x02, 0x00, 0x02, 0x06, 0x03, 0xC3};

	u8 	loop = 0;
	u16 timeWait = 0;
	sttUartRcv_sysDat *rptr_uartDatsRcv = NULL;
	osEvent evt;
	bool result_Init = false;

	for(loop = 0; loop < zigbInit_loopTry; loop ++){

		HWCON_ZIGB_MOUDLE_PIN_RST = ZIGB_HARDWARE_RESET_LEVEL;
		osDelay(10);
		HWCON_ZIGB_MOUDLE_PIN_RST = ZIGB_HARDWARE_NORMAL_LEVEL;

//		os_printf("[Tips_uartZigb]: Zigbee hwReset try loop : %d\n", loop + 1);
		{
			memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
			snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zigbHwInit try:%d.\n", loop + 1);
			osDelay(10);
			DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
		}
		
		timeWait = zigbInit_onceWait;
		
		while(timeWait --){
		
			osDelay(1);
			evt = osMessageGet(mqID_uartZigbToutDats_dataSysRespond, 0);
			while(evt.status == osEventMessage){
			
				rptr_uartDatsRcv = evt.value.p;
				
				if(!memcmp(rptr_uartDatsRcv->dats, initCmp_Frame, 11)){

					result_Init = true;
					osPoolFree(ZigbUartMsgDataSysRespond_poolAttr_id, rptr_uartDatsRcv);
					break;
				}
				
				osPoolFree(ZigbUartMsgDataSysRespond_poolAttr_id, rptr_uartDatsRcv);
				evt = osMessageGet(mqID_uartZigbToutDats_dataSysRespond, 0);
			}if(true == result_Init)break;
		}if(true == result_Init)break;
	}

//	if(result_Init)os_printf("[Tips_uartZigb]: Zigbee hwReset success!\n");
//	else os_printf("[Tips_uartZigb]: Zigbee hwReset fail!\n");

	return result_Init;
}

LOCAL u16 ZigB_getPanIDCurrent(void){

	u16 PANID_temp = 0;
	datsZigb_reqGet local_datsParam = {0};
	const u8 frameREQ_zigbPanIDGet[6] 	= {0xFE, 0x01, 0x26, 0x06, 0x06, 0x27};	//
	const u8 cmdResp_zigbPanIDGet[2] 	= {0x66, 0x06};	//
	bool resultREQ = false;
	const remoteDataReq_method datsReq_method = {0};

	resultREQ = zigb_datsRequest((u8 *)frameREQ_zigbPanIDGet,
								 6,
								 (u8 *)cmdResp_zigbPanIDGet,
								 &local_datsParam,
								 30,
								 datsReq_method);

	if(true == resultREQ){

		PANID_temp |= (((u16)(local_datsParam.frameResp[5]) << 0)	& 0x00FF);
		PANID_temp |= (((u16)(local_datsParam.frameResp[6]) << 8)	& 0xFF00);

//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam.frameResp, local_datsParam.frameRespLen);
	}

	return PANID_temp;
}

LOCAL bool ZigB_getIEEEAddr(void){

	datsZigb_reqGet local_datsParam = {0};
	const u8 frameREQ_zigbPanIDGet[6] 	= {0xFE, 0x01, 0x26, 0x06, 0x01, 0x20};	//
	const u8 cmdResp_zigbPanIDGet[2] 	= {0x66, 0x06};	//
	bool resultREQ = false;
	const remoteDataReq_method datsReq_method = {0};

	resultREQ = zigb_datsRequest((u8 *)frameREQ_zigbPanIDGet,
								 6,
								 (u8 *)cmdResp_zigbPanIDGet,
								 &local_datsParam,
								 50,
								 datsReq_method);

	if(true == resultREQ){

		u16 loop = 0;
		
#if(DEV_MAC_SOURCE_DEF == DEV_MAC_SOURCE_ZIGBEE)
		for(loop = 0; loop < DEV_MAC_LEN; loop ++){

			MACSTA_ID[DEV_MAC_LEN - loop - 1] = local_datsParam.frameResp[5 + loop];
		}
#else
#endif	
//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam.frameResp, local_datsParam.frameRespLen);
	}

	return resultREQ;
}

LOCAL bool ZigB_getRandom(void){

	datsZigb_reqGet local_datsParam = {0};
	const u8 frameREQ_zigbPanIDGet[5] 	= {0xFE, 0x00, 0x21, 0x0C, 0x2D};	//
	const u8 cmdResp_zigbPanIDGet[2] 	= {0x61, 0x0C};	//
	bool resultREQ = false;
	const remoteDataReq_method datsReq_method = {0};

	resultREQ = zigb_datsRequest((u8 *)frameREQ_zigbPanIDGet,
								 5,
								 (u8 *)cmdResp_zigbPanIDGet,
								 &local_datsParam,
								 30,
								 datsReq_method);

	if(true == resultREQ){

		sysZigb_random = 0;

		sysZigb_random |= (u16)local_datsParam.frameResp[4] << 0;
		sysZigb_random |= (u16)local_datsParam.frameResp[5] << 8;

		sysZigb_random %= ZIGB_PANID_MAXVAL;
	
//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam.frameResp, local_datsParam.frameRespLen);
	}
	
	return resultREQ;
}

LOCAL bool ZigB_inspectionSelf(bool hwReset_IF){ //
	
	datsZigb_reqGet local_datsParam = {0};

	const datsAttr_ZigbInit default_param = {{0x26,0x08}, {0xFC,0xFF,0x00}, 0x03, {0xFE,0x01,0x66,0x08,0x00,0x6F}, 0x06, 500};	//
	const u8 frameREQ_zigbJoinNWK[5] 	= {0xFE, 0x00, 0x26, 0x00, 0x26};	//zigb
	const u8 cmdResp_zigbJoinNWK[2] 	= {0x45, 0xC0};	//zigb
	const remoteDataReq_method datsReq_method = {0};
	bool resultREQ = false;

	if(hwReset_IF)resultREQ = ZigB_resetInit();
	else resultREQ = true;
	
	if(true == resultREQ){

		resultREQ = zigb_datsRequest((u8 *)frameREQ_zigbJoinNWK,
									 5,
									 (u8 *)cmdResp_zigbJoinNWK,
									 &local_datsParam,
									 800,
									 datsReq_method);
	}

	if(true == resultREQ){

//		printf_datsHtoA("[Tips_uartZigb]: resultDats:", local_datsParam.frameResp, local_datsParam.frameRespLen);

		if(local_datsParam.frameResp[4] != 0x09)resultREQ = false;
		else{

			resultREQ = zigb_clusterSet(ZIGB_CLUSTER_DEFAULT_DEVID, ZIGB_ENDPOINT_CTRLSECENARIO);	//
			if(resultREQ)resultREQ = zigb_clusterSet(ZIGB_CLUSTER_DEFAULT_DEVID, ZIGB_ENDPOINT_CTRLNORMAL);	//
			if(resultREQ)resultREQ = zigb_clusterSet(ZIGB_CLUSTER_DEFAULT_DEVID, ZIGB_ENDPOINT_CTRLSYSZIGB); //
//			if(resultREQ)resultREQ = zigb_clusterCtrlEachotherCfg(); //
			if(resultREQ)resultREQ = zigbNetwork_OpenIF(0, 0); //
		}
	}

	if(true == resultREQ){

		resultREQ = zigb_VALIDA_INPUT(	(u8 *)default_param.zigbInit_reqCMD,
										(u8 *)default_param.zigbInit_reqDAT,
										default_param.reqDAT_num,
										(u8 *)default_param.zigbInit_REPLY,
										default_param.REPLY_num,
										3,		//
										default_param.timeTab_waitAnsr);	
	}

//	os_printf("[Tips_uartZigb]: Zigbee inspection result is : %d\n", resultREQ);
	return resultREQ;
}

LOCAL void ZigB_nwkReconnect(void){

	static u8 reconnectStep = 1;
	static u8 reconnectTryloop = 0;

	switch(reconnectStep){

//		os_printf("[Tips_uartZigb]: ZigbeeNwk reconnect start.\n");

		case 1:{

			if(ZigB_inspectionSelf(false)){
			
				reconnectTryloop = 0;
				reconnectStep = 2;
			}
			else{
			
				reconnectTryloop ++;
//				os_printf("[Tips_uartZigb]: Zigbee reconnectStep 1 try loop%d.\n", reconnectTryloop);
				if(reconnectTryloop > 3){
			
					reconnectTryloop = 0;
					reconnectStep = 1;
				}
			}
			
		}break;

		case 2:{

			reconnectStep = 1;
			
//			xQueueReset(xMsgQ_timeStampGet);
//			xQueueReset(xMsgQ_Socket2Zigb);
//			xQueueReset(xMsgQ_zigbFunRemind);
			
			nwkZigbOnline_IF = true;
			
//			os_printf("[Tips_uartZigb]: ZigbeeNwk reconnect compelete.\n");
			
		}break;
	}
}

/*zigbee???*/
LOCAL bool ZigB_NwkCreat(uint16_t PANID, uint8_t CHANNELS){		

#define	zigbNwkCrateCMDLen 	10	//
	
#define loop_PANID		5	//
#define loop_CHANNELS	6	//

	const datsAttr_ZigbInit ZigbInit_dats[zigbNwkCrateCMDLen] = {

		{	{0x41,0x00},	{0x00},					0x01,	{0xFE,0x06,0x41,0x80,0x02,0x02,0x00,0x02,0x06,0x03,0xC0},	0x0B,	5000 },	//	
		{	{0x41,0x00},	{0x00},					0x01,	{0xFE,0x06,0x41,0x80,0x02,0x02,0x00,0x02,0x06,0x03,0xC0},	0x0B,	5000 },	//
		{	{0x41,0x00},	{0x00},					0x01,	{0xFE,0x06,0x41,0x80,0x02,0x02,0x00,0x02,0x06,0x03,0xC0},	0x0B,	5000 },	//
		{	{0x26,0x05},	{0x03,0x01,0x03},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	100  },	//
		{	{0x41,0x00},	{0x00},					0x01,	{0xFE,0x06,0x41,0x80,0x02,0x02,0x00,0x02,0x06,0x03,0xC0},	0x0B,	5000 },	//
		{	{0x27,0x02},	{0x34,0x12},			0x02,	{0xFE,0x01,0x67,0x02,0x00,0x64},							0x06,	100  },	//
		{	{0x27,0x03},	{0x00,0x80,0x00,0x00},	0x04,	{0xFE,0x01,0x67,0x03,0x00,0x65},							0x06,	100	 },	//
		{	{0x26,0x05},	{0x87,0x01,0x00},		0x03,	{0xFE,0x01,0x66,0x05,0x00,0x62},							0x06,	100	 },	//
		{	{0x26,0x00},	{0},					0x00,	{0xFE,0x01,0x45,0xC0,0x09,0x8D},							0x06,	8000 },	//
		{	{0x26,0x08}, 	{0xFC,0xFF,0x00}, 		0x03, 	{0xFE,0x01,0x66,0x08,0x00,0x6F}, 							0x06, 	200  },	//
	};
	
#define zigbNwkCrate_paramLen 100
	u8 paramTX_temp[zigbNwkCrate_paramLen] = {0};
	
	u8  loop = 0;
	u32 chnl_temp = 0x00000800UL << CHANNELS;
	bool result_Set = 0;
	
	for(loop = 1; loop < zigbNwkCrateCMDLen; loop ++){
		
		memset(paramTX_temp, 0, zigbNwkCrate_paramLen * sizeof(uint8_t));

		if(loop == 1){

			if(!ZigB_resetInit())loop = 0;
			else{

//				os_printf("[Tips_uartZigb]: Zigbee nwkCreat step:%d complete !!!\n", loop);
				{
					memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
					snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zbNwkcreat step:%d.\n", loop);
					osDelay(10);
					DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
				}
			}			
		}
		
		switch(loop){	//
		
			case loop_PANID:{
			
				paramTX_temp[0] = (uint8_t)((PANID & 0x00ff) >> 0);
				paramTX_temp[1] = (uint8_t)((PANID & 0xff00) >> 8);
			}break;
			
			case loop_CHANNELS:{
			
				paramTX_temp[0] = (uint8_t)((chnl_temp & 0x000000ff) >>  0);
				paramTX_temp[1] = (uint8_t)((chnl_temp & 0x0000ff00) >>  8);
				paramTX_temp[2] = (uint8_t)((chnl_temp & 0x00ff0000) >> 16);
				paramTX_temp[3] = (uint8_t)((chnl_temp & 0xff000000) >> 24);
			}break;
			
			default:{
			
				memcpy(paramTX_temp,ZigbInit_dats[loop].zigbInit_reqDAT,ZigbInit_dats[loop].reqDAT_num);
				
			}break;
		}

		if(loop > 1){

			if(false == zigb_VALIDA_INPUT((u8 *)ZigbInit_dats[loop].zigbInit_reqCMD,
										  (u8 *)paramTX_temp,
										  ZigbInit_dats[loop].reqDAT_num,
										  (u8 *)ZigbInit_dats[loop].zigbInit_REPLY,
										  ZigbInit_dats[loop].REPLY_num,
										  3,
										  ZigbInit_dats[loop].timeTab_waitAnsr)
										 )loop = 0;
			else{
			
//				os_printf("[Tips_uartZigb]: Zigbee nwkCreat step:%d complete !!!\n", loop);
				{
					memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
					snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zbNwkcreat step:%d.\n", loop);
					osDelay(10);
					DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
				}
			}
		}
	}

//	os_printf("[Tips_uartZigb]: Zigbee nwkCreat all complete !!!\n");
	{
		memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
		snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zbNwkcreat done.\n");
		osDelay(10);
		DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
	}
	
	result_Set = zigb_clusterSet(ZIGB_CLUSTER_DEFAULT_DEVID, ZIGB_ENDPOINT_CTRLSECENARIO); //
	if(result_Set)result_Set =zigb_clusterSet(ZIGB_CLUSTER_DEFAULT_DEVID, ZIGB_ENDPOINT_CTRLNORMAL); //
	if(result_Set)result_Set = zigb_clusterSet(ZIGB_CLUSTER_DEFAULT_DEVID, ZIGB_ENDPOINT_CTRLSYSZIGB); //
//	if(result_Set)result_Set = zigb_clusterCtrlEachotherCfg();

	return result_Set;
}

LOCAL bool ZigB_datsRemoteRX(datsAttr_ZigbTrans *datsRX, u32 timeWait){
	
#define zigbcmdRX_Len 2
	const u8 cmdRX[zigbcmdRX_Len][2] = {
	
		{0x44,0x81},	//
		{0x45,0xCA},	//
	};

	u8 *ptr = NULL;
	u8 loop = 0;
	
	sttUartZigbRcv_rmoteDatComming *rptr_uartDatsRcv;
	osEvent evt;

	datsRX->datsType = zigbTP_NULL;

	evt = osMessageGet(mqID_uartZigbToutDats_dataRemoteComing, 0);
	if(evt.status == osEventMessage){
		
		rptr_uartDatsRcv= evt.value.p;
	
		for(loop = 0;loop < zigbcmdRX_Len;loop ++){
		
			ptr = memmem(rptr_uartDatsRcv->dats, rptr_uartDatsRcv->datsLen, (u8 *)cmdRX[loop], 2);
			
			if(ptr != NULL){
			
				switch(loop){
				
					case 0:{
					
						if(ZIGB_FRAME_HEAD == *(ptr - 2) && 
						   rptr_uartDatsRcv->dats[rptr_uartDatsRcv->datsLen - 1] == XOR_CHECK(&rptr_uartDatsRcv->dats[1], rptr_uartDatsRcv->datsLen - 2)){		
						   
							datsRX->datsSTT.stt_MSG.ifBroadcast = *(ptr + 10);
							datsRX->datsSTT.stt_MSG.Addr_from	= (((u16)*(ptr + 6) & 0x00FF) << 0) + (((u16)*(ptr + 7) & 0x00FF) << 8);
							datsRX->datsSTT.stt_MSG.srcEP		= *(ptr + 8);
							datsRX->datsSTT.stt_MSG.dstEP		= *(ptr + 9);
							datsRX->datsSTT.stt_MSG.ClusterID	= (((u16)*(ptr + 4) & 0x00FF) << 0) + (((u16)*(ptr + 5) & 0x00FF) << 8);
							datsRX->datsSTT.stt_MSG.LQI 		= *(ptr + 11);
							datsRX->datsSTT.stt_MSG.datsLen 	= *(ptr + 18);
							memset(datsRX->datsSTT.stt_MSG.dats, 0, datsRX->datsSTT.stt_MSG.datsLen * sizeof(u8));
							memcpy(datsRX->datsSTT.stt_MSG.dats, (ptr + 19), *(ptr + 18));
							
							datsRX->datsType = zigbTP_MSGCOMMING;
							return true;
						}
						else{
						   
							rptr_uartDatsRcv->dats[memloc((u8 *)rptr_uartDatsRcv->dats, rptr_uartDatsRcv->datsLen, (u8 *)cmdRX[loop], 2)] = 0xFF;	//
							loop --;	//
						}
					}break;
					
					case 1:{
					
						if(ZIGB_FRAME_HEAD == *(ptr - 2) && //
						   rptr_uartDatsRcv->dats[rptr_uartDatsRcv->datsLen - 1] == XOR_CHECK(&rptr_uartDatsRcv->dats[1], rptr_uartDatsRcv->datsLen - 2)){
						
							
							datsRX->datsSTT.stt_ONLINE.nwkAddr_fresh = (((u16)*(ptr + 2) & 0x00FF) << 0) + (((u16)*(ptr + 3) & 0x00FF) << 8);
							
							datsRX->datsType = zigbTP_ntCONNECT;
							return true;
						}
						else{
						   
							rptr_uartDatsRcv->dats[memloc((u8 *)rptr_uartDatsRcv->dats, rptr_uartDatsRcv->datsLen, (u8 *)cmdRX[loop], 2)] = 0xFF;	//
							loop --;	//
						}
					}break;
					
					default:break;
				}
			}
		}
		
		osPoolFree(ZigbUartMsgDataRemoteComing_poolAttr_id, rptr_uartDatsRcv);
	}
	
	return false;
}

LOCAL bool ZigB_PANIDReales(bool inspection_IF){ //

//	stt_usrDats_privateSave *datsRead_Temp = devParam_flashDataRead();
	stt_usrDats_privateSave datsRead_samp = {0};
	stt_usrDats_privateSave *datsRead_Temp = &datsRead_samp;
	stt_usrDats_privateSave datsSave_Temp = {0};
	u16 panID_temp = ZigB_getPanIDCurrent();

	bool result = false; 
	static u8 inspectionFail_count = 0; //

//	os_printf("PANID_current is : %d.\n", panID_temp);
//	os_printf("PANID_flash is : %d.\n", datsRead_Temp->panID_default);

	{
		memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
		snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zbPanid c[%04X], f[%04X].\n", panID_temp,
																					  datsRead_Temp->panID_default);
		osDelay(10);
		DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
	}
	
//	datsRead_Temp->panID_default = 0;

	if((datsRead_Temp->panID_default != panID_temp) || //
	   (!datsRead_Temp->panID_default) || //
	   (inspectionFail_count >= 3)){ //

		inspectionFail_count = 0;

		if((!datsRead_Temp->panID_default) || (datsRead_Temp->panID_default == 0xFFFF))datsSave_Temp.panID_default = sysZigb_random; //
		else{

			datsSave_Temp.panID_default = datsRead_Temp->panID_default;
		}

		nwkZigb_currentPANID = datsSave_Temp.panID_default;
		panID_temp = datsSave_Temp.panID_default;
//		devParam_flashDataSave(obj_panID_default, datsSave_Temp);
		
		result = ZigB_NwkCreat(panID_temp, 4);
		
	}else{

		nwkZigb_currentPANID = datsRead_Temp->panID_default; //

		if(inspection_IF){

			result = ZigB_inspectionSelf(1);
			
		}else{

			result = true;
		}
	}

//	if(datsRead_Temp)os_free(datsRead_Temp);

	if(result){

//		os_printf("panID reales success.\n");
		inspectionFail_count = 0;
	}
	else{

//		os_printf("panID reales fail.\n");
		inspectionFail_count ++;
	}

	return result;
}

LOCAL void ZigB_remoteDatsSend_straightforward(u16 DstAddr, //
											    u8 port, //
											    u8 dats[], //
					                            u8 datsLen){ //

	const u8 zigbProtocolCMD_dataSend[2] = {0x24,0x01};
	const u8 TransID = 13;
	const u8 Option	 = 0;
	const u8 Radius	 = 7;

	u8 buf_datsTX[32] = {0};
	u8 datsTX[48] = {0};
	u8 datsTX_Len = 0;

	buf_datsTX[0] = (uint8_t)((DstAddr & 0x00ff) >> 0);
	buf_datsTX[1] = (uint8_t)((DstAddr & 0xff00) >> 8);
	buf_datsTX[2] = port;
	buf_datsTX[3] = port;
	buf_datsTX[4] = ZIGB_CLUSTER_DEFAULT_CULSTERID;
	buf_datsTX[6] = TransID;
	buf_datsTX[7] = Option;
	buf_datsTX[8] = Radius;
	buf_datsTX[9] = datsLen;
	memcpy(&buf_datsTX[10],dats,datsLen);	

	datsTX_Len = ZigB_TXFrameLoad(datsTX, (u8 *)zigbProtocolCMD_dataSend, 2, (u8 *)buf_datsTX, datsLen + 10);

	uartDrvObj_local->Send(datsTX, datsTX_Len);
}

LOCAL bool ZigB_datsTX_ASY(uint16_t DstAddr,
						   uint8_t  SrcPoint,
						   uint8_t  DstPoint,
						   uint8_t 	ClustID,
						   uint8_t  dats[],
						   uint8_t  datsLen,
						   stt_dataRemoteReq bufQueue[],
						   uint8_t  BQ_LEN){

	const u8 TransID = 13;
	const u8 Option	 = 0;
	const u8 Radius	 = 7;

	const u8 cmd_dataReq[2] = {0x24, 0x01};
	const u8 cmd_dataResp[2] = {0x44, 0x80};

#define zigbTX_datsTransLen_ASR (128 + 25)
	uint8_t buf_datsTX[zigbTX_datsTransLen_ASR] = {0};
#define zigbRX_datsTransLen_ASR (8 + 25)
	uint8_t buf_datsRX[zigbRX_datsTransLen_ASR] = {0};

	u8 loop = 0;

	for(loop = 0; loop < BQ_LEN; loop ++){

		if(!bufQueue[loop].repeat_Loop){

			memset(bufQueue[loop].dataReq, 0, sizeof(u8) * zigbTX_datsTransLen_ASR);
			memset(bufQueue[loop].dataResp, 0, sizeof(u8) * 8);

			buf_datsTX[0] = (uint8_t)((DstAddr & 0x00ff) >> 0);
			buf_datsTX[1] = (uint8_t)((DstAddr & 0xff00) >> 8);
			buf_datsTX[2] = DstPoint;
			buf_datsTX[3] = SrcPoint;
			buf_datsTX[4] = ClustID;
			buf_datsTX[6] = TransID;
			buf_datsTX[7] = Option;
			buf_datsTX[8] = Radius;
			buf_datsTX[9] = datsLen;
			memcpy(&buf_datsTX[10], dats, datsLen);
			bufQueue[loop].dataReq_Len = ZigB_TXFrameLoad(bufQueue[loop].dataReq, (u8 *)cmd_dataReq, 2, (u8 *)buf_datsTX, datsLen + 10);

			buf_datsRX[0] = 0x00;
			buf_datsRX[1] = SrcPoint;
			buf_datsRX[2] = TransID;

//			bufQueue[loop].dataResp_Len = 3;
			bufQueue[loop].dataResp_Len = ZigB_TXFrameLoad(bufQueue[loop].dataResp, (u8 *)cmd_dataResp, 2, (u8 *)buf_datsRX, 3);

			bufQueue[loop].repeat_Loop = zigB_remoteDataTransASY_txReapt; //

			if(!memcmp(bufQueue[loop].dataReq, bufQueue[loop - 1].dataReq, bufQueue[loop].dataReq_Len) && 
			   !memcmp(bufQueue[loop].dataResp, bufQueue[loop - 1].dataResp, bufQueue[loop].dataResp_Len)){ //

				memset(&bufQueue[loop - 1], 0, sizeof(stt_dataRemoteReq));
			}

			return true;
		}
	}

//	os_printf(">>>dataRM reqQ full.\n"); //
	
	memcpy(&localZigbASYDT_bufQueueRemoteReq[0], &localZigbASYDT_bufQueueRemoteReq[zigB_remoteDataTransASY_QbuffLen / 2], sizeof(stt_dataRemoteReq) * zigB_remoteDataTransASY_QbuffLen / 2); //
	memset(&localZigbASYDT_bufQueueRemoteReq[zigB_remoteDataTransASY_QbuffLen / 2], 0, sizeof(stt_dataRemoteReq) * zigB_remoteDataTransASY_QbuffLen / 2); //

	return false; 
}

static /*链表优化定时器回调函数*/
void TimerZigbDevManage_Callback(void const *arg){
	
	const u8 dispLen = 150;
	char disp[dispLen];

	nwkStateAttr_Zigb *pHead_listDevInfo = (nwkStateAttr_Zigb *)arg;
	
	zigbDev_delSame(pHead_listDevInfo);		//链表优化，去重
	
//	DbugP1TX("ZigBdev Online detect counting tips\r\n",37);	
//	osDelay(20);
	
	if(0 == zigbDev_chatLenDectect(pHead_listDevInfo)){	//表内无节点，直接返回
		
		osDelay(10);
		return;
	}

	while(pHead_listDevInfo->next != NULL){
	
		pHead_listDevInfo = pHead_listDevInfo->next;
		
		if(pHead_listDevInfo->onlineDectect_LCount > 0)pHead_listDevInfo->onlineDectect_LCount --;
		else{
			
			u16 p_nwkRemove = pHead_listDevInfo->nwkAddr;
			
			while(false == zigbDev_eptRemove_BYnwk((nwkStateAttr_Zigb *)arg,p_nwkRemove));
		
			memset(disp,0,dispLen * sizeof(char));
			snprintf(disp,dispLen,"zigb node[%04X] hbDs rmv.\r\n",p_nwkRemove);
			DbugP1TX(disp,strlen((char *)disp));	
			osDelay(20);
		}
	}
}

static void zigbThreadPrt_uartDataRcv_process(void){

	osEvent evt;
	static uint32_t uartDataRcv_len = 0;
	static bool uartRcv_startFlg = false;
	static uint8_t uartDataRcv_temp[UART_DATA_RCV_TEMP_LENGTH] = {0};
	uint16_t uartBuff_rcvLength = 0;
	
	if(!uartRcv_startFlg){
	
		uartDrvObj_local->Receive(uartDataRcv_temp, UART_DATA_RCV_TEMP_LENGTH);
		uartRcv_startFlg = true;
	}
	
	evt = osSignalWait(localSigVal_uartWifi_rxTout, 0);
	if(osEventSignal == evt.status){
		
		uartDrvObj_local->Control(ARM_USART_ABORT_RECEIVE, 0);
		
		uartBuff_baseInsert =  uartBuff_baseInsertStart;
		uartBuff_rcvLength = uartBuff_baseInsertStart + uartDrvObj_local->GetRxCount();
		memcpy(&uartDataZigbRcv_temp[uartBuff_baseInsertStart], uartDataRcv_temp, uartDrvObj_local->GetRxCount());

//		{
//			memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
//			snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "uartZigb dataRcv[len:%d]:%02X, %02X, %02X.\n", uartDrvObj_local->GetRxCount(),
//																										   uartDataRcv_temp[0],
//																										   uartDataRcv_temp[1],
//																										   uartDataRcv_temp[2]);
//			osDelay(20);
//			DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
//		}
		
		{
			const u8 cmd_remoteDataComing[2] 	= {0x44, 0x81};
			const u8 cmd_remoteNodeOnline[2] 	= {0x45, 0xCA};
			const u8 cmd_rmdataReqResp[2] 		= {0x44, 0x80}; 
			u16 frameNum_reserve = uartBuff_rcvLength;
			u16 frameHead_insert = 0; //
			u8 frameParsing_num = 0; //
			u8 frameTotal_Len = 0; //

			const u8 dataProcess_loopLimit = 40; //
			u8 dataProcess_loopout = 0;  //
			
			while(frameNum_reserve){
				
				frameTotal_Len = uartDataZigbRcv_temp[frameHead_insert + 1] + 5; //
			
				if((uartDataZigbRcv_temp[frameHead_insert] == ZIGB_FRAME_HEAD) && (frameNum_reserve >= frameTotal_Len)){ //

					frameParsing_num ++;

					if(!memcmp(&uartDataZigbRcv_temp[frameHead_insert + 2], cmd_remoteDataComing, 2) || !memcmp(&uartDataZigbRcv_temp[frameHead_insert + 2], cmd_remoteNodeOnline, 2)){ //

						if(uartDataZigbRcv_temp[frameHead_insert + 21] == ZIGB_FRAMEHEAD_CTRLLOCAL && uartDataZigbRcv_temp[frameHead_insert + 24] == FRAME_MtoSCMD_cmdConfigSearch){ //

							sttUartZigbRcv_rmoteDatComming *mptr_rmoteDatComming = (sttUartZigbRcv_rmoteDatComming *)osPoolAlloc(ZigbUartMsgDataRemoteComing_poolAttr_id);
							
							memcpy(mptr_rmoteDatComming->dats, &uartDataZigbRcv_temp[frameHead_insert], frameTotal_Len);
							mptr_rmoteDatComming->datsLen = frameTotal_Len;
							
							osMessagePut(mqID_uartZigbToutDats_dataRemoteComing, (uint32_t)mptr_rmoteDatComming, 0);
							
						}else
						if((uartDataZigbRcv_temp[frameHead_insert + 10] > CTRLEATHER_PORT_NUMSTART) && (frameTotal_Len == 27)){ //

//							mptr_ctrlEachDat.prot_Ep = uartDataZigbRcv_temp[frameHead_insert + 10]; //
//							mptr_ctrlEachDat.ctrlEachOther_dat = uartDataZigbRcv_temp[frameHead_insert + 21]; //
//							mptr_ctrlEachDat.ctrlEachOther_loop = uartDataZigbRcv_temp[frameHead_insert + 22]; //
//							xQueueSend(xMsgQ_uartToutDats_datsComingCtrlEach, (void *)&mptr_ctrlEachDat, ( portTickType ) 0);
							
						}else
						if(uartDataZigbRcv_temp[frameHead_insert + 21] == CTRLSECENARIO_RESPCMD_SPECIAL && frameTotal_Len == 26){ //

//							mptr_scenarioCtrlResp.respNwkAddr = (((u16)uartDataZigbRcv_temp[frameHead_insert + 8] & 0x00FF) << 0) | (((u16)uartDataZigbRcv_temp[frameHead_insert + 9] & 0x00FF) << 8); //
//							xQueueSend(xMsgQ_uartToutDats_scenarioCtrlResp, (void *)&mptr_scenarioCtrlResp, ( portTickType ) 0);
//							os_printf("Q_s push.\n");
							
						}else{

//							xQueueSend(xMsgQ_uartToutDats_dataRemoteComing, (void *)&mptr_rmoteDatComming, ( portTickType ) 0);
						}

//							os_printf("Q_r push.\n");
						
					}
					else
					if(!memcmp(&uartDataZigbRcv_temp[frameHead_insert + 2], cmd_rmdataReqResp, 2)){ //
					
						sttUartZigbRcv_rmDatsReqResp *mptr_rmDatsReqResp = (sttUartZigbRcv_rmDatsReqResp *)osPoolAlloc(ZigbUartMsgRmDataReqResp_poolAttr_id);
						
						memcpy(mptr_rmDatsReqResp->dats, &uartDataZigbRcv_temp[frameHead_insert], frameTotal_Len);
						mptr_rmDatsReqResp->datsLen = frameTotal_Len;

						osMessagePut(mqID_uartZigbToutDats_rmDataReqResp, (uint32_t)mptr_rmDatsReqResp, 0);

					}else{ //
						
						sttUartZigbRcv_sysDat *mptr_sysDat = (sttUartZigbRcv_sysDat *)osPoolAlloc(ZigbUartMsgDataSysRespond_poolAttr_id);
					
						memcpy(mptr_sysDat->dats, &uartDataZigbRcv_temp[frameHead_insert], frameTotal_Len);
						mptr_sysDat->datsLen = frameTotal_Len;

						osMessagePut(mqID_uartZigbToutDats_dataSysRespond, (uint32_t)mptr_sysDat, 0);

//						os_printf("Q_s push.\n");

					}

					frameHead_insert += frameTotal_Len; //
					frameNum_reserve -= frameTotal_Len; //
					uartBuff_baseInsertStart = 0; //
					
				}
				else{

//					os_printf("tout_err.\n");
					if(frameNum_reserve > 0){

						if(uartDataZigbRcv_temp[frameHead_insert] == ZIGB_FRAME_HEAD){ //

							memcpy(uartDataZigbRcv_temp, &uartDataZigbRcv_temp[frameHead_insert], frameNum_reserve); //
							uartBuff_baseInsertStart = frameNum_reserve; //
							frameNum_reserve = 0;
							
						}else{ //

							u8 	loop = 0;
							bool frameHead_bingo = false;
							for(loop = 1; loop < frameTotal_Len; loop ++){ //

								if(uartDataZigbRcv_temp[frameHead_insert - loop] == ZIGB_FRAME_HEAD){

									frameHead_bingo = true;
									break;
								}
							}
							if(frameHead_bingo == true){

								frameHead_insert -= loop; //
								frameNum_reserve += loop; //
								continue;
								
							}else{

								uartBuff_baseInsertStart = 0; //
								frameNum_reserve = 0; //
							}	
						}

//						os_printf(">>>>>>>>>errFrame tail:%02X-%02X-%02X, legal_impStart:%04X.\n",	uartDataZigbRcv_temp[uartBuff_rcvLength - 3],
//																									uartDataZigbRcv_temp[uartBuff_rcvLength - 2],
//																									uartDataZigbRcv_temp[uartBuff_rcvLength - 1],
//																									uartBuff_baseInsertStart);
					}
				}				

				if(dataProcess_loopout < dataProcess_loopLimit){ //
				
					dataProcess_loopout ++;
//					if(dataProcess_loopout % 10)system_soft_wdt_feed();
				}
				else{

					uartBuff_baseInsertStart = 0; //
					frameNum_reserve = 0; //
					break;
				}
			}
		}
		
		uartRcv_startFlg = false;
	}
}

static void uartZigbDataRcvHandle_Thread(const void *argument){

	tid_zigbUartDatsRcvThread = osThreadGetId();
	
	for(;;){
	
		zigbThreadPrt_uartDataRcv_process();
	}
}

static void threadProcessZigb_initialization(type_uart_OBJ UART_OBJ){
	
	ZigbnwkState_poolAttr_id 				= osPoolCreate(osPool(nwkZigbDevStateAttr_pool));	//zigbee节点设备信息 数据结构堆内存池初始化
	mqID_threadDP_Z2W 					 	= osMessageCreate(osMessageQ(MsgBox_threadDP_Z2W), NULL);	//zigbee到wifi消息队列初始化
	
	ZigbFuncRemind_poolAttr_id 				= osPoolCreate(osPool(ZigbFuncRemind_pool));	//zigbee节点设备信息 数据结构堆内存池初始化
	mqID_zigbFuncRemind 					= osMessageCreate(osMessageQ(MsgBox_zigbFuncRemind), NULL);	//zigbee到wifi消息队列初始化
	
	ZigbUartMsgDataSysRespond_poolAttr_id	= osPoolCreate(osPool(ZigbUartMsgDataSysRespond_pool));
	mqID_uartZigbToutDats_dataSysRespond 	= osMessageCreate(osMessageQ(MsgBox_uartZigbToutDats_dataSysRespond), NULL);
	ZigbUartMsgDataRemoteComing_poolAttr_id	= osPoolCreate(osPool(ZigbUartMsgDataRemoteComing_pool));
	mqID_uartZigbToutDats_dataRemoteComing 	= osMessageCreate(osMessageQ(MsgBox_uartZigbToutDats_dataRemoteComing), NULL);
	ZigbUartMsgRmDataReqResp_poolAttr_id	= osPoolCreate(osPool(ZigbUartMsgRmDataReqResp_pool));
	mqID_uartZigbToutDats_rmDataReqResp 	= osMessageCreate(osMessageQ(MsgBox_uartZigbToutDats_rmDataReqResp), NULL);
	
	usrApp_usartEventCb_register(UART_OBJ, uartConZigb_eventFunCb); //串口回调注册
	tid_uartZigbDataRcvHandle_Thread = osThreadCreate(osThread(uartZigbDataRcvHandle_Thread), NULL);
	
	tid_zigbThread = osThreadGetId();
	uartObj_current = UART_OBJ;
	localSigVal_uartWifi_rxTout = uartRxTimeout_signalDefGet(UART_OBJ);
	uartDrvObj_local = uartDrvObjGet(UART_OBJ);
	
	osDelay(10);
	DbugP1TX("uartZigb init.\n", 40);
}

void zigbFunction_callFromThread(enum_zigbFunMsg funcType){

	if(!nwkZigbOnline_IF)return;
	else{
	
		stt_zigbFunMsg *mptr_zigbFuncRemind = (stt_zigbFunMsg *)osPoolAlloc(ZigbFuncRemind_poolAttr_id);
		
		mptr_zigbFuncRemind->funcType = funcType;
		osMessagePut(mqID_zigbFuncRemind, (uint32_t)mptr_zigbFuncRemind, 0);
	}
}

/*zigbee主线程*/
void ZigB_mainThread(type_uart_OBJ UART_OBJ){

	osEvent evt;
	
	u8 	local_insertRecord_datsReqNormal = 0;
	
	nwkStateAttr_Zigb *zigbDevList_Head = (nwkStateAttr_Zigb *)osPoolAlloc(ZigbnwkState_poolAttr_id);
	
#define zigB_datsTX_bufferLen 		128	//zigB数据发送缓存
	u8 	datsKernel_TXbuffer[zigB_datsTX_bufferLen];
	
	threadProcessZigb_initialization(UART_OBJ); //任务线程资源初始化
	
	osDelay(100);
	while(!ZigB_resetInit());
	
	osDelay(100);
	{
		memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
		snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zigb rand r res:%d.\n", ZigB_getRandom());
		osDelay(10);
		DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
	}
	osDelay(100);
	{
		memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
		snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zigb ieee2Mac res:%d.\n", ZigB_getIEEEAddr());
		osDelay(10);
		DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
	}
	
	osDelay(100);
	while(!ZigB_PANIDReales(1));
	
	nwkZigbOnline_IF = true;
	
	for(;;){
		
		if(nwkZigbOnline_IF){
			
			{ /*设备统计链表管理*/
			
				if(zigbNodeDevDetectManage_runningEN && zigbNodeDevDetectManage_runningFLG){
					
					nwkStateAttr_Zigb *pHead_listDevInfo = NULL;
				
					if(zigbNodeDevDetectManage_runningFLG)zigbNodeDevDetectManage_runningFLG = false;
				
					pHead_listDevInfo = (nwkStateAttr_Zigb *)zigbDevList_Head;
					if(zigbDev_chatLenDectect(pHead_listDevInfo)){
					
						while(pHead_listDevInfo->next != NULL){
						
							pHead_listDevInfo = pHead_listDevInfo->next;
							
							if(pHead_listDevInfo->onlineDectect_LCount > 0)pHead_listDevInfo->onlineDectect_LCount --; //
							else{
								
								u16 p_nwkRemove = pHead_listDevInfo->nwkAddr;
					
								zigbNwkReserveNodeNum_currentValue --; //
					
//								os_printf("[Tips_uartZigb]: nodeDev remove result is %d\n", zigbDev_eptRemove_BYnwk((nwkStateAttr_Zigb *)zigbDevList_Head, p_nwkRemove));
//					
//								os_printf("[Tips_uartZigb]: nodeDev(nwkAddr: 0x%04X) has been optimized cause inactive.\n", p_nwkRemove);

//								(zigbNwkReserveNodeNum_currentValue)?(ZigBdevListInfoSaveToFlash(zigbDevList_Head)):(ZigBdevListInfoSaveToFlash(NULL)); //
							}
						}
					}
				}
			}
			
			{ /*周期更新PANDID缓存*/
			
				if(!zigbee_currentPanID_reslesCounter){

					zigbee_currentPanID_reslesCounter = ZIGBPANID_CURRENT_REALESPERIOD;
					nwkZigb_currentPANID = ZigB_getPanIDCurrent();
				}
			}
		
			{ /*系统功能响应*/
			
				stt_zigbFunMsg *rptr_zigbFunRm = NULL;
				
				evt = osMessageGet(mqID_zigbFuncRemind, 0);
				if((evt.status == osEventMessage)){
				
					frame_zigbSysCtrl datsTemp_zigbSysCtrl = {0}; //
					bool nodeCMDtranslate_EN = false; //
					
					rptr_zigbFunRm = evt.value.p;

					switch(rptr_zigbFunRm->funcType){

						case msgFun_nwkOpen:{ //

							zigbNetwork_OpenIF(1, ZIGBNWKOPENTIME_DEFAULT); //

						}break;
						
						case msgFun_rootSystimeReales:{
						
							mcu_get_system_time();
							
//							{
//								memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
//								snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zb timeSys reales.\n");
//								osDelay(10);
//								DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
//							}
						
						}break;

						case msgFun_nodeSystimeSynchronous:{ //

							uint32_t timeStmap_temp = 0UL;

							timeStmap_temp = systemUTC_current;
							
							datsTemp_zigbSysCtrl.command = ZIGB_SYSCMD_TIMESET;
							datsTemp_zigbSysCtrl.dats[0] = (u8)((timeStmap_temp & 0x000000FF) >> 0); //
							datsTemp_zigbSysCtrl.dats[1] = (u8)((timeStmap_temp & 0x0000FF00) >> 8);
							datsTemp_zigbSysCtrl.dats[2] = (u8)((timeStmap_temp & 0x00FF0000) >> 16);
							datsTemp_zigbSysCtrl.dats[3] = (u8)((timeStmap_temp & 0xFF000000) >> 24);
							datsTemp_zigbSysCtrl.dats[4] = (u8)(sysTimeZone_H); //
							datsTemp_zigbSysCtrl.dats[5] = (u8)(sysTimeZone_M); //
							datsTemp_zigbSysCtrl.dats[6] = 0; //
							datsTemp_zigbSysCtrl.datsLen = 6;
							
							nodeCMDtranslate_EN = true;
							
//							{
//								memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
//								snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zb sysTime syn.\n");
//								osDelay(10);
//								DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
//							}
						
						}break;

						case msgFun_localSystimeZigbAdjust:{ //

							zigB_sysTimeGetRealesWithLocal();
							nodeCMDtranslate_EN = false;
							
//							{
//								memset(printfLog_bufTemp, 0, DBLOB_BUFTEMP_LEN * sizeof(char));
//								snprintf(printfLog_bufTemp, DBLOB_BUFTEMP_LEN, "zb timeSys cali.\n");
//								osDelay(10);
//								DbugP1TX(printfLog_bufTemp,strlen((char *)printfLog_bufTemp));	
//							}
							
						}break;

						case msgFun_portCtrlEachoRegister:{ //

							nodeCMDtranslate_EN = false;
						
						}break;

						case msgFun_panidRealesNwkCreat:{ //

							ZigB_PANIDReales(false);
							nodeCMDtranslate_EN = false;
						
						}break;

						case msgFun_scenarioCrtl:{ //
							
							nodeCMDtranslate_EN = false;

						}break;

						case msgFun_dtPeriodHoldPst:{

							datsTemp_zigbSysCtrl.command = ZIGB_SYSCMD_DATATRANS_HOLD;
							datsTemp_zigbSysCtrl.dats[0] = 15; //
							datsTemp_zigbSysCtrl.datsLen = 1;
							
							nodeCMDtranslate_EN = true;
							
						}break;

						case msgFun_dtPeriodHoldCancelAdvance:{

							datsTemp_zigbSysCtrl.command = ZIGB_SYSCMD_DATATRANS_HOLD;
							datsTemp_zigbSysCtrl.dats[0] = 1; //
							datsTemp_zigbSysCtrl.datsLen = 1;
							
							nodeCMDtranslate_EN = true;

						}break;

						default:{

							nodeCMDtranslate_EN = false;
								
						}break;
					}

					if(nodeCMDtranslate_EN){

						bool TXCMP_FLG = false;

						memset(datsKernel_TXbuffer, 0, sizeof(u8) * zigB_datsTX_bufferLen);
						ZigB_sysCtrlFrameLoad(datsKernel_TXbuffer, datsTemp_zigbSysCtrl);

						TXCMP_FLG = ZigB_datsTX_ASY(0xFFFF, 
													ZIGB_ENDPOINT_CTRLSYSZIGB,
													ZIGB_ENDPOINT_CTRLSYSZIGB,
													ZIGB_CLUSTER_DEFAULT_CULSTERID,
													(u8 *)datsKernel_TXbuffer,
													2 + datsTemp_zigbSysCtrl.datsLen, //
													localZigbASYDT_bufQueueRemoteReq,
													zigB_remoteDataTransASY_QbuffLen);
					}
					
					osPoolFree(ZigbFuncRemind_poolAttr_id, rptr_zigbFunRm);
				}
			}
			
			{ /*wifi数据处理*/
				
				stt_threadDatsPass *rptr_W2Z = NULL;
				
				evt = osMessageGet(mqID_threadDP_W2Z, 0);
				if(evt.status == osEventMessage){
				
					rptr_W2Z = evt.value.p;
					
					switch(rptr_W2Z->msgType){
		
						case conventional:{
		
							u16 zigb_nwkAddrTemp = 0xFFFF;
							bool TXCMP_FLG		 = false;
		
							if((rptr_W2Z->dats.dats_conv.dats[3] == FRAME_MtoSCMD_cmdConfigSearch) && //
							   (rptr_W2Z->dats.dats_conv.datsFrom == datsFrom_ctrlLocal) ){	
							   
								{

									const  u8 localPeriod_nwkTrig = 1;
									static u8 localCount_searchREQ = 2;

									if(localCount_searchREQ < localPeriod_nwkTrig)localCount_searchREQ ++;
									else{ //

										localCount_searchREQ = 0;
										if(!deviceLock_flag)zigbFunction_callFromThread(msgFun_nwkOpen); //
									}
								}

								//os_printf("[Tips_ZIGB-NWKmsg]: rcvMsg local cmd: %02X !!!\n", rptr_W2Z->dats.dats_conv.dats[3]);
								zigb_nwkAddrTemp = 0xFFFF;
								
							}
							else{
		
								nwkStateAttr_Zigb *infoZigbDevRet_temp = zigbDev_eptPutout_BYpsy(zigbDevList_Head, rptr_W2Z->dats.dats_conv.macAddr, DEVZIGB_DEFAULT, false);
								if(NULL != infoZigbDevRet_temp){
								
									zigb_nwkAddrTemp =	infoZigbDevRet_temp->nwkAddr;
									osPoolFree(ZigbnwkState_poolAttr_id, infoZigbDevRet_temp);
								
								}else{
		
									zigb_nwkAddrTemp = 0;
									//os_printf(">>>insert list fail:with mac: %02X %02X %02X %02X %02X.\n", rptr_W2Z->dats.dats_conv.macAddr[0],
									//																		 rptr_W2Z->dats.dats_conv.macAddr[1],
									//																		 rptr_W2Z->dats.dats_conv.macAddr[2],
									//																		 rptr_W2Z->dats.dats_conv.macAddr[3],
									//																		 rptr_W2Z->dats.dats_conv.macAddr[4]);
								}
							}
		
							if(zigb_nwkAddrTemp){
		
								memset(datsKernel_TXbuffer, 0, sizeof(u8) * zigB_datsTX_bufferLen);
								memcpy(datsKernel_TXbuffer, rptr_W2Z->dats.dats_conv.dats, rptr_W2Z->dats.dats_conv.datsLen);
							
								TXCMP_FLG = ZigB_datsTX_ASY(zigb_nwkAddrTemp,	
															ZIGB_ENDPOINT_CTRLNORMAL,
															ZIGB_ENDPOINT_CTRLNORMAL,
															ZIGB_CLUSTER_DEFAULT_CULSTERID,
															(u8 *)datsKernel_TXbuffer,
															rptr_W2Z->dats.dats_conv.datsLen,
															localZigbASYDT_bufQueueRemoteReq,
															zigB_remoteDataTransASY_QbuffLen);
							}
							
						}break;
		
						case listDev_query:{
		
		
						}break;
		
						default:{
		
		
						}break;
					}
					
					osPoolFree(threadDP_poolAttr_id, rptr_W2Z);
				}
				else
				{

				}
			}
			
			{ /*非阻塞远端数据传输可用执行*/
			
				sttUartZigbRcv_rmDatsReqResp *rptr_uartDatsRcv = NULL;
				u8 loop_Insert_temp = 0; //
				
				do{ //

					evt = osMessageGet(mqID_uartZigbToutDats_rmDataReqResp, 0);
					if(evt.status == osEventMessage){

						rptr_uartDatsRcv= evt.value.p;
						
						//os_printf(">>>:Qcome: Len-%02d, H-%02X, T-%02X.\n", rptr_uartDatsRcv->datsLen, rptr_uartDatsRcv->dats[0], rptr_uartDatsRcv->dats[rptr_uartDatsRcv->datsLen - 1]);

						while(loop_Insert_temp < zigB_remoteDataTransASY_QbuffLen){ //

							if( localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].repeat_Loop){

								if(!memcmp(rptr_uartDatsRcv->dats, localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp, localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp_Len)){

									localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].repeat_Loop = 0; //
									memset(localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp, 0, sizeof(u8) * localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp_Len); //
									memcpy(&localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp], &localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp + 1], (zigB_remoteDataTransASY_QbuffLen - loop_Insert_temp - 1) * sizeof(stt_dataRemoteReq)); //

									//os_printf("bingo.\n");

									break; //
									
								}else{

									//os_printf(">>>:mShud: Len-%02d, h-%02X, t-%02X.\n", localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp_Len, localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp[0], localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp[localZigbASYDT_bufQueueRemoteReq[loop_Insert_temp].dataResp_Len - 1]);
								}
							}

							loop_Insert_temp ++;	
						}
						
						loop_Insert_temp = 0;  //
						
						osPoolFree(ZigbUartMsgRmDataReqResp_poolAttr_id, rptr_uartDatsRcv);
					}
					
				}
				while(evt.status == osEventMessage);

				if(local_insertRecord_datsReqNormal < zigB_remoteDataTransASY_QbuffLen){ //

					if(!localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReqPeriod && localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].repeat_Loop){
					
						localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReqPeriod = zigB_remoteDataTransASY_txPeriod; //
						localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].repeat_Loop --; //
					
						uartDrvObj_local->Send(localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReq, localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReq_Len);
						osDelay(zigB_remoteDataTransASY_txUartOnceWait);

						if(!localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].repeat_Loop){

						//os_printf("preFail_rmDatatx warning, nwkAddr<0x%02X%02X>.\n", localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReq[5], localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReq[4]);

							if((localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReq[5] == 0xff) && //
							   (localZigbASYDT_bufQueueRemoteReq[local_insertRecord_datsReqNormal].dataReq[4] == 0xff)){

							   if(dataRemoteRequest_failCount < zigB_reconnectCauseDataReqFailLoop){
							   
								   dataRemoteRequest_failCount ++;
								   
							   }else{
							   
								   dataRemoteRequest_failCount = 0;
								   nwkZigbOnline_IF = false; //
							   }
							}
						}
					}
					
					local_insertRecord_datsReqNormal ++;
					
				}
				else{

					local_insertRecord_datsReqNormal = 0;
				}
			}
		}
		else
		{
			ZigB_nwkReconnect();
		}
	}
}

