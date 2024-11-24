#include "luat_base.h"
#include "luat_spi.h"
#include "luat_sdio.h"
#include "luat_rtos.h"
#include "luat_gpio.h"
#include "luat_fs.h"
#include "luat_vfs.h"

#include "common_api.h"
#include "luat_lib_fatfs.h"

static FATFS *fs = NULL;		/* FatFs work area needed for each volume */
extern BYTE FATFS_DEBUG; // debug log, 0 -- disable , 1 -- enable
extern BYTE FATFS_POWER_PIN;
extern uint16_t FATFS_POWER_DELAY;
DRESULT diskio_open_ramdisk(BYTE pdrv, size_t len);
DRESULT diskio_open_spitf(BYTE pdrv, void* userdata);
DRESULT diskio_open_sdio(BYTE pdrv, void* userdata);

#ifdef LUAT_USE_FS_VFS
extern const struct luat_vfs_filesystem vfs_fs_fatfs;
#endif

static int fatfs_spi_mount(uint8_t spi_id, uint8_t cs_pin, uint32_t speed)
{
	luat_fatfs_spi_t *spit;

	spit = calloc(sizeof(luat_fatfs_spi_t), 1);
	spit->type = 0;
	spit->spi_id = spi_id; // SPI_1
	spit->spi_cs = cs_pin; // GPIO_3
	spit->fast_speed = speed;
	return diskio_open_spitf(0, (void*)spit);
}
int luat_fatfs_mount(uint8_t disk_mode, uint32_t device, uint8_t cs_pin, uint32_t speed, uint8_t power_pin, uint16_t power_on_delay, const char *mount_point, const char *vfs_mount_point)
{
	if (disk_mode != DISK_SPI)
	{
		DBG("not support disk mode %d", disk_mode);
		return -1;
	}
	FATFS_POWER_PIN = power_pin;
	FATFS_POWER_DELAY = power_on_delay;
	if (fs == NULL)
	{
		fs = calloc(sizeof(FATFS), 1);
	}
	FRESULT re;
	switch (disk_mode)
	{
	case DISK_SPI:
		re = fatfs_spi_mount(device, cs_pin, speed);
		break;
	default:
		return -1;
	}
	if (re)
	{
		DBG("init tf fail %d", re);
		return -1;
	}
	re = f_mount(fs, mount_point, 1);
	if (!re)
	{
#ifdef LUAT_USE_FS_VFS
		luat_vfs_reg(&vfs_fs_fatfs);
	    luat_fs_conf_t conf2 = {
			.busname = (char*)fs,
			.type = "fatfs",
			.filesystem = "fatfs",
			.mount_point = vfs_mount_point,
		};
		luat_fs_mount(&conf2);
#endif
	}
	else
	{
		DBG("mount tf fail %d", re);
		return -1;
	}
}

int luat_fatfs_unmount(const char *mount_point)
{
	return f_unmount(mount_point);
}

void luat_fatfs_debug_mode(uint8_t on_off)
{
	FATFS_DEBUG = on_off;

}

int luat_fatfs_format(const char *mount_point)
{

	// BYTE sfd = luaL_optinteger(L, 2, 0);
	// DWORD au = luaL_optinteger(L, 3, 0);
	BYTE work[FF_MAX_SS] = {0};

	MKFS_PARM parm = {
		.fmt = FM_ANY, // 暂时应付一下ramdisk
		.au_size = 0,
		.align = 0,
		.n_fat = 0,
		.n_root = 0,
	};
	if (!strcmp("ramdisk", mount_point) || !strcmp("ram", mount_point)) {
		parm.fmt = FM_ANY | FM_SFD;
	}
	return f_mkfs(mount_point, &parm, work, FF_MAX_SS);
}
