## GSM module library (BETA)

* http://www.github.com/NimaLTD   
* https://www.instagram.com/github.nimaltd/   
* https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw   

[ ] F0 tested 
[X] F1 tested 
[ ] F2 tested 
[ ] F3 tested 
[ ] F4 tested 

* Enable USART (LL Library) and a Gpio as output(GSM POWER PIN).
* Select `General peripheral Initalizion as a pair of '.c/.h' file per peripheral` on project settings.
* Config `gsmConf.h`.
* Call `gsmInit()`. 
* Call `gsmProcess()` in infinite loop.

* Dont forget to erase page/sector/block before write.


