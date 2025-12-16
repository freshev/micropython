#include "string.h"
#include <stdio.h>
#include "iot_os.h"
#include "iot_debug.h"
#include "iot_fs.h"
#include "iot_flash.h"
#include "iot_pmd.h"

#define fs_print iot_debug_print
#define DEMO_FS_FILE_PATH "demo_file"
#define DEMO_FS_FILE_PATH_SDCARD "/sdcard0/demo_file"

VOID demo_fs_delete(char* file)
{
    INT32 err;

    err = iot_fs_delete_file(file);

    if (err < 0)
        return;

    fs_print("[fs] delete demo_file");
}

BOOL demo_fs_create(char* file)
{
    INT32 fd;

    fd = iot_fs_open_file(file, FS_O_RDONLY);

    if (fd >= 0) //DEMO_FS_FILE_PATHÎÄ¼þ´æÔÚ
    {
        iot_fs_close_file(fd);
        return FALSE;
    }
    
    // ´´½¨ÎÄ¼þDEMO_FS_FILE_PATH
    iot_fs_create_file(file);

    fs_print("[fs] create demo_file");
    iot_fs_close_file(fd);

    return TRUE;
}

VOID demo_fs_read(char* file)
{
    INT32 fd;
    UINT8 read_buff[64] = {0};
    INT32 read_len;
    
    fd = iot_fs_open_file(file, FS_O_RDONLY);
    

    if (fd < 0)
        return;
   
    read_len = iot_fs_read_file(fd, read_buff, sizeof(read_buff));

    if (read_len < 0)
        return;
    
    fs_print("[fs] readlen %d, read_buff %s", read_len, read_buff);

    iot_fs_close_file(fd);
}

VOID demo_fs_write(char* file)
{
    INT32 fd;
    char *write_buff = "hello world";
    INT32 write_len;
    
    fd = iot_fs_open_file(file, FS_O_RDWR);

    if (fd < 0)
        return;
    
    write_len = iot_fs_write_file(fd, (UINT8 *)write_buff, strlen(write_buff));

    if (write_len < 0)
        return;
    
    fs_print("[fs] write_len %d, write_buff %s", write_len, write_buff);

    iot_fs_close_file(fd);
}

VOID demo_fs_init(VOID)
{
    // îä¼þ² »´æ, ´½³é¹¦, ð´êýrioýases
    if (demo_fs_create(DEMO_FS_FILE_PATH))  
    {
        demo_fs_write(DEMO_FS_FILE_PATH); // Ð´ÎÄ¼þ
        demo_fs_read(DEMO_FS_FILE_PATH); // ¶ÁÎÄ¼þ
    }
    //ÎÄ¼þ´æÔÚÖ±½Ó¶Á,
    else
    {
        demo_fs_read(DEMO_FS_FILE_PATH); // ¶ÁÎÄ¼þ
    }
	
	
	/*SDcardĿ¼*/
	// îä¼þ² »´æ, ´½³é¹¦, ð´êýrioýases
	/*
    if (demo_fs_create(DEMO_FS_FILE_PATH_SDCARD))  
    {
        demo_fs_write(DEMO_FS_FILE_PATH_SDCARD); // Ð´ÎÄ¼þ
        demo_fs_read(DEMO_FS_FILE_PATH_SDCARD); // ¶ÁÎÄ¼þ
    }
    //ÎÄ¼þ´æÔÚÖ±½Ó¶Á,
    else
    {
        demo_fs_read(DEMO_FS_FILE_PATH_SDCARD); // ¶ÁÎÄ¼þ
    }
    */
}

