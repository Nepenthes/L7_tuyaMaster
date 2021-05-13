#ifndef __DATSPROCESS_UARTWIFI_H__
#define __DATSPROCESS_UARTWIFI_H__

#include "dataManage.h"
#include "usrAppMethod.h"

#include "UART_dataTransfer.h"

#define FRAME_HEAD_MOBILE		0xAA
#define FRAME_HEAD_SERVER		0xCC
#define FRAME_HEAD_HEARTB		0xAA

#define DATATRANS_WORKMODE_HEARTBEAT		0x0A //
#define DATATRANS_WORKMODE_KEEPACESS		0x0B //
#define ZIGB_DATATRANS_WORKMODE				DATATRANS_WORKMODE_KEEPACESS //

#define timer_heartBeatKeep_Period			1000UL

#if(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_HEARTBEAT) //
	#define PERIOD_HEARTBEAT_ASR		8000UL  	//
#elif(ZIGB_DATATRANS_WORKMODE == DATATRANS_WORKMODE_KEEPACESS)
	#define PERIOD_HEARTBEAT_ASR		20000UL  	//
	#define PERIOD_HEARTBEAT_PST		2000UL		//
#endif

#define INTERNET_SERVERPORT_SWITCHPERIOD	(u8)(PERIOD_HEARTBEAT_ASR / 1000UL * 3UL)

#define dataTransLength_objLOCAL			33
#define dataTransLength_objREMOTE			45
#define dataHeartBeatLength_objSERVER		96

#define FRAME_TYPE_MtoS_CMD					0xA0	
#define FRAME_TYPE_StoM_RCVsuccess			0x0A	
//#define FRAME_TYPE_MtoZB_CMD				0xA1	
//#define FRAME_TYPE_ZBtoM_CMD				0x1A	
#define FRAME_TYPE_StoM_RCVfail				0x0C
#define FRAME_TYPE_StoM_upLoad				0x0D
#define FRAME_TYPE_StoM_reaptSMK			0x0E

#define FRAME_HEARTBEAT_cmdOdd				0x23	
#define FRAME_HEARTBEAT_cmdEven				0x22	

#define FRAME_MtoSCMD_cmdControl			0x10	
#define FRAME_MtoSCMD_cmdConfigSearch		0x39	
#define FRAME_MtoSCMD_cmdQuery				0x11	
#define FRAME_MtoSCMD_cmdInterface			0x15	
#define FRAME_MtoSCMD_cmdReset				0x16	
#define FRAME_MtoSCMD_cmdLockON				0x17	
#define FRAME_MtoSCMD_cmdLockOFF			0x18	
#define FRAME_MtoSCMD_cmdswTimQuery			0x19	
#define FRAME_MtoSCMD_cmdConfigAP			0x50	
#define FRAME_MtoSCMD_cmdBeepsON			0x1A	
#define FRAME_MtoSCMD_cmdBeepsOFF			0x1B	
#define FRAME_MtoSCMD_cmdftRecoverRQ		0x22	
#define FRAME_MtoSCMD_cmdRecoverFactory		0x1F	
#define FRAME_MtoSCMD_cmdCfg_swTim			0x14	
#define FRAME_MtoZIGBCMD_cmdCfg_PANID		0x40	
#define FRAME_MtoZIGBCMD_cmdCfg_ctrlEachO	0x41	
#define FRAME_MtoZIGBCMD_cmdQue_ctrlEachO	0x42	
#define FRAME_MtoZIGBCMD_cmdCfg_ledBackSet	0x43	
#define FRAME_MtoZIGBCMD_cmdQue_ledBackSet	0x44	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioSet	0x45	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioCtl	0x47	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioDel	0x48	
#define FRAME_MtoZIGBCMD_cmdCfg_scenarioReg 0x50	

#define	cmdConfigTim_normalSwConfig			0xA0	
#define cmdConfigTim_onoffDelaySwConfig		0xA1	
#define cmdConfigTim_closeLoopSwConfig		0xA2	
#define cmdConfigTim_nightModeSwConfig		0xA3	

#define HWCON_WIFI_MOUDLE_PIN_RST			DataTransP3_RST

typedef enum{

	DATATRANS_objFLAG_REMOTE = 0,
	DATATRANS_objFLAG_LOCAL
}socketDataTrans_obj;

typedef enum{

	Obj_udpLocal_A = 0,
	Obj_udpRemote_B,
	Obj_tcpRemote_A,
	Obj_tcpRemote_B
}socket_OBJ;

typedef enum{

	obj_toWIFI = 0,
	obj_toZigB,
	obj_toALL
}datsTrans_dstObj;

typedef struct{

	bool 				heartBeat_IF;	//??????
	datsTrans_dstObj	dstObj;			//????wifi??zigb
	socket_OBJ 			portObj;		//????(??????)
	u8 command;		//??
	u8 dats[128];	//??
	u8 datsLen;		//????
}stt_socketDats;

void WIFI_mainThread(type_uart_OBJ UART_OBJ);
void uartConWifi_dataTx(unsigned char dataHex);

#endif

