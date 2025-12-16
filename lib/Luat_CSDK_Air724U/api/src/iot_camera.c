#include "iot_camera.h"
#include "iot_fs.h"

/**Éãïñn · ³õê¼ »¯
*@param Camerapam: ³õê¼ »¯²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_init(T_AMOPENAT_CAMERA_PARAM *cameraParam)
{
  return IVTBL(InitCamera)(cameraParam);
}

/**´ò ‘ªéãïñn ·
*@stop video:
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_poweron(BOOL videoMode)
{
  return IVTBL(CameraPoweron)(videoMode);
}
/**¹Ø ± õéãïñn ·
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_poweroff(void)
{
  IVTBL(CameraPowerOff)();
  return TRUE;
}
/**
*@param Preview PREVARE: O¤àà²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü

**/
BOOL iot_camera_preview_open(T_AMOPENAT_CAM_PREVIEW_PARAM *previewParam)
{
  return IVTBL(CameraPreviewOpen)(previewParam);
}
/** Íë³Töh to¤9à
* @ Return TRUE: ³é¹|
* FALSE: ê ° ü

**/
BOOL iot_camera_preview_close(void)
{
  return IVTBL(CameraPreviewClose)();
}
/**Åäõõ
*@param Filename: ± £ ´ænsideà¬µäîä¼þãû
*@param Captured: O¤àà²îêý
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_capture(char *fileName, T_AMOPENAT_CAM_CAPTURE_PARAM *captureParam)
{ 
	INT32 fd;
		
	if (!fileName)
	{
		return FALSE;
	}
	
	iot_fs_delete_file(fileName);
	fd = iot_fs_create_file(fileName);

	if (fd < 0)
	{
		return FALSE; 
	}
	
	if(!IVTBL(CameraCapture)(captureParam))
	{
		iot_fs_close_file(fd);
		return FALSE;
	}

	if (!IVTBL(CameraSavePhoto)(fd))
	{
		iot_fs_close_file(fd);
		return FALSE; 
	}

	iot_fs_close_file(fd);

	return TRUE;
}

/**Éèöãcamerserseà´ææ ÷
*@param initregtable_p: Camsy
*@param Len: Cam mention´ææ ÷ ³³
*@Return True: ³é¹¦
* FALSE: § ° ü
**/
BOOL iot_camera_WriteReg(PAMOPENAT_CAMERA_REG initRegTable_p, int len)
{
	return IVTBL(CameraWriteReg)(initRegTable_p, len);
}
