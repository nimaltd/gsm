
#include "gsm.h"

Gsm_t gsm;

//#####################################################################################################
//#######################             AT COMMANDS             #########################################
//#####################################################################################################
void gsm_at_rxCallback(void)
{
  if (LL_USART_IsActiveFlag_RXNE(_GSM_USART))
  {
    if (gsm.at.rxIndex < _GSM_RXSIZE - 1)
    {
      gsm.at.rxBuffer[gsm.at.rxIndex] = LL_USART_ReceiveData8(_GSM_USART);      
      gsm.at.rxIndex++;
    }
    else
      LL_USART_ReceiveData8(_GSM_USART);      
  }
  gsm.at.rxTime = HAL_GetTick();
}
//#####################################################################################################
void  gsm_at_txCallback(void)
{
  #if _GSM_TXDMA_ENABLE == 1
  if (_GSM_DMA_IS_ACTIVE(_GSM_DMA))
  {
    _GSM_DMA_CLEAR(_GSM_DMA);
    gsm.at.txDone = 1;    
  }      
  #endif
}
//#####################################################################################################
void gsm_at_foundCllback(const char *foundString, char *str)
{
  if (strcmp(foundString, "\r\n+CLIP:") == 0)
  {
    if (sscanf(str,"\r\n+CLIP: \"%[^\"]\"", gsm.call.incommingNumber) == 1)
      gsm.call.ringing = 1;
    return;
  }
  if (strcmp(foundString, "\r\nNO CARRIER\r\n") == 0)
  {
    gsm.call.inUse = 0;
    gsm_user_endCall();
  }
  
}
//#####################################################################################################
bool gsm_at_addAutoSearchString(const char *str)
{
  for (uint8_t s = 0; s < _GSM_MAX_ALWAYS_SEARCHING_STRING ; s++)
  {  
    if(gsm.at.alwaysSearchingString[s] == NULL)
    {
      #if (_GSM_USE_FREEEROS == 0)
      gsm.at.alwaysSearchingString[s] = malloc(strlen(str));            
      #else
      gsm.at.alwaysSearchingString[s] = pvPortMalloc(strlen(str));            
      #endif
      if (gsm.at.alwaysSearchingString[s] != NULL)
      {
        strcpy(gsm.at.alwaysSearchingString[s], str);
        return true;
      }
      else      
        break;
    }   
  }
  return false;
}
//#####################################################################################################
bool gsm_at_sendString(char *str, uint32_t timeout)
{
  uint32_t startTime = HAL_GetTick();
  while (gsm.at.txBusy == 1)
  {
    gsm_delay(1);
    if(HAL_GetTick() - startTime >= timeout)
      return false;
  }
  gsm.at.txBusy = 1;
  dprintf("<<GSM>> TX : %s\r\n", str);  
  #if (_GSM_TXDMA_ENABLE == 0)
  for(uint16_t i = 0 ; i<strlen(str) ; i++)
  {
    while (!LL_USART_IsActiveFlag_TXE(_GSM_USART))
    {
      if(HAL_GetTick() - startTime > timeout)
      {
        dprintf("<<GSM>> TX timeout\r\n");
        gsm.at.txBusy = 0;
        return false;
      }
    }
    LL_USART_TransmitData8(_GSM_USART, str[i]);   
  }
  while (!LL_USART_IsActiveFlag_TC(_GSM_USART))
  {
    if(HAL_GetTick() - startTime > timeout)
    {
      dprintf("<<GSM>> TX timeout\r\n");
      gsm.at.txBusy = 0;
      return false;
    }      
  }
  #else
  gsm.at.txDone = 0;
  LL_DMA_ConfigAddresses(_GSM_DMA, _GSM_DMA_CHANNEL_OR_STREAM,\
    (uint32_t)str, LL_USART_DMA_GetRegAddr(_GSM_USART),\
    LL_DMA_GetDataTransferDirection(_GSM_DMA, _GSM_DMA_CHANNEL_OR_STREAM));
  LL_DMA_SetDataLength(_GSM_DMA, _GSM_DMA_CHANNEL_OR_STREAM, strlen(str));
  _GSM_DMA_ENABLE_CHANNEL_OR_STREAM(_GSM_DMA, _GSM_DMA_CHANNEL_OR_STREAM);
  while(gsm.at.txDone == 0)
  {
    if(HAL_GetTick() - startTime > timeout)
    {
      _GSM_DMA_DISABLE_CHANNEL_OR_STREAM(_GSM_DMA, _GSM_DMA_CHANNEL_OR_STREAM);
      dprintf("<<GSM>> TX timeout\r\n");
      gsm.at.txBusy = 0;
      return false;
    }
    gsm_delay(1);
  }
  #endif
  gsm.at.txBusy = 0;
  return true;
}
//#####################################################################################################
void gsm_at_process(void)
{    
  if ((gsm.at.rxIndex > 0) && (HAL_GetTick() - gsm.at.rxTime > _GSM_RXTIMEOUT))
  {
    //  +++ answer AtCommand
    for (uint8_t ans = 0 ; ans < _GSM_MAX_ANSWER_SEARCHING_STRING ; ans++)
    {
      if (gsm.at.answerSearchingString[ans] == NULL)
        break;
      char *ansStr = strstr((char*)gsm.at.rxBuffer, gsm.at.answerSearchingString[ans]);         
      if(ansStr != NULL)
      {
        if(gsm.at.foundAnswerString != NULL)
          strncpy(gsm.at.foundAnswerString, ansStr, gsm.at.foundAnswerSize);
        gsm.at.foundAnswer = ans; 
        break;          
      }        
    }
    //  --- answer AtCommand     
    //  +++ auto searching  
    for(uint8_t au = 0 ; au < _GSM_MAX_ALWAYS_SEARCHING_STRING ; au++)
    {
      if(gsm.at.alwaysSearchingString[au] == NULL)
        break;
      char *autoStr = strstr((char*)gsm.at.rxBuffer, gsm.at.alwaysSearchingString[au]);
      if(autoStr != NULL)
      {
        dprintf("<<GSM>> found : %s\r\n", gsm.at.alwaysSearchingString[au]);
        gsm_at_foundCllback(gsm.at.alwaysSearchingString[au], autoStr);
      }
    }
    //  --- auto searching
    dprintf("<<GSM>> RX : %s\r\n", gsm.at.rxBuffer);
    gsm.at.rxIndex = 0;
    memset(gsm.at.rxBuffer, 0, _GSM_RXSIZE);
  }  
}
//#####################################################################################################
uint8_t gsm_at_sendCommand(char *atCommand, uint32_t wait_ms, char *answerString, uint16_t answerSize, uint8_t searchingItems,...)
{
  gsm_at_process();
  while (gsm.at.atBusy == 1)
    gsm_delay(1);
  gsm.at.atBusy = 1;
  uint32_t startTime;
  gsm.at.foundAnswerSize = answerSize;
  gsm.at.foundAnswerString = answerString;
  va_list tag;
  va_start (tag,searchingItems);
  for (uint8_t i = 0; i < searchingItems ; i++)
  {
    char *str = va_arg (tag, char*);
    #if _GSM_USE_FREEEROS == 0
    gsm.at.answerSearchingString[i] = malloc(strlen(str));
    #else
    gsm.at.answerSearchingString[i] = pvPortMalloc(strlen(str));
    #endif
    if (gsm.at.answerSearchingString[i] != NULL)
      strcpy(gsm.at.answerSearchingString[i], str);
  }
  va_end(tag);
  gsm.at.foundAnswer = -1;
  uint8_t retValue = 0;
  if (gsm_at_sendString(atCommand, 1000) == 0)
    goto END;  
  if (answerString != NULL)
    memset(answerString, 0 , answerSize);
  startTime = HAL_GetTick();  
  while (1)
  {
    gsm_delay(1);
    gsm_at_process();  
    if (gsm.at.foundAnswer >= 0)
    {
      retValue = gsm.at.foundAnswer + 1;
      break;
    }
    if (HAL_GetTick() - startTime > wait_ms)
    {
      if (searchingItems == 0)
        retValue = 1;  
      break;        
    }
  }
  END:
  for (uint8_t i = 0 ; i < _GSM_MAX_ANSWER_SEARCHING_STRING ; i++)
  {
    #if _GSM_USE_FREEEROS == 0
    free(gsm.at.answerSearchingString[i]);
    #else
    vPortFree(gsm.at.answerSearchingString[i]);
    #endif
    gsm.at.answerSearchingString[i] = NULL;
  }
  gsm.at.atBusy = 0;
  return retValue;
}  
//#####################################################################################################
//#######################             GSM FUNCTIONS           #########################################
//#####################################################################################################
bool gsm_init(void)
{
  if (gsm.inited == 1)
    return 0;
  LL_GPIO_SetOutputPin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN);
  memset(&gsm, 0, sizeof(gsm));    
  #if _GSM_TXDMA_ENABLE == 1
  LL_DMA_EnableIT_TC(_GSM_DMA, _GSM_DMA_CHANNEL_OR_STREAM);
  LL_USART_EnableDMAReq_TX(_GSM_USART);
  #endif
  LL_USART_EnableIT_RXNE(_GSM_USART);
  while (HAL_GetTick() < 1000)
    gsm_delay(1);
  for (uint8_t i = 0; i < 10 ; i++)
  {
    if (gsm_at_sendCommand("AT\r\n", 100, NULL, 0, 1, "\r\nOK\r\n") == 1) 
    goto INIT;
  }
  LL_GPIO_ResetOutputPin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN);
  gsm_delay(1200);
  LL_GPIO_SetOutputPin(_GSM_POWERKEY_GPIO, _GSM_POWERKEY_PIN);
  gsm_delay(2000);
  for (uint8_t i = 0; i < 10 ; i++)
  {
    if (gsm_at_sendCommand("AT\r\n", 100, NULL, 0, 1, "\r\nOK\r\n") == 1) 
      goto INIT;
  }
  return 0;
  INIT:
  gsm_at_addAutoSearchString("\r\n+CLIP:");
  gsm_at_addAutoSearchString("\r\nNO CARRIER\r\n");
  gsm_at_addAutoSearchString("\r\nNO ANSWER\r\n");
  gsm_at_addAutoSearchString("\r\n+CMTI:");
  gsm_at_sendCommand("AT+CNMI=2,1,0,0,0\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+COLP=1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+CREG=1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_at_sendCommand("AT+FSHEX=0\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  gsm_setEcho(true);
  gsm_getIMEI(NULL);
  gsm_msg_getCharacterSet();
  gsm_getGlobalObjectIdentification(NULL);
  gsm_getLoadSpeakerLevel();
  gsm_getManufacturerIdentification(NULL);
  gsm_getMicrophoneGainLevel_auxChannel();
  gsm_getMicrophoneGainLevel_auxChannelHF();
  gsm_getMicrophoneGainLevel_mainChannel();
  gsm_getMicrophoneGainLevel_mainChannelHF();
  gsm_getModelIdentification(NULL);
  gsm_getProductIdentificationInformation(NULL);
  gsm_getRingLevel();
  gsm_getServiceProviderNameFromSimcard(NULL);
  gsm_getPinStatus();
  gsm_msg_updateStorage();
  gsm_updateEchoCancellationControl();
  gsm_user_init();
  gsm.inited = 1;
  return 1;
}
//#####################################################################################################
void gsm_process(void)
{
  if (gsm.at.atBusy == 0)
    gsm_at_process(); 
  if (gsm.call.ringing == 1)
  {
    gsm.call.ringing = 0;
    gsm_user_incommingCall(gsm.call.incommingNumber);
  }    
}
//#####################################################################################################
bool gsm_getRegisteredStatus(void)
{
  char  str[64];
  uint8_t p1,p2;
  if (gsm_at_sendCommand("AT+CREG?\r\n", 1000, str, sizeof(str), 2, "\r\n+CREG:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str, "\r\n+CREG: %hhd,%hhd\r\n", &p1, &p2) != 2)
    return 0;
  if (p2 == 1 || p2 == 5)
    return 1;
  else
    return 0;    
}
//#####################################################################################################
uint8_t gsm_getSignalQuality(void)
{
  char  str[64];
  uint8_t p1,p2;
  if (gsm_at_sendCommand("AT+CSQ\r\n", 1000, str, sizeof(str), 2, "\r\n+CSQ:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str, "\r\n+CSQ: %hhd,%hhd\r\n", &p1, &p2) != 2)
    return 0;
  if (p1 == 99)
    return 0;
  return (p1 * 100) / 31;
}
//#####################################################################################################
bool gsm_sendDtmf(char *dtmfString, uint16_t duration_ms)
{
  char  str[128];
  if (duration_ms < 100)
    duration_ms = 100;
  if (duration_ms > 2550)
    duration_ms = 2550;
  sprintf(str, "AT+VTS=%s,%d\r\n", dtmfString, duration_ms / 100);
  if (gsm_at_sendCommand(str, duration_ms * strlen(dtmfString) , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return 1;   
}
//#####################################################################################################
bool gsm_setFlightMode(bool enable)
{
  if (enable)
  {
    if (gsm_at_sendCommand("AT+CFUN=1\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.config.flightMode = 1;
      return 1;  
    }
  }
  else
  {
    if (gsm_at_sendCommand("AT+CFUN=4\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.config.flightMode = 0;
      return 1;  
    }
  }
  return 0;
}
//#####################################################################################################
bool gsm_getFlightMode(void)
{
  uint8_t ans =gsm_at_sendCommand("AT+CFUN?\r\n", 1000, NULL, 0, 3, "\r\n+CFUN: 1\r\n", "\r\n+CFUN: 4\r\n", "\r\nERROR\r\n");
  if (ans == 1)
  {
    gsm.config.flightMode = 0;
    return 0;
  }
  else
  {
    gsm.config.flightMode = 1;
    return 1;
  }
}
//#####################################################################################################
bool gsm_setRingLevel(uint8_t level_0_100)
{
  char  str[64];
  if (level_0_100 > 100)
    level_0_100 = 100;
  sprintf(str, "AT+CRSL=%d\r\n", level_0_100);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.ringLevel = level_0_100;
  return 1;   
}
//#####################################################################################################
uint8_t gsm_getRingLevel(void)
{
  char  str[64];
  uint8_t p1;
  if (gsm_at_sendCommand("AT+CRSL?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CRSL:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\n+CRSL: %hhd\r\n", &p1) != 1)
    return 0;
  gsm.config.ringLevel = p1;
  return p1;  
}
//#####################################################################################################
bool gsm_setLoadSpeakerLevel(uint8_t level_0_100)
{
  char  str[64];
  if (level_0_100 > 100)
    level_0_100 = 100;
  sprintf(str, "AT+CLVL=%d\r\n", level_0_100);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.loadSpeakerLevel = level_0_100;
  return 1;   
}
//#####################################################################################################
uint8_t gsm_getLoadSpeakerLevel(void)
{
  char  str[64];
  uint8_t p1;
  if (gsm_at_sendCommand("AT+CLVL?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CLVL:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\n+CLVL: %hhd\r\n", &p1) != 1)
    return 0;
  gsm.config.loadSpeakerLevel = p1;
  return p1;  
}
//#####################################################################################################
bool gsm_setMicrophoneGainLevel_mainChannel(int8_t level_0_15)
{
  char  str[64];
  if (level_0_15 > 15)
    level_0_15 = 15;
  sprintf(str, "AT+CLVL=0,%d\r\n", level_0_15);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.micGainLevel_mainCh = level_0_15;
  return 1;   
}
//#####################################################################################################
uint8_t gsm_getMicrophoneGainLevel_mainChannel(void)
{
  char  str[64];
  uint8_t p1;
  if (gsm_at_sendCommand("AT+CMIC?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CMIC:", "\r\nERROR\r\n") != 1)
    return 0;
  char *s = strstr(str, "(0,");
  if (s == NULL)
    return 0;
  if (sscanf(s,"(0,%hhd)", &p1) != 1)
    return 0;
  gsm.config.micGainLevel_mainCh = p1;
  return p1;  
}
//#####################################################################################################
bool gsm_setMicrophoneGainLevel_auxChannel(int8_t level_0_15)
{
  char  str[64];
  if (level_0_15 > 15)
    level_0_15 = 15;
  sprintf(str, "AT+CLVL=1,%d\r\n", level_0_15);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.micGainLevel_auxCh = level_0_15;
  return 1;   
}
//#####################################################################################################
uint8_t gsm_getMicrophoneGainLevel_auxChannel(void)
{
  char  str[64];
  uint8_t p1;
  if (gsm_at_sendCommand("AT+CMIC?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CMIC:", "\r\nERROR\r\n") != 1)
    return 0;
  char *s = strstr(str, "(1,");
  if (s == NULL)
    return 0;
  if (sscanf(s,"(1,%hhd)", &p1) != 1)
    return 0;
  gsm.config.micGainLevel_auxCh = p1;
  return p1;  
}
//#####################################################################################################
bool gsm_setMicrophoneGainLevel_mainChannelHF(int8_t level_0_15)
{
  char  str[64];
  if (level_0_15 > 15)
    level_0_15 = 15;
  sprintf(str, "AT+CLVL=2,%d\r\n", level_0_15);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.micGainLevel_mainHfCh = level_0_15;
  return 1;   
}
//#####################################################################################################
uint8_t gsm_getMicrophoneGainLevel_mainChannelHF(void)
{
  char  str[64];
  uint8_t p1;
  if (gsm_at_sendCommand("AT+CMIC?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CMIC:", "\r\nERROR\r\n") != 1)
    return 0;
  char *s = strstr(str, "(2,");
  if (s == NULL)
    return 0;
  if (sscanf(s,"(2,%hhd)", &p1) != 1)
    return 0;
  gsm.config.micGainLevel_mainHfCh = p1;
  return p1;  
}
//#####################################################################################################
bool gsm_setMicrophoneGainLevel_auxChannelHF(int8_t level_0_15)
{
  char  str[64];
  if (level_0_15 > 15)
    level_0_15 = 15;
  sprintf(str, "AT+CLVL=3,%d\r\n", level_0_15);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.micGainLevel_auxHfCh = level_0_15;
  return 1;   
}
//#####################################################################################################
uint8_t gsm_getMicrophoneGainLevel_auxChannelHF(void)
{
  char  str[64];
  uint8_t p1;
  if (gsm_at_sendCommand("AT+CMIC?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CMIC:", "\r\nERROR\r\n") != 1)
    return 0;
  char *s = strstr(str, "(3,");
  if (s == NULL)
    return 0;
  if (sscanf(s,"(3,%hhd)", &p1) != 1)
    return 0;
  gsm.config.micGainLevel_auxHfCh = p1;
  return p1;  
}
//#####################################################################################################
bool gsm_getServiceProviderNameFromSimcard(char *string)
{
  char  str[64];
  if (gsm_at_sendCommand("AT+CSPN?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CSPN:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\n+CSPN: \"%[^\"]\"", gsm.config.providerSimcardName) != 1)
    return 0;
  if (string != NULL)
    strcpy(string, gsm.config.providerSimcardName);
  return 1;
}
//#####################################################################################################
bool gsm_getManufacturerIdentification(char *string)
{
  char  str[64];
  if (string == NULL)
    return 0;
  if (gsm_at_sendCommand("AT+GMI\r\n", 1000 , str, sizeof(str), 2, "AT+GMI", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\nAT+GMI\r\n %[^\r\n]", string) != 1)
    return 0;
  return 1;
}
//#####################################################################################################
bool gsm_getModelIdentification(char *string)
{
  char  str[64];
  if (gsm_at_sendCommand("AT+GMM\r\n", 1000 , str, sizeof(str), 2, "AT+GMM", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\nAT+GMM\r\n %[^\r\n]", gsm.config.modelIdentification) != 1)
    return 0;
  if (string != NULL)
    strcpy(string, gsm.config.modelIdentification);
  return 1;
}
//#####################################################################################################
bool gsm_getGlobalObjectIdentification(char *string)
{
  char  str[64];
  if (string == NULL)
    return 0;
  if (gsm_at_sendCommand("AT+GOI\r\n", 1000 , str, sizeof(str), 2, "AT+GOI", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\nAT+GOI\r\n %[^\r\n]", string) != 1)
    return 0;
  return 1;
}
//#####################################################################################################
bool gsm_getProductIdentificationInformation(char *string)
{
  char  str[64];
  if (string == NULL)
    return 0;
  if (gsm_at_sendCommand("ATI\r\n", 1000 , str, sizeof(str), 2, "ATI", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\nATI\r\n %[^\r\n]", string) != 1)
    return 0;
  return 1;
}
//#####################################################################################################
bool gsm_getIMEI(char *string)
{
  char  str[64];
  if (gsm_at_sendCommand("AT+GSN\r\n", 1000 , str, sizeof(str), 2, "AT+GSN", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str,"\r\nAT+GSN\r\n %[^\r\n]", gsm.config.imei) != 1)
    return 0;
  if (string != NULL)
    strcpy(string, gsm.config.imei);
  return 1;
}
//#####################################################################################################
bool gsm_setEcho(bool enable)
{
  if (enable)
  {
    if (gsm_at_sendCommand("ATE1\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
      return 1;  
  }
  else
  {
    if (gsm_at_sendCommand("ATE0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
      return 1;  
  }
  return 0;
}
//#####################################################################################################
bool gsm_setMonitorSpeakerLoudness(uint8_t level_0_9)
{
  char  str[16];
  if (level_0_9 > 15)
    level_0_9 = 15;
  sprintf(str, "ATL%d\r\n", level_0_9);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.loadSpeakerLevel = level_0_9;
  return 1;   
}
//#####################################################################################################
bool gsm_setMonitorSpeakerMode(uint8_t level_0_9)
{
  char  str[16];
  if (level_0_9 > 15)
    level_0_9 = 15;
  sprintf(str, "ATM%d\r\n", level_0_9);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.loadSpeakerLevel = level_0_9;
  return 1;   
}
//#####################################################################################################
bool gsm_setNumRingsAutoAnsweringCall(uint8_t level_0_255)
{
  char  str[16];
  sprintf(str, "ATS0=%d\r\n", level_0_255);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.config.autoAnswerCall = level_0_255;
  return 1;   
}
//#####################################################################################################
Gsm_characterSet_t gsm_msg_getCharacterSet(void)
{
  char  str[32];
  char  name[16];
  if (gsm_at_sendCommand("AT+CSCS?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CSCS:", "\r\nERROR\r\n") != 1)
    return Gsm_characterSet_ERROR;
  if (sscanf(str,"\r\n+CSCS: \"%[^\r\n\"]", name) != 1)
    return Gsm_characterSet_ERROR;
  if (strcmp(name, "8859-1") == 0)
    gsm.msg.characterSet = Gsm_characterSet_8859_1;
  else if (strcmp(name, "GSM") == 0)
  {
    gsm.msg.characterSet = Gsm_characterSet_GSM;
    return Gsm_characterSet_GSM;
  }
  else if (strcmp(name, "HEX") == 0)
  {
    gsm.msg.characterSet = Gsm_characterSet_HEX;
    return Gsm_characterSet_HEX;
  }
  else if (strcmp(name, "IRA") == 0)
  {
    gsm.msg.characterSet = Gsm_characterSet_IRA;
    return Gsm_characterSet_IRA;
  }
  else if (strcmp(name, "PCCP") == 0)
  {
    gsm.msg.characterSet = Gsm_characterSet_PCCP;
    return Gsm_characterSet_PCCP;    
  }
  else if (strcmp(name, "PCDN") == 0)
  {
    gsm.msg.characterSet = Gsm_characterSet_PCDN;
    return Gsm_characterSet_PCDN;
  }
  else if (strcmp(name, "UCS2") == 0)
  {
    gsm.msg.characterSet = Gsm_characterSet_UCS2;
    return Gsm_characterSet_UCS2;
  }
  else
    return Gsm_characterSet_ERROR;      
  return Gsm_characterSet_ERROR;
}
//#####################################################################################################
bool gsm_msg_setCharacterSet(Gsm_characterSet_t Gsm_characterSet_)
{
  char  str[20];
  switch (Gsm_characterSet_)
  {
    case Gsm_characterSet_8859_1:
      sprintf(str, "AT+CSCS=8859-1\r\n");        
    break;
    case Gsm_characterSet_GSM:
      sprintf(str, "AT+CSCS=GSM\r\n");        
    break;
    case Gsm_characterSet_HEX:
      sprintf(str, "AT+CSCS=HEX\r\n");        
    break;
    case Gsm_characterSet_IRA:
      sprintf(str, "AT+CSCS=IRA\r\n");        
    break;
    case Gsm_characterSet_PCCP:
      sprintf(str, "AT+CSCS=PCCP\r\n");        
    break;
    case Gsm_characterSet_PCDN:
      sprintf(str, "AT+CSCS=PCDN\r\n");        
    break;
    case Gsm_characterSet_UCS2:
      sprintf(str, "AT+CSCS=UCS2\r\n");        
    break;
    default:
      return 0;   
  }  
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.msg.characterSet = Gsm_characterSet_;
  return 1;
}
//#####################################################################################################
Gsm_pin_t gsm_getPinStatus(void)
{
  char str[32];
  char state[16];  
  if (gsm_at_sendCommand("AT+CPIN?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CPIN:", "\r\nERROR\r\n") != 1)
    return Gsm_pin_ERROR;
  if (sscanf(str,"\r\n+CPIN: %[^\r\n]", state) != 1)
    return Gsm_pin_ERROR;
  if (strcmp(state,"READY") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_READY;
    return Gsm_pin_READY;
  }    
  else if (strcmp(state,"SIM PIN") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_PIN;
    return Gsm_pin_PIN;
  }    
  else if (strcmp(state,"SIM PUK") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_PUK;
    return Gsm_pin_PUK;
  }    
  else if (strcmp(state,"PH_SIM PIN") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_PHPIN;
    return Gsm_pin_PHPIN;
  }    
  else if (strcmp(state,"PH_SIM PUK") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_PHPUK;
    return Gsm_pin_PHPUK;
  }    
  else if (strcmp(state,"SIM PIN2") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_PIN2;
    return Gsm_pin_PIN2;
  }    
  else if (strcmp(state,"SIM PUK2") == 0)
  {
    gsm.config.pinStatus = Gsm_pin_PUK2;
    return Gsm_pin_PUK2;
  }    
  else
    return Gsm_pin_ERROR;
}
//#####################################################################################################
bool gsm_setPin(char *enterPin)
{
  char  str[64];
  if (enterPin == NULL)
    return 0;
  sprintf(str, "AT+CPIN=%s\r\n",enterPin);
  if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return 1;
}
//#####################################################################################################
bool gsm_ussdSend(char *command, char *answer, uint16_t sizeOfAnswer, uint8_t  wait_s)
{
  if (command == NULL)
  {
    if (gsm_at_sendCommand("AT+CUSD=2\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return 0;
    return 1;    
  }
  else if (answer == NULL)
  {
    char str[16 + strlen(command)];
    sprintf(str, "AT+CUSD=0,\"%s\"\r\n", command);    
    if (gsm_at_sendCommand(str, wait_s * 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return 0;
    return 1;
  }
  else
  {
    char str[16 + strlen(command)];
    char str2[sizeOfAnswer + 32]; 
    sprintf(str, "AT+CUSD=1,\"%s\"\r\n", command);    
    if (gsm_at_sendCommand(str, wait_s * 1000 , str2, sizeof(str2), 2, "\r\n+CUSD:", "\r\nERROR\r\n") != 1)
    {
      gsm_at_sendCommand("AT+CUSD=2\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
      return 0;
    }
    char *start = strstr(str2, "\"");
    char *end = strstr(str2, "\", ");
    if (start != NULL && end != NULL)
    {
      start++;
      strncpy(answer, start, end - start);
      return 1;      
    }
    else
      return 0;    
  }
}
//#####################################################################################################
//#######################               GSM CALL              #########################################
//#####################################################################################################
bool gsm_callAnswer(void)
{
  if (gsm_at_sendCommand("ATA\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.call.inUse = 1;
  return 1;
}
//#####################################################################################################
bool gsm_callEnd(void)
{
  if (gsm_at_sendCommand("ATH\r\n", 20000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.call.inUse = 0;
  return 1;
}
//#####################################################################################################
bool gsm_call(char *number, uint8_t wait_s)
{
  char str[32];
  sprintf(str,"ATD%s;\r\n", number);  
  uint8_t ans = gsm_at_sendCommand(str, wait_s * 1000 , NULL, 0, 5, "\r\nNO DIALTONE\r\n", "\r\nBUSY\r\n", "\r\nNO CARRIER\r\n", "\r\nNO ANSWER\r\n", "\r\nOK\r\n");
  if (ans == 5)
  {
    gsm.call.inUse = 1;
    return 1;
  }
  else
    return 0;
}
//#####################################################################################################
//#######################               GSM MSG               #########################################
//#####################################################################################################
bool gsm_msg_setTextMode(bool enable)
{
  if (enable)
  {
    if (gsm_at_sendCommand("AT+CMGF=1\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.msg.isTextMode = 1;
      gsm_at_sendCommand("AT+CSMP=17,167,0,0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
      return 1;  
    }
  }
  else
  {
    if (gsm_at_sendCommand("AT+CMGF=0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.msg.isTextMode = 0;
      return 1;  
    }
  }
  return 0;
}
//#####################################################################################################
bool gsm_msg_isTextMode(void)
{
  uint8_t ans;
  ans = gsm_at_sendCommand("AT+CMGF?\r\n", 1000, NULL, 0, 3, "\r\n+CMGF: 0\r\n", "\r\n+CMGF: 1\r\n", "\r\nERROR\r\n");
  if (ans == 1)
  {
    gsm.msg.isTextMode = 0;
    return 0;
  }
  else if (ans == 1)
  {
    gsm.msg.isTextMode = 1;
    return 1;
  }
  else
    return 0;      
}
//#####################################################################################################
bool gsm_msg_read(uint16_t index)
{
  if (gsm.msg.isTextMode == 1)
  {
    char str[20];
    char str2[1024 + 64]; 
    sprintf(str, "AT+CMGR=%d\r\n", index);  
    if (gsm_at_sendCommand(str, 5000, str2, sizeof(str2), 3, "\r\n+CMGR:", "\r\nOK\r\n","\r\nERROR\r\n") != 1)
      return 0;
    sscanf(str2, "\r\n+CMGR: \"%[^\"]\",\"%[^\"]\",\"\",\"%hhd/%hhd/%hhd,%hhd:%hhd:%hhd%*d\"",
      gsm.msg.status, gsm.msg.number, &gsm.msg.year, &gsm.msg.month, &gsm.msg.day , &gsm.msg.hour, &gsm.msg.minute, &gsm.msg.second);
    uint8_t cnt = 0;
    char *s = strtok(str2, "\"");
    while( s != NULL)
    {
      s = strtok(NULL, "\"");
      if (cnt == 6)
      {
        s+=2;
        char *end = strstr(s, "\r\nOK\r\n");
        if (end != NULL)
        {
          strncpy(gsm.msg.message, s, end - s);
          return 1;
        }
        else
          return 0;
      }
      cnt++;    
    }
  }
  else
  {
    return 0;
  }
  return 0;
}
//#####################################################################################################
bool gsm_msg_updateStorage(void)
{
  char str[64];
  char s[3][5]; 
  if (gsm_at_sendCommand("AT+CPMS?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CPMS:", "\r\nERROR\r\n") != 1)
    return 0;
  if (sscanf(str, "\r\n+CPMS: \"%[^\"]\",%hhd,%hhd,\"%[^\"]\",%hhd,%hhd,\"%[^\"]\",%hhd,%hhd\r\n", s[0], &gsm.msg.storageReadUsed, &gsm.msg.storageReadTotal,\
    s[1], &gsm.msg.storageSentUsed, &gsm.msg.storageSentTotal,\
    s[2], &gsm.msg.storageReceivedUsed, &gsm.msg.storageReceivedTotal) != 9)
    return 0;
  if (strcmp(s[0], "SM") == 0)
    gsm.msg.storageRead = Gsm_msg_storage_SIMCARD;
  else if (strcmp(s[0], "ME") == 0)
    gsm.msg.storageRead = Gsm_msg_storage_MODULE;
  else if (strcmp(s[0], "SM_P") == 0)
    gsm.msg.storageRead = Gsm_msg_storage_SIMCARD_PREFERRED;
  else if (strcmp(s[0], "ME_P") == 0)
    gsm.msg.storageRead = Gsm_msg_storage_MODULE_PREFERRED;
  else if (strcmp(s[0], "MT") == 0)
    gsm.msg.storageRead = Gsm_msg_storage_SIMCARD_OR_MODULE;
  else
    gsm.msg.storageRead = Gsm_msg_storage_ERROR;
  if (strcmp(s[1], "SM") == 0)
    gsm.msg.storageSent = Gsm_msg_storage_SIMCARD;
  else if (strcmp(s[1], "ME") == 0)
    gsm.msg.storageSent = Gsm_msg_storage_MODULE;
  else if (strcmp(s[1], "SM_P") == 0)
    gsm.msg.storageSent = Gsm_msg_storage_SIMCARD_PREFERRED;
  else if (strcmp(s[1], "ME_P") == 0)
    gsm.msg.storageSent = Gsm_msg_storage_MODULE_PREFERRED;
  else if (strcmp(s[1], "MT") == 0)
    gsm.msg.storageSent = Gsm_msg_storage_SIMCARD_OR_MODULE;
  else
    gsm.msg.storageSent = Gsm_msg_storage_ERROR;
  if (strcmp(s[2], "SM") == 0)
    gsm.msg.storageReceived = Gsm_msg_storage_SIMCARD;
  else if (strcmp(s[2], "ME") == 0)
    gsm.msg.storageReceived = Gsm_msg_storage_MODULE;
  else if (strcmp(s[2], "SM_P") == 0)
    gsm.msg.storageReceived = Gsm_msg_storage_SIMCARD_PREFERRED;
  else if (strcmp(s[2], "ME_P") == 0)
    gsm.msg.storageReceived = Gsm_msg_storage_MODULE_PREFERRED;
  else if (strcmp(s[2], "MT") == 0)
    gsm.msg.storageReceived = Gsm_msg_storage_SIMCARD_OR_MODULE;
  else
    gsm.msg.storageReceived = Gsm_msg_storage_ERROR;
  return 1;
}
//#####################################################################################################
bool gsm_msg_setStorage(Gsm_msg_storage_t Gsm_msg_storage_)
{
  char str[64];
  if (Gsm_msg_storage_ == Gsm_msg_storage_SIMCARD)
    sprintf(str, "AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");
  else if (Gsm_msg_storage_ == Gsm_msg_storage_MODULE)
    sprintf(str, "AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n");
  else if (Gsm_msg_storage_ == Gsm_msg_storage_SIMCARD_PREFERRED)
    sprintf(str, "AT+CPMS=\"SM_P\",\"SM_P\",\"SM_P\"\r\n");
  else if (Gsm_msg_storage_ == Gsm_msg_storage_MODULE_PREFERRED)
    sprintf(str, "AT+CPMS=\"ME_P\",\"ME_P\",\"ME_P\"\r\n");
  else if (Gsm_msg_storage_ == Gsm_msg_storage_SIMCARD_OR_MODULE)
    sprintf(str, "AT+CPMS=\"MT\",\"MT\",\"MT\"\r\n");
  else 
    return 0;
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return gsm_msg_updateStorage();  
}
//#####################################################################################################
bool gsm_msg_deleteAll(void)
{
  if (gsm.msg.isTextMode)
  {
    if (gsm_at_sendCommand("AT+CMGDA=\"DEL ALL\"\r\n", 25000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return 0;    
    return 1;    
  }
  else
  {
    if (gsm_at_sendCommand("AT+CMGDA=6\r\n", 25000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return 0;    
    return 1;    
  }
  
}
//#####################################################################################################
uint8_t gsm_msg_getFreeSpaceReceiveStorage(void)
{
  if (gsm_msg_updateStorage() == 0)
    return 0;
  return gsm.msg.storageReceivedTotal - gsm.msg.storageReceivedUsed;  
}
//#####################################################################################################
uint8_t gsm_msg_getFreeSpaceSentStorage(void)
{
  if (gsm_msg_updateStorage() == 0)
    return 0;
  return gsm.msg.storageSentTotal - gsm.msg.storageSentUsed;
}
//#####################################################################################################
uint8_t gsm_msg_getFreeSpaceReadStorage(void)
{
  if (gsm_msg_updateStorage() == 0)
    return 0;
  return gsm.msg.storageReadTotal - gsm.msg.storageReadUsed;
}
//#####################################################################################################
bool gsm_updateEchoCancellationControl(void)
{
  char str[128];
  if (gsm_at_sendCommand("AT+ECHO?\r\n", 1000 , str, sizeof(str), 2, "\r\n+ECHO:", "\r\nERROR\r\n") != 1)
    return 0;
  char *str1 = strchr(str, '(');
  if (str1 == NULL)
    return 0;
  if (sscanf(str1,"(0,%hd,%hd,%hd,%hd)", &gsm.config.echoChannel[0].nonlinearProcessing, &gsm.config.echoChannel[0].acousticEcho, &gsm.config.echoChannel[0].noiseReduction, &gsm.config.echoChannel[0].noiseSuppression) != 4)
    return 0;
  str1++;
  str1 = strchr(str1, '(');
  sscanf(str1,"(1,%hd,%hd,%hd,%hd)", &gsm.config.echoChannel[1].nonlinearProcessing, &gsm.config.echoChannel[1].acousticEcho, &gsm.config.echoChannel[1].noiseReduction, &gsm.config.echoChannel[1].noiseSuppression);
  return 1;
}
//#####################################################################################################
uint16_t gsm_getEchoCancellationControl_nonlinearProcessing(Gsm_channel_t Gsm_channel_)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  return gsm.config.echoChannel[Gsm_channel_].nonlinearProcessing;
}
//#####################################################################################################
uint16_t gsm_getEchoCancellationControl_acousticEcho(Gsm_channel_t Gsm_channel_)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  return gsm.config.echoChannel[Gsm_channel_].acousticEcho;
}
//#####################################################################################################
uint16_t gsm_getEchoCancellationControl_noiseReduction(Gsm_channel_t Gsm_channel_)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
 return gsm.config.echoChannel[Gsm_channel_].noiseReduction;
}
//#####################################################################################################
uint16_t gsm_getEchoCancellationControl_noiseSuppression(Gsm_channel_t Gsm_channel_)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  return gsm.config.echoChannel[Gsm_channel_].noiseSuppression;
}
//#####################################################################################################
bool gsm_setEchoCancellationControl_nonlinearProcessing(Gsm_channel_t Gsm_channel_, uint16_t value)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  char str[64];
  sprintf(str, "AT+ECHO=%d,%d,%d,%d,%d\r\n", Gsm_channel_, value, gsm.config.echoChannel[Gsm_channel_].acousticEcho, gsm.config.echoChannel[Gsm_channel_].noiseReduction, gsm.config.echoChannel[Gsm_channel_].noiseSuppression);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return gsm_updateEchoCancellationControl();  
}
//#####################################################################################################
bool gsm_setEchoCancellationControl_acousticEcho(Gsm_channel_t Gsm_channel_, uint16_t value)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  char str[64];
  sprintf(str, "AT+ECHO=%d,%d,%d,%d,%d\r\n", Gsm_channel_, gsm.config.echoChannel[Gsm_channel_].nonlinearProcessing, value, gsm.config.echoChannel[Gsm_channel_].noiseReduction, gsm.config.echoChannel[Gsm_channel_].noiseSuppression);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return gsm_updateEchoCancellationControl();  
}
//#####################################################################################################
bool gsm_setEchoCancellationControl_noiseReduction(Gsm_channel_t Gsm_channel_, uint16_t value)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  char str[64];
  sprintf(str, "AT+ECHO=%d,%d,%d,%d,%d\r\n", Gsm_channel_, gsm.config.echoChannel[Gsm_channel_].nonlinearProcessing, gsm.config.echoChannel[Gsm_channel_].acousticEcho, value, gsm.config.echoChannel[Gsm_channel_].noiseSuppression);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return gsm_updateEchoCancellationControl();  
}
//#####################################################################################################
bool gsm_setEchoCancellationControl_noiseSuppression(Gsm_channel_t Gsm_channel_, uint16_t value)
{
  if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  char str[64];
  sprintf(str, "AT+ECHO=%d,%d,%d,%d,%d\r\n", Gsm_channel_, gsm.config.echoChannel[Gsm_channel_].nonlinearProcessing, gsm.config.echoChannel[Gsm_channel_].acousticEcho, gsm.config.echoChannel[Gsm_channel_].noiseReduction, value);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return gsm_updateEchoCancellationControl();    
}
//#####################################################################################################
bool gsm_setEchoCancellationControl_enable(Gsm_channel_t Gsm_channel_, bool enable)
{
    if (gsm_updateEchoCancellationControl() == 0)
    return 0;
  char str[64];
  sprintf(str, "AT+ECHO=%d,%d,%d,%d,%d,%d\r\n", Gsm_channel_, gsm.config.echoChannel[Gsm_channel_].nonlinearProcessing, gsm.config.echoChannel[Gsm_channel_].acousticEcho, gsm.config.echoChannel[Gsm_channel_].noiseReduction, gsm.config.echoChannel[Gsm_channel_].noiseSuppression, enable);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;
  return gsm_updateEchoCancellationControl();    
}
//#####################################################################################################

