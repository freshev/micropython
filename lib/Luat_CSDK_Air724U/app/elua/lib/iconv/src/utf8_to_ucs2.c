/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    utf8_to_ucs2.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/7/19
 *
 * Description:
 *          utf8 ×ª»»Îª ucs2 le/be
 **************************************************************************/

#include "stdio.h"
#include "errno.h"

#if 0
/******************************************************************************************. 
 * ½“I”µ××èleµÀ (UCS-2ºÂâ-4)»´ÂâÂ³”µÂÂÂÂÂÂÂÂ. 
 * 
 * ²-AS: 
 * unic ×Ö ·shµÂÂ 
 * pOutput Ö³³³culâ «´´ughÂ²»µ³Cøµ² 
 * outsize pOutput”³å´óóóóóóóóóóó. 
 * 
 * 
 *  µ”µ³”³³µ××â »µÂÂÂÂÂÂÂÂÂÂÂÂÂxâÂ¼µ²À²´s ¼³´µµµµµ´ØØ 0 . 
 * 
 * ×¢give: 
 * 1. TOF8Ã”ßcult½culcult «LY, µ ‘WhitenicdeÑçUÐcult´ñooO″; 
 ×èço‟cult‟cult‟ço³³´ó³(s)´èov(Little Endian)Ágram; 
 * ÔUTYug´¦²²´¶âS´uggn«uggn´, NA´è²¶â ¼¨uggnâ. (µÖ ´ earlyµë”) 
 * 2. So±±¤ pOutput º³óóóóóóóÑçöyCççoóâ »µÀ²µè´µthóóçóóR! 
 ********************************************************************************************.*/  
static int enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput,  
        int outSize)  
{  
    ASSERT(pOutput != NULL);  
    ASSERT(outSize >= 6);  

    if ( unic <= 0x0000007F )  
    {  
        // * U-00000000 - U-0000007F:  0xxxxxxx  
        *pOutput     = (unic & 0x7F);  
        return 1;  
    }  
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )  
    {  
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx  
        *(pOutput+1) = (unic & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;  
        return 2;  
    }  
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )  
    {  
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx  
        *(pOutput+2) = (unic & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;  
        return 3;  
    }  
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )  
    {  
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+3) = (unic & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;  
        return 4;  
    }  
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )  
    {  
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+4) = (unic & 0x3F) | 0x80;  
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;  
        return 5;  
    }  
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )  
    {  
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+5) = (unic & 0x3F) | 0x80;  
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;  
        return 6;  
    }  

    return 0;  
}  
#endif

static int enc_get_utf8_size(char c)
{
    int count = 0;
    while (c & (1<<7))
    {
        c = c << 1;
        count++;
    }
    return count == 0 ? 1 : count;
}

/******************************************************************************************. 
 * » «I”µ×èÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂâÂ³”ÉÉ (UCS-2«UCS-4)±ÂÂ 
 * 
 * ²-AS: 
 * pIYâ öïâ »³âóâ, OLEUTF-8±aÂ 
 * pOutput Ö³³³â³”³â, Æü´µµ²®uggâ‟âÂanÂÂÂÂâÂanÂânÂ, 
 * ended 1 - bigan
 * 0 - liters state
 * 
 * 
 * ³Éug´´µµµØµÃÃÃµÂÂÂÂÂÂÂÂÂÂÂÂânelÂâÂâÂèÃµnÕïïÕ½; â€ deassed”Øµ”Ø0. 
 * 
 * ×¢give: 
 * 1. TOF8Ã”ßcult½culcult «LY, µ ‘WhitenicdeÑçUÐcult´ñooO″; 
 ×èço‟cult‟cult‟ço³³´ó³(s)´èov(Little Endian)Ágram; 
 * ÔUTYug´¦²²´¶âS´uggn«uggn´, NA´è²¶â ¼¨uggnâ. (µÖ ´ earlyµë”) 
 ********************************************************************************************.*/  