static void demo_fs_createDir(void)
{
	int i,j, fd;
	char path[64];
	char dir[16];
	char file[16];
	
	for (i=0; i<4; i++)
	{
		sprintf(dir, "dir_%d", i);
		
		iot_fs_make_dir(dir, 0);
		for (j=0; j<9; j++)
		{
			sprintf(file, "file_%d", j);
			sprintf(path, "%s/%s", dir, file);

			fd = iot_fs_create_file(path);
			iot_fs_write_file(fd, (UINT8 *)path, strlen(path));
			iot_fs_close_file(fd);
		}

		sprintf(dir, "dir_%d/subdir_%d", i, i);
		iot_fs_make_dir(dir, 0);
		for (j=0; j<9; j++)
		{
			sprintf(file, "subfile_%d", j);
			sprintf(path, "%s/%s", dir, file);

			fd = iot_fs_create_file(path);
			iot_fs_write_file(fd, (UINT8 *)path, strlen(path));
			iot_fs_close_file(fd);
		}
		
	}

	
}

static void demo_fs_ls(char *dirName)
{
    AMOPENAT_FS_FIND_DATA findResult;
    INT32 iFd = -1;

    iFd = iot_fs_find_first(dirName, &findResult);


	fs_print("[fs] %s ls:", dirName);
    fs_print("[fs] \t%s:\t%s\t%d\t%d\t", ((findResult.st_mode&E_FS_ATTR_ARCHIVE)? "FILE":"DIR"),    
                                                findResult.st_name,
                                                findResult.st_size,
                                                findResult.mtime);

    while(iot_fs_find_next(iFd, &findResult) == 0)
    {
        fs_print("[fs] \t%s:\t%s\t%d\t%d\t", ((findResult.st_mode&E_FS_ATTR_ARCHIVE)? "FILE":"DIR"),    
                                                findResult.st_name,
                                                findResult.st_size,
                                                findResult.mtime);
    }

    if(iFd >= 0)
    {
        iot_fs_find_close(iFd);
    }
    
}

static void demo_mountExternFlash(void)
{
	/*½½õ5q64h £ ¬ 8mflashî;*/
	BOOL ret;
	T_AMOPENAT_USER_FSMOUNT param;
	T_AMOPENAT_FILE_INFO fileInfo;

	/*¸'óandcdμäl½å ¬ðo =nlcdcdμäldo*/
	iot_pmd_poweron_ldo(OPENAT_LDO_POWER_VLCD, 15);

	iot_os_sleep(100);
	
	/*0-4m´´½¨îärd*/
	param.exFlash = E_AMOPENAT_FLASH_EXTERN_PINLCD;
	param.offset = 0;
	param.size = 0x400000;
	param.path = "/ext1";
	param.clkDiv = 4; 
	ret = iot_fs_mount(&param);
	if (!ret)
	{
		ret = iot_fs_format(&param);

		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}

		
		ret = iot_fs_mount(&param);
	
		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}
	}

	demo_fs_create("/ext1/ext1_file");
	demo_fs_write("/ext1/ext1_file");
	demo_fs_ls(param.path);
	iot_fs_get_fs_info(param.path, &fileInfo);
	fs_print("[fs] %s path %s, ret %d, mem Info (%d,%d)", __FUNCTION__, param.path, ret, fileInfo.totalSize, fileInfo.usedSize);
	

	/*4-8M´´½¨ÎÄ¼þÏµÍ³ /ext2*/
	param.exFlash = E_AMOPENAT_FLASH_EXTERN_PINLCD;
	param.offset = 0x400000;
	param.size = 0x400000;
	param.path = "/ext2";
	param.clkDiv = 4;
	ret = iot_fs_mount(&param);
	if (!ret)
	{
		ret = iot_fs_format(&param);

		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}

		ret = iot_fs_mount(&param);
	
		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}
	}

	demo_fs_create("/ext2/ext2_file");
	demo_fs_write("/ext2/ext2_file");
	demo_fs_ls(param.path);
	iot_fs_get_fs_info(param.path, &fileInfo);
	fs_print("[fs] %s path %s, ret %d, mem Info (%d,%d)", __FUNCTION__, param.path, ret, fileInfo.totalSize, fileInfo.usedSize);
	
}

