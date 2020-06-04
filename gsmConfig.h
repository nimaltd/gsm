#ifndef _GSMCONFIG_H
#define _GSMCONFIG_H

#define   _GSM_DEBUG                          1
#define   _GSM_USE_FREEEROS                   1
#define   _GSM_USART                          USART3
#define   _GSM_RXSIZE                         512  
#define   _GSM_RXTIMEOUT                      10  
#define   _GSM_POWERKEY_GPIO                  GSM_KEY_GPIO_Port
#define   _GSM_POWERKEY_PIN                   GSM_KEY_Pin

#define   _GSM_TXDMA_ENABLE                   0   
#if       _GSM_TXDMA_ENABLE == 1
#define   _GSM_DMA                            DMA1
#define   _GSM_DMA_CHANNEL_OR_STREAM          LL_DMA_CHANNEL_2
#define   _GSM_DMA_ENABLE_CHANNEL_OR_STREAM   LL_DMA_EnableChannel
#define   _GSM_DMA_DISABLE_CHANNEL_OR_STREAM  LL_DMA_DisableChannel
#define   _GSM_DMA_IS_ACTIVE                  LL_DMA_IsActiveFlag_TC2
#define   _GSM_DMA_CLEAR                      LL_DMA_ClearFlag_TC2                 
#endif

#endif
