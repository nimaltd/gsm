
#include "gsm.h"

#if (_GSM_MSG_ENABLE == 1)
//#############################################################################################
bool gsm_msg_updateStorage(void)
{
  char str[64];
  char s[5]; 
  if (gsm_at_sendCommand("AT+CPMS?\r\n", 1000 , str, sizeof(str), 2, "\r\n+CPMS:", "\r\nERROR\r\n") != 1)
    return false;
  if (sscanf(str, "\r\n+CPMS: \"%[^\"]\",%*hhd,%*hhd,\"%*[^\"]\",%*hhd,%*hhd,\"%*[^\"]\",%hhd,%hhd\r\n", s, &gsm.msg.storageUsed, &gsm.msg.storageTotal) != 3)
    return false;
  if (strcmp(s, "SM") == 0)
    gsm.msg.storage = Gsm_Msg_Store_SIMCARD;
  else if (strcmp(s, "ME") == 0)
    gsm.msg.storage = Gsm_Msg_Store_MODULE;
  else if (strcmp(s, "SM_P") == 0)
    gsm.msg.storage = Gsm_Msg_Store_SIMCARD_PREFERRED;
  else if (strcmp(s, "ME_P") == 0)
    gsm.msg.storage = Gsm_Msg_Store_MODULE_PREFERRED;
  else if (strcmp(s, "MT") == 0)
    gsm.msg.storage = Gsm_Msg_Store_SIMCARD_OR_MODULE;
  else
    gsm.msg.storage = Gsm_Msg_Store_ERROR;
  return true;
}
//#############################################################################################
uint16_t gsm_msg_getStorageUsed(void)
{
  gsm_msg_updateStorage();
  return gsm.msg.storageUsed;
}  
//#############################################################################################
uint16_t gsm_msg_getStorageTotal(void)
{
  gsm_msg_updateStorage();
  return gsm.msg.storageTotal;
}  
//#############################################################################################
uint16_t gsm_msg_getStorageFree(void)
{
  gsm_msg_updateStorage();
  return gsm.msg.storageTotal - gsm.msg.storageUsed;
}  
//#############################################################################################
bool gsm_msg_textMode(bool on_off)
{
  if (on_off)
  {
    if (gsm_at_sendCommand("AT+CMGF=1\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.msg.textMode = 1;
      gsm_at_sendCommand("AT+CSMP=17,167,0,0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
      return true;  
    }
  }
  else
  {
    if (gsm_at_sendCommand("AT+CMGF=0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    {
      gsm.msg.textMode = 0;
      return true;  
    }
  }
  return false;
}
//#############################################################################################
bool gsm_msg_isTextMode(void)
{
  uint8_t ans;
  ans = gsm_at_sendCommand("AT+CMGF?\r\n", 1000, NULL, 0, 3, "\r\n+CMGF: 0\r\n", "\r\n+CMGF: 1\r\n", "\r\nERROR\r\n");
  if (ans == 1)
  {
    gsm.msg.textMode = 0;
    return false;
  }
  else if (ans == 1)
  {
    gsm.msg.textMode = 1;
    return true;
  }
  else
    return false;      
}
//#############################################################################################
bool gsm_msg_deleteAll(void)
{
  if (gsm.msg.textMode)
  {
    if (gsm_at_sendCommand("AT+CMGDA=\"DEL ALL\"\r\n", 25000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;    
    return true;    
  }
  else
  {
    if (gsm_at_sendCommand("AT+CMGDA=6\r\n", 25000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;    
    return true;    
  }  
}
//#############################################################################################
bool gsm_msg_delete(uint16_t index)
{
  char str[32];
  sprintf(str, "AT+CMGD=%d\r\n", index);
  if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    return true;    
  return false;  
}
//#############################################################################################
bool gsm_msg_send(const char *number, const char *msg)
{
  if ((number == NULL) || (msg == NULL))
    return false;
  if (gsm.msg.textMode)
  {
    char str[32];
    sprintf(str, "AT+CMGS=\"%s\"\r\n", number);
    if (gsm_at_sendCommand(str, 5000 , NULL, 0, 2, "\r\r\n> ", "\r\nERROR\r\n") != 1)
    {
      sprintf(str, "%c", 27);
      gsm_at_sendCommand(str, 1000, NULL, 0, 0);
      return 0;    
    }
    sprintf((char*)gsm.msg.buff, "%s%c", msg, 26);
    if (gsm_at_sendCommand((char*)gsm.msg.buff, 60000 , NULL, 0, 2, "\r\n+CMGS:", "\r\nERROR\r\n") != 1)    
      return false;
    return true;    
  }
  else
    return false;
}
//#############################################################################################
bool gsm_msg_selectStorage(Gsm_Msg_Store_t Gsm_Msg_Store_)
{
  char str[64];
  switch(Gsm_Msg_Store_)
  {
    case Gsm_Msg_Store_SIMCARD:
      sprintf(str, "AT+CPMS=\"SM\",\"SM\",\"SM\"\r\n");        
    break;
    case Gsm_Msg_Store_MODULE:
      sprintf(str, "AT+CPMS=\"ME\",\"ME\",\"ME\"\r\n");
    break;
    case Gsm_Msg_Store_SIMCARD_PREFERRED:
      sprintf(str, "AT+CPMS=\"SM_P\",\"SM_P\",\"SM_P\"\r\n");        
    break;
    case Gsm_Msg_Store_MODULE_PREFERRED:
      sprintf(str, "AT+CPMS=\"ME_P\",\"ME_P\",\"ME_P\"\r\n");        
    break;
    case Gsm_Msg_Store_SIMCARD_OR_MODULE:
      sprintf(str, "AT+CPMS=\"MT\",\"MT\",\"MT\"\r\n");
    break;
    default:
    return false;
  }    
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  gsm.msg.storage = Gsm_Msg_Store_;
  gsm_msg_updateStorage();
  return true;
}
//#############################################################################################
bool gsm_msg_selectCharacterSet(Gsm_Msg_ChSet_t Gsm_Msg_ChSet_)
{
  char str[64];
  switch(Gsm_Msg_ChSet_)
  {
    case Gsm_Msg_ChSet_8859_1:
      sprintf(str, "AT+CSCS=\"8859-1\"\r\n");         
    break;
    case Gsm_Msg_ChSet_GSM:
      sprintf(str, "AT+CSCS=\"GSM\"\r\n");         
    break;
    case Gsm_Msg_ChSet_IRA:
      sprintf(str, "AT+CSCS=\"IRA\"\r\n");         
    break;
    case Gsm_Msg_ChSet_PCCP:
      sprintf(str, "AT+CSCS=\"PCCP\"\r\n");         
    break;
    case Gsm_Msg_ChSet_HEX:
      sprintf(str, "AT+CSCS=\"HEX\"\r\n");         
    break;
    case Gsm_Msg_ChSet_UCS2:
      sprintf(str, "AT+CSCS=\"UCS2\"\r\n");         
    break;
    case Gsm_Msg_ChSet_PCDN:
      sprintf(str, "AT+CSCS=\"PCDN\"\r\n");         
    break;
    default:
    return false;      
   }  
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  gsm.msg.characterSet = Gsm_Msg_ChSet_;
  return true;
}
//#############################################################################################
bool gsm_msg_read(uint16_t index)
{
  //  +++ text mode 
  if (gsm.msg.textMode == 1)
  {
    char str[20];
    sprintf(str, "AT+CMGR=%d\r\n", index);  
    if (gsm_at_sendCommand(str, 5000, (char*)gsm.msg.buff, sizeof(gsm.msg.buff), 3, "\r\n+CMGR:", "\r\nOK\r\n","\r\nERROR\r\n") != 1)
      return 0;
    sscanf((char*)gsm.msg.buff, "\r\n+CMGR: \"%[^\"]\",\"%[^\"]\",\"\",\"%hhd/%hhd/%hhd,%hhd:%hhd:%hhd%*d\"",
      gsm.msg.status, gsm.msg.number, &gsm.msg.time.year, &gsm.msg.time.month, &gsm.msg.time.day , &gsm.msg.time.hour, &gsm.msg.time.minute, &gsm.msg.time.second);
    uint8_t cnt = 0;
    char *s = strtok((char*)gsm.msg.buff, "\"");
    while( s != NULL)
    {
      s = strtok(NULL, "\"");
      if (cnt == 6)
      {
        s+=2;
        char *end = strstr(s, "\r\nOK\r\n");
        if (end != NULL)
        {
          strncpy((char*)&gsm.msg.buff[0], s, end - s);
          memset(&gsm.msg.buff[end - s], 0 , sizeof(gsm.msg.buff) - (end - s));
          return true;
        }
        else
          return false;
      }
      cnt++;    
    }
  }
  //  --- text mode 
  
  //  +++ pdu mode 
  else
  {
    
    return false;
  }
  //  --- pdu mode 
  return false;
}
//#############################################################################################
#endif
