#ifndef __DATSMANAGE_H__
#define __DATSMANAGE_H__

#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stdbool.h"

#include "stm32f10x.h"

#include "IO_Map.h"
#include "delay.h"

#define LOCAL	static

#define DBLOB_BUFTEMP_LEN		128

#define DEV_MAC_LEN				6		//MACLEN

#define SWITCH_TYPE_SWBIT1	 	(0xA0 + 0x01) //设备类型：一位开关
#define SWITCH_TYPE_SWBIT2	 	(0xA0 + 0x02) //设备类型：二位开关
#define SWITCH_TYPE_SWBIT3	 	(0xA0 + 0x03) //设备类型：三位开关
#define SWITCH_TYPE_CURTAIN		(0xA0 + 0x08) //设备类型：窗帘开关

#define SWITCH_TYPE_dIMMER		(0x38 + 0x04) //设备类型：调光
#define SWITCH_TYPE_FANS		(0x38 + 0x05) //设备类型：风扇
#define SWITCH_TYPE_INFRARED	(0x38 + 0x06) //设备类型：红外转发器
#define SWITCH_TYPE_SOCKETS		(0x38 + 0x07) //设备类型：插座
#define	SWITCH_TYPE_SCENARIO	(0x38 + 0x09) //设备类型：场景开关
#define SWITCH_TYPE_HEATER		(0x38 + 0x1F) //设备类型：热水器

typedef struct{

	u8 time_Year;
	u8 time_Month;
	u8 time_Week;
	u8 time_Day;
	u8 time_Hour;
	u8 time_Minute;
	u8 time_Second;
}stt_localTime;

typedef struct{

	u8  FLG_factory_IF;

	u8 	rlyStaute_flg:3;	//

	u8  dev_lockIF:1;	//

	u8 	test_dats:3;	//

	u8  timeZone_H;
	u8  timeZone_M;

	u8 	serverIP_default[4]; //

	u8	devCurtain_orbitalPeriod;	//
	u8	devCurtain_orbitalCounter;	//

	u8  swTimer_Tab[3 * 8];	//
	u8  swDelay_flg; //
	u8  swDelay_periodCloseLoop; //
	u8  devNightModeTimer_Tab[3 * 2]; //

//	u8  port_ctrlEachother[USRCLUSTERNUM_CTRLEACHOTHER]; //

	u16 panID_default; //PANID

//	bkLightColorInsert_paramAttr param_bkLightColorInsert;
}stt_usrDats_privateSave;

typedef enum{

	datsFrom_ctrlRemote = 0,
	datsFrom_ctrlLocal,
	datsFrom_heartBtRemote
}threadDatsPass_objDatsFrom;

typedef struct STTthreadDatsPass_conv{	//

	threadDatsPass_objDatsFrom datsFrom;	//
	u8	macAddr[5];
	u8	devType;
	u8	dats[128];
	u8  datsLen;
}stt_threadDatsPass_conv;

typedef struct STTthreadDatsPass_devQuery{	//

	u8	infoDevList[100];
	u8  infoLen;
}stt_threadDatsPass_devQuery;

typedef union STTthreadDatsPass_dats{	//

	stt_threadDatsPass_conv 	dats_conv;		//
	stt_threadDatsPass_devQuery	dats_devQuery;	//
}stt_threadDatsPass_dats;

typedef enum threadDatsPass_msgType{	//

	listDev_query = 0,
	conventional,
}threadDP_msgType;

typedef struct STTthreadDatsPass{	//

	threadDP_msgType msgType;		//
	stt_threadDatsPass_dats dats;	//
}stt_threadDatsPass;

typedef void (* functionCallBcak_normalDef)(void);


extern osPoolId (normalMemUint8_t_poolAttr_id);

extern char printfLog_bufTemp[DBLOB_BUFTEMP_LEN];

extern u8 zigbNwkReserveNodeNum_currentValue;

extern int8_t sysTimeZone_H; 
extern int8_t sysTimeZone_M; 
extern uint32_t systemUTC_current;
extern u16 sysTimeKeep_counter;
extern stt_localTime systemTime_current;

extern u8 DEV_actReserve;
extern u8 SWITCH_TYPE;

extern bool deviceLock_flag;

extern u8 MACSTA_ID[DEV_MAC_LEN];
extern u8 MACAP_ID[DEV_MAC_LEN];
extern u8 MACDST_ID[DEV_MAC_LEN];

u8 switchTypeReserve_GET(void);
void bsp_dataManageInit(void);

#endif

