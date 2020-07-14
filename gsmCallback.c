
#include "gsm.h"
#include "functions.h"

char  dtmf[25];
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
__weak void gsm_callback_newMsg(char *number, Gsm_Time_t time, char *msg)
{
  //gsm_msg_send(number, "I got a message");
  if (strcmp((char*)ADMIN_NUMBER, number) == 0)
  {
    smsInstruction(number, msg);
    return;
  }
  for (uint8_t i = 0; i < PHONEBOOK_MAX; i++)
  {
    if (strcmp((char*)phonebook[i], number) == 0)
      smsInstruction(number, msg);
  }
}
//#################################################################################
__weak void gsm_callback_newCall(char *number)
{
  //gsm_call_answer();
  if (strcmp((char*)ADMIN_NUMBER, number) == 0)
  {
    gsm_call_answer();
    return;
  }
  for (uint8_t i = 0; i < PHONEBOOK_MAX; i++)
  {
    if (strcmp((char*)phonebook[i], number) == 0)
    {
      gsm_call_answer();
      return;
    }
  }
  gsm_call_end();
}
//#################################################################################
__weak void gsm_callback_endCall(void)
{
  
}
//#################################################################################
__weak void gsm_callback_dtmf(char key, uint8_t cnt)  //  do not use any AT-COMMAND and gsm functions here
{
  dtmf[cnt] = key;
}
//#################################################################################
