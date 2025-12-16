/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    pmd.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/28
 *
 * Description:
 *          lua.pmd¿â µçÔ´¹ÜÀn¿â
 **************************************************************************/

#include <stdlib.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lplatform.h"
#include "lrotable.h"
#include "platform_conf.h"
#include "platform_pmd.h"

/*+\NEWikiang\2013.89*/
static int getFiledInt(lua_State *L, int index, const char *key, int defval)
{
    lua_getfield(L, index, key);
    return luaL_optint(L, -1, defval);
}

// pmd.init
static int pmd_init(lua_State *L) {
    #define GET_FIELD_VAL(fIELD, dEFault) pmdcfg.fIELD = getFiledInt( L, 1, #fIELD, dEFault)

    PlatformPmdCfg pmdcfg;

    luaL_checktype(L, 1, LUA_TTABLE);
    
/*+\EW\RUFEWUFE\2015.8\Æ\è³µcµc´*/
    GET_FIELD_VAL(ccLevel, PMD_CFG_INVALID_VALUE);
    GET_FIELD_VAL(cvLevel, PMD_CFG_INVALID_VALUE);
    GET_FIELD_VAL(ovLevel, PMD_CFG_INVALID_VALUE);
    GET_FIELD_VAL(pvLevel, PMD_CFG_INVALID_VALUE);
    GET_FIELD_VAL(poweroffLevel, PMD_CFG_INVALID_VALUE);
    GET_FIELD_VAL(ccCurrent, PMD_CFG_INVALID_VALUE);
    GET_FIELD_VAL(fullCurrent, PMD_CFG_INVALID_VALUE);
    /*+\NEW rage\ c [2014.11.11.6\´tâ‐rack´´‐quar µrdâ€™t*/
    GET_FIELD_VAL(batdetectEnable, PMD_CFG_INVALID_VALUE);
    /*-\ENEW\gat\2014.11.6\´tâ‐tix´t µâ O½OC µ‐s µ‐ µrdu´»*/
/*-\NEW\RUFE\2015.8\Æ\è³µcµc´*/

    lua_pushinteger(L, platform_pmd_init(&pmdcfg));

    return 1;
}
/*-\NEWikiangiang\2013.9.8\ïöö¼îîîîÖçóäµç±Óî′à½Óî′î′à′.*/

// pmd.ldoset
static int pmd_ldo_set(lua_State *L) {
    int total = lua_gettop(L);
    int level = luaL_checkinteger(L, 1);
    int i;
    int ldo;

    for(i = 2; i <= total; i++)
    {
        ldo = luaL_checkinteger(L, i);
        platform_ldo_set(ldo, level);
    }

    return 0; 
}

// pmd.sleep(sleepornot)
static int pmd_deepsleep(lua_State *L) {    
    int sleep = luaL_checkinteger(L,1);

    platform_pmd_powersave(sleep);
    return 0; 
}

/*+\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/
//pmd.charger()
static int pmd_charger(lua_State *L) {
    lua_pushboolean(L, platform_pmd_get_charger());
    return 1;
}
/*-\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/


static int pmd_chargingcurrent(lua_State *L) {
    lua_pushinteger(L, platform_pmd_getChargingCurrent());
    return 1;
}
static int pmd_chg_param_get(lua_State *L)
{
	BOOL    battStatus = TRUE;
    BOOL    chargerStatus = 0;
    u32      chargeState = 0;
    u8      battLevel = 100;
    u16     battVolt = 4200;

	platform_pmd_get_chg_param(&battStatus, &battVolt, &battLevel, &chargerStatus, &chargeState);
	lua_pushboolean(L, battStatus);
	lua_pushinteger(L, battVolt);
	lua_pushinteger(L, battLevel);
	lua_pushboolean(L, chargerStatus);
	lua_pushinteger(L, chargeState);
	
	return 5;
}

static int pmd_speakerMode_set(lua_State *L) {
    int mode = luaL_checkinteger(L,1);

    //platform_pmd_speakerMode_set(mode);
    return 0;
}


#define MIN_OPT_LEVEL 2
#include "lrodefs.h"  

// Module function map
const LUA_REG_TYPE pmd_map[] =
{ 
/*+\NEWikiang\2013.89*/
  { LSTRKEY( "init" ),  LFUNCVAL( pmd_init ) },
