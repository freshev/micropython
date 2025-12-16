/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_stubs.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2012/12/1
 *
 * Description:
 *    ÐèÒªÆ½Ì¨Ö§³ÖµÄ×®º¯Êý½Ó¿Ú
 **************************************************************************/
#ifndef __PLATFORM_STUBS_H__
#define __PLATFORM_STUBS_H__

// *****************************************************************************
// platform Allocator support
#include "platform_malloc.h"

// *****************************************************************************
// platform vfprintf
int platform_vfprintf(FILE *fp, const char *fmt, va_list ap);

#endif //__PLATFORM_STUBS_H__
