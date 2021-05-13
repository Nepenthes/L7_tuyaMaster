#include "dataManage.h"

#include "UART_dataTransfer.h"

#define FLASH_TIMEOUT_VALUE   ((uint32_t)(60 * 1000))/* 50 s */

const char devType_strTab[8][10] = {

	"sw1","sw2","sw3","cur","dim","hea","fans","soc",
};

const u8 devRootNode_subidMac[5] = {0, 0, 0, 0, 1};
	
const u8 debugLogOut_targetMAC[5] = {0x20, 0x15, 0x08, 0x48, 0x86};

osPoolId  (normalMemUint8_t_poolAttr_id); // Memory pool ID
osPoolDef (normalMemUint8_t_pool, 1024, uint8_t);

char printfLog_bufTemp[DBLOB_BUFTEMP_LEN] = {0};

int8_t sysTimeZone_H = 0; 
int8_t sysTimeZone_M = 0; 
uint32_t systemUTC_current = 0;
u16	sysTimeKeep_counter	= 0;
stt_localTime systemTime_current = {0};

u8 zigbNwkReserveNodeNum_currentValue = 0;

u8 DEV_actReserve = 0x07; //有效操作位
u8 SWITCH_TYPE = SWITCH_TYPE_SWBIT3;	

bool deviceLock_flag = false;

u8 MACSTA_ID[DEV_MAC_LEN] = {9, 9, 9, 9, 9, 9};
u8 MACAP_ID[DEV_MAC_LEN] = {0};
u8 MACDST_ID[DEV_MAC_LEN] = {1,1,1,1,1,1}; //

static stt_usrDats_privateSave usrApp_localRecordDatas = {

	.devCurtain_orbitalPeriod = 33,
	.tuyaDevGroupParamTab[DEV_GROUP_NUM_MAX - 1].devNodeMacTab[DEV_GROUP_NODE_NUM_MAX - 1][0] = 77,
};

u8 switchTypeReserve_GET(void){

	u8 act_Reserve = 0x07;

	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT3){
		
		act_Reserve = 0x07;
		
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT2){
		
		act_Reserve = 0x05;
	
	}else
	if(SWITCH_TYPE == SWITCH_TYPE_SWBIT1){
	
		act_Reserve = 0x02;
	}
	
	return act_Reserve;
}

const char *typeStrgetByNum(uint8_t typeNum){

	const char *ptr = NULL;
	
	switch(typeNum){

		case SWITCH_TYPE_SWBIT1:	{ptr = devType_strTab[0];}break;
			
		case SWITCH_TYPE_SWBIT2:	{ptr = devType_strTab[1];}break;
		
		case SWITCH_TYPE_SWBIT3:	{ptr = devType_strTab[2];}break;
			
		case SWITCH_TYPE_CURTAIN:	{ptr = devType_strTab[3];}break;		

		case SWITCH_TYPE_DIMMER:	{ptr = devType_strTab[4];}break;
			
		case SWITCH_TYPE_FANS:		{ptr = devType_strTab[6];}break;
			
		case SWITCH_TYPE_SOCKETS:	{ptr = devType_strTab[7];}break;
			
		case SWITCH_TYPE_HEATER:	{ptr = devType_strTab[5];}break;
	}
	
	return ptr;
}

void usrApp_MEM_If_Init_FS(void){
 
    HAL_FLASH_Unlock(); 
//    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP    | FLASH_FLAG_OPERR  | FLASH_FLAG_WRPERR | 
//                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
 
}


void usrApp_MEM_If_DeInit_FS(void){
	
    HAL_FLASH_Lock();
}

uint32_t usrApp_GetFlashSector(uint32_t Address){
	
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_DEF_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_DEF_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_DEF_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_DEF_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_DEF_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_DEF_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_DEF_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_DEF_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_DEF_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_DEF_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_DEF_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_SECTOR_DEF_11;  
  }

  return sector;
}

void appFuncStr2Mac(uint8_t devMac[DEV_MAC_LEN], char *strmac){

	uint8_t loop = 0;
	int tarTmp[DEV_MAC_LEN] = {0};
	
	if(12 > strlen(strmac))return;
	for(loop = 0; loop < DEV_MAC_LEN; loop ++){
		sscanf(&strmac[loop * 2], "%02x", &tarTmp[loop]);
		devMac[loop] = tarTmp[loop];
	}
}

