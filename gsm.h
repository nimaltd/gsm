#ifndef _GSM_H
#define _GSM_H

/*
  Author:     Nima Askari
  WebSite:    http://www.github.com/NimaLTD
  Instagram:  http://instagram.com/github.NimaLTD
  Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw
  
  Version:    3.0.0
  
  
  Reversion History:
  
  (3.0.0)
  Rewrite again.

*/

#include "gsmConfig.h"
#include "usart.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#if _GSM_USE_FREEEROS == 1
#include "cmsis_os.h"
#define gsm_delay(x)  osDelay(x)
#else
#define gsm_delay(x)  HAL_Delay(x)
#endif

#define   _GSM_MAX_ANSWER_SEARCHING_STRING   5
#define   _GSM_MAX_ALWAYS_SEARCHING_STRING   10

#if _GSM_DEBUG == 1
#define dprintf(...)    {printf(__VA_ARGS__);}
#else
#define dprintf(...)    {}
#endif

//######################################################################################
typedef enum
{
  Gsm_characterSet_ERROR = 0,
  Gsm_characterSet_GSM, 
  Gsm_characterSet_UCS2,
  Gsm_characterSet_IRA,
  Gsm_characterSet_HEX,
  Gsm_characterSet_PCCP,
  Gsm_characterSet_PCDN,
  Gsm_characterSet_8859_1
  
}Gsm_characterSet_t;

typedef enum
{
  Gsm_pin_ERROR = 0,
  Gsm_pin_READY,
  Gsm_pin_PIN,
  Gsm_pin_PUK,
  Gsm_pin_PHPIN,
  Gsm_pin_PHPUK,
  Gsm_pin_PIN2,
  Gsm_pin_PUK2

}Gsm_pin_t;

typedef enum
{
  Gsm_msg_storage_ERROR = 0,
  Gsm_msg_storage_SIMCARD,
  Gsm_msg_storage_MODULE,
  Gsm_msg_storage_SIMCARD_PREFERRED,
  Gsm_msg_storage_MODULE_PREFERRED,
  Gsm_msg_storage_SIMCARD_OR_MODULE,
  
}Gsm_msg_storage_t;

typedef enum
{
  Gsm_channel_MAIN = 0,
  Gsm_channel_MAINHANDSFREE
  
}Gsm_channel_t;
//######################################################################################
typedef struct
{
  uint8_t               rxBuffer[_GSM_RXSIZE];
  uint16_t              rxIndex;
  uint32_t              rxTime;
  uint8_t               txBusy;
  uint8_t               atBusy;
  #if                   _GSM_TXDMA_ENABLE == 1
  uint8_t               txDone;
  #endif  
  int8_t                foundAnswer;  
  uint16_t              foundAnswerSize; 
  char*                 foundAnswerString;
  char*                 answerSearchingString[_GSM_MAX_ANSWER_SEARCHING_STRING];
  char*                 alwaysSearchingString[_GSM_MAX_ALWAYS_SEARCHING_STRING]; 
  
}Gsm_atCommand_t;
//##############
typedef struct
{
  uint8_t               year;
  uint8_t               month;
  uint8_t               day;
  uint8_t               hour;
  uint8_t               minute;
  uint8_t               second;
  
}Gsm_msg_time_t;
//##############
typedef struct
{
  char                  status[16];
  char                  number[16];
  Gsm_msg_time_t        time;
  char                  message[1024];
  uint8_t               isTextMode;
  char                  serviceCenterNum[16];
  Gsm_characterSet_t    characterSet;
  uint8_t               storageTotal;               
  uint8_t               storageUsed;               
  Gsm_msg_storage_t     storage;
  uint16_t              newIndex;
  
}Gsm_msg_t;
//##############
typedef struct
{
  char                  incommingNumber[16];
  uint8_t               ringing;
  uint8_t               inUse;
  
}Gsm_call_t;
//##############
typedef struct
{
  uint16_t              nonlinearProcessing;             
  uint16_t              acousticEcho;
  uint16_t              noiseReduction;
  uint16_t              noiseSuppression;
  
}Gsm_config_echo_t;
//##############
typedef struct
{
  uint8_t               flightMode;
  uint8_t               ringLevel;
  uint8_t               loadSpeakerLevel;
  uint8_t               micGainLevel_mainCh;
  uint8_t               micGainLevel_auxCh;
  uint8_t               micGainLevel_mainHfCh;
  uint8_t               micGainLevel_auxHfCh;
  uint8_t               autoAnswerCall;
  Gsm_pin_t             pinStatus;
  char                  providerSimcardName[32];
  char                  imei[18];
  char                  modelIdentification[32];  
  Gsm_config_echo_t     echoChannel[2];  
  
}Gsm_config_t;
//##############
typedef struct
{
  uint8_t               inited;
  uint8_t               powerUp;
  Gsm_atCommand_t       at;
  Gsm_config_t          config;
  Gsm_msg_t             msg;
  Gsm_call_t            call;
  
}Gsm_t;
//##############
extern Gsm_t            gsm;
//######################################################################################
void                    gsm_at_rxCallback(void);                          //  rx irq  
void                    gsm_at_txCallback(void);                          //  dma tx callback, if use txdma

