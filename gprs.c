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
bool gsm_gprs_connect(void)
{
  char str[32];
  gsm_at_sendCommand("AT+SAPBR=0,1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  if (gsm_at_sendCommand("AT+SAPBR=1,1\r\n", 85000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  if (gsm_at_sendCommand("AT+SAPBR=2,1\r\n", 1000 , str, sizeof(str), 2, "\r\n+SAPBR: 1,1,", "\r\nERROR\r\n") != 1)
  {
    gsm.gprs.connected = false;  
    return false;
  }
  sscanf(str, "\r\n+SAPBR: 1,1,\"%[^\"\r\n]", gsm.gprs.ip); 
  gsm.gprs.connected = true;  
  gsm.gprs.connectedLast = true;  
  return true;  
}
//#############################################################################################
bool gsm_gprs_disconnect(void)
{
  if (gsm_at_sendCommand("AT+SAPBR=0,1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
  {
    gsm.gprs.connected = false;  
    gsm.gprs.connectedLast = false;
    return true;
  }
  return false;
}
//#############################################################################################
bool gsm_gprs_httpInit(void)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+HTTPINIT\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;
}
//#############################################################################################
bool gsm_gprs_httpSetContent(const char *content)
{
  if (gsm.gprs.connected == false)
    return false;
  char str[strlen(content) + 32];
  sprintf(str, "AT+HTTPPARA=\"CONTENT\",\"%s\"\r\n", content);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;
}
//#############################################################################################
bool gsm_gprs_httpSetUserData(const char *data)
{
  if (gsm.gprs.connected == false)
    return false;
  while (gsm.taskBusy == 1)
    osDelay(1);
  osThreadSuspend(gsmTaskHandle);
  osDelay(100);
  gsm_at_sendString("AT+HTTPPARA=\"USERDATA\",\"");
  gsm_at_sendString(data);
  if (gsm_at_sendCommand("\"\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    osThreadResume(gsmTaskHandle);
    return false;
  }
  osThreadResume(gsmTaskHandle);
  return true;  
}
//#############################################################################################
bool gsm_gprs_httpSendData(const char *data, uint16_t timeoutMs)
{
  if (gsm.gprs.connected == false)
    return false;
  char str[32];
  sprintf(str, "AT+HTTPDATA=%d,%d\r\n", strlen(data), timeoutMs);
  while (gsm.taskBusy == 1)
    osDelay(1);
  osThreadSuspend(gsmTaskHandle);
  osDelay(100);
  do
  {
    if (gsm_at_sendCommand(str, timeoutMs, NULL, 0, 2, "\r\nDOWNLOAD\r\n", "\r\nERROR\r\n") != 1)
      break;        
    if (gsm_at_sendCommand(data, timeoutMs, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;    
    osDelay(timeoutMs);  
    osThreadResume(gsmTaskHandle);
    return true;
  }
  while(0);
  osThreadResume(gsmTaskHandle);
  return false;
}
//#############################################################################################
int16_t gsm_gprs_httpGet(const char *url, bool ssl, uint16_t timeoutMs)
{
  if (gsm.gprs.connected == false)
    return -1;
  gsm.gprs.code = -1;
  gsm.gprs.dataLen = 0;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"CID\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  char str[strlen(url) + 32];
  sprintf(str, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"REDIR\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (ssl)
  {
    if (gsm_at_sendCommand("AT+HTTPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return gsm.gprs.code;
  }
  else
  {
    if (gsm_at_sendCommand("AT+HTTPSSL=0\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return gsm.gprs.code;
  }
  if (gsm_at_sendCommand("AT+HTTPACTION=0\r\n", timeoutMs , str, sizeof(str), 2, "\r\n+HTTPACTION:", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  sscanf(str, "\r\n+HTTPACTION: 0,%hd,%d\r\n", &gsm.gprs.code, &gsm.gprs.dataLen);
  return gsm.gprs.code;

}  
//#############################################################################################
int16_t gsm_gprs_httpPost(const char *url, bool ssl, uint16_t timeoutMs)
{
  if (gsm.gprs.connected == false)
    return -1;
  gsm.gprs.code = -1;
  gsm.gprs.dataLen = 0;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"CID\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  char str[strlen(url) + 32];
  sprintf(str, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (gsm_at_sendCommand("AT+HTTPPARA=\"REDIR\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  if (ssl)
  {
    if (gsm_at_sendCommand("AT+HTTPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return gsm.gprs.code;
  }
  else
  {
    if (gsm_at_sendCommand("AT+HTTPSSL=0\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return gsm.gprs.code;
  }
  if (gsm_at_sendCommand("AT+HTTPACTION=1\r\n", timeoutMs , str, sizeof(str), 2, "\r\n+HTTPACTION:", "\r\nERROR\r\n") != 1)
    return gsm.gprs.code;
  sscanf(str, "\r\n+HTTPACTION: 1,%hd,%d\r\n", &gsm.gprs.code, &gsm.gprs.dataLen);
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
  if (gsm.gprs.connected == false)
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
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+HTTPTERM\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
    return true;
  return false;
}
//#############################################################################################
bool gsm_gprs_ftpLogin(char *ftpAddress, char *ftpUserName, char *ftpPassword, uint16_t port)
{
  if (gsm.gprs.connected == false)
    return false;
  char str[128];
  if (gsm_at_sendCommand("AT+FTPMODE=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
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
Gsm_Ftp_Error_t gsm_gprs_ftpUploadBegin(bool asciiFile, bool append, const char *path, const char *fileName, const uint8_t *data, uint16_t len)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char *s;
  char str[128];  
  char answer[64];
  gsm_at_sendCommand("AT+FTPEXTPUT=0\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  if (asciiFile)
    sprintf(str, "AT+FTPTYPE=\"A\"\r\n");
  else
    sprintf(str, "AT+FTPTYPE=\"I\"\r\n");
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  if (append)
    sprintf(str, "AT+FTPPUTOPT=\"APPE\"\r\n");
  else
    sprintf(str, "AT+FTPPUTOPT=\"STOR\"\r\n");
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  sprintf(str, "AT+FTPPUTPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  sprintf(str, "AT+FTPPUTNAME=\"%s\"\r\n", fileName);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  if (gsm_at_sendCommand("AT+FTPPUT=1\r\n", 75000 , answer, sizeof(answer), 2, "\r\n+FTPPUT: 1,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  s = strchr(answer, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  if (atoi(s) != 1)
    return (Gsm_Ftp_Error_t)atoi(s);
  sprintf(str, "AT+FTPPUT=2,%d\r\n", len); 
  if (gsm_at_sendCommand(str, 5000 , answer, sizeof(answer), 2, "\r\n+FTPPUT: 2,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  s = strchr(answer, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  if (atoi(s) != len)
    return Gsm_Ftp_Error_Error;
  while (gsm.taskBusy == 1)
    osDelay(1);
  osThreadSuspend(gsmTaskHandle);
  osDelay(100);
  gsm_at_sendData(data, len);
  gsm_at_sendCommand("", 120 * 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  osThreadResume(gsmTaskHandle);
  return Gsm_Ftp_Error_None;
}
//#############################################################################################
Gsm_Ftp_Error_t gsm_gprs_ftpUpload(const uint8_t *data, uint16_t len)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char *s;
  char str[128];  
  char answer[64];
  sprintf(str, "AT+FTPPUT=2,%d\r\n", len); 
  if (gsm_at_sendCommand(str, 5000 , answer, sizeof(answer), 2, "\r\n+FTPPUT: 2,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  s = strchr(answer, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  if (atoi(s) != len)
    return Gsm_Ftp_Error_Error;
  while (gsm.taskBusy == 1)
    osDelay(1);
  osThreadSuspend(gsmTaskHandle);
  osDelay(100);
  gsm_at_sendData(data, len);
  gsm_at_sendCommand("", 120 * 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  osThreadResume(gsmTaskHandle);
  return Gsm_Ftp_Error_None;
}  
//#############################################################################################
bool gsm_gprs_ftpUploadEnd(void)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+FTPPUT=2,0\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;     
}
//#############################################################################################
bool gsm_gprs_ftpExtUploadBegin(bool asciiFile, bool append, const char *path, const char *fileName)
{
  if (gsm.gprs.connected == false)
    return false;
  char str[128];  
  gsm_at_sendCommand("AT+FTPEXTPUT=0\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
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
  if (gsm_at_sendCommand("AT+FTPEXTPUT=1\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  gsm.gprs.ftpExtOffset = 0;
  return true;
}
//#############################################################################################
bool gsm_gprs_ftpExtUpload(uint8_t *data, uint16_t len)
{
  if (gsm.gprs.connected == false)
    return false;
  char str[64];  
  char answer[64];  
  while (gsm.taskBusy == 1)
    osDelay(1);
  osThreadSuspend(gsmTaskHandle);
  do
  {
    sprintf(str, "AT+FTPEXTPUT=2,%d,%d,5000\r\n", gsm.gprs.ftpExtOffset, len);
    if (gsm_at_sendCommand(str, 5000, answer, sizeof(answer), 2, "\r\n+FTPEXTPUT: ", "\r\nERROR\r\n") != 1)
      break;
    char *s = strchr(answer, ',');
    if (s == NULL)
      break;
    s++;
    uint32_t d = atoi(s);
    if (d != len)
      break;
    osDelay(100);
    gsm_at_sendData(data, len);
    if (gsm_at_sendCommand("", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break; 
    gsm.gprs.ftpExtOffset += len;
    osThreadResume(gsmTaskHandle);
    return true;
  }
  while(0);
  osThreadResume(gsmTaskHandle);
  return false;
}
//#############################################################################################
Gsm_Ftp_Error_t gsm_gprs_ftpExtUploadEnd(void)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char answer[64];  
  if (gsm_at_sendCommand("AT+FTPPUT=1\r\n", 75000, answer, sizeof(answer), 2, "\r\n+FTPPUT: 1,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;
  char *s = strchr(answer, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  Gsm_Ftp_Error_t e = (Gsm_Ftp_Error_t)atoi(s);
  gsm_at_sendCommand("AT+FTPEXTPUT=0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
  return e;
}
//#############################################################################################
Gsm_Ftp_Error_t gsm_gprs_ftpCreateDir(const char *path)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char str[128];  
  sprintf(str, "AT+FTPGETPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  if (gsm_at_sendCommand("AT+FTPMKD\r\n", 75000 , str, sizeof(str), 2, "\r\n+FTPMKD: 1,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  char *s = strchr(str, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  return (Gsm_Ftp_Error_t)atoi(s);
}
//#############################################################################################
Gsm_Ftp_Error_t gsm_gprs_ftpRemoveDir(const char *path)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char str[128];  
  sprintf(str, "AT+FTPGETPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  if (gsm_at_sendCommand("AT+FTPRMD\r\n", 75000 , str, sizeof(str), 2, "\r\n+FTPRMD: 1,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  char *s = strchr(str, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  return (Gsm_Ftp_Error_t)atoi(s);
}
//#############################################################################################
uint32_t gsm_gprs_ftpGetSize(const char *path, const char *name)
{
  if (gsm.gprs.connected == false)
    return 0;
  char str[128];  
  sprintf(str, "AT+FTPGETPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;  
  sprintf(str, "AT+FTPGETNAME=\"%s\"\r\n", name);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return 0;  
  if (gsm_at_sendCommand("AT+FTPSIZE\r\n", 75000 , str, sizeof(str), 2, "\r\n+FTPSIZE: 1,", "\r\nERROR\r\n") != 1)
    return 0;  
  char *s = strchr(str, ',');
  if (s == NULL)
    return 0;
  s++;
  if (atoi(s) == 0)
  {
    s = strchr(str, ',');
    if (s == NULL)
      return 0;
    s++;    
    return atoi(s);
  }
  return 0;
}
//#############################################################################################
Gsm_Ftp_Error_t gsm_gprs_ftpRemove(const char *path, const char *name)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char str[128];  
  sprintf(str, "AT+FTPGETPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  sprintf(str, "AT+FTPGETNAME=\"%s\"\r\n", name);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  if (gsm_at_sendCommand("AT+FTPDELE\r\n", 75000 , str, sizeof(str), 2, "\r\n+FTPDELE: 1,", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  char *s = strchr(str, ',');
  if (s == NULL)
    return Gsm_Ftp_Error_Error;
  s++;
  return (Gsm_Ftp_Error_t)atoi(s);
}
//#############################################################################################
Gsm_Ftp_Error_t gsm_gprs_ftpIsExistFolder(const char *path)
{
  if (gsm.gprs.connected == false)
    return Gsm_Ftp_Error_Error;
  char str[strlen(path) + 16];  
  char answer[32];  
  sprintf(str, "AT+FTPGETPATH=\"%s\"\r\n", path);
  if (gsm_at_sendCommand(str, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;  
  if (gsm_at_sendCommand("AT+FTPLIST=1\r\n", 75000 , answer, sizeof(answer), 2, "\r\n+FTPLIST: ", "\r\nERROR\r\n") != 1)
    return Gsm_Ftp_Error_Error;    
  uint8_t i1 = 0,i2 = 0;
  if (sscanf(answer,"\r\n+FTPLIST: %hhd,%hhd", &i1, &i2) != 2)
    return Gsm_Ftp_Error_Error;    
  if (i1 == 1 && i2 == 1)
  {
    gsm_at_sendCommand("AT+FTPQUIT\r\n", 75000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
    return Gsm_Ftp_Error_None;
  }
  if (i1 == 1 && i2 == 77)
    return Gsm_Ftp_Error_NotExist; 
  return Gsm_Ftp_Error_Error; 
}
//#############################################################################################
bool gsm_gprs_ftpIsBusy(void)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+FTPSTATE\r\n", 75000 , NULL, 0, 3, "\r\n+FTPSTATE: 0\r\n", "\r\n+FTPSTATE: 1\r\n", "\r\nERROR\r\n") == 1)
    return true;
  return false;  
}
//#############################################################################################
bool gsm_gprs_ftpQuit(void)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+FTPQUIT\r\n", 5000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;       
}
//#############################################################################################
bool gsm_gprs_tcpConnect(const char *address, uint16_t port, bool ssl)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+CIPMUX=0\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  if (ssl)
  {
    if (gsm_at_sendCommand("AT+SSLOPT=1,1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;
    if (gsm_at_sendCommand("AT+CIPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;
  }
  else
  {
    if (gsm_at_sendCommand("AT+CIPSSL=0\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      return false;
  }
  char str[128];  
  sprintf(str, "AT+CIPSTART=\"TCP\",\"%s\",\"%d\"\r\n", address, port);
  uint8_t answer = gsm_at_sendCommand(str, 160000 , NULL, 0, 4, "\r\nCONNECT OK\r\n", "\r\nALREADY CONNECT\r\n", "\r\nCONNECT FAIL\r\n", "\r\nERROR\r\n");
  if ((answer == 1) || (answer == 2))
  {
    gsm.gprs.tcpConnection = 1;
    return true;
  }
  return false;  
}
//#############################################################################################
bool gsm_gprs_tcpSend(const uint8_t *data, uint16_t len)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm.gprs.tcpConnection == 0)
    return false;
  char str[32];
  bool err = false;
  while (gsm.taskBusy == 1)
    osDelay(1);
  osThreadSuspend(gsmTaskHandle);
  osDelay(100);
  do
  {
    sprintf(str, "AT+CIPSEND=%d\r\n", len);
    if (gsm_at_sendCommand(str, 10000 , NULL, 0, 2, "\r\n>", "\r\nERROR\r\n") != 1)
      break;
    osDelay(500);
    gsm_at_sendData(data, len);
    if (gsm_at_sendCommand("", 10000 , NULL, 0, 2, "\r\nSEND OK\r\n", "\r\nERROR\r\n") != 1)
      break;
    err = true;    
  }while(0);
  osThreadResume(gsmTaskHandle);
  return err;
}  
//#############################################################################################
bool gsm_gprs_tcpClose(void)
{
  if (gsm.gprs.connected == false)
    return false;
  if (gsm_at_sendCommand("AT+CIPCLOSE\r\n", 10000 , NULL, 0, 2, "\r\nCLOSE OK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  gsm.gprs.tcpConnection = 0;
  return true;
}  
//#############################################################################################
bool gsm_gprs_ntpServer(char *server)
{
  if (gsm.gprs.connected == false)
    return false;
  char str[64];
  sprintf(str, "AT+CNTP=\"%s\",0\r\n", server);
  if (gsm_at_sendCommand(str, 10000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  return true;  
}
//#############################################################################################
bool gsm_gprs_ntpGetTime(char *timeString)
{
  if (gsm.gprs.connected == false)
    return false;
  if (timeString == NULL)
    return false;
  char str[32];
  if (gsm_at_sendCommand("AT+CNTP\r\n", 10000 , NULL, 0, 2, "\r\n+CNTP: 1\r\n", "\r\nERROR\r\n") != 1)
    return false;
  if (gsm_at_sendCommand("AT+CCLK?\r\n", 10000 , str, sizeof(str), 2, "\r\n+CCLK:", "\r\nERROR\r\n") != 1)
    return false;
  sscanf(str, "\r\n+CCLK: \"%[^\"\r\n]", timeString);
  return true;
}
//#############################################################################################

#endif
