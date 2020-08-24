#ifndef _GSMCONFIG_H
#define _GSMCONFIG_H

//  board config
#define   _GSM_DEBUG                1
#define   _GSM_USART                USART3
#define   _GSM_POWERKEY_GPIO        GSM_KEY_GPIO_Port
#define   _GSM_POWERKEY_PIN         GSM_KEY_Pin

//  enable/disable  
#define   _GSM_CALL_ENABLE          1
#define   _GSM_MSG_ENABLE           1
#define   _GSM_DTMF_DETECT_ENABLE   1
#define   _GSM_GPRS_ENABLE          1
#define   _GSM_BLUETOOTH_ENABLE     0 //  not support yet

//  do not change
#define   _GSM_AT_MAX_ANSWER_ITEMS  5                  
#define   _GSM_TASKSIZE             512  
#define   _GSM_BUFFTASKSIZE         128
#define   _GSM_RXSIZE               512  
#define   _GSM_RXTIMEOUT            100  

#endif
