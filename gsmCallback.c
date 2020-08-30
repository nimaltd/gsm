
#include "gsm.h"

//#################################################################################
__weak void gsm_callback_simcardReady(void)
{

}
//#################################################################################
__weak void gsm_callback_simcardPinRequest(void)
{
  //gsm_enterPinPuk(const char* string);  
}
//#################################################################################
__weak void gsm_callback_simcardPukRequest(void)
{
  //gsm_enterPinPuk(const char* string);
}
//#################################################################################
__weak void gsm_callback_simcardNotInserted(void)
{

}
//#################################################################################
__weak void gsm_callback_newMsg(char *number, Gsm_Time_t time, char *msg)
{
  //gsm_msg_send(number, "I got a message");
}
//#################################################################################
__weak void gsm_callback_newCall(char *number)
{
  //gsm_call_answer();
}
//#################################################################################
__weak void gsm_callback_endCall(void)
{
  
}
//#################################################################################
__weak void gsm_callback_dtmf(char key)
{
  
}
//#################################################################################
__weak void gsm_callback_gprsDisconnected(void)
{
  gsm_gprs_connect();
}
//#################################################################################
__weak void gsm_callback_gprsConnected(void)
{
  
}
//#################################################################################
__weak void gsm_callback_gprsGotData(uint8_t *data, uint16_t len)
{
  
}
