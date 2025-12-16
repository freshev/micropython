/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_fs.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2012/11/27
 *
 * Description:
 * 
 **************************************************************************/

#include "string.h"
#include "am_openat.h"
#include "devman.h"
#include "assert.h"
#include "platform_fs.h"



WCHAR* strtows(WCHAR* dst, const char* src)
{
    while(*src)
    {
        *dst++ = *src++;
    }
    *dst = 0;
    
    return (dst);
}

static int platformfs_open_r( const char *path, int flags, int mode )
{
    int fd;
    fd = IVTBL(open_file)(path, flags, mode);

    return fd;
}


int platformfs_makedir_r( const char *path, int mode )
{
    int fd;
    return IVTBL(make_dir)(path, mode);
}



int platformfs_removedir_r( const char *path )
{
    int fd;
    return IVTBL(remove_dir)(path);
}


int platformfs_removedir_rec_r( const char *path )
{
    int fd;
    return IVTBL(remove_dir_rec)(path);
}



int platformfs_changedir_r( const char *path)
{
    return IVTBL(change_dir)(path);
}

int platformfs_decompress_file_r(const char *Comfilename,const char *Decomdfilename)
{
    return decompress_file(Comfilename,Decomdfilename);
}





static int platformfs_close_r( int fd )
{
    return IVTBL(close_file)(fd);
}

static _ssize_t platformfs_write_r( int fd, const void* ptr, size_t len )
{
    int ret = 0;

    ret = IVTBL(write_file)(fd, (UINT8 *)ptr, len);

    return ret < 0 ? 0 : ret;
}

static _ssize_t platformfs_read_r( int fd, void* ptr, size_t len )
{
    int ret = 0;

    ret = IVTBL(read_file)(fd, ptr, len);

    return ret < 0 ? 0 : ret;
}

static off_t platformfs_lseek_r( int fd, off_t off, int whence )
{
    int ret = 0;

    ret = IVTBL(seek_file)(fd, off, whence);

    return ret < 0 ? -1 : ret;
}

_ssize_t platformfs_get_size_r(const char *path)
{

    return IVTBL(get_file_size(path));
}

static const DM_DEVICE platform_fs_device = 
{
  "/",
  platformfs_open_r,         // open
  platformfs_close_r,        // close
  platformfs_write_r,        // write
  platformfs_read_r,         // read
  platformfs_lseek_r,        // lseek
  NULL,      // opendir
  NULL,      // readdir
  NULL      // closedir
};

const DM_DEVICE* platform_fs_init(void)
{
    return &platform_fs_device;
}
/*+\new\wj\2020.9.1\ÍêÉÆmount£¬unmount£¬format½Ó¿Ú*/
BOOL platform_fs_mount(PLATFORM_FS_MOUNT_PARAM *param)
{
	OPENAT_print("platform_fs_mount %s",param->path);
	T_AMOPENAT_USER_FSMOUNT mount_param;
	memset(&mount_param,0,sizeof(T_AMOPENAT_USER_FSMOUNT));
	mount_param.clkDiv = param->clkDiv;
	mount_param.exFlash = param->exFlash;
	mount_param.offset = param->offset;
	mount_param.path = param->path;
	mount_param.size = param->size;
	return OPENAT_fs_mount(&mount_param);
}

BOOL platform_fs_format(PLATFORM_FS_MOUNT_PARAM *param)
{
	OPENAT_print("platform_fs_mount %s",param->path);
	T_AMOPENAT_USER_FSMOUNT mount_param;
	memset(&mount_param,0,sizeof(T_AMOPENAT_USER_FSMOUNT));
	mount_param.clkDiv = param->clkDiv;
	mount_param.exFlash = param->exFlash;
	mount_param.offset = param->offset;
	mount_param.path = param->path;
	mount_param.size = param->size;
	return OPENAT_fs_format(&mount_param);
}

BOOL platform_fs_unmount(PLATFORM_FS_MOUNT_PARAM *param)
{	
	OPENAT_print("platform_fs_unmount %s",param->path);
	T_AMOPENAT_USER_FSMOUNT mount_param;
	memset(&mount_param,0,sizeof(T_AMOPENAT_USER_FSMOUNT));
	mount_param.clkDiv = param->clkDiv;
	mount_param.exFlash = param->exFlash;
	mount_param.offset = param->offset;
	mount_param.path = param->path;
	mount_param.size = param->size;
	return OPENAT_fs_unmount(&mount_param);
}

/*+\BUGogyuan\wgyuan\2020.06.1\uggØuggØ´¶¶ug´«²¯¯¼µated*/
#ifdef  LUA_SDCARD_SUPPORT
BOOL platformfs_Mount_sdcard(void)
{
	return IVTBL(fs_mount_sdcard)();
}

BOOL platformfs_uMount_sdcard(void)
{
	return IVTBL(fs_umount_sdcard)();
}

