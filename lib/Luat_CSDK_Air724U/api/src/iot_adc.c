#include "iot_adc.h"

/**ADC³õêmpt »¯ 
*@param Channel: Adci¨µà
*@param mode: adcģʽ
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_adc_init(
                        E_AMOPENAT_ADC_CHANNEL channel  /*Adc ± toºl*/,
    					E_AMOPENAT_ADC_CFG_MODE mode
                )
{
    return OPENAT_InitADC(channel,mode);
}

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
                )
{
    return OPENAT_ReadADC(channel, adcValue, voltage);
}
