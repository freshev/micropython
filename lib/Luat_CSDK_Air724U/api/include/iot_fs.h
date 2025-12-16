#ifndef __IOT_FS_H__
#define __IOT_FS_H__

#include "iot_os.h"


/**
 * @defgroup iot_sdk_fs ÎÄ¼þÏµÍ³½Ó¿Ú
 * @{*/
	/**@example fs/demo_fs.c
	* fs½Ó¿ÚÊ¾Àý*/ 

/**'Ò¿¿ªî¼¼þ
* @ param pszfilenameunile: î¼¼þè "Â · ¾¶Ã³³³Æ
* @ param IFLAG:'ò¿ªª ± êÖ¾ïêï¸çë²¼¼¼e_amopenat_file_open_flag
* @ Return Int32: · μ »øI¼¼¼¾ ± ú
**/
INT32 iot_fs_open_file(                           
                        char* pszFileNameUniLe,
                        UINT32 iFlag         
                  );

/**««¼À µ
*@param iFd: Å¼´±ú±ú±±thed_fire µ create_µ”µØµ×
*@return INT32: ·µ”ðµï¯èïèïÕ§°Ü, ÆÉ³Éagon¹¹ become
**/
INT32 iot_fs_close_file(                     
                        INT32 iFd           
                   );

/**¶çÈ¡Îeïe
*@param iFd,
*@param pBuf:
*@param iLen: bufin¤¶È
*@return INT32: · ÐÓ℃
**/
INT32 iot_fs_read_file(                            
                        INT32 iFd,            
                        UINT8 *pBuf,            
                        UINT32 iLen            
                  );

/**Ð´àÈçöli µ
*@param iFd: Å¼´±ú±ú±±thed_fire µ create_µ”µØµ×
*@param pBuf: çoday´µµâ
*@param iLen: Iýµ³µµµµ
*@return INT32: ·µ”ð´è³³, Ð¡ÓÓ´â ¾Ê§ ³³óóóóóóó.
**/
INT32 iot_fs_write_file(                           
                        INT32 iFd,            
                        UINT8 *pBuf,          
                        UINT32 iLen             
                   );

/**Á¼ugÑ´öàÑ´ö.
*@param iFd: Å¼´±ú±ú±±thed_fire µ create_µ”µØµ×
*@retturn INT32: ·µ”üug´´´³¶ov, Ð¡Óº´èè§°è
**/
INT32 iot_fs_flush_file(                           
                        INT32 iFd              
                   );    

/** ¼ug¨ugFed”
*@note ²-OffsetµüµÀ´OÀ¾OµµµÓµ.
*@param iFd: Å¼´±ú±ú±±thed_fire µ create_µ”µØµ×
*@param iOffset: I’m ‘OUR»
*@param iOrigin: ²-EWOURY¼²òòòòòçENT_FEEK_FLAG
*@return INT32: ·µ”Øæ¼µ¼¼
**/
INT32 iot_fs_seek_file(                           
                        INT32 iFd,            
                        INT32 iOffset,         
                        UINT8 iOrigin          
                  );

/**´«ughugâ€
*@param pszFileNamniLe: ÂÂÂÂÂÂÂÂÂÂÂ¼³
*@return INT32: ·µ”Øx¼¾ü´±ú, Ð¡Ó¯´â ¾³ ³³â″
**/
INT32 iot_fs_create_file(                          
                        char* pszFileNameUniLe   
                    );

/**â€™óÀ³òâ 
*@param pszFileNamniLe: ÂÂÂÂÂÂÂÂÂÂÂ¼³
*@return INT32: ·µ”ï»ugculal´´â ¾³ ³³Éagon²
**/
INT32 iot_fs_delete_file(                          
                        char* pszFileNameUniLe
                    );

/** Çð »» μ ± Ç ° ¹¤ × ÷ Ä¿Â¼
* @ Param Pszdirnameunile: ¿¿Â¼Â · ¾¶
* @ return Int32: · μ »øÖμ¡óú0 ± Nê¾ê§ ° ü, æäóà³³é¹|
**/
INT32 iot_fs_change_dir(                            
                        char* pszDirNameUniLe  
                   );

/**''½¨ä¿â¼
* @ Param Pszdirnameunile: ¿¿Â¼Â · ¾¶
* @ param imode: ä¿â¼êôôô £ ¬¬ê¸¸¸ë²ë²¼ûe_амopenat_file_attr_tag
* @ return Int32: · μ »øÖμ¡óú0 ± Nê¾ê§ ° ü, æäóà³³é¹|
**/
INT32 iot_fs_make_dir(                              
                        char* pszDirNameUniLe, 
                        UINT32 iMode          
                 );

/**â€™èý³Â
*@param pszirNamniLeLe: Ä¼ÂÂ¼¾¶,µ²»«²»«²à»²µ³³³óç
*@return INT32: ·µ”ï»ugculal´´â ¾³ ³³Éagon²
**/
INT32 iot_fs_remove_dir(                          
                        char* pszDirNameUniLe  
                   );