static uint8_t devMacTabOperate_nodeAdd(uint8_t groupIndex, uint8_t devNodeMac[DEV_MAC_LEN]){

	const uint8_t indexNull = 0xff;
	const uint8_t devMacNull[DEV_MAC_LEN] = {0};
	uint8_t res = groupAddErr_other;
	uint8_t loopA = 0;
	uint8_t macTabIndex_available = indexNull;
	
	if(groupIndex >= DEV_GROUP_NUM_MAX)return res;
	
	for(loopA = 0; loopA < DEV_GROUP_NODE_NUM_MAX; loopA ++){
	
		if(!memcmp(devNodeMac, usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab[loopA], sizeof(uint8_t) * DEV_MAC_LEN)){
			DBLOG_PRINTF("[GROUP DBG]same devNode detected, tabIndex:%d.\n", loopA);
			res = groupAddRes_ok;
			break;
		}else
		if(0 == memcmp(devMacNull, usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab[loopA], sizeof(uint8_t) * DEV_MAC_LEN)){
			if(indexNull == macTabIndex_available)macTabIndex_available = loopA;
		}
	}
	if(DEV_GROUP_NODE_NUM_MAX == loopA){
		
		if(indexNull != macTabIndex_available){
			memcpy(usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab[macTabIndex_available], devNodeMac, sizeof(uint8_t) * DEV_MAC_LEN);
			DBLOG_PRINTF("[GROUP DBG]new devNode created, tabIndex:%d.\n", macTabIndex_available);
			res = groupAddRes_ok;
		}else{
			DBLOG_PRINTF("[GROUP DBG]group devNode is full.\n");
			res = groupAddErr_overLoad;
		}
	}
	
	return res;
}

uint8_t devGroupOperate_paramAdd(uint32_t gid, uint8_t devNodeMac[DEV_MAC_LEN]){

	const uint8_t indexNull = 0xff;
	uint8_t loop = 0;
	uint8_t res = groupAddErr_other;
	uint8_t groupTabIndex_available = indexNull;
	
	for(loop = 0; loop < DEV_GROUP_NUM_MAX; loop ++){
	
		if(gid == usrApp_localRecordDatas.tuyaDevGroupParamTab[loop].gid){
			
			DBLOG_PRINTF("[GROUP DBG]same group detected, groupIndex:%d.\n", loop);
			res = devMacTabOperate_nodeAdd(loop, devNodeMac);
			break;
			
		}else
		if(0 == usrApp_localRecordDatas.tuyaDevGroupParamTab[loop].gid){
		
			if(indexNull == groupTabIndex_available){
				groupTabIndex_available = loop;
			}
		}
	}
	if(DEV_GROUP_NUM_MAX == loop){
	
		if(indexNull != groupTabIndex_available){
			usrApp_localRecordDatas.tuyaDevGroupParamTab[groupTabIndex_available].gid = gid;
			DBLOG_PRINTF("[GROUP DBG]new group created, groupIndex:%d.\n", groupTabIndex_available);
			res = devMacTabOperate_nodeAdd(groupTabIndex_available, devNodeMac);
		}else{
			res = groupAddErr_overLoad;
			DBLOG_PRINTF("[GROUP DBG]group tab is full.\n");
		}
	}
	
	if(groupAddRes_ok == res){
		usrApp_nvsOpreation_dataBurnning();
	}
	
	return res;
}

static uint8_t devMacTabOperate_nodeDel(uint8_t groupIndex, uint8_t devNodeMac[DEV_MAC_LEN]){

	const uint8_t devMacNull[DEV_MAC_LEN] = {0};
	uint8_t res = groupAddRes_ok;
	uint8_t loopA = 0;
	bool devNodeCatchFlg = false;
	bool groupIsEmptyFlg = true;
	
	if(groupIndex >= DEV_GROUP_NUM_MAX)return res;
	
	for(loopA = 0; loopA < DEV_GROUP_NODE_NUM_MAX; loopA ++){
	
		if(!memcmp(devNodeMac, usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab[loopA], sizeof(uint8_t) * DEV_MAC_LEN)){
			memset(usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab[loopA], 0, sizeof(uint8_t) * DEV_MAC_LEN);
			DBLOG_PRINTF("[GROUP DBG]devNode retrieved, tabIndex:%d.\n", loopA);
			res = groupAddRes_ok;
			devNodeCatchFlg = true;
			break;
		}
	}
	
	if(false == devNodeCatchFlg){
		DBLOG_PRINTF("[GROUP DBG]devNode not retrieved, automatic success.\n");
	}
	else{
		for(loopA = 0; loopA < DEV_GROUP_NODE_NUM_MAX; loopA ++){ //查当前组是否空
			if(memcmp(devMacNull, usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab[loopA], sizeof(uint8_t) * DEV_MAC_LEN)){
				groupIsEmptyFlg = false;
				break;
			}
		}
		if(true == groupIsEmptyFlg){
			usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].gid = 0;
			memset(usrApp_localRecordDatas.tuyaDevGroupParamTab[groupIndex].devNodeMacTab, 0, sizeof(uint8_t) * DEV_MAC_LEN * DEV_GROUP_NODE_NUM_MAX);
			DBLOG_PRINTF("[GROUP DBG]group detected null to init, groupIndex:%d.\n", groupIndex);
		}
	}
	
	return res;
}

