#ifndef _GSM_H
#define _GSM_H

/*
  Author:     Nima Askari
  WebSite:    http://www.github.com/NimaLTD
  Instagram:  http://instagram.com/github.NimaLTD
  Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw
  
  Version:    4.1.3
  
  
  Reversion History:
  
  (4.1.3)
  Fix FTPPUT, Add FTPEXTPUT
  
  (4.1.2)
  Add NTP.
  
  (4.1.1)
  Add FTP Upload,DeleteDir,CreateDIR.
  
  (4.1.0)
  Add GPRS. Change somethings. Improve performance.

  (4.0.4)
  Change AT+DDET place.
  
  (4.0.3)
  Change always search to flash memory, Add dtmf detect, fix somethings
  
  (4.0.2)
  Add ussd.
  
  (4.0.1)
  Solve power on problem.
  
  (4.0.0)
  New release.
*/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "usart.h"
#include "gsmConfig.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>

//###################################################################

typedef struct
{
  uint8_t       year;
  uint8_t       month;
  uint8_t       day;
  uint8_t       hour;
  uint8_t       minute;
  uint8_t       second;
  
}Gsm_Time_t;

typedef enum
{
  Gsm_Tone_DialTone = 1,
  Gsm_Tone_CalledSubscriberBusy = 2,
  Gsm_Tone_Congestion = 3, 
  Gsm_Tone_RadioPathAcknowledge = 4,
  Gsm_Tone_RadioPathNotAvailableOrCallDropped = 5,
  Gsm_Tone_ErrorOrSpecialinformation = 6,
  Gsm_Tone_CallWaitingTone = 7,
  Gsm_Tone_RingingTone = 8,
  Gsm_Tone_GeneralBeep = 16,
  Gsm_Tone_PositiveAcknowledgementTone = 17,
  Gsm_Tone_NegativeAcknowledgementOrErrorTone = 18, 
  Gsm_Tone_IndianDialTone = 19,
  Gsm_Tone_AmericanDialTone = 20

}Gsm_Tone_t;

typedef enum
{
  Gsm_Msg_ChSet_ERROR = 0,
  Gsm_Msg_ChSet_GSM, 
  Gsm_Msg_ChSet_UCS2,
  Gsm_Msg_ChSet_IRA,
  Gsm_Msg_ChSet_HEX,
  Gsm_Msg_ChSet_PCCP,
  Gsm_Msg_ChSet_PCDN,
  Gsm_Msg_ChSet_8859_1
  
}Gsm_Msg_ChSet_t;

typedef enum
{
  Gsm_Msg_Store_ERROR = 0,
  Gsm_Msg_Store_SIMCARD,
  Gsm_Msg_Store_MODULE,
  Gsm_Msg_Store_SIMCARD_PREFERRED,
  Gsm_Msg_Store_MODULE_PREFERRED,
  Gsm_Msg_Store_SIMCARD_OR_MODULE,
  
}Gsm_Msg_Store_t;

typedef enum
{
  Gsm_Ftp_Error_None = 0,
  Gsm_Ftp_Error_Error = 1,
  Gsm_Ftp_Error_NetError = 61,
  Gsm_Ftp_Error_DNSError = 62,
  Gsm_Ftp_Error_ConnectError = 63,
  Gsm_Ftp_Error_Timeout = 64,
  Gsm_Ftp_Error_ServerError = 65,
  Gsm_Ftp_Error_OperationNotAllow = 66,
  Gsm_Ftp_Error_ReplayError = 70,
  Gsm_Ftp_Error_UserError = 71,
  Gsm_Ftp_Error_PasswordError = 72,
  Gsm_Ftp_Error_TypeError = 73,
  Gsm_Ftp_Error_RestError = 74,
  Gsm_Ftp_Error_PassiveError = 75,
  Gsm_Ftp_Error_ActiveError = 76,
  Gsm_Ftp_Error_OperateError = 77,
  Gsm_Ftp_Error_UploadError = 78,
  Gsm_Ftp_Error_DownloadError = 79,
  Gsm_Ftp_Error_ManualQuit = 86,  
  
}Gsm_Ftp_Error_t;

