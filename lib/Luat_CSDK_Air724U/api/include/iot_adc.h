#ifndef __IOT_ADC_H__
#define __IOT_ADC_H__

#include "iot_os.h"

/**
 * @ingroup iot_sdk_device ÍâÉè½Ó¿Ú
 * @{*/
/**
 * @defgroup iot_sdk_adc adc½Ó¿Ú
 * @{*/

/**ADC³õêmpt »¯ 
*@param Channel: Adci¨µà
*@param mode: adcģʽ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_adc_init(
                        E_AMOPENAT_ADC_CHANNEL channel  /*Adc ± toºl*/,
    					E_AMOPENAT_ADC_CFG_MODE mode
                );


/**             
*@note ADCYµug´¿â
*@pannel: adcÍµug.
*@parV adcVale: ADCâ x¼²¿¿â™
*@param Volume: µcö¹µug»¿â
*@return TRUE: ³É¹´
* FALSE: SE§°Ü
**/
BOOL iot_adc_read(
                        E_AMOPENAT_ADC_CHANNEL channel,    
                        UINT32* adcValue,                
                        UINT32* voltage                    
                );

/** @}*/
/** @}*/





#endif