void                    gsm_user_init(void);                              //  auto call after init    
void                    gsm_user_incommingCall(char *number);             //  auto call after ringing
void                    gsm_user_endCall(void);                           //  auto call after end call
void                    gsm_user_newMsg(char *msg, char *number, Gsm_msg_time_t time);  //  auto call after receive new msg

bool                    gsm_init(void);                                   //  init gsm
void                    gsm_process(void);                                //  call in infinite loop    

bool                    gsm_callAnswer(void);                             //  answer incomming call
bool                    gsm_callEnd(void);                                //  end call
bool                    gsm_call(char *number, uint8_t wait_s);           //  dial

bool                    gsm_msg_setTextMode(bool enable);
bool                    gsm_msg_isTextMode(void);
bool                    gsm_msg_read(uint16_t index);
Gsm_characterSet_t      gsm_msg_getCharacterSet(void);
bool                    gsm_msg_setCharacterSet(Gsm_characterSet_t Gsm_characterSet_);
bool                    gsm_msg_updateStorage(void);
uint8_t                 gsm_msg_getFreeSpace(void);
bool                    gsm_msg_setStorage(Gsm_msg_storage_t Gsm_msg_storage_);
bool                    gsm_msg_deleteAll(void);
bool                    gsm_msg_delete(uint16_t index);
bool                    gsm_msg_send(char *msg, char *number);

bool                    gsm_ussdSend(char *command, char *answer, uint16_t sizeOfAnswer, uint8_t  wait_s);

bool                    gsm_powerOff(void);
bool                    gsm_powerOn(void);
bool                    gsm_getRegisteredStatus(void);
uint8_t                 gsm_getSignalQuality(void);
bool                    gsm_sendDtmf(char *dtmfString, uint16_t duration_ms);
bool                    gsm_setFlightMode(bool enable);
bool                    gsm_getFlightMode(void);
bool                    gsm_setRingLevel(uint8_t level_0_100);
uint8_t                 gsm_getRingLevel(void);
bool                    gsm_setLoadSpeakerLevel(uint8_t level_0_100);
uint8_t                 gsm_getLoadSpeakerLevel(void);
bool                    gsm_setMicrophoneGainLevel_mainChannel(int8_t level_0_15);
uint8_t                 gsm_getMicrophoneGainLevel_mainChannel(void);
bool                    gsm_setMicrophoneGainLevel_auxChannel(int8_t level_0_15);
uint8_t                 gsm_getMicrophoneGainLevel_auxChannel(void);
bool                    gsm_setMicrophoneGainLevel_mainChannelHF(int8_t level_0_15);
uint8_t                 gsm_getMicrophoneGainLevel_mainChannelHF(void);
bool                    gsm_setMicrophoneGainLevel_auxChannelHF(int8_t level_0_15);
uint8_t                 gsm_getMicrophoneGainLevel_auxChannelHF(void);
bool                    gsm_getServiceProviderNameFromSimcard(char *string);
bool                    gsm_getManufacturerIdentification(char *string);
bool                    gsm_getModelIdentification(char *string);
bool                    gsm_getProductIdentificationInformation(char *string);
bool                    gsm_getGlobalObjectIdentification(char *string);
bool                    gsm_getIMEI(char *string);
bool                    gsm_setEcho(bool enable);
bool                    gsm_setMonitorSpeakerLoudness(uint8_t level_0_9);
bool                    gsm_setMonitorSpeakerMode(uint8_t level_0_9);
bool                    gsm_setNumRingsAutoAnsweringCall(uint8_t level_0_255);
Gsm_pin_t               gsm_getPinStatus(void);
bool                    gsm_setPin(char *enterPin);
bool                    gsm_updateEchoCancellationControl(void);
uint16_t                gsm_getEchoCancellationControl_nonlinearProcessing(Gsm_channel_t Gsm_channel_);  
uint16_t                gsm_getEchoCancellationControl_acousticEcho(Gsm_channel_t Gsm_channel_);
uint16_t                gsm_getEchoCancellationControl_noiseReduction(Gsm_channel_t Gsm_channel_);
uint16_t                gsm_getEchoCancellationControl_noiseSuppression(Gsm_channel_t Gsm_channel_);
bool                    gsm_setEchoCancellationControl_nonlinearProcessing(Gsm_channel_t Gsm_channel_, uint16_t value);
bool                    gsm_setEchoCancellationControl_acousticEcho(Gsm_channel_t Gsm_channel_, uint16_t value);
bool                    gsm_setEchoCancellationControl_noiseReduction(Gsm_channel_t Gsm_channel_, uint16_t value);
bool                    gsm_setEchoCancellationControl_noiseSuppression(Gsm_channel_t Gsm_channel_, uint16_t value);
bool                    gsm_setEchoCancellationControl_enable(Gsm_channel_t Gsm_channel_, bool enable);


#endif
