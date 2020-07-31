
#include "gsm.h"
//#############################################################################################
osThreadId            gsmTaskHandle;
osMutexId             gsmMutexID;  
Gsm_t                 gsm;

const char            *_GSM_ALWAYS_SEARCH[] =
{
  "\r\n+CLIP:",               //  0
  "POWER DOWN\r\n",           //  1
  "\r\n+CMTI:",               //  2  
  "\r\nNO CARRIER\r\n",       //  3
  "\r\n+DTMF:"                //  4
};
//#############################################################################################
void                  gsm_init_config(void);
//#############################################################################################
void gsm_at_rxCallback(void)
{
  if (LL_USART_IsActiveFlag_RXNE(_GSM_USART))
  {
    uint8_t tmp = LL_USART_ReceiveData8(_GSM_USART);
    if (gsm.at.index < _GSM_RXSIZE - 1)
    {
      gsm.at.buff[gsm.at.index] = tmp;
      gsm.at.index++;
    }
    gsm.at.rxTime = HAL_GetTick();
  }    
}
//#############################################################################################
void gsm_at_checkRxBuffer(void)
{
  if (gsm.at.rxCheckBusy == 1)
    return;
  gsm.at.rxCheckBusy = 1;
  if ((gsm.at.index > 0) && (HAL_GetTick() - gsm.at.rxTime >= _GSM_RXTIMEOUT))
  {
    #if _GSM_DEBUG == 1
    printf("%s",(const char*)gsm.at.buff);
    #endif
    //  +++ search answer atcommand
    for (uint8_t i = 0; i < _GSM_AT_MAX_ANSWER_ITEMS; i++)
    {
      if (gsm.at.answerSearch[i] != NULL)
      {
        char *found = strstr((char*)gsm.at.buff, gsm.at.answerSearch[i]);
        if (found != NULL)
        {
          gsm.at.answerFound = i;
          if (gsm.at.answerString != NULL)
            strncpy(gsm.at.answerString, found, gsm.at.answerSize);
          break;
        }
      }
      else
        break;        
    }    
    //  --- search answer atcommand
    
    //  +++ search always 
    for (uint8_t i = 0; i < sizeof(_GSM_ALWAYS_SEARCH) / 4; i++)
    {
      char *str = strstr((char*)gsm.at.buff, _GSM_ALWAYS_SEARCH[i]);
      if(str != NULL)
      {
        switch (i)
        {
          case 0:   //  found   "\r\n+CLIP:"      
            if (sscanf(str,"\r\n+CLIP: \"%[^\"]\"", gsm.call.number) == 1)
              gsm.call.ringing = 1;
          break;
          case 1:   //  found   "POWER DOWN\r\n"
            gsm.power = 0;    
          break;
          case 2:   //  found   "\r\n+CMTI:"
            str = strchr(str, ',');
            if (str != NULL)
            {
              str++;
              gsm.msg.newMsg = atoi(str);
              break;
            }              
          break;
          case 3:   //  found   "\r\nNO CARRIER\r\n"
            gsm.call.callbackEndCall = 1;
          break;  
          case 4:   //  found   "\r\n+DTMF:"
            if (sscanf(str,"\r\n+DTMF: %c", &gsm.call.dtmfKey) == 1)
            {
              gsm_callback_dtmf(gsm.call.dtmfKey, gsm.call.dtmfIndex);
              gsm.call.dtmfIndex++;
            }
          break;
          
        }
      }
    //  --- search always 
    }
    memset(gsm.at.buff,0, _GSM_RXSIZE);
    gsm.at.index = 0;
  }
  gsm.at.rxCheckBusy = 0;
}
//#############################################################################################
void gsm_at_sendString(const char *string)
{
  for(uint16_t i = 0; i<strlen(string); i++)
  {
    while (!LL_USART_IsActiveFlag_TXE(_GSM_USART))
      osDelay(1);
    LL_USART_TransmitData8(_GSM_USART, string[i]);   
  }
  while (!LL_USART_IsActiveFlag_TC(_GSM_USART))
    osDelay(1);
}
//#############################################################################################
uint8_t gsm_at_sendCommand(const char *command, uint32_t waitMs, char *answer, uint16_t sizeOfAnswer, uint8_t items, ...)
{
  if (osMutexWait(gsmMutexID, 10000) != osOK)
    return 0;
  va_list tag;
  va_start (tag, items);
  for (uint8_t i = 0; i < items; i++)
  {
    char *str = va_arg (tag, char*);
    gsm.at.answerSearch[i] = pvPortMalloc(strlen(str));
    if (gsm.at.answerSearch[i] != NULL)
      strcpy(gsm.at.answerSearch[i], str);
  }
  va_end(tag);
  if ((answer != NULL) && (sizeOfAnswer > 0))
  {
    gsm.at.answerSize = sizeOfAnswer;
    gsm.at.answerString = pvPortMalloc(sizeOfAnswer);        
    memset(gsm.at.answerString, 0, sizeOfAnswer); 
  }
  gsm.at.answerFound = -1;
  uint32_t startTime = HAL_GetTick();
  gsm_at_sendString(command);
  while (HAL_GetTick() - startTime < waitMs)
  {
    if (gsm.at.answerFound != -1)
      break;
    gsm_at_checkRxBuffer();
    osDelay(_GSM_RXTIMEOUT);
  }
  for (uint8_t i = 0; i < items; i++)
  {
    //memset(gsm.at.answerSearch[i], 0, strlen(gsm.at.answerSearch[i]));
    vPortFree(gsm.at.answerSearch[i]);
    gsm.at.answerSearch[i] = NULL;
  }
  if ((answer != NULL) && (sizeOfAnswer > 0))
  {
    //memset(gsm.at.answerString, 0, strlen(gsm.at.answerString));
    if (gsm.at.answerFound >= 0)
      strncpy(answer, gsm.at.answerString, sizeOfAnswer);
    vPortFree(gsm.at.answerString);  
    gsm.at.answerString = NULL;
  }  
  osMutexRelease(gsmMutexID);
  return gsm.at.answerFound + 1;   
}
//#############################################################################################
//#############################################################################################
//#############################################################################################
bool gsm_power(bool on_off)
{
  if (on_off)
  {
    if (gsm_at_sendCommand("AT\r\n", 500, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.power = 1;
      gsm_init_config();
      return true;
    }
    HAL_GPIO_WritePin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN, GPIO_PIN_RESET);
    osDelay(1200);
    HAL_GPIO_WritePin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN, GPIO_PIN_SET);
    osDelay(2000);    
    for (uint8_t  i = 0; i < 10; i++)
    {
      if (gsm_at_sendCommand("AT\r\n", 500, NULL, 0, 1, "\r\nOK\r\n") == 1)
      {
        gsm.power = 1;
        gsm_init_config();
        return true;
      }        
    }
    gsm.power = 0;
    return false;
  }
  else
  {
    if (gsm_at_sendCommand("AT\r\n", 500, NULL, 0, 1, "\r\nOK\r\n") == 0)
    {
      gsm.ready = 0;
      gsm.power = 0;
      return true;
    }
    HAL_GPIO_WritePin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN, GPIO_PIN_RESET);
    osDelay(1200);
    HAL_GPIO_WritePin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN, GPIO_PIN_SET);
    osDelay(2000);    
    if (gsm_at_sendCommand("AT\r\n", 500, NULL, 0, 1, "\r\nOK\r\n") == 0)
    {
      gsm.ready = 0;
      gsm.power = 0;
      return true;
    }
    else
    {
      gsm.power = 1;
      return false;
    }      
  }  
}
//#############################################################################################
bool gsm_setDefault(void)
{
  if (gsm_at_sendCommand("AT&F0\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return true;
  return false;
}
//#############################################################################################
bool gsm_saveProfile(void)
{
  if (gsm_at_sendCommand("AT&W\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return true;
  return false;
}
//#############################################################################################
bool gsm_enterPinPuk(const char* string)
{
  char str[32];
  if (string == NULL)
    return false;
  sprintf(str, "AT+CPIN=%s\r\n", string);
  if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;  
}
//#############################################################################################
bool gsm_getIMEI(char* string, uint8_t sizeOfString)
{
  if ((string == NULL) || (sizeOfString < 15))
    return false;
  char str[32];
  if (gsm_at_sendCommand("AT+GSN\r\n", 1000 , str, sizeof(str), 2, "AT+GSN", "\r\nERROR\r\n") != 1)
    return false;
  if (sscanf(str,"\r\nAT+GSN\r\n %[^\r\n]", string) != 1)
    return false;
  return true;  
}
//#############################################################################################
bool gsm_getVersion(char* string, uint8_t sizeOfString)
{
  if (string == NULL)
    return false;
  char str1[16 + sizeOfString];
  char str2[sizeOfString + 1];
  if (gsm_at_sendCommand("AT+CGMR\r\n", 1000 , str1, sizeof(str1), 2, "AT+GMM", "\r\nERROR\r\n") != 1)
    return false;
  if (sscanf(str1,"\r\nAT+CGMR\r\n %[^\r\n]", str2) != 1)
    return false;
  strncpy(string, str2, sizeOfString);
  return true; 
}
//#############################################################################################
bool gsm_getModel(char* string, uint8_t sizeOfString)
{
  if (string == NULL)
    return false;
  char str1[16 + sizeOfString];
  char str2[sizeOfString + 1];
  if (gsm_at_sendCommand("AT+GMM\r\n", 1000 , str1, sizeof(str1), 2, "AT+GMM", "\r\nERROR\r\n") != 1)
    return false;
  if (sscanf(str1,"\r\nAT+GMM\r\n %[^\r\n]", str2) != 1)
    return false;
  strncpy(string, str2, sizeOfString);
  return true;  
}
//#############################################################################################
bool gsm_getServiceProviderName(char* string, uint8_t sizeOfString)
{
  if (string == NULL)
    return false;
  char str1[16 + sizeOfString];
  char str2[sizeOfString + 1];
  if (gsm_at_sendCommand("AT+CSPN?\r\n", 1000 , str1, sizeof(str1), 2, "\r\n+CSPN:", "\r\nERROR\r\n") != 1)
    return false;
  if (sscanf(str1,"\r\n+CSPN: \"%[^\"]\"", str2) != 1)
    return false;
  strncpy(string, str2, sizeOfString);
  return true;  
}
//#############################################################################################
uint8_t gsm_getSignalQuality_0_to_100(void)
{
  char str[32];
  uint8_t p1,p2;
  if (gsm_at_sendCommand("AT+CSQ\r\n", 1000, str, sizeof(str), 2, "\r\n+CSQ:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str, "\r\n+CSQ: %hhd,%hhd\r\n", &p1, &p2) != 2)
    return 0;
  if (p1 == 99)
    gsm.signal = 0;
  else
    gsm.signal = (p1 * 100) / 31;
  return gsm.signal;
}
//#############################################################################################
bool gsm_ussd(char *command, char *answer, uint16_t sizeOfAnswer, uint8_t waitSecond)
{
  if (command == NULL)
  {
    if (gsm_at_sendCommand("AT+CUSD=2\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;
    return true;    
  }
  else if (answer == NULL)
  {
    char str[16 + strlen(command)];
    sprintf(str, "AT+CUSD=0,\"%s\"\r\n", command);    
    if (gsm_at_sendCommand(str, waitSecond * 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;
    return true;
  }
  else
  {
    char str[16 + strlen(command)];
    char str2[sizeOfAnswer + 32]; 
    sprintf(str, "AT+CUSD=1,\"%s\"\r\n", command);    
    if (gsm_at_sendCommand(str, waitSecond * 1000 , str2, sizeof(str2), 2, "\r\n+CUSD:", "\r\nERROR\r\n") != 1)
    {
      gsm_at_sendCommand("AT+CUSD=2\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
      return false;
    }
    char *start = strstr(str2, "\"");
    char *end = strstr(str2, "\", ");
    if (start != NULL && end != NULL)
    {
      start++;
      strncpy(answer, start, end - start);
      return true;      
    }
    else
      return false;    
  }
}
//#############################################################################################
bool gsm_waitForReady(uint8_t waitSecond)
{
  uint32_t startTime = HAL_GetTick();
  while (HAL_GetTick() - startTime < (waitSecond * 1000))
  {
    osDelay(100);
    if (gsm.ready == 1)
      return true;
  } 
  return false;  
}
//#############################################################################################
bool gsm_tonePlay(Gsm_Tone_t Gsm_Tone_, uint32_t durationMiliSecond, uint8_t level_0_100)
{
  char str[32];
  sprintf(str, "AT+SNDLEVEL=0,%d\r\n", level_0_100);
  if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+STTONE=1,%d,%d\r\n", Gsm_Tone_, durationMiliSecond);
  if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;  
}
//#############################################################################################
bool gsm_toneStop(void)
{
  if (gsm_at_sendCommand("AT+STTONE=0\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return true;
  return false;
}
//#############################################################################################
bool gsm_dtmf(char *string, uint32_t durationMiliSecond)
{
  char str[32];
  sprintf(str, "AT+VTS=\"%s\",%d\r\n", string, durationMiliSecond / 100);
  if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true; 
}
//#############################################################################################
void gsm_init_config(void)
{
  char str1[32];
  char str2[16];
  gsm_setDefault();
  gsm_at_sendCommand("ATE1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+COLP=1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+CLIP=1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+CREG=1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+FSHEX=0\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  for (uint8_t i = 0; i < 5 ; i++)
  {
    if (gsm_at_sendCommand("AT+CPIN?\r\n", 1000, str1, sizeof(str1), 2, "\r\n+CPIN:", "\r\nERROR\r\n") == 1)
    {
      if (sscanf(str1, "\r\n+CPIN: %[^\r\n]", str2) == 1)
      {
        if (strcmp(str2, "READY") == 0)
        {
          gsm_callback_simcardReady();
          break;
        }
        if (strcmp(str2, "SIM PIN") == 0)
        {
          gsm_callback_simcardPinRequest();
          break;
        }
        if (strcmp(str2, "SIM PUK") == 0)
        {
          gsm_callback_simcardPukRequest();
          break;
        }
      }      
    }
    else
    {
      osDelay(2000);
    }
  } 
  for (uint8_t i = 0; i < 5 ; i++)
  {  
    if (gsm_msg_textMode(true))
      break;
    else
      osDelay(2000);    
  }
  for (uint8_t i = 0; i < 5 ; i++)
  {  
    if (gsm_msg_selectStorage(Gsm_Msg_Store_MODULE))
      break;
    else
      osDelay(2000);    
  }
  for (uint8_t i = 0; i < 5 ; i++)
  {  
    if (gsm_msg_selectCharacterSet(Gsm_Msg_ChSet_IRA))
      break;
    else
      osDelay(2000);    
  }
  gsm_at_sendCommand("AT+DDET=1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
}  
//#############################################################################################
//#############################################################################################
//#############################################################################################
void gsm_task(void const * argument)
{
  static uint32_t gsm10sTimer = 0;
  static uint32_t gsm60sTimer = 0;
  static uint8_t  gsmError = 0;
  gsm.inited = 1;
  while (HAL_GetTick() < 2000)
    osDelay(100);
  LL_USART_EnableIT_RXNE(_GSM_USART);  
  gsm_power(true);
  if (gsm.msg.storageUsed > 0)
  {
    for (uint16_t i = 0; i < 150 ; i++)
    {
      if (gsm_msg_read(i))
      {
        gsm_callback_newMsg(gsm.msg.number, gsm.msg.time, (char*)gsm.msg.buff);
        gsm_msg_delete(i);
      }
    }
  }
  gsm.ready = 1;
  while (1)
  {
    if (gsm.power == 1)
    {
      gsm_at_checkRxBuffer();
      if (gsm.call.callbackEndCall == 1)  // \r\nNO CARRIER\r\n detect
      {
        gsm.call.dtmfIndex = 0;
        gsm.call.callbackEndCall = 0;
        gsm_callback_endCall();
        gsm.call.busy = 0;
      }
      if (HAL_GetTick() - gsm10sTimer >= 10000)
      {
        gsm10sTimer = HAL_GetTick();
        if (gsm_getSignalQuality_0_to_100() < 10) //  update signal value every 10 seconds
        {
          gsmError++;
          if (gsmError == 6)  //  restart after 60 seconds while low signal
          {
            gsm_power(false);
            osDelay(2000);
            gsm_power(true);
          }
        }
        else
          gsmError = 0;
      }
      if (HAL_GetTick() - gsm60sTimer >= 60000) 
      {
        gsm60sTimer = HAL_GetTick();
        if (gsm_msg_getStorageUsed() > 0) //  check sms memory every 60 seconds
        {
          for (uint16_t i = 0; i < 150 ; i++)
          {
            if (gsm_msg_read(i))
            {
              gsm_callback_newMsg(gsm.msg.number, gsm.msg.time, (char*)gsm.msg.buff);
              gsm_msg_delete(i);
            }
          }
        }        
      }
      if (gsm.msg.newMsg != -1)
      {
        if (gsm_msg_read(gsm.msg.newMsg))
        {
          gsm_callback_newMsg(gsm.msg.number, gsm.msg.time, (char*)gsm.msg.buff);
          gsm_msg_delete(gsm.msg.newMsg);
        }        
        gsm.msg.newMsg = -1;
      }
      if (gsm.call.ringing == 1)
      {
        gsm_callback_newCall(gsm.call.number);
        gsm.call.ringing = 0;
      }      
    }
    else
    {
      gsm_power(true);  //  turn on again, after power down
    }
    osDelay(_GSM_RXTIMEOUT);
  }
}
//#############################################################################################
bool gsm_init(osPriority osPriority_)
{
  if (gsm.inited == 1)
    return true;
  HAL_GPIO_WritePin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN, GPIO_PIN_SET);
  osMutexDef(gsmMutex); 
  gsmMutexID = osMutexCreate(osMutex (gsmMutex));
  osThreadStaticDef(gsmTask, gsm_task, osPriority_, 0, _GSM_TASKSIZE, NULL, NULL);
  gsmTaskHandle = osThreadCreate(osThread(gsmTask), NULL);
  if ((gsmTaskHandle != NULL) && (gsmMutexID != NULL))
    return true;
  else
    return false;
}