uint8_t devGroupOperate_paramDel(uint32_t gid, uint8_t devNodeMac[DEV_MAC_LEN]){

	uint8_t loop = 0;
	uint8_t res = groupAddRes_ok;
	
	for(loop = 0; loop < DEV_GROUP_NUM_MAX; loop ++){
	
		if(gid == usrApp_localRecordDatas.tuyaDevGroupParamTab[loop].gid){
			
			DBLOG_PRINTF("[GROUP DBG]group retrieved, groupIndex:%d.\n", loop);
			res = devMacTabOperate_nodeDel(loop, devNodeMac);
			break;
		}
	}
	if(DEV_GROUP_NUM_MAX == loop){
	
		DBLOG_PRINTF("[GROUP DBG]group not retrieved, automatic success.\n");
	}
	
	if(groupAddRes_ok == res){
		usrApp_nvsOpreation_dataBurnning();
	}
	
	return res;
}

uint8_t devGroupOperate_groupIndexGet(int gid){
	
	uint8_t loop = 0;
	uint8_t res = 255;
	
	for(loop = 0; loop < DEV_GROUP_NUM_MAX; loop ++){
	
		if(gid == usrApp_localRecordDatas.tuyaDevGroupParamTab[loop].gid){
			
			res = loop;
			break;
		}
	}
	
	return res;
}

stt_usrDats_devGroupUnitParam *systemGroupParamGet(void){

	return usrApp_localRecordDatas.tuyaDevGroupParamTab;
}

void usrApp_nvsOpreation_dataSave(stt_usrDats_privateSave *dataParam){

	uint32_t *dptr = (uint32_t *)dataParam;
	uint32_t addr_a = ADDR_USRAPP_DATANVS_DEF,
			 addr_b = ADDR_USRAPP_DATANVS_DEF + sizeof(stt_usrDats_privateSave),
			 addr_count = ADDR_USRAPP_DATANVS_DEF;
	
	FLASH_EraseInitTypeDef EraseInitStruct = {0};
	uint32_t PageError = 0;
	uint8_t loop = 0;
	uint8_t writePeriod = 0;
	HAL_StatusTypeDef fopRes = HAL_OK;
	
	usrApp_MEM_If_Init_FS();
	
	while(addr_count < addr_b){
	
		if(0xffffffff != *(__IO uint32_t *)addr_count){
		
			/* Fill EraseInit structure*/
			EraseInitStruct.TypeErase 		= TYPEERASE_SECTORS;
			EraseInitStruct.Sector 			= (uint32_t)usrApp_GetFlashSector(addr_count);
			EraseInitStruct.NbSectors 		= 1;
			EraseInitStruct.VoltageRange	= FLASH_VOLTAGE_RANGE_3;
			
			if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){}
			FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE); //等待超时很重要
		}	

		addr_count += sizeof(uint32_t);
	}

//	DBLOG_PRINTF("[NVS DBG]dataTemp[1] check, var:0x%08X.\n", dptr[1]);
	writePeriod = sizeof(stt_usrDats_privateSave) / sizeof(uint32_t) + 1;
	
	for(loop = 0; loop < writePeriod; loop ++){
	
		fopRes = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(ADDR_USRAPP_DATANVS_DEF + (uint32_t)loop * sizeof(uint32_t)), dptr[loop]);
		if(HAL_OK != fopRes){
			DBLOG_PRINTF("[NVS DBG]HAL_FLASH_Program fail, err:%d.\n", fopRes);
		}
		FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE); //等待超时很重要
	}
	
	usrApp_MEM_If_DeInit_FS();
}

