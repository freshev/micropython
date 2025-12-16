/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_pmd.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/26
 *
 * Description:
 *          platform power manage ½Ó¿Ú
 **************************************************************************/

#ifndef _PLATFORM_PMD_H_
#define _PLATFORM_PMD_H_

typedef enum PlatformLdoIdTag
{
    PLATFORM_LDO_VLCD,
    PLATFORM_LDO_VMMC,
	/*+\new\wj\2020.4.14\Ìn¼ÓµçÑ¹ÓòVSIM1¿ØÖÆgpio29£¬30£¬31*/
    PLATFORM_LDO_VSIM1,
	/*-\new\wj\2020.4.14\Ìn¼ÓµçÑ¹ÓòVSIM1¿ØÖÆgpio29£¬30£¬31*/
	/*++ December jailer \202020*/
	PLATFORM_LDO_VCAMA,
	PLATFORM_LDO_VCAMD,
	/*-extangleguys \2020.5. answered ´¬CCAV ² ³V »MCAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ‐year*/
	/*+\GGY\ 2020.22/2883:nga¢°°°°3*/
	PLATFORM_LDO_VIBR,
	/*or*/
	/*+\BUG3154\zhuwangbin\2020.10.10\Ìn¼ÓbacklightÉèÖÃ*/
	PLATFORM_LDO_VBACKLIGHT_R,
	PLATFORM_LDO_VBACKLIGHT_G,
	PLATFORM_LDO_VBACKLIGHT_B,
	PLATFORM_LDO_VBACKLIGHT_W,
	/*-\BUG3154\zhuwangbin\2020.10.10\Ìn¼ÓbacklightÉèÖÃ*/
	
	/*+\BUG3753\zhuwangbin\2020.12.4\É¼Óaudio hmic bias ldoÉèèÃ*/
	PLATFORM_LDO_POWER_HMICBIAS,
	/*-\BUG3753\zhuwangbin\2020.12.4\É¼Óaudio hmic bias ldoÉèèÃ*/
    PLATFORM_LDO_QTY
}PlatformLdoId;

/*+\NEWikiang\2013.89*/
/*+\NEW\liweiqiang\2014.2.8\ÍêÉÆµçÔ´¹ÜÀnÅäÖÃ½Ó¿Ú*/
#define PMD_CFG_INVALID_VALUE           (0xffff)

typedef struct PlatformPmdCfgTag
{
/*+\EW\RUFEWUFE\2015.8\Æ\è³µcµc´*/
    u16             ccLevel;/*ºãÁ÷½×¶Î:4.1*/
    u16             cvLevel;/*ºÃñ¹½ × ¶Î: 4.2*/
    u16             ovLevel;/*³µcâ ugg£££4.*/
    u16             pvLevel;/*»Ø nation4.1*/
    u16             poweroffLevel;/*¹ø »úµçñ¹ £ º3.4 £ ¬½ÖóÃÓÚ¼æËÃµÇe¿ ° ù · Ö ± È £ ¬Êµ¼ÊóÉÉÏ²Ã¿øÖæ¹ø» ú*/
    u16             ccCurrent;/*ºãÁ÷½×¶ÎµçÁ÷*/
    u16             fullCurrent;/*ºÃñ¹³äâúµÇe ÷ £ º30*/
/*-\NEW\RUFE\2015.8\Æ\è³µcµc´*/
    /*+\NEW rage\ c [2014.11.11.6\´tâ‐rack´´‐quar µrdâ€™t*/
    u16             batdetectEnable;
    /*-\ENEW\gat\2014.11.6\´tâ‐tix´t µâ O½OC µ‐s µ‐ µrdu´»*/
}PlatformPmdCfg;
/*-\NEW\liweiqiang\2014.2.8\ÍêÉÆµçÔ´¹ÜÀnÅäÖÃ½Ó¿Ú*/

int platform_pmd_init(PlatformPmdCfg *pmdCfg);
/*-\NEWikiangiang\2013.9.8\ïöö¼îîîîÖçóäµç±Óî′à½Óî′î′à′.*/

int platform_ldo_set(PlatformLdoId id, int level);

//sleep_wake: 1 sleep 0 wakeup
int platform_pmd_powersave(int sleep_wake);

/*+\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/
int platform_pmd_get_charger(void);
/*-\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/

#endif//_PLATFORM_PMD_H_
