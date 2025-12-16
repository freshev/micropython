#include "lplatform.h"
#include "platform_watchdog.h"


/*+\ New \ Brezen \ 2016.03.03 \ rôö¼ówatchdogê¹äunu¿ú¿ú*/
int platform_watchdog_open(watchdog_info_t *info){
    return PLATFORM_OK;
}

int platform_watchdog_close(void){
    return PLATFORM_OK;
}
/*-\ new \ brezen \ 2016.03.03 \ rôö¼ówatchdogêőäunu¿ú¿ú*/

int platform_watchdog_kick(void){
    return PLATFORM_ERR;
}

