/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_pmd.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/28
 *
 * Description:
 *          lua pmd API
  * History:
 *     panjun 2015.06.09 OS don't control LCD's power, only LUA.
 **************************************************************************/

#include <stdio.h>

#if 1
#include "lplatform.h"
#include "platform_pmd.h"

/*+\BUG\wangyuan\2020.04.10\BUG_1454: misc.vbatt»ñÈ¡µÄµçÔ´µçÑ¹Ê¼ÖÕÎª4200*/
//#include "drv_charger.h"
/*-\BUG\wangyuan\2020.04.10\BUG_1454: misc.vbatt»ñÈ¡µÄµçÔ´µçÑ¹Ê¼ÖÕÎª4200*/

static const E_AMOPENAT_PM_LDO ldo2OpenatLdo[PLATFORM_LDO_QTY] = {
   
    OPENAT_LDO_POWER_VLCD,

   
    OPENAT_LDO_POWER_MMC,
	/*+\new\wj\2020.4.14\Ìn¼ÓµçÑ¹ÓòVSIM1¿ØÖÆgpio29£¬30£¬31*/
	OPENAT_LDO_POWER_VSIM1,
	/*-\new\wj\2020.4.14\Ìn¼ÓµçÑ¹ÓòVSIM1¿ØÖÆgpio29£¬30£¬31*/

	/*++ December jailer \202020*/
	OPENAT_LDO_POWER_VCAMA,
	OPENAT_LDO_POWER_VCAMD,
	/*-extangleguys \2020.5. answered ´¬CCAV ² ³V »MCAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ‐year*/
	/*+\GGY\ 2020.22/2883:nga¢°°°°3*/
	OPENAT_LDO_POWER_VIBR,
	/*or*/

	/*+\BUG3154\zhuwangbin\2020.10.10\Ìn¼ÓbacklightÉèÖÃ*/
	OPENAT_LDO_POWER_VBACKLIGHT_R,
	OPENAT_LDO_POWER_VBACKLIGHT_G,
	OPENAT_LDO_POWER_VBACKLIGHT_B,
	OPENAT_LDO_POWER_VBACKLIGHT_W,
	/*-\BUG3154\zhuwangbin\2020.10.10\Ìn¼ÓbacklightÉèÖÃ*/
	
	/*+\BUG3753\zhuwangbin\2020.12.4\É¼Óaudio hmic bias ldoÉèèÃ*/
	OPENAT_LDO_POWER_HMICBIAS
	/*-\BUG3753\zhuwangbin\2020.12.4\É¼Óaudio hmic bias ldoÉèèÃ*/
};

/*+\NEWikiang\2013.89*/
extern BOOL cust_pmd_init(PlatformPmdCfg *cfg);

static E_OPENAT_CHARGE_CURRENT getOpenatCurrent(u16 current)
{
/*+\EW\RUFEWUFE\2015.8\Æ\è³µcµc´*/
    static const u16 openatCurrentVal[OPENAT_PM_CHARGE_CURRENT_QTY] =
    {
        0,
        20,
        30,
        40,
        50,
        60,
        70,
        200,
        300,
        400,
        500,
        600,
        700,
        800,
        900,
        1000,
        1100,
        1200,
        1300,
        1400,
        1500
    };
/*-\NEW\RUFE\2015.8\Æ\è³µcµc´*/
    uint16 i;

    for(i = 1/*Labor »is ¥ åðient*/; i < OPENAT_PM_CHARGE_CURRENT_QTY; i++)
    {
        if(openatCurrentVal[i] == current)
        {
/*+\BUG WM-1015\2013.11.19\ Ð³³µØð*/
            return i;
/*-\BUG WM-1015\2013.11.19\ ÐÞ³³µóóóØL.*/
        }
        else if(openatCurrentVal[i] > current)
        {
            break;
        }
    }

    return OPENAT_PM_CHARGE_CURRENT_QTY;
}
/*+\EW\RUFEWUFE\2015.8\Æ\è³µcµc´*/
static E_OPENAT_PM_VOLT getOpenatVolt(u16 volt)
{
    static const u16 openatVoltVal[OPENAT_PM_VOLT_QTY] =
    {
        0,
        1800,
        2800,
        3000,
        3200,
        3400,
        3600,
        3800,
        3850,
        3900,
        4000,
        4050,
        4100,
        4120,
        4130,
        4150,
        4160,
        4170,
        4180,
        4200,
        4210,
        4220,
        4230,
        4250,
        4260,
        4270,
        4300,
        4320,
        4350,
        4370,
        4400,
        4420
    };
    uint16 i;

    for(i = 1/*Labor »is ¥ åðient*/; i < OPENAT_PM_VOLT_QTY; i++)
    {
        if(openatVoltVal[i] >= volt)
        {
            return i;
        }
    }

    return OPENAT_PM_VOLT_QTY;
}
/*-\NEW\RUFE\2015.8\Æ\è³µcµc´*/

