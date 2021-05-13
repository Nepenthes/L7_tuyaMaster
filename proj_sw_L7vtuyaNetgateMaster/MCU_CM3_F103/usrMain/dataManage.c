#include "dataManage.h"

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
u8 SWITCH_TYPE = SWITCH_TYPE_CURTAIN;	

bool deviceLock_flag = false;

u8 MACSTA_ID[DEV_MAC_LEN] = {0};
u8 MACAP_ID[DEV_MAC_LEN] = {0};
u8 MACDST_ID[DEV_MAC_LEN] = {1,1,1,1,1,1}; //

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

void bsp_dataManageInit(void){

	normalMemUint8_t_poolAttr_id = osPoolCreate(osPool(normalMemUint8_t_pool));
}