typedef struct
{
  uint32_t      rxTime;  
  uint16_t      index;
  uint8_t       buff[_GSM_RXSIZE];
  int8_t        answerFound;
  char*         answerSearch[_GSM_AT_MAX_ANSWER_ITEMS];          
  char*         answerString;
  uint16_t      answerSize;  
  
}Gsm_At_t;

typedef struct
{
  uint8_t         textMode;
  uint8_t         buff[_GSM_RXSIZE];
  Gsm_Msg_ChSet_t characterSet;
  Gsm_Msg_Store_t storage;
  uint8_t         storageTotal;               
  uint8_t         storageUsed;    
  Gsm_Time_t      time;
  int16_t         newMsg;
  char            status[16];
  char            number[16];
  
}Gsm_Msg_t;

typedef struct
{
  uint8_t         ringing;
  uint8_t         busy;
  uint8_t         callbackEndCall;
  char            number[16];  
  #if (_GSM_DTMF_DETECT_ENABLE == 1)
  uint8_t         dtmf;
  #endif
    
}Gsm_Call_t;

typedef struct
{
  bool            connected;
  bool            connectedLast;
  char            ip[16];
  uint32_t        dataLen;
  uint32_t        dataCurrent;
  int16_t         code;
  uint8_t         buff[_GSM_RXSIZE];
  uint8_t         tcpConnection;
  uint8_t         gotData;
  uint32_t        ftpExtOffset;
  
}Gsm_Gprs_t;

typedef struct
{
  uint8_t       inited;
  uint8_t       power;
  uint8_t       started;
  uint8_t       registred;
  uint8_t       signal; 
  uint8_t       taskBusy;
  Gsm_At_t      at;
  #if (_GSM_MSG_ENABLE == 1)
  Gsm_Msg_t     msg;
  #endif
  #if (_GSM_CALL_ENABLE == 1)
  Gsm_Call_t    call;
  #endif
  #if (_GSM_GPRS_ENABLE == 1)
  Gsm_Gprs_t    gprs;
  #endif
  
}Gsm_t;

//###################################################################
extern          osThreadId  gsmTaskHandle;
extern          Gsm_t       gsm;
//###################################################################   at commands
void            gsm_at_rxCallback(void);
void            gsm_at_sendString(const char *string);
void            gsm_at_sendData(const uint8_t *data, uint16_t len);
uint8_t         gsm_at_sendCommand(const char *command, uint32_t waitMs, char *answer, uint16_t sizeOfAnswer, uint8_t items, ...);
//###################################################################   general functions
bool            gsm_init(osPriority osPriority_);
bool            gsm_power(bool on_off);
bool            gsm_setDefault(void);
bool            gsm_saveProfile(void);
bool            gsm_waitForStarted(uint8_t waitSecond);
bool            gsm_waitForRegister(uint8_t waitSecond);
bool            gsm_enterPinPuk(const char* string);
bool            gsm_getIMEI(char* string, uint8_t sizeOfString);
bool            gsm_getVersion(char* string, uint8_t sizeOfString);
bool            gsm_getModel(char* string, uint8_t sizeOfString);
bool            gsm_getServiceProviderName(char* string, uint8_t sizeOfString);
uint8_t         gsm_getSignalQuality_0_to_100(void);
bool            gsm_ussd(char *command, char *answer, uint16_t sizeOfAnswer, uint8_t waitSecond);
bool            gsm_tonePlay(Gsm_Tone_t Gsm_Tone_, uint32_t durationMiliSecond, uint8_t level_0_100); 
bool            gsm_toneStop(void);
bool            gsm_dtmf(char *string, uint32_t durationMiliSecond);
//###################################################################   message functions
bool            gsm_msg_textMode(bool on_off);
bool            gsm_msg_isTextMode(void);
bool            gsm_msg_selectStorage(Gsm_Msg_Store_t Gsm_Msg_Store_);
bool            gsm_msg_selectCharacterSet(Gsm_Msg_ChSet_t Gsm_Msg_ChSet_);
bool            gsm_msg_deleteAll(void);
bool            gsm_msg_delete(uint16_t index);
bool            gsm_msg_send(const char *number,const char *msg);
bool            gsm_msg_read(uint16_t index);
uint16_t        gsm_msg_getStorageUsed(void);
uint16_t        gsm_msg_getStorageTotal(void);
uint16_t        gsm_msg_getStorageFree(void);
//###################################################################   call functions
bool            gsm_call_answer(void);
bool            gsm_call_end(void);
bool            gsm_call_dial(const char* number, uint8_t waitSecond);
//###################################################################   gprs functions
bool            gsm_gprs_setApName(char *apName);
bool            gsm_gprs_connect(void);
bool            gsm_gprs_disconnect(void);