void usrApp_nvsOpreation_dataGet(stt_usrDats_privateSave *dataParam){
	
	__IO uint32_t *dataTemp = (__IO uint32_t*)ADDR_USRAPP_DATANVS_DEF;
	
	memcpy((void *)dataParam, (const void *)dataTemp, sizeof(stt_usrDats_privateSave));
}

void systemNvsOpreationDataSet(void *param, nvsOperateDataItemType type){

//	uint32_t dataTemp[128] = {0};
//	
//	FLASH_EraseInitTypeDef EraseInitStruct = {0};
//	uint32_t PageError = 0;
//	uint8_t loop = 0;
//	uint8_t writePeriod = 0;
//	
//    /* Fill EraseInit structure*/
//    EraseInitStruct.TypeErase 		= TYPEERASE_SECTORS;
//	EraseInitStruct.Sector 			= (uint32_t)usrApp_GetFlashSector(ADDR_USRAPP_DATANVS_DEF);
//	EraseInitStruct.NbSectors 		= 1;
//	EraseInitStruct.VoltageRange	= FLASH_VOLTAGE_RANGE_3;
//	
//	usrApp_MEM_If_Init_FS();
//	
//	if(HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){}
//		
//	switch(type){
//	
//		case nvsDpItemType_deviceRuningParam:{}break;
//		
//		case nvsDpItemType_groupParam:{}break;
//		
//		default:break;
//	}
//	
//	memcpy((stt_usrDats_privateSave *)dataTemp, dataParam, sizeof(stt_usrDats_privateSave));
//	writePeriod = sizeof(stt_usrDats_privateSave) / sizeof(uint32_t);
//	writePeriod += 1;
//	
//	for(loop = 0; loop < writePeriod; loop ++){
//	
//		if(HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)(ADDR_USRAPP_DATANVS_DEF + loop * sizeof(uint32_t)), dataTemp[loop])){}
//		
//		FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE); //等待超时很重要
//	}
//	
//	usrApp_MEM_If_DeInit_FS();
}

void systemNvsOpreationDataGet(void *param, nvsOperateDataItemType type){

	
}

void usrApp_nvsOpreation_test(void){

	stt_usrDats_privateSave nvsDataRead_temp = {0};
	
	usrApp_nvsOpreation_dataSave(&usrApp_localRecordDatas);
	usrApp_nvsOpreation_dataGet(&nvsDataRead_temp);
	
//	DBLOG_PRINTF("nvs dataRD:%d.\n", nvsDataRead_temp.devCurtain_orbitalPeriod);
	DBLOG_PRINTF("nvs dataRD:%d.\n", nvsDataRead_temp.tuyaDevGroupParamTab[0].devNodeMacTab[0][0]);
}

void bsp_dataManageInit(void){

	normalMemUint8_t_poolAttr_id = osPoolCreate(osPool(normalMemUint8_t_pool));
}

static uint8_t STMFLASH_GetFlashSector(uint32_t addr)
{
    if (addr < ADDR_FLASH_SECTOR_1)
        return FLASH_SECTOR_0;
    else if (addr < ADDR_FLASH_SECTOR_2)
        return FLASH_SECTOR_1;
    else if (addr < ADDR_FLASH_SECTOR_3)
        return FLASH_SECTOR_2;
    else if (addr < ADDR_FLASH_SECTOR_4)
        return FLASH_SECTOR_3;
    else if (addr < ADDR_FLASH_SECTOR_5)
        return FLASH_SECTOR_4;
    else if (addr < ADDR_FLASH_SECTOR_6)
        return FLASH_SECTOR_5;
    else if (addr < ADDR_FLASH_SECTOR_7)
        return FLASH_SECTOR_6;
    else if (addr < ADDR_FLASH_SECTOR_8)
        return FLASH_SECTOR_7;
}

static uint32_t STMFLASH_ReadWord(uint32_t faddr)
{
    return *(__IO uint32_t *)faddr;
}

#define BOARD_NUM_ADDR 0x0800C000

#define STM32_FLASH_BASE 0x08000000 //STM32 FLASH?????
#define FLASH_WAITETIME 50000       //FLASH??????

