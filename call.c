
#include "gsm.h"

//#############################################################################################
bool gsm_call_answer(void)
{
  if (gsm_at_sendCommand("ATA\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  gsm.call.busy = 1;
  return true;
}
//#############################################################################################
bool gsm_call_dial(const char* number, uint8_t waitSecond)
{
  char str[32];
  sprintf(str,"ATD%s;\r\n", number);  
  uint8_t ans = gsm_at_sendCommand(str, waitSecond * 1000 , NULL, 0, 5, "\r\nNO DIALTONE\r\n", "\r\nBUSY\r\n", "\r\nNO CARRIER\r\n", "\r\nNO ANSWER\r\n", "\r\nOK\r\n");
  if (ans == 5)
  {
    gsm.call.busy = 1;
    return true;
  }
  else
  {
    gsm_call_end();
    return false;  
  }
}
//#############################################################################################
bool gsm_call_end(void)
{
  if (gsm_at_sendCommand("ATH\r\n", 20000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
    return false;
  gsm.call.busy = 0;
  return true;
}

//#############################################################################################


//#############################################################################################