/*-\NEWikiangiang\2013.9.8\ïöö¼îîîîÖçóäµç±Óî′à½Óî′î′à′.*/
  { LSTRKEY( "ldoset" ),  LFUNCVAL( pmd_ldo_set ) },
  { LSTRKEY( "sleep" ),  LFUNCVAL( pmd_deepsleep ) },
  /*+\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/
  { LSTRKEY( "charger" ),  LFUNCVAL( pmd_charger ) },
  /*-\NEW\liweiqiang\2014.2.13\Ôö¼Ópmd.charger²éÑ¯³äµçÆ÷×´Ì¬½Ó¿Ú*/

  { LSTRKEY( "chrcurrent" ),  LFUNCVAL( pmd_chargingcurrent ) },
/*+ +ENW\’zangbin\2717.2.10.10.10*/
  { LSTRKEY( "param_get" ),  LFUNCVAL( pmd_chg_param_get ) },
/*+ +ENW\’zangbin\2717.2.10.10.10*/
  { LSTRKEY( "speakerMode_set" ),  LFUNCVAL( pmd_speakerMode_set ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_pmd( lua_State *L )
{
    luaL_register( L, AUXLIB_PMD, pmd_map );

/*+ \ NEW \ Loweiqianang \ 2013.10.10.110 \ éóódo_vócd¿ømower_vlcd_vlcd*/
    MOD_REG_NUMBER(L, "LDO_VLCD", PLATFORM_LDO_VLCD);
/*- \ new \ liwiqianang \ 2013.10.10 \ éóóldo_vólcd¿ømower_vlcd_vlcd*/

/*+\NEW accordancement\2013.11.11.8\ a dictionary¼â‐duesâ€™t*/
    MOD_REG_NUMBER(L, "LDO_VMMC", PLATFORM_LDO_VMMC);
/*+\new\wj\2020.4.14\Ìn¼ÓµçÑ¹ÓòVSIM1¿ØÖÆgpio29£¬30£¬31*/
	MOD_REG_NUMBER(L, "LDO_VSIM1", PLATFORM_LDO_VSIM1);
/*-\new\wj\2020.4.14\Ìn¼ÓµçÑ¹ÓòVSIM1¿ØÖÆgpio29£¬30£¬31*/
	/*++ December jailer \202020*/
	MOD_REG_NUMBER(L, "LDO_VCAMA", PLATFORM_LDO_VCAMA);
	MOD_REG_NUMBER(L, "LDO_VCAMD", PLATFORM_LDO_VCAMD);
	/*-extangleguys \2020.5. answered ´¬CCAV ² ³V »MCAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ²CAM ‐year*/
	/*+\GGY\ 2020.22/2883:nga¢°°°°3*/
	MOD_REG_NUMBER(L, "LDO_VIBR", PLATFORM_LDO_VIBR);
	/*or*/
	
	/*+\BUG3154\zhuwangbin\2020.10.10\Ìn¼ÓbacklightÉèÖÃ*/
	MOD_REG_NUMBER(L, "LDO_VBACKLIGHT_R", PLATFORM_LDO_VBACKLIGHT_R);
	MOD_REG_NUMBER(L, "LDO_VBACKLIGHT_G", PLATFORM_LDO_VBACKLIGHT_G);
	MOD_REG_NUMBER(L, "LDO_VBACKLIGHT_B", PLATFORM_LDO_VBACKLIGHT_B);
	MOD_REG_NUMBER(L, "LDO_VBACKLIGHT_W", PLATFORM_LDO_VBACKLIGHT_W);
	/*-\BUG3154\zhuwangbin\2020.10.10\Ìn¼ÓbacklightÉèÖÃ*/
	/*+\BUG3753\zhuwangbin\2020.12.4\É¼Óaudio hmic bias ldoÉèèÃ*/
	MOD_REG_NUMBER(L, "LDO_HMICBIAS", PLATFORM_LDO_POWER_HMICBIAS);
	/*-\BUG3753\zhuwangbin\2020.12.4\É¼Óaudio hmic bias ldoÉèèÃ*/
    return 1;
}  
