#ifndef __DATSMANAGE_H__
#define __DATSMANAGE_H__

#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stddef.h"
#include "stdbool.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#include "cmsis_os.h"               // CMSIS RTOS header file

#define VAR_DISAVAILABLE_U16	0xffff
#define VAR_DISAVAILABLE_U32	0xffffffff

#define LOCAL	static

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbyte */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbyte */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbyte */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbyte */

#define FLASH_SECTOR_DEF_0     	((uint16_t)0x0000) /*!< Sector Number 0 */
#define FLASH_SECTOR_DEF_1     	((uint16_t)0x0008) /*!< Sector Number 1 */
#define FLASH_SECTOR_DEF_2     	((uint16_t)0x0010) /*!< Sector Number 2 */
#define FLASH_SECTOR_DEF_3     	((uint16_t)0x0018) /*!< Sector Number 3 */
#define FLASH_SECTOR_DEF_4     	((uint16_t)0x0020) /*!< Sector Number 4 */
#define FLASH_SECTOR_DEF_5     	((uint16_t)0x0028) /*!< Sector Number 5 */
#define FLASH_SECTOR_DEF_6     	((uint16_t)0x0030) /*!< Sector Number 6 */
#define FLASH_SECTOR_DEF_7     	((uint16_t)0x0038) /*!< Sector Number 7 */
#define FLASH_SECTOR_DEF_8    	((uint16_t)0x0040) /*!< Sector Number 8 */
#define FLASH_SECTOR_DEF_9     	((uint16_t)0x0048) /*!< Sector Number 9 */
#define FLASH_SECTOR_DEF_10    	((uint16_t)0x0050) /*!< Sector Number 10 */
#define FLASH_SECTOR_DEF_11    	((uint16_t)0x0058) /*!< Sector Number 11 */

#define ADDR_USRAPP_DATANVS_DEF			(uint32_t)0x08061000

#define FACTORY_PRODUCT_SPACIFY			0	//

#define SMARTCONFIG_TIMEOPEN_DEFULT		240

#define DBLOB_BUFTEMP_LEN				128

#define DEV_MAC_LEN						6		//MACLEN

#define DEV_GROUP_NUM_MAX				12		//????????
#define DEV_GROUP_NODE_NUM_MAX			8		//??????????????

#define SWITCH_TYPE_SWBIT1	 			 (0x38 + 0x01) //??????????????????
#define SWITCH_TYPE_SWBIT2	 			 (0x38 + 0x02) //??????????????????
#define SWITCH_TYPE_SWBIT3	 			 (0x38 + 0x03) //??????????????????
#define	SWITCH_TYPE_CURTAIN				 (0x38 + 0x08) //??????????????

#define SWITCH_TYPE_DIMMER				 (0x38 + 0x04) //??????????????
#define SWITCH_TYPE_FANS				 (0x38 + 0x05) //??????????????
#define SWITCH_TYPE_INFRARED			 (0x38 + 0x06) //????????????????????
#define SWITCH_TYPE_SOCKETS				 (0x38 + 0x07) //??????????????
#define	SWITCH_TYPE_SCENARIO			 (0x38 + 0x09) //??????????????????
#define SWITCH_TYPE_HEATER				 (0x38 + 0x1F) //????????????????

#define USRAPP_DEVNODE_MAX_SUM			 72

/*!< STM32F10x Standard Peripheral Library old types (maintained for legacy purpose) */
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  /*!< Read Only */
typedef const int16_t sc16;  /*!< Read Only */
typedef const int8_t sc8;   /*!< Read Only */

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  /*!< Read Only */
typedef __I int16_t vsc16;  /*!< Read Only */
typedef __I int8_t vsc8;   /*!< Read Only */

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  /*!< Read Only */
typedef __I uint16_t vuc16;  /*!< Read Only */
typedef __I uint8_t vuc8;   /*!< Read Only */

typedef struct agingDataSet_bitHold{ //????????_????????;	????????????????????agingCmd_swOpreat????????????????bit0, ????????<tips,??????????????????????bit0????????????????>

	u8 agingCmd_swOpreat:1; //????_???????? -bit0
	u8 agingCmd_devLock:1; //????_?????????? -bit1
	u8 agingCmd_delaySetOpreat:1; //????_???????? -bit2
	u8 agingCmd_greenModeSetOpreat:1; //????_???????????? -bit3
	u8 agingCmd_timerSetOpreat:1; //????_???????? -bit4
	u8 agingCmd_nightModeSetOpreat:1; //????_???????????? -bit5
	u8 agingCmd_bkLightSetOpreat:1; //????_?????????? -bit6
	u8 agingCmd_devResetOpreat:1; //????_???????????????????? -bit7
	
	u8 agingCmd_horsingLight:1; //????_?????????? -bit0
	u8 agingCmd_switchBitBindSetOpreat:3; //????_????????????????_?????????????????????? -bit1...bit3
	u8 agingCmd_curtainOpPeriodSetOpreat:1; //????_???????????????????????????? -bit4
	u8 agingCmd_infrareOpreat:1;	//????_?????????????????? -bit5
	u8 agingCmd_scenarioSwOpreat:1;	//????_???????????????? -bit6
	u8 agingCmd_timeZoneReset:1; //????_???????????? -bit7
	
	u8 agingCmd_byteReserve[4];	//4????????????
	
}stt_agingDataSet_bitHold; //standard_length = 6Bytes

