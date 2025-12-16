/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
#ifndef __PLATFORM_CAMERA_H__
#define __PLATFORM_CAMERA_H__

/*-\NEW\zhuwangbin\2020.8.22\ lua°æ±¾µÄcamera¼Ä´æÆÓÉ½Å±¾ÅäÖÃ*/
typedef struct T_PLATFORM_CAMERA_REG_TAG
{
    UINT16      addr;
    UINT16      value;
}PLATFORM_CAMERA_REG, *PPLATFORM_CAMERA_REG;

typedef enum
{
    PLATFORM_SPI_MODE_NO = 0,         // parallel sensor in use
    PLATFORM_SPI_MODE_SLAVE ,        // SPI sensor as SPI slave
    PLATFORM_SPI_MODE_MASTER1,     // SPI sensor as SPI master, 1 sdo output with SSN 
    PLATFORM_SPI_MODE_MASTER2_1, // SPI sensor as SPI master, 1 sdo output without SSN
    PLATFORM_SPI_MODE_MASTER2_2, // SPI sensor as SPI master, 2 sdos output 
    PLATFORM_SPI_MODE_MASTER2_4, // SPI sensor as SPI master, 4 sdos output
    PLATFORM_SPI_MODE_UNDEF,
} PLATFORM_CAMERA_SPI_MODE_E;

typedef enum
{
    PLATFORM_SPI_OUT_Y0_U0_Y1_V0 = 0,
    PLATFORM_SPI_OUT_Y0_V0_Y1_U0,
    PLATFORM_SPI_OUT_U0_Y0_V0_Y1,
    PLATFORM_SPI_OUT_U0_Y1_V0_Y0,
    PLATFORM_SPI_OUT_V0_Y1_U0_Y0,
    PLATFORM_SPI_OUT_V0_Y0_U0_Y1,
    PLATFORM_SPI_OUT_Y1_V0_Y0_U0,
    PLATFORM_SPI_OUT_Y1_U0_Y0_V0,
} PLATFORM_CAMERA_SPI_YUV_OUT_E;

typedef enum
{
	PLATFORM_SPI_SPEED_DEFAULT,
	PLATFORM_SPI_SPEED_SDR, /*¥ ± ¶ ¶ ·*/
	PLATFORM_SPI_SPEED_DDR, /*Ë«±¶ËÙÂÊ*/
	PLATFORM_SPI_SPEED_QDR, /*ËÄ ± ¶ËÙÂÊ ÔÝ² »Ö§³ö*/
}PLATFORM_SPI_SPEED_MODE_E;

/*bed sensorâ MERY*/
typedef struct T_PLATFORM_CAMERA_PARAM_TAG
{
    UINT8       i2cSlaveAddr;               /*Éãã · i2c · ão impitation ·*/
    UINT16      sensorWidth;                /*Oneiñn · µä¿n*/
    UINT16      sensorHeight;				/*Éãñ · - μ¸ß*/    
    PPLATFORM_CAMERA_REG initRegTable_p;  /*Éãã · ·³ 3 »Taiā¸¸î ± n*/
    UINT16 initRegTableCount;          /*Éãã ·³ê »» »îîîîîîîîýýý*/
    PLATFORM_CAMERA_REG idReg;          /*Intuhn · Id haiää''! ÷ ÷ ö*/
    PLATFORM_CAMERA_SPI_MODE_E       spi_mode; /*Éãñ · SPIRA + +*/
    PLATFORM_CAMERA_SPI_YUV_OUT_E  spi_yuv_out; /*Éãn · yuv¸ñi½½*/
	PLATFORM_SPI_SPEED_MODE_E spi_speed; /*Éñ · · · · · ·*/
}T_PLATFORM_CAMERA_PARAM;
/*-\NEW\zhuwangbin\2020.8.22\ lua°æ±¾µÄcamera¼Ä´æÆÓÉ½Å±¾ÅäÖÃ*/

BOOL platform_camera_poweron(BOOL video_mode, int nCamType, BOOL bZbarScan, BOOL bMirror, BOOL bJump);


BOOL platform_camera_poweroff(void);


BOOL platform_camera_preview_open(u16 offsetx, u16 offsety,u16 startx, u16 starty, u16 endx, u16 endy);



BOOL platform_camera_preview_close(void);



BOOL platform_camera_capture(u16 width, u16 height, u16 quality);


BOOL platform_camera_save_photo(const char* filename);


/*+\NEW\zhuwangbin\2020.7.14\Ìn¼Ócamera sensorÐ¼Ä´æÆ÷½Ó¿ Ú*/
BOOL platform_CameraWriteReg(int *pInitCmd, int nInitCmdSize);
/*+\NEW\zhuwangbin\2020.7.14\Ìn¼Ócamera sensorÐ¼Ä´æÆ÷½Ó¿ Ú*/

/*+\ENEW administrative Pruts\2020.20.20\augue µÓ*/
BOOL platform_camera_preview_zoom(int zoom);
BOOL platform_camera_preview_rotation(int rotation);
/*-\ENEW administrators\2020.20.20″\*/


#endif
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
