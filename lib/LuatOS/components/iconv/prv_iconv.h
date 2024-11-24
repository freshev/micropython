/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    prv_iconv.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/7/15
 *
 * Description:
 *          ×Ö·û±àÂë×ª»»ÄÚ²¿ÉùÃ÷ÎÄ¼þ
 **************************************************************************/

#ifndef __PRV_ICONV_H__
#define __PRV_ICONV_H__

/*±àÂë×ª»»´¦Ànº¯Êý iconv*/
typedef size_t (*iconv_fct) (char ** __inbuf,
                      size_t * __inbytesleft,
                      char ** __outbuf,
                      size_t * __outbytesleft);

size_t iconv_ucs2_to_gb2312(char **, size_t *, char **, size_t *);
/*+\NEW\liweiqiang\2013.11.26\ÍêÉÆgb2312<->ucs2(ucs2be)±àÂë×ª»»*/
size_t iconv_ucs2be_to_gb2312(char **, size_t *, char **, size_t *);
size_t iconv_gb2312_to_ucs2(char **, size_t *, char **, size_t *);
size_t iconv_gb2312_to_ucs2be(char **, size_t *, char **, size_t *);
/*-\NEW\liweiqiang\2013.11.26\ÍêÉÆgb2312<->ucs2(ucs2be)±àÂë×ª»»*/

/*+\NEW\liweiqiang\2013.7.19\Ôö¼Óutf8<->ucs2,ucs2be±àÂë×ª»»*/
size_t iconv_utf8_to_ucs2(char **, size_t *, char **, size_t *);

size_t iconv_utf8_to_ucs2be(char **, size_t *, char **, size_t *);

size_t iconv_ucs2_to_utf8(char **, size_t *, char **, size_t *);

size_t iconv_ucs2be_to_utf8(char **, size_t *, char **, size_t *);
/*-\NEW\liweiqiang\2013.7.19\Ôö¼Óutf8<->ucs2,ucs2be±àÂë×ª»»*/

#endif/*__PRV_ICONV_H__*/
