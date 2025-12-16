/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    lzmalib.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/5/10
 *
 * Description:
 *          lzmaÑ¹ËõÎÄ¼þ½Ó¿Ú,target¶Ë½öº¬½âÑ¹ËõÔ´Âë
 **************************************************************************/

#ifndef _LZMA_LIB_H_
#define _LZMA_LIB_H_

/*+\NEW\2013.7.11\liweiqiang\Ôö¼Ólzma½âÑ¹bufµ½ÎÄ¼þµÄ½Ó¿Ú*/
int LzmaDecodeBufToFile(const unsigned char *inbuff, const unsigned int inlen,
                        const char *outfile);
/*-\NEW\2013.7.11\liweiqiang\Ôö¼Ólzma½âÑ¹bufµ½ÎÄ¼þµÄ½Ó¿Ú*/

int LzmaUncompressFile(const char *infile, const char *outfile);

#endif/*_LZMA_LIB_H_*/