/**”anÂµµ»´¶
*@param pCurDirUniLe: ÂÂ¼Â¾¶
*@param uUniniSize: ´s´â´¿Â¿Â»²´´´ócul
*@return INT32: ·µ”ï»ugculal´´â ¾³ ³³Éagon²
**/
INT32 iot_fs_get_current_dir(                    
                        char* pCurDirUniLe,   
                        UINT32 uUnicodeSize   
                        );

/**”niçoçöliµÀ³µ³²¢
*@param path: Åµî³µ³µÂÂÂÂÂÂºµ
*@param fileInfo: â₼Ïµ³®
*@return INT32 ·µ”èµÐculâ¡çï¯´â ¾èn²³²ung´
**/
INT32 iot_fs_get_fs_info(                         
                        char       *path,         
                        T_AMOPENAT_FILE_INFO               *fileInfo                 
                   );


/**”anµóóóóóóóçoÐ«¿Ú»
*@param pszFileNamniLe: ÂÂÂÂÂÂÂÂÂÂÂ¼³
*@return UINT32: ·Øà¼¼µ³³.
**/
UINT32 iot_fs_file_size(
                    char* pszFileNameUniLe
                );

/**ugo´²²²²²¼¼¼¼¼uggöli
*@param dyerName: Â ¾¶
*@param finedRessult: ¶´µµÂµÀµÀµÀµâ¼
*@return fileInfo: ²àOUOUØµúØ¾ü»µµµµculâµ¯«´èâÀ³nâ 
**/                   
INT32 iot_fs_find_first(
     char* dirName,
     PAMOPENAT_FS_FIND_DATA findResult
);

/**»Ñè¡îä¼þ¼ÐÏÂÎÄ¼þÃÛ
*@param ifd: iot_fs_find_first½ó¿ú · µ »øöµ
*@Param Findresult: ¶Ôó¦â · ¾¶ÏÂµÄÎÄ¼Þ
*@return fileinfo: · µ »øð¡óú0 ± nê¾Ã» óðÊ £ óàîä¼þ £ ¬ÆÄËÛÖµ ± NÊ¾ »¹óÐÎÄ¼Þ
**/                   
INT32 iot_fs_find_next(
     INT32 iFd, 
     PAMOPENAT_FS_FIND_DATA findResult
);

/**¹ø ± Õ²ÉõÒ
*@param ifd: iot_fs_find_first½ó¿ú · µ »øöµ
*@Return Fileinfo: · µ »ØÖµÐ¡óÚ0 ± nÊ¾Ê§ ° ü, Æäóà³é¹¦
**/           
INT32 iot_fs_find_close(
     INT32 iFd
);

/**ü¼®ugugice ½â
*@param iFd: Å¼´±ú±ú±±thed_fire µ create_µ”µØµ×
*@return INT32: ·µ”Øæ¼µ¼¼
**/
INT32 iot_fs_ftell(    
							INT32 iFd			
					  );


/**Mountóã »§ × ôtepert
*@Param Param îä directly
*@Note Path: îämicþïµn³¸ù¿??ten,
*@Note Offset; Flash µøö · æ æ «òæe.
*@Note Size: îä thorn
*@Note Exflash: êç · ñêçnâ² creva
*@Note × ¢: Pathòô/¿ªn ·, ³ 50èðèðèòth úóóúµèóú5 àýèç "/app1"
*@Note ö§³öµäflashðnºå
			XT25W32B 0x16600b
			XT25W64B 0x17600b
			Xm25qu64a 0x173820
			Xm25qu64b 0x175020
			Xm25qu32c 0x165020
			Xm25qu16c 0x155020
			P25q64h 0x176085
			Gd25le64e 0x1760c8
			GD25LQ128C 0x1860C8
			W25Q64JV 0x1740EF
			Gd25q127c 0x1840c8
*@Return Bool: True-By
**/ 
BOOL iot_fs_mount(T_AMOPENAT_USER_FSMOUNT * param);

/**Unmountóã »§ × Ó¼ººRºµäîä¼þïµn³
*@param param îä¼þïµn³åäöã½e¹¹ìå
*@Return Bool: True-³é¹¦ False-° ° ü
**/ 
BOOL iot_fs_unmount(T_AMOPENAT_USER_FSMOUNT * param);

/**Unmountóã »§ × Ó¼ººRºµäîä¼þïµn³
*@param param îä¼þïµn³åäöã½e¹¹ìå
*@Return Bool: True-³é¹¦ False-° ° ü
**/
BOOL iot_fs_format(T_AMOPENAT_USER_FSMOUNT * param);

/**SDÉÏÃæmount fatÎÄ¼þÏµÍ³
*@return	BOOL: TRUE-³É¹¦ FALSE-Ê§°Ü
**/
BOOL iot_fs_mount_sdcard(void);

/**umount SDÉÏÃæµÄfatÎÄ¼þÏµÍ³
*@return	BOOL: TRUE-³É¹¦ FALSE-Ê§°Ü
**/
BOOL iot_fs_umount_sdcard(void);

/**Format Sdéïãæµäîä¼µn³
*@Return Bool: True-³é¹¦ False-° ° ü
**/
BOOL iot_fs_format_sdcard(void);


/** @}*/

#endif