static void STMFLASH_Write(uint32_t WriteAddr, uint32_t *pBuffer, uint32_t Num){
	
    FLASH_EraseInitTypeDef FlashEraseInit;
    HAL_StatusTypeDef FlashStatus = HAL_OK;
    uint32_t SectorError = 0;
    uint32_t addrx = 0;
    uint32_t endaddr = 0;
    if (WriteAddr < STM32_FLASH_BASE || WriteAddr % 4)
        return; //

    HAL_FLASH_Unlock();            //
    addrx = WriteAddr;             //
    endaddr = WriteAddr + Num * 4; //
	uint8_t loopWaitCount = 0;
	uint8_t fwloopCount = 0;

    if (addrx < 0X080C1000)
    {
        while (addrx < endaddr)
        {
            if (STMFLASH_ReadWord(addrx) != 0XFFFFFFFF)
            {
                FlashEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;     //
                FlashEraseInit.Sector = STMFLASH_GetFlashSector(addrx); //
                FlashEraseInit.NbSectors = 1;                           //
                FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;    //
                if (HAL_FLASHEx_Erase(&FlashEraseInit, &SectorError) != HAL_OK)
                {
                    break; //
                }
            }
            else
                addrx += 4;
            FLASH_WaitForLastOperation(FLASH_WAITETIME); //
        }
    }
	
//	DBLOG_PRINTF("[FW DBG]nvs program start, WriteAddr:0x%08x, endaddr:0x%08x, num:%d\n", WriteAddr, endaddr, Num);	
	
    FlashStatus = FLASH_WaitForLastOperation(FLASH_WAITETIME); //
    if (FlashStatus == HAL_OK)
    {
        while (WriteAddr < endaddr) //
        {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, WriteAddr, *pBuffer) != HAL_OK) //
            {
                break; //
            }
			
//			DBLOG_PRINTF("[FW DBG](%d)adr:0x%08X.\n", fwloopCount, WriteAddr);	
			
            WriteAddr += 4;
            pBuffer ++;
			
			fwloopCount ++;
			
			if(loopWaitCount)loopWaitCount --;
			else{
//				loopWaitCount = 200;
//				osDelay(5);
			}
        }
    }
    HAL_FLASH_Lock(); //
}

static void STMFLASH_Read(uint32_t ReadAddr, uint32_t *pBuffer, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < size; i++)
    {
        pBuffer[i] = STMFLASH_ReadWord(ReadAddr); //
        ReadAddr += 4;                            //
    }
}

void usrApp_nvsOpreation_test_cc(void){

	uint32_t dp[64] = {0x12345678, 0x12345678, 0x12345678, 0x12345678};
	stt_usrDats_privateSave nvsDataRead_temp = {0};
	
//	STMFLASH_Write(ADDR_USRAPP_DATANVS_DEF, dp, 64);
//	STMFLASH_Read(ADDR_USRAPP_DATANVS_DEF, dp, 64);
	
	STMFLASH_Write(ADDR_USRAPP_DATANVS_DEF, (uint32_t *)&usrApp_localRecordDatas, sizeof(usrApp_localRecordDatas) / 4);
	STMFLASH_Read(ADDR_USRAPP_DATANVS_DEF, (uint32_t *)&nvsDataRead_temp, sizeof(usrApp_localRecordDatas) / 4);
	
	DBLOG_PRINTF("[NVS DBG]sizeof(usrApp_localRecordDatas) = %d.\n", sizeof(stt_usrDats_privateSave));	
//	DBLOG_PRINTF("nvs dataRD:%08X.\n", dp[3]);	
//	DBLOG_PRINTF("nvs dataRD:%d.\n", nvsDataRead_temp.devCurtain_orbitalPeriod);
	DBLOG_PRINTF("nvs dataRD:%d.\n", nvsDataRead_temp.tuyaDevGroupParamTab[DEV_GROUP_NUM_MAX - 1].devNodeMacTab[DEV_GROUP_NODE_NUM_MAX - 1][0]);	
}

void usrApp_nvsOpreation_dataRecovery(void){

	STMFLASH_Read(ADDR_USRAPP_DATANVS_DEF, (uint32_t *)&usrApp_localRecordDatas, (uint32_t)(sizeof(stt_usrDats_privateSave) / (size_t)4));
}

void usrApp_nvsOpreation_dataBurnning(void){

	STMFLASH_Write(ADDR_USRAPP_DATANVS_DEF, (uint32_t *)&usrApp_localRecordDatas, (uint32_t)(sizeof(stt_usrDats_privateSave) / (size_t)4));
}

void usrApp_nvsOpreation_dataReset(void){

	memset(&usrApp_localRecordDatas, 0, sizeof(usrApp_localRecordDatas));
	usrApp_nvsOpreation_dataBurnning();
}






