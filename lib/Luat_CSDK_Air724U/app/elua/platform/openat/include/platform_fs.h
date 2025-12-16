/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_fs.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2012/11/27
 *
 * Description:
 * 
 **************************************************************************/

#ifndef __PLATFORM_FS_H__
#define __PLATFORM_FS_H__
#include <devman.h>
#include "am_openat_fs.h"

/*+\new\wj\2020.9.1\ÍêÉÆmount£¬unmount£¬format½Ó¿Ú*/
typedef enum
{
	E_PLATFROM_FLASH_INTERNAL, // mount ÄÚ²¿µÄflashÇøÓò
    E_PLATFROM_FLASH_EXTERN_PINLCD, // mount Íâ²¿µÄflashÇøÓò£¬Ê¹ÓÃLCD pin½Å¸´ÓÃ  V_LCD¹©µç
    E_PLATFROM_FLASH_EXTERN_PINGPIO,// mount Íâ²¿µÄflashÇøÓò£¬Ê¹ÓÃGPIO pin½Å¸´ÓÃ V_PAD_1V8¹©µç
}PLATFORM_FLASH_TYPE;


typedef struct
{
	char *path;    //mountµÄÎÄ¼þÏµÍ³¸ùÄ¿Â¼ ³¤¶È>=5
	UINT32 offset; // flash µøö · Æ «ÒÆe¿
	UINT32 size;  //ÎÄ¼þÏµÍ³µÄ´óÐ¡
	PLATFORM_FLASH_TYPE exFlash;
	UINT8 clkDiv; //Íâ²¿flash·ÖÆµ·¶Î§2-255. clk=166M/clkDiv
}PLATFORM_FS_MOUNT_PARAM;

/*-\new\wj\2020.9.1\ÍêÉÆmount£¬unmount£¬format½Ó¿Ú*/
const DM_DEVICE* platform_fs_init(void);

int platformfs_removedir_r( const char *path );

int platformfs_changedir_r( const char *path );

int platformfs_removedir_rec_r( const char *path );


int platformfs_findfirst_r( const char *path, PAMOPENAT_FS_FIND_DATA find_data);


int platformfs_findnext_r(int find_id, PAMOPENAT_FS_FIND_DATA find_data);


int platformfs_findclose_r(int find_id);

int platformfs_makedir_r( const char *path, int mode );

_ssize_t platformfs_get_size_r(const char *path);
/*+\BUGogyuan\wgyuan\2020.06.1\uggØuggØ´¶¶ug´«²¯¯¼µated*/
/*+\new\wj\2020.9.1\ÍêÉÆmount£¬unmount£¬format½Ó¿Ú*/
BOOL platform_sdcard_fsMount(void);

BOOL platform_sdcard_fsUMount(void);

BOOL platformfs_Format_sdcard(void);
/*-\new\wj\2020.9.1\ÍêÉÆmount£¬unmount£¬format½Ó¿Ú*/
/*-\BUG\wgyuan\2020.06.1\uggØuggØ¶gà¶´«ugï´¯µn×µµµ²âSculâSöäâL.*/
#endif //__PLATFORM_FS_H__

/*+\bug2991\zhuwangbin\2020.06.11\Ôö¼Ólua otp½Ó¿Ú.*/
BOOL platformfs_otp_erase(UINT16 address, UINT16 size);

BOOL platformfs_otp_write(UINT16 address, char * data, UINT32 size);

BOOL platformfs_otp_read(UINT16 address, char * data, UINT32 size);

BOOL platformfs_otp_lock(UINT16 address, UINT16 size);
/*-\bug2991\zhuwangbin\2020.06.11\ÔöµÓlua otp½Ó¼ÚµÚ.*/