bool            gsm_gprs_httpInit(void);
bool            gsm_gprs_httpSetContent(const char *content);
bool            gsm_gprs_httpSetUserData(const char *data);
bool            gsm_gprs_httpSendData(const char *data, uint16_t timeoutMs);
int16_t         gsm_gprs_httpGet(const char *url, bool ssl, uint16_t timeoutMs);
int16_t         gsm_gprs_httpPost(const char *url, bool ssl, uint16_t timeoutMs);
uint16_t        gsm_gprs_httpRead(uint16_t len);  //  data into gsm.gprs.buff
bool            gsm_gprs_httpTerminate(void);

bool            gsm_gprs_ftpLogin(char *ftpAddress, char *ftpUserName, char *ftpPassword, uint16_t port);
Gsm_Ftp_Error_t gsm_gprs_ftpUploadBegin(bool asciiFile, bool append, const char *path, const char *fileName, const uint8_t *data, uint16_t len);
Gsm_Ftp_Error_t gsm_gprs_ftpUpload(const uint8_t *data, uint16_t len);
bool            gsm_gprs_ftpUploadEnd(void);
bool            gsm_gprs_ftpExtUploadBegin(bool asciiFile, bool append, const char *path, const char *fileName);
bool            gsm_gprs_ftpExtUpload(uint8_t *data, uint16_t len);
Gsm_Ftp_Error_t gsm_gprs_ftpExtUploadEnd(void);
Gsm_Ftp_Error_t gsm_gprs_ftpCreateDir(const char *path);
Gsm_Ftp_Error_t gsm_gprs_ftpRemoveDir(const char *path);
uint32_t        gsm_gprs_ftpGetSize(const char *path, const char *name);
Gsm_Ftp_Error_t gsm_gprs_ftpRemove(const char *path, const char *name);
bool            gsm_gprs_ftpIsBusy(void);
bool            gsm_gprs_ftpQuit(void);

bool            gsm_gprs_tcpConnect(const char *address, uint16_t port, bool ssl);
bool            gsm_gprs_tcpSend(const uint8_t *data, uint16_t len);

bool            gsm_gprs_ntpServer(char *server);
bool            gsm_gprs_ntpGetTime(char *timeString);
//###################################################################   bluetooth functions

//###################################################################   library callback functions
void            gsm_callback_simcardReady(void);
void            gsm_callback_simcardPinRequest(void);
void            gsm_callback_simcardPukRequest(void);
void            gsm_callback_simcardNotInserted(void);
void            gsm_callback_newMsg(char *number, Gsm_Time_t time, char *msg);
void            gsm_callback_newCall(char *number);
void            gsm_callback_endCall(void);
void            gsm_callback_nowAnswer(void);
void            gsm_callback_dtmf(char key);
void            gsm_callback_gprsDisconnected(void);
void            gsm_callback_gprsConnected(void);
void            gsm_callback_gprsGotData(uint8_t *data, uint16_t len);
//###################################################################


#endif