static int enc_utf8_to_unicode_one(const char* pInput, char* pOutput, int endian)
{
    char b1, b2, b3/*, b4, b5, b6*/;
    int utfbytes = enc_get_utf8_size(*pInput);

    switch ( utfbytes )
    {
        case 1:
            if(endian)
            {
                *pOutput++ = 0x00;
                *pOutput = *pInput;
            }
            else
            {
                *pOutput++ = *pInput;
                *pOutput = 0x00;
            }
            return 2;
            //break;
        case 2:
            b1 = *pInput;
            b2 = *(pInput + 1);
            /*+ \ Bug \ Kangyuan \ 2020.11.18 \ööö½ò »· · Ö · ö'n» ø'n »*/
			/*UTF-8¶þ½øÖÆÐÎÊ½Îª 1100xxxx 10xxxxxx
			ÀýÈç:'¡¤'µÄ  UTF-8±àÂë 11000010 10110111*/
            if ( (b2 & 0xC0) != 0x80 )
            	return -1;
			/*- \ bug \ cheyyuan \ 2020.11.18 \.so ð © ð ©ìêê × · · ö μ »ø'n»*/
            if(endian)
            {
                *pOutput++ = (b1 >> 2) & 0x07;
                *pOutput     = (b1 << 6) + (b2 & 0x3F);
            }
            else
            {
                *pOutput++     = (b1 << 6) + (b2 & 0x3F);
                *pOutput = (b1 >> 2) & 0x07;
            }
            return 2;
            //break;
        case 3:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )
                return -1;
            if(endian)
            {
                *pOutput++ = (b1 << 4) + ((b2 >> 2) & 0x0F);
                *pOutput = (b2 << 6) + (b3 & 0x3F);
            }
            else
            {
                *pOutput++ = (b2 << 6) + (b3 & 0x3F);
                *pOutput = (b1 << 4) + ((b2 >> 2) & 0x0F);
            }
            return 2;
            //break;
#if 0
        case 4:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            b4 = *(pInput + 3);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) 
            || ((b4 & 0xC0) != 0x80) )
                return -1;
            *pOutput     = (b3 << 6) + (b4 & 0x3F);
            *(pOutput+1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
            *(pOutput+2) = ((b1 << 2) & 0x1C)  + ((b2 >> 4) & 0x03);
            return 3;
        break;
        case 5:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            b4 = *(pInput + 3);
            b5 = *(pInput + 4);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) 
            || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) )
                return -1;
            *pOutput     = (b4 << 6) + (b5 & 0x3F);
            *(pOutput+1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
            *(pOutput+2) = (b2 << 2) + ((b3 >> 4) & 0x03);
            *(pOutput+3) = (b1 << 6);
            return 4;
        break;
        case 6:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            b4 = *(pInput + 3);
            b5 = *(pInput + 4);
            b6 = *(pInput + 5);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) 
            || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) 
            || ((b6 & 0xC0) != 0x80) )
                return -1;
            *pOutput     = (b5 << 6) + (b6 & 0x3F);
            *(pOutput+1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
            *(pOutput+2) = (b3 << 2) + ((b4 >> 4) & 0x03);
            *(pOutput+3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
            return 4;
        break;
#endif
        default:
        break;
    }

    return -1;
}

static size_t enc_utf8_to_unicode(char **_inbuf, size_t *inbytesleft, char **_outbuf, size_t *outbytesleft, int endian)
{
    size_t maxOLen = *outbytesleft;
    size_t iLen = *inbytesleft;
    char *src = *_inbuf;
    char *dst = *_outbuf;
    size_t iPos, oPos;
    size_t utfbytes = 0;
    size_t unicodeBytes = 0;
    int result = 0;

    for(iPos = 0, oPos = 0; iPos < iLen; )
    {
        if(oPos + 2 > maxOLen)
        {
            errno = E2BIG;
            result = -1;
            goto utf8_to_ucs2_exit;
        }

        utfbytes = enc_get_utf8_size(src[iPos]);

        if(utfbytes == 0)
            utfbytes = 1;

        if((unicodeBytes = enc_utf8_to_unicode_one(&src[iPos], &dst[oPos], endian)) == -1)
        {
            errno = EINVAL;
            result = -1;
            break;
        }

        oPos += unicodeBytes;
        iPos += utfbytes;
    }

utf8_to_ucs2_exit:
    *inbytesleft -= iPos;
    *_inbuf += iPos;
    *outbytesleft -= oPos;

    return (size_t)result;
}