typedef struct swDevStatus_reference{ //????????_????????????

	u8 statusRef_swStatus:3; //????_???????????? -bit0...bit2
	u8 statusRef_reserve:2; //????_reserve -bit3...bit4
	u8 statusRef_swPush:3; //????_???????? -bit5...bit7
	
	u8 statusRef_timer:1; //????_?????????? -bit0
	u8 statusRef_devLock:1; //????_?????? -bit1
	u8 statusRef_delay:1; //????_???????? -bit2
	u8 statusRef_greenMode:1; //????_???????????? -bit3
	u8 statusRef_nightMode:1; //????_???????????? -bit4
	u8 statusRef_horsingLight:1; //????_?????????? -bit5
	u8 statusRef_bitReserve:2; //????_reserve -bit6...bit7
	
	u8 statusRef_byteReserve[2];   //????_reserve -bytes2...3
	
}stt_swDevStatusReference_bitHold; //standard_length = 4Bytes

typedef struct dataPonit{ //????????_??????

	stt_agingDataSet_bitHold 			devAgingOpreat_agingReference; //????????????, 6Bytes
	stt_swDevStatusReference_bitHold	devStatus_Reference; //????????????, 4Bytes 			
	u8 						 			devData_timer[24]; //?????????? 8??, 24Bytes
	u8									devData_delayer; //????????, 1Bytes
	u8									devData_delayUpStatus; //????????????????????????, 1Bytes
	u8									devData_greenMode; //????????????, 1Bytes
	u8									devData_nightMode[6]; //????????????, 6Bytes
	u8									devData_bkLight[2]; //??????????????, 2Bytes
	u8									devData_devReset; //????????????, 1Bytes
	u8									devData_switchBitBind[3]; //??????????????????, 3Bytes
	
	union devClassfication{ //??????????????????
	
		struct funParam_curtain{ //????
		
			u8 orbital_Period; //????????????
			
		}curtain_param;
		
		struct funParam_socket{ //????
		
			u8 data_elePower[4]; //????????
			u8 data_eleConsum[3]; //????????
			u8 data_corTime; //??????????????????
			
			u8 dataDebug_powerFreq[4]; //debug????-power????
			
		}socket_param;
		
		struct funParam_infrared{
		
			u8 opreatAct; //????????
			u8 opreatInsert; //????????????????
			u8 currentTemperature_integerPrt; //????????-????????
			u8 currentTemperature_decimalPrt; //????????-????????
			u8 currentOpreatReserveNum; //????????????????????????????????????????????????
			
			u8 irTimeAct_timeUpNum[8]; //????????????????????????
			
		}infrared_param;
		
		struct funParam_scenarioSw{
		
			u8 scenarioOpreatCmd; //????????
			u8 scenarioKeyBind[3]; //????????????????????
			
		}scenarioSw_param;
		
	}union_devParam;
	
//	u8	devData_byteReserve[63];
	
}stt_devOpreatDataPonit; //standard_length = 49Bytes + class_extension

typedef enum{

	nvsDpItemType_null = -1,
	nvsDpItemType_deviceRuningParam,
	nvsDpItemType_groupParam,
}nvsOperateDataItemType;

typedef struct{

	u8 time_Year;
	u8 time_Month;
	u8 time_Week;
	u8 time_Day;
	u8 time_Hour;
	u8 time_Minute;
	u8 time_Second;
}stt_localTime;

typedef enum{

	groupAddRes_ok = 0,
	groupAddErr_overLoad,
	groupAddErr_childDevTimeOut,
	groupAddErr_paramIllegal,
	groupAddErr_fileWriteFail,
	groupAddErr_other,
}enum_devGroupAddResType;

typedef struct{

	uint32_t gid;
	uint8_t devNodeMacTab[DEV_GROUP_NODE_NUM_MAX][DEV_MAC_LEN];
}stt_usrDats_devGroupUnitParam;

typedef struct{

	stt_usrDats_devGroupUnitParam tuyaDevGroupParamTab[DEV_GROUP_NUM_MAX];
	
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

typedef struct toutMeansure_param{

	uint16_t toutTrig_flg:1;
	uint16_t tout_counter:15;
}stt_toutMeansureParam;

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

extern const u8 devRootNode_subidMac[5];

extern const u8 debugLogOut_targetMAC[5];

extern u8 MACSTA_ID[DEV_MAC_LEN];
extern u8 MACAP_ID[DEV_MAC_LEN];
extern u8 MACDST_ID[DEV_MAC_LEN];

u8 switchTypeReserve_GET(void);
void bsp_dataManageInit(void);
const char *typeStrgetByNum(uint8_t typeNum);

void usrApp_nvsOpreation_test(void);
void usrApp_nvsOpreation_test_cc(void);
uint8_t devGroupOperate_paramAdd(uint32_t gid, uint8_t devNodeMac[DEV_MAC_LEN]);
uint8_t devGroupOperate_paramDel(uint32_t gid, uint8_t devNodeMac[DEV_MAC_LEN]);
uint8_t devGroupOperate_groupIndexGet(int gid);
void usrApp_nvsOpreation_dataRecovery(void);
void usrApp_nvsOpreation_dataBurnning(void);
void usrApp_nvsOpreation_dataReset(void);
stt_usrDats_devGroupUnitParam *systemGroupParamGet(void);
void appFuncStr2Mac(uint8_t devMac[DEV_MAC_LEN], char *strmac);

#endif

