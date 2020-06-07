
#include "gsm.h"
#include "functions.h"

bool checkPhonebook(char *number)
{
  if (strcmp(ADMIN_NUMBER, number) == 0)
    return 1;
  for (uint8_t i=0 ;i<PHONEBOOK_MAX ; i++)
  {
    if (strcmp(phonebook[i], number) == 0)
      return 1;
  }  
  return 0;
}
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
__weak void gsm_user_newMsg(char *msg, char *number, Gsm_msg_time_t time)
{
  if (checkPhonebook(number) == 0)
    return;
    

}
//##########################################################################################