size_t iconv_utf8_to_ucs2(char **_inbuf, size_t *inbytesleft, char **_outbuf, size_t *outbytesleft)
{
    return enc_utf8_to_unicode(_inbuf, inbytesleft, _outbuf, outbytesleft, 0);
}

size_t iconv_utf8_to_ucs2be(char **_inbuf, size_t *inbytesleft, char **_outbuf, size_t *outbytesleft)
{
    return enc_utf8_to_unicode(_inbuf, inbytesleft, _outbuf, outbytesleft, 1);
}


static size_t enc_unicode_to_utf8(char **_inbuf, size_t *inbytesleft, char **_outbuf, size_t *outbytesleft, int endian)
{
    size_t maxOLen = *outbytesleft;
    size_t iLen = *inbytesleft;
    char *src = *_inbuf;
    char *dst = *_outbuf;
    size_t iPos, oPos;
    size_t utfbytes = 0;
    int result = 0;
    size_t unicodeVal = 0;

    for(iPos = 0, oPos = 0; iPos+1 < iLen;)
    {
        unicodeVal = ((endian==1) ? ((unsigned char)src[iPos]*256+(unsigned char)src[iPos+1]) : ((unsigned char)src[iPos+1]*256+(unsigned char)src[iPos]));
        //printf("test1:%d,%X,%X,%X\n",endian,unicodeVal,src[iPos],src[iPos+1]);

    	 if(unicodeVal <= 0x7F)
    	 {
    	     utfbytes = 1;    	 	
    	 }
    	 else if(unicodeVal > 0x7F && unicodeVal <= 0x07FF)
    	 {
    	     utfbytes = 2;
    	 }
    	 else if(unicodeVal > 0x07FF)
    	 {
    	     utfbytes = 3;
    	 }
    	 else
    	 {
    	     errno = EINVAL;
            result = -1;
            goto ucs2_to_utf8_exit;
    	 }

    	 
        if(oPos + utfbytes > maxOLen)
        {
            errno = E2BIG;
            result = -1;
            goto ucs2_to_utf8_exit;
        }

        switch ( utfbytes )
        {
            case 1:
                dst[oPos] = unicodeVal;
                break;

            case 2:
                dst[oPos] = ((unicodeVal>>6)|0xE0)&0xDF;
                dst[oPos+1] = ((char)(unicodeVal&0xFF)|0xC0)&0xBF;
                break;

            case 3: 
                dst[oPos] = (((unicodeVal>>12)&0xFF)|0xF0)&0xEF;
                dst[oPos+1] = (((unicodeVal>>6)&0xFF)|0xC0)&0xBF;
                //printf("test:%X,%X,%X,%X,%X,",unicodeVal,unicodeVal>>6,((unicodeVal>>6)&0xFF),((unicodeVal>>6)&0xFF)|0xC0,dst[oPos+1]);
                dst[oPos+2] =((unicodeVal&0xFF)|0xC0)&0xBF;
                break;

            default:
                break;
        }       

        iPos += 2;
        oPos += utfbytes;
    }

ucs2_to_utf8_exit:
    *inbytesleft -= iPos;
    *_inbuf += iPos;
    *outbytesleft -= oPos;

    return (size_t)result;
}

size_t iconv_ucs2_to_utf8(char **_inbuf, size_t *inbytesleft, char **_outbuf, size_t *outbytesleft)
{
    return enc_unicode_to_utf8(_inbuf, inbytesleft, _outbuf, outbytesleft, 0);
}

size_t iconv_ucs2be_to_utf8(char **_inbuf, size_t *inbytesleft, char **_outbuf, size_t *outbytesleft)
{
    return enc_unicode_to_utf8(_inbuf, inbytesleft, _outbuf, outbytesleft, 1);
}

