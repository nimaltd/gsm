## GSM module library 

* http://www.github.com/NimaLTD   
* https://www.instagram.com/github.nimaltd/   
* https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw   

* [x] CMSIS V1 Supported.
* [ ] CMSIS V2 Supported.

* [x] SIM800C tested.
* [ ] SIM800 tested.
* [ ] SIM800A tested.
    
* [ ] F0 tested.
* [ ] L0 tested.
* [x] F1 tested.
* [ ] L1 tested.
* [ ] F2 tested.
* [ ] F3 tested.
* [ ] F4 tested.
* [ ] L4 tested.
* [ ] F7 tested.
* [ ] H7 tested.
    

* Enable USART (LL Library) and RX interrupt.
* Set a Gpio as output open drain to connect gsm power pin.
* Put `gsm_at_rxCallback()` into USART callback.
* Enable FREERTOS and increase heap size at least to 8192 bytes.
* Select `General peripheral Initalizion as a pair of '.c/.h' file per peripheral` on project settings.
* Config `gsmConfig.h`.
* Call `gsm_init(osPriotary_low)` in your task. 





