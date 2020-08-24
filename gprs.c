#include "gsm.h"

#if (_GSM_GPRS_ENABLE == 1)
//#############################################################################################
bool gsm_gprs_setApName(char *apName)
{
  char str[128];
  if (apName == NULL)
    return false;
  if (gsm_at_sendCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;    
  sprintf(str, "AT+SAPBR=3,1,\"APN\",\"%s\"\r\n", apName);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    return true;    
  return false;  
}
//#############################################################################################
bool gsm_gprs_open(void)
{
  gsm.gprs.open = false; 
  gsm_at_sendCommand("AT+SAPBR=0,1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  if (gsm_at_sendCommand("AT+SAPBR=1,1\r\n", 85000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  osDelay(2000);
  char str[32];
  if (gsm_at_sendCommand("AT+SAPBR=2,1\r\n", 1000 , str, sizeof(str), 2, "\r\n+SAPBR: 1,1,", "\r\nERROR\r\n") != 1)
    return false;
  sscanf(str, "+SAPBR: 1,1,\"%[^\"\r\n]", gsm.gprs.ip); 
  gsm.gprs.open = true;  
  return true;  
}
//#############################################################################################
bool gsm_gprs_close(void)
{
  if (gsm_at_sendCommand("AT+SAPBR=0,1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
  {
    gsm.gprs.open = false;  
    return true;
  }
  return false;
}
//#############################################################################################
int16_t gsm_gprs_httpGet(char *url)
{
  if (gsm.gprs.open == false)
    return -1;
  gsm.gprs.code = -1;
  gsm.gprs.dataLen = 0;
  gsm_at_sendCommand("AT+HTTPTERM\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  if (gsm_at_sendCommand("AT+HTTPINIT\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"CID\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  char str[128];
  sprintf(str, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"REDIR\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPACTION=0\r\n", 20000 , str, sizeof(str), 2, "\r\n+HTTPACTION:", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  sscanf(str, "+HTTPACTION: 0,%hd,%d\r\n", &gsm.gprs.code, &gsm.gprs.dataLen);
  return gsm.gprs.code;
}  
//#############################################################################################
int16_t gsm_gprs_httpPost(char *url)
{
  if (gsm.gprs.open == false)
    return -1;
  gsm.gprs.code = -1;
  gsm.gprs.dataLen = 0;
  gsm_at_sendCommand("AT+HTTPTERM\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  if (gsm_at_sendCommand("AT+HTTPINIT\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"CID\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  char str[128];
  sprintf(str, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"REDIR\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPACTION=1\r\n", 20000 , str, sizeof(str), 2, "\r\n+HTTPACTION:", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  sscanf(str, "+HTTPACTION: 1,%hd,%d\r\n", &gsm.gprs.code, &gsm.gprs.dataLen);
  return gsm.gprs.code;
}  
//#############################################################################################
uint32_t gsm_gprs_httpDataLen(void)
{
  return gsm.gprs.dataLen;
}  
//#############################################################################################
uint16_t gsm_gprs_httpRead(uint16_t len)
{
  if (gsm.gprs.open == false)
    return 0;
  memset(gsm.gprs.buff, 0, sizeof(gsm.gprs.buff)); 
  char str[32];
  if (len >= _GSM_RXSIZE - 16)
    len = _GSM_RXSIZE - 16;    
  sprintf(str, "AT+HTTPREAD=%d,%d\r\n", gsm.gprs.dataCurrent, len);
  if (gsm_at_sendCommand(str, 1000 , (char*)gsm.gprs.buff, _GSM_RXSIZE, 2, "\r\n+HTTPREAD: ", "\r\nERROR\r\n") != 1)
    return 0;
  gsm.gprs.dataCurrent += len;
  if (gsm.gprs.dataCurrent >= gsm.gprs.dataLen)
    gsm.gprs.dataCurrent = gsm.gprs.dataLen;
  uint16_t readLen;
  char *s = strchr((char*)gsm.gprs.buff, ':');
  s++;
  readLen = atoi(s);
  s = strchr(s, '\n');
  if (s != NULL)
  {    
    s++;
    for (uint16_t i = 0; i < readLen; i++)
      gsm.gprs.buff[i] = *s++;
    return readLen;
  }
  return 0;
}  
//#############################################################################################
bool gsm_gprs_httpTerminate(void) 
{
  if (gsm.gprs.open == false)
    return false;
  if (gsm_at_sendCommand("AT+HTTPTERM\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    return true;
  return false;
}
//#############################################################################################
bool gsm_gprs_ftpParameters(char *ftpAddress, char *ftpUserName, char *ftpPassword, uint16_t port)
{
  if (gsm.gprs.open == false)
    return false;
  char str[128];
  if (gsm_at_sendCommand("AT+FTPCID=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+FTPSERV=\"%s\"\r\n", ftpAddress);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+FTPPORT=%d\r\n", port);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+FTPUN=\"%s\"\r\n", ftpUserName);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+FTPPW=\"%s\"\r\n", ftpPassword);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;
}  
//#############################################################################################
bool gsm_gprs_ftpUpload(bool asciiFile, bool append, const char *path, const char *fileName, const uint8_t *data, uint16_t len)
{
  if (gsm.gprs.open == false)
    return false;
  char *s;
  char str[128];  
  char answer[64];
  if (asciiFile)
    sprintf(str, "AT+FTPTYPE=\"A\"\r\n");
  else
    sprintf(str, "AT+FTPTYPE=\"I\"\r\n");
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  if (append)
    sprintf(str, "AT+FTPPUTOPT=\"APPE\"\r\n");
  else
    sprintf(str, "AT+FTPPUTOPT=\"STOR\"\r\n");
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+FTPPUTPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  sprintf(str, "AT+FTPPUTNAME=\"%s\"\r\n", fileName);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  if (gsm_at_sendCommand("AT+FTPPUT=1\r\n", 75000 , answer, sizeof(answer), 2, "\r\n+FTPPUT: 1,", "\r\nERROR\r\n") != 1)
    return false;
  s = strchr(answer, ',');
  if (s == NULL)
    return false;
  s++;
  if (atoi(s) != 1)
    return false;
  sprintf(str, "AT+FTPPUT=2,%d\r\n", len); 
  if (gsm_at_sendCommand(str, 75000 , answer, sizeof(answer), 2, "\r\n+FTPPUT: 2,", "\r\nERROR\r\n") != 1)
    return false;
  s = strchr(answer, ',');
  if (s == NULL)
    return false;
  s++;
  if (atoi(s) != len)
    return false;
  osDelay(100);
  //gsm_at_sendData(data, len);
  if (gsm_at_sendCommand("12345", 75000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;
}  
//#############################################################################################
bool gsm_gprs_ftpUploadEnd(void)
{
  if (gsm.gprs.open == false)
    return false;
  if (gsm_at_sendCommand("AT+FTPPUT=2,0\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;     
}
//#############################################################################################
#endif
