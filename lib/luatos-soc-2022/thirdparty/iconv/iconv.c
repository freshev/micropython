/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    iconv.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/7/15
 *
 * Description:
 *          ×Ö·û±àÂë×ª»»
 **************************************************************************/
 /*@Modules  iconv
@summary ×Ö·û±àÂë×ª»»
@version V0.1
@data    2021Äê12ÔÂ13ÈÕ*/

#include <string.h>
#include "iconv.h"
#include "prv_iconv.h"
#include "luat_debug.h"

typedef struct builtin_iconv_map_tag
{
    const char      *from;
    const char      *to;
    const iconv_fct fct;
}builtin_iconv_map;

static const builtin_iconv_map iconv_map[] =
{
    {"ucs2",    "gb2312",   iconv_ucs2_to_gb2312},
/*+\NEW\liweiqiang\2013.11.26\ÍêÉÆgb2312<->ucs2(ucs2be)±àÂë×ª»»*/
    {"ucs2be",  "gb2312",   iconv_ucs2be_to_gb2312},
    {"gb2312",  "ucs2",     iconv_gb2312_to_ucs2},
    {"gb2312",  "ucs2be",   iconv_gb2312_to_ucs2be},
/*-\NEW\liweiqiang\2013.11.26\ÍêÉÆgb2312<->ucs2(ucs2be)±àÂë×ª»»*/
/*+\NEW\liweiqiang\2013.7.19\Ôö¼Óutf8<->ucs2,ucs2be±àÂë×ª»»*/
    {"utf8",    "ucs2",     iconv_utf8_to_ucs2},
    {"utf8",    "ucs2be",   iconv_utf8_to_ucs2be},
    {"ucs2",    "utf8",     iconv_ucs2_to_utf8},
    {"ucs2be",    "utf8",     iconv_ucs2be_to_utf8},
/*-\NEW\liweiqiang\2013.7.19\Ôö¼Óutf8<->ucs2,ucs2be±àÂë×ª»»*/
};

/*´ò¿ªÏàÓ¦×Ö·û±àÂë×ª»»º¯Êý
@function iconv.open(tocode, fromcode) 
@string tocode$Ä¿±ê±àÂë¸ñÊ½$gb2312/ucs2/ucs2be/utf8
@string fromcode$Ô´±àÂë¸ñÊ½$gb2312/ucs2/ucs2be/utf8
@return table$cd$±àÂë×ª»»º¯ÊýµÄ×ª»»¾ä±ú$ 
@usage
--unicode´ó¶Ë±àÂë ×ª»¯Îª utf8±àÂë
local cd = iconv.open("utf8", "ucs2be")*/
iconv_t iconv_open (const char * to_code, const char * from_code)
{
    size_t i;

    for(i = 0; i < sizeof(iconv_map)/sizeof(iconv_map[0]); i++)
    {
        if(strcmp(iconv_map[i].from, from_code) == 0 &&
            strcmp(iconv_map[i].to, to_code) == 0)
        {
            return (iconv_t)&iconv_map[i];
        }
    }

    return (iconv_t)-1;
}

/*×Ö·û±àÂë×ª»»
@function cd:iconv(inbuf) 
@string inbuf$ÊäÈë×Ö·û´®$ÀýÈç:ucs2s 
@return number$result$·µ»Ø±àÂë×ª»»ºóµÄ½e¹û$0³É¹¦,-1Ê§°Ü
@usage
--unicode´ó¶Ë±àÂë ×ª»¯Îª utf8±àÂë
function ucs2beToUtf8(ucs2s)
    local cd = iconv.open("utf8", "ucs2be")
    return cd:iconv(ucs2s)
end*/
size_t iconv_convert (iconv_t __cd, char ** __inbuf, size_t * __inbytesleft, char ** __outbuf, size_t * __outbytesleft)
{
    builtin_iconv_map *_map_cd = (builtin_iconv_map *)__cd;

    if(__inbuf == NULL || *__inbuf == NULL)
        return (size_t)-1;

    if(_map_cd < &iconv_map[0] && _map_cd > &iconv_map[sizeof(iconv_map)/sizeof(iconv_map[0])])
        return (size_t)-1;

    return _map_cd->fct(__inbuf, __inbytesleft, __outbuf, __outbytesleft);
}

/*¹Ø±Õ×Ö·û±àÂë×ª»»
@function iconv.close(cd) 
@string cd$iconv.open·µ»ØµÄ¾ä±ú$ 
@return  
@usage
--¹Ø±Õ×Ö·û±àÂë×ª»»
local cd = iconv.open("utf8", "ucs2be")
iconv.close(cd)*/
int iconv_close (iconv_t cd)
{
    return 0;
}

