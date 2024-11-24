/**************************************************** ***********************
 * Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name: iconv.c
 * Author: liweiqiang
 * Version: V0.1
 * Date: 2013/7/15
 *
 * Description:
 * Character encoding conversion
 *************************************************** ************************/
 /*@Modules iconv
@summary Character encoding conversion
@version V0.1
@data December 13, 2021*/

#include <string.h>
#include "iconv.h"
#include "prv_iconv.h"

typedef struct builtin_iconv_map_tag
{
    const char      *from;
    const char      *to;
    const iconv_fct fct;
}builtin_iconv_map;

static const builtin_iconv_map iconv_map[] =
{
    {"ucs2",    "gb2312",   iconv_ucs2_to_gb2312},
/*+\NEW\liweiqiang\2013.11.26\Improve gb2312<->ucs2(ucs2be) encoding conversion*/
    {"ucs2be",  "gb2312",   iconv_ucs2be_to_gb2312},
    {"gb2312",  "ucs2",     iconv_gb2312_to_ucs2},
    {"gb2312",  "ucs2be",   iconv_gb2312_to_ucs2be},
/*-\NEW\liweiqiang\2013.11.26\Improve gb2312<->ucs2(ucs2be) encoding conversion*/
/*+\NEW\liweiqiang\2013.7.19\Add utf8<->ucs2,ucs2be encoding conversion*/
    {"utf8",    "ucs2",     iconv_utf8_to_ucs2},
    {"utf8",    "ucs2be",   iconv_utf8_to_ucs2be},
    {"ucs2",    "utf8",     iconv_ucs2_to_utf8},
    {"ucs2be",    "utf8",     iconv_ucs2be_to_utf8},
/*-\NEW\liweiqiang\2013.7.19\Add utf8<->ucs2,ucs2be encoding conversion*/
};

/*Open the corresponding character encoding conversion function
@function iconv.open(tocode, fromcode)
@string tocode$target encoding format$gb2312/ucs2/ucs2be/utf8
@string fromcode$Source encoding format$gb2312/ucs2/ucs2be/utf8
@return table$cd$Conversion handle of encoding conversion function$
@usage
--unicode big-endian encoding converted to utf8 encoding
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

/*Character encoding conversion
@function cd:iconv(inbuf)
@string inbuf$Input string$For example: ucs2s
@return number$result$ returns the result after encoding conversion $0 for success, -1 for failure
@usage
--unicode big-endian encoding converted to utf8 encoding
function ucs2beToUtf8(ucs2s)
    local cd = iconv.open("utf8", "ucs2be")
    return cd:iconv(ucs2s)
end*/
size_t iconv_convert (iconv_t __cd, char ** __inbuf, size_t * __inbytesleft, char ** __outbuf, size_t * __outbytesleft)
{
    builtin_iconv_map *_map_cd = (builtin_iconv_map *)__cd;

    if(__inbuf == NULL || *__inbuf == NULL)
        return (size_t)-1;

    if(_map_cd < &iconv_map[0] &&
        _map_cd > &iconv_map[sizeof(iconv_map)/sizeof(iconv_map[0])])
        return (size_t)-1;

    return _map_cd->fct(__inbuf, __inbytesleft, __outbuf, __outbytesleft);
}

/*Turn off character encoding conversion
@function iconv.close(cd)
@string cd$iconv.open handle returned $
@return
@usage
--Turn off character encoding conversion
local cd = iconv.open("utf8", "ucs2be")
iconv.close(cd)*/
int iconv_close (iconv_t cd)
{
    return 0;
}