BOOL platformfs_Format_sdcard(void)
{
	return IVTBL(fs_format_sdcard)();
}
#endif
/*-\BUG\wgyuan\2020.06.1\uggØuggØ¶gà¶´«ugï´¯µn×µµµ²âSculâSöäâL.*/
/*-\new\wj\2020.9.1\ÍêÉÆmount£¬unmount£¬format½Ó¿Ú*/


/*+\bug2991\zhuwangbin\2020.06.11\Ôö¼Ólua otp½Ó¿Ú.*/
#define OTP_BLOCK_SIZE (0x400)
#define OTP_BLOCK_COUNT (2)
#define OTP_BASE_BLOCK (2)
#define OTP_END (OTP_BLOCK_SIZE*OTP_BLOCK_COUNT)
BOOL platformfs_otp_erase(UINT16 address, UINT16 size)
{
	UINT8 num, cur, end;

	/*Åð¶Ï²ÁµÄµøÖ · ÊÇ · ñ³¬¹ÝotpÇØóò*/
	if (address > OTP_END)
	{
		return FALSE;
	}

	cur = address / OTP_BLOCK_SIZE;
	end = (address + size -1) / OTP_BLOCK_SIZE;

	/*²el ± çpodated çalo*/
	openat_flash_eraseSecurity(cur+OTP_BASE_BLOCK);

	/*Lista east of ¬² ñBlocköð*/
	if (cur != end)
	{
		openat_flash_eraseSecurity(end+OTP_BASE_BLOCK);
	}

	return TRUE;
}

BOOL platformfs_otp_lock(UINT16 address, UINT16 size)
{
	UINT8 num, cur, end;

	/*Åð¶ïëøµäµøö · êç · ñ³¬¹ÝotpÇØóò*/
	if ((address+size-1) > OTP_END)
	{
		return FALSE;
	}
	
	cur = address / OTP_BLOCK_SIZE;
	end = (address + size -1) / OTP_BLOCK_SIZE;

	/*¡¡Μ ± çalo çalo*/
	openat_flash_lockSecurity(cur+OTP_BASE_BLOCK);

	/*At the FrenchÇ DOCH YONth Nôôпоöööð*/
	if (cur != end)
	{
		openat_flash_lockSecurity(end+OTP_BASE_BLOCK);
	}

	return TRUE;
}


BOOL platformfs_otp_write(UINT16 address, char * data, UINT32 size)
{
	UINT8 num, cur, end;
	UINT16 offset,len;

	/*Åð¶Ïð´µäÇØóÒÊÇ · ñ³¬¹ÝotpÇØóò*/
	if ((address + size) > OTP_END)
	{
		return FALSE;
	}
	
	cur = address / OTP_BLOCK_SIZE;
	end = (address + size -1) / OTP_BLOCK_SIZE;

	/*Ð Waçççççççççççççççççççççç> n² »ö*/
	if (cur != end)
	{
		offset = address % OTP_BLOCK_SIZE;
		len = OTP_BLOCK_SIZE-offset;
		openat_flash_writeSecurity(cur+OTP_BASE_BLOCK, offset, data, len);
		offset = 0;
		openat_flash_writeSecurity(end+OTP_BASE_BLOCK, offset, &data[len], size-len);
	}
	/*Ð´ÇøÓòÔÚÒ»¸öBLOCKÖÐ*/
	else 
	{
		offset = address % OTP_BLOCK_SIZE;
		openat_flash_writeSecurity(cur+OTP_BASE_BLOCK, offset, data, size);
	}

	return TRUE;
}

BOOL platformfs_otp_read(UINT16 address, char * data, UINT32 size)
{
	UINT8 num, cur, end;
	UINT16 offset,len;

	/*Åð¶Ï¶eµäçøóòêç · ñ³¬¹ýotpçøóò*/
	if ((address + size) > OTP_END)
	{
		return FALSE;
	}
	
	cur = address / OTP_BLOCK_SIZE;
	end = (address + size -1) / OTP_BLOCK_SIZE;

	/*• Pernuncap ú »» ¬blocköb*/
	if (cur != end)
	{
		offset = address % OTP_BLOCK_SIZE;
		len = OTP_BLOCK_SIZE-offset;
		openat_flash_readSecurity(cur+OTP_BASE_BLOCK, offset, data, len);
		offset = 0;
		openat_flash_readSecurity(end+OTP_BASE_BLOCK, offset, &data[len], size-len);
	}
	/*¶ÁÇøÓòÔÚÒ»¸öBLOCKÖÐ*/
	else 
	{
		offset = address % OTP_BLOCK_SIZE;
		openat_flash_readSecurity(cur+OTP_BASE_BLOCK, offset, data, size);
	}

	return TRUE;
}
/*-\bug2991\zhuwangbin\2020.06.11\ÔöµÓlua otp½Ó¼ÚµÚ.*/