int platform_pmd_init(PlatformPmdCfg *pmdCfg)
{
    #define CHECK_FILED(fIELD) do{ \
        if(pmdCfg->fIELD != PMD_CFG_INVALID_VALUE && (pmdCfg->fIELD = getOpenatCurrent(pmdCfg->fIELD)) == OPENAT_PM_CHARGE_CURRENT_QTY) \
        { \
            PUB_TRACE("[platform_pmd_init]: error filed " #fIELD); \
            return PLATFORM_ERR; \
        } \
    }while(0)
/*+\EW\RUFEWUFE\2015.8\Æ\è³µcµc´*/
    #define CHECK_VOLTAGE_FILED(fIELD) do{ \
        if(pmdCfg->fIELD != PMD_CFG_INVALID_VALUE && (pmdCfg->fIELD = getOpenatVolt(pmdCfg->fIELD)) == OPENAT_PM_VOLT_QTY) \
        { \
            PUB_TRACE("[platform_pmd_init]: error filed " #fIELD); \
            return PLATFORM_ERR; \
        } \
    }while(0)

    CHECK_FILED(ccCurrent);
    CHECK_FILED(fullCurrent);
    
    CHECK_VOLTAGE_FILED(ccLevel);
    CHECK_VOLTAGE_FILED(cvLevel);
    CHECK_VOLTAGE_FILED(ovLevel);
    CHECK_VOLTAGE_FILED(pvLevel);
    CHECK_VOLTAGE_FILED(poweroffLevel);
/*-\NEW\RUFE\2015.8\Æ\è³µcµc´*/

    return cust_pmd_init(pmdCfg) ? PLATFORM_OK : PLATFORM_ERR;
}
/*-\NEWikiangiang\2013.9.8\ïöö¼îîîîÖçóäµç±Óî′à½Óî′î′à′.*/

int platform_ldo_set(PlatformLdoId id, int level)
{
    if(ldo2OpenatLdo[id] >= OPENAT_LDO_POWER_INVALID){
        return PLATFORM_ERR;
    }
	/*+\ Bug \ wangyuan \ 2020.04.07 \ êêåäldoèöãöú¿ú*/
    OPENAT_poweron_ldo(ldo2OpenatLdo[id], level);
	/*-\ bug \ wangyuan \ 2020.04.07 \ êêåäldoèöãöãó¿ú*/
    return PLATFORM_OK;
}


int platform_pmd_powersave(int sleep_wake)
{
    if(sleep_wake){
        /*the*/
        //IVTBL(sys_request_freq)(OPENAT_SYS_FREQ_32K);
        OPENAT_enter_deepsleep();
    } else {
        OPENAT_exit_deepsleep();
        //IVTBL(sys_request_freq)(OPENAT_SYS_FREQ_208M);
        /*-\NEW\\\lein Qi*/
    }

    return PLATFORM_OK;
}

/*+\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/
int platform_pmd_get_charger(void)
{
    UINT32 chargerStatus;
    
    chargerStatus = IVTBL(get_chargerHwStatus)();
    return chargerStatus == OPENAT_PM_CHR_HW_STATUS_AC_ON ? 1 : 0;
}
/*-\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/



UINT32 platform_pmd_getChargingCurrent(void)
{
    return OPENAT_pmd_getChargingCurrent();
}

int platform_pmd_get_chg_param(BOOL *battStatus, u16 *battVolt, u8 *battLevel, BOOL *chargerStatus, u32 *chargeState)
{	
	/*+\BUG\wangyuan\2020.04.10\BUG_1454: misc.vbatt»ñÈ¡µÄµçÔ´µçÑ¹Ê¼ÖÕÎª4200*/
    uint8 nBcs = 0;
    uint8 nBcl = 0;
	uint32 nvol = 0;

    drvChargerGetInfo(&nBcs, &nBcl);
	drvChargerGetBatVol(&nvol);
	if(nBcs == 0)
	{
		*chargeState = 0;
	}
	else if(nBcs == 2)
	{
		*chargeState = 1;	
	}
	else
	{
		*chargeState = 2;	
	}
    *battLevel = nBcl;
	*battVolt = nvol;
	/*-\BUG\wangyuan\2020.04.10\BUG_1454: misc.vbatt»ñÈ¡µÄµçÔ´µçÑ¹Ê¼ÖÕÎª4200*/
    return PLATFORM_OK;
}

#endif
