
#include "gsm.h"


//##########################################################################################
__weak void gsm_user_init(void)
{
//  Gsm_pin_t pinStatus = gsm_getPinStatus();
//  if (pinStatus == Gsm_pin_PIN)
//  {
//    gsm_setPin("1234");
//  }
//  else if (pinStatus == Gsm_pin_PUK)
//  {
//    gsm_setPin("1234");
//  }
}
//##########################################################################################
__weak void gsm_user_incommingCall(char *number)
{
//  gsm_callAnswer();
}
//##########################################################################################
__weak void gsm_user_endCall(void)
{
  
}
//##########################################################################################
__weak void gsm_user_newMsg(char *msg, Gsm_msg_time_t time)
{

}
//##########################################################################################
