/*********************************************************
  Copyright (C), AirM2M Tech. Co., Ltd.
  Author: Jack.li
  Description: AMOPENAT ¿ª·ÅÆ½Ì¨
  Others:
  History: 
    Version£º Date:       Author:   Modification:
    V0.1      2013.1.28   Jack.li     ´´½¨ÎÄ¼þ
*********************************************************/
#ifndef OPENAT_CAMERA_H
#define OPENAT_CAMERA_H

BOOL OPENAT_InitCamera(T_AMOPENAT_CAMERA_PARAM *cameraParam);

BOOL OPENAT_CameraPoweron(BOOL videoMode);
BOOL OPENAT_CameraPowerOff(void);
BOOL OPENAT_CameraPreviewOpen(T_AMOPENAT_CAM_PREVIEW_PARAM *previewParam);
BOOL OPENAT_CameraPreviewClose(void);
BOOL OPENAT_CameraCapture(T_AMOPENAT_CAM_CAPTURE_PARAM *captureParam);
BOOL OPENAT_CameraSavePhoto(INT32 iFd);
/*what*/
BOOL OPENAT_CameraLcdUpdateEnable(BOOL lcdUpdateEnable);
/*-\bug2406\zensually · Author has £28\\\'IÉki World comes in more*/

/*+\ New \ Jack.Li \ 2013.2.9 \ ôö¼óé · · · êóµöæčó¿ú*/
BOOL OPENAT_CameraVideoRecordStart(INT32 iFd);
BOOL OPENAT_CameraVideoRecordPause(void);
BOOL OPENAT_CameraVideoRecordResume(void);
BOOL OPENAT_CameraVideoRecordStop(void);
/*-\ New \ Jack.Li \ 2013.2.9 \ ôö¼óé · · · êóµöæčó¿ú*/

/*+\NEW\zhuwangbin\2020.7.14\Ìn¼Ócamera sensorÐ¼Ä´æÆ÷½Ó¿ Ú*/
BOOL OPENAT_CameraWriteReg(PAMOPENAT_CAMERA_REG initRegTable_p, int len);
/*+\NEW\zhuwangbin\2020.7.14\Ìn¼Ócamera sensorÐ¼Ä´æÆ÷½Ó¿ Ú*/

BOOL OPENAT_CameraLedSet(UINT8 level);

#endif /* OPENAT_CAMERA_H */

