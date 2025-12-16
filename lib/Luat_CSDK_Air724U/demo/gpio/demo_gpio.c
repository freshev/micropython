#include "string.h"
#include "iot_debug.h"
#include "iot_gpio.h"
#include "iot_pmd.h"

#define gpio_print iot_debug_print
#define DEMO_GPIO_7 7
#define DEMO_GPIO_9 9

VOID demo_gpio_handle (E_OPENAT_DRV_EVT evt, 
                    E_AMOPENAT_GPIO_PORT gpioPort,
                unsigned char state)
{
    UINT8 status;

    // ÅÐ¶ÏÊÇgpioÖÐ¶Ï
    if (OPENAT_DRV_EVT_GPIO_INT_IND == evt)
    {
        // ÅÐ¶Ï´¥·¢ÖÐ¶ÏµÄ¹Ü½Å
        if (DEMO_GPIO_7 == gpioPort)
        {   
            // ´¥·¢µçÆ½µÄ×´Ì¬
            gpio_print("[gpio] input handle gpio %d, state %d", gpioPort, state);

            // ¶Áµ±Ç°gpio×´Ì¬, 1:¸ßµçÆ½ 0:µÍµçÆ½
            iot_gpio_read(gpioPort, &status);
            gpio_print("[gpio] input handle gpio %d, status %d", gpioPort, state);
            
        }
    }
}


VOID demo_gpio_input(VOID)
{
    T_AMOPENAT_GPIO_CFG  input_cfg;
    BOOL err;
    
    memset(&input_cfg, 0, sizeof(T_AMOPENAT_GPIO_CFG));
    
    input_cfg.mode = OPENAT_GPIO_INPUT_INT; //ÅäÖÃÊäÈëÖÐ¶Ï
    input_cfg.param.defaultState = FALSE;    
    input_cfg.param.intCfg.debounce = 50;  //·À¶¶50ms
    input_cfg.param.intCfg.intType = OPENAT_GPIO_INT_BOTH_EDGE; //ÖÐ¶Ï´¥·¢·½Ê½Ë«±ßÑØ
    input_cfg.param.intCfg.intCb = demo_gpio_handle; //ÖÐ¶Ï´¦Ànº¯Êý
    err = iot_gpio_open(DEMO_GPIO_7, &input_cfg);

    if (!err)
        return;

    gpio_print("[gpio] set gpio7 input");
}

VOID demo_gpio_output(VOID)
{
    T_AMOPENAT_GPIO_CFG  output_cfg;
    BOOL err;
    
    memset(&output_cfg, 0, sizeof(T_AMOPENAT_GPIO_CFG));
    
    output_cfg.mode = OPENAT_GPIO_OUTPUT; // Åäöãêä³ö
    output_cfg.param.defaultState = FALSE; // Ä¬ÈÏµÍµçÆ½

    err = iot_gpio_open(DEMO_GPIO_9, &output_cfg);

    if (!err)
        return;
        
    iot_gpio_set(DEMO_GPIO_9, TRUE); // étoª¸ßtenª¸æ½
    
    gpio_print("[gpio] set gpio9 output");
}

VOID demo_gpio_init(VOID)
{
    demo_gpio_output(); // åÄö ª ª ª ª³³
    demo_gpio_input(); // åÄöögpio9î ªE
}

int appimg_enter(void *param)
{    
    gpio_print("[gpio] app_main");

    demo_gpio_init();

    return 0;
}

void appimg_exit(void)
{
    gpio_print("[gpio] appimg_exit");
}
