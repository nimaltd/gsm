## GSM module library (BETA)

* http://www.github.com/NimaLTD   
* https://www.instagram.com/github.nimaltd/   
* https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw   

    
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
* If using TX DMA, put `gsm_at_txCallback()` into dma callback. ( not tested yet)
* Select `General peripheral Initalizion as a pair of '.c/.h' file per peripheral` on project settings.
* Config `gsmConfig.h`.
* Call `gsm_init()`. 
* Call `gsm_process()` in infinite loop.