static void demo_mountAppFlash(void)
{
	/*OVERY ID WAY Make IWhillion.Ð £ € £ £ ¬ ¬ ¬ Ohai*/
	BOOL ret;
	T_AMOPENAT_USER_FSMOUNT param;
	T_AMOPENAT_FILE_INFO fileInfo;
		
	/*0x260000-0X2A0000´´½¨ÎÄ¼þÏµÍ³ /ext1*/
	param.exFlash = E_AMOPENAT_FLASH_INTERNAL;
	param.offset = 0x260000;
	param.size = 0x40000;
	param.path = "/app1";
	param.clkDiv = 2;
	ret = iot_fs_mount(&param);
	if (!ret)
	{
		ret = iot_fs_format(&param);

		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}

		
		ret = iot_fs_mount(&param);
	
		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}
	}

	demo_fs_create("/app1/app1_file");
	demo_fs_write("/app1/app1_file");
	demo_fs_ls(param.path);
	iot_fs_get_fs_info(param.path, &fileInfo);
	fs_print("[fs] %s path %s, ret %d, mem Info (%d,%d)", __FUNCTION__, param.path, ret, fileInfo.totalSize, fileInfo.usedSize);
	

	/*0x2a0000-0x2E0000´´½¨ÎÄ¼þÏµÍ³ /ext2*/
	param.exFlash = E_AMOPENAT_FLASH_INTERNAL;
	param.offset = 0x2a0000;
	param.size = 0x40000;
	param.path = "/app2";
	param.clkDiv = 2;
	ret = iot_fs_mount(&param);
	if (!ret)
	{
		ret = iot_fs_format(&param);

		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}

		
		ret = iot_fs_mount(&param);
	
		if (!ret)
		{
			fs_print("[fs] %s path %s, ret %d fail", __FUNCTION__, param.path, ret);
			return;
		}
	}

	demo_fs_create("/app2/app2_file");
	demo_fs_write("/app2/app2_file");
	demo_fs_ls(param.path);
	iot_fs_get_fs_info(param.path, &fileInfo);
	fs_print("[fs] %s path %s, ret %d, mem Info (%d,%d)", __FUNCTION__, param.path, ret, fileInfo.totalSize, fileInfo.usedSize);
	
}


int appimg_enter(void *param)
{    
    // ¿ª ª ª ¢ ¢ ¿¿ì ì ì ìä îï³³ £ »¿´²» µ quarter
    INT32 ret;
	
	iot_os_sleep(1000);
    fs_print("[fs] appimg_enter");	
	
	/*ÏÔÊ¾Ô¤ÖÃÎÄ¼þsffs_file.txt*/
	demo_fs_ls("/");
	//demo_fs_ls("/sdcard0");

	/*ÏÔÊ¾Ô¤ÖÃÎÄ¼þsffs_dir/sub_sffs_file.txt*/
	demo_fs_ls("/sffs_dir");
	
    demo_fs_init();
	
	/*´´ request*/
	demo_fs_createDir();

	/*LS¸ùÄ¿Â¼*/
	demo_fs_ls("/");
	/*LS SDcardĿ¼*/
	//demo_fs_ls("/sdcard0");
	/*LS dir_1*/
	demo_fs_ls("dir_1");
	/*DEL dir_1*/
	iot_fs_remove_dir("dir_1");
	/*LS dir_1*/
	demo_fs_ls("dir_1");

	/*LS dir_2*/
	demo_fs_ls("dir_2");

	/*LS dir_2/subdir_2*/
	demo_fs_ls("dir_2/subdir_2");

	/*µ±Ç°Â·¾¶ÉèÖÃÎªdir_2/subdir_2*/
	iot_fs_change_dir("dir_2/subdir_2");

	/*Ôúμ ± Ç ° ¿¿â¼¼¼âæ'''½¨change_subdir*/
	ret = iot_fs_create_file("change_subdir_2");
	iot_fs_write_file(ret, (UINT8 *)"change_subdir_2", strlen("change_subdir_2"));
	iot_fs_close_file(ret);
	/*LS dir_2/subdir_2*/
	demo_fs_ls("./");
	
	/*App £ £àçømoounteath*/
	demo_mountAppFlash();

	/*Íâ¹òflashéïeentîä¼þïµn³*/
	demo_mountExternFlash();
	return 0;
}

void appimg_exit(void)
{
    fs_print("[fs] appimg_exit");
}

