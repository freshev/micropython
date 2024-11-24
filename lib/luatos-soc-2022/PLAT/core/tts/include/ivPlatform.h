/*--------------------------------------------------------------+
 |											        			|
 |	ivPlatform.h - Platform Config			        			|
 |											        			|
 |		Platform: Win32 (X86)				        			|
 |										                 		|
 |		Copyright (c) 1999-2008, ANHUI USTC iFLYTEK CO.,LTD.	|
 |		All rights reserved.				        			|
 |											        			|
 +--------------------------------------------------------------*/


/** TODO: ÔÚÕâÀï°üº¬Ä¿±êÆ½Ì¨³ÌÐòÐèÒªµÄ¹«¹²Í·ÎÄ¼þ*/

//#include <crtdbg.h>
//#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <tchar.h>
//#include <windows.h>


/** EVERYTHING:*/

#define IV_UNIT_BITS			8			/*ÄÚ´æ»ù±¾µ¥ÔªÎ»Êý*/
#define IV_BIG_ENDIAN			0			/*ÊÇ·ñÊÇ Big-Endian ×Ö½ÚÐò*/
#define IV_PTR_GRID				4			/*×î´óÖ¸Õë¶ÔÆëÖµ*/

#define IV_PTR_PREFIX						/*Ö¸ÕëÐÞÊÎ¹Ø¼ü×Ö(µäÐÍÈ¡ÖµÓÐ near | far, ¿ÉÒÔÎª¿Õ)*/
#define IV_CONST				const		/*³£Á¿¹Ø¼ü×Ö(¿ÉÒÔÎª¿Õ)*/
#define IV_EXTERN				extern		/*Íâ²¿¹Ø¼ü×Ö*/
#define IV_STATIC				static		/*¾²Ì¬º¯Êý¹Ø¼ü×Ö(¿ÉÒÔÎª¿Õ)*/
#define IV_INLINE				__inline	/*ÄÚÁª¹Ø¼ü×Ö(µäÐÍÈ¡ÖµÓÐ inline, ¿ÉÒÔÎª¿Õ)*/
#define IV_CALL_STANDARD					/*ÆÕÍ¨º¯ÊýÐÞÊÎ¹Ø¼ü×Ö(µäÐÍÈ¡ÖµÓÐ stdcall | fastcall | pascal, ¿ÉÒÔÎª¿Õ)*/
#define IV_CALL_REENTRANT					/*µÝ¹éº¯ÊýÐÞÊÎ¹Ø¼ü×Ö(µäÐÍÈ¡ÖµÓÐ stdcall | reentrant, ¿ÉÒÔÎª¿Õ)*/
#define IV_CALL_VAR_ARG						/*±ä²Îº¯ÊýÐÞÊÎ¹Ø¼ü×Ö(µäÐÍÈ¡ÖµÓÐ cdecl, ¿ÉÒÔÎª¿Õ)*/

#define IV_TYPE_INT8			char		/*8Î»Êý¾ÝÀàÐÍ*/
#define IV_TYPE_INT16			short		/*16Î»Êý¾ÝÀàÐÍ*/
#define IV_TYPE_INT24			int			/*24Î»Êý¾ÝÀàÐÍ*/
#define IV_TYPE_INT32			long		/*32Î»Êý¾ÝÀàÐÍ*/

#if 1 /*48/64 Î»Êý¾ÀÀÐÍÊÇ¿ÉÑ¡µÄ, Èç·Ç±ØÒªÔò²»Ò¶¨Òå, ÔÚÄ³Ð© 32 Î£»ÆÄÊÊÊ Ê1⁄2Ìe©µÄ 48/64 Î»Êý¾ÀÀÐÐÍÔËËãÐ§ÂÊoÜÄ*/
#define IV_TYPE_INT48			long long	/*48Î»Êý¾ÝÀàÐÍ*/
#define IV_TYPE_INT64			long long	/*64Î»Êý¾ÝÀàÐÍ*/
#endif

#define IV_TYPE_ADDRESS			long		/*µØÖ·Êý¾ÝÀàÐÍ*/
#define IV_TYPE_SIZE			long		/*´óÐ¡Êý¾ÝÀàÐÍ*/

#define IV_ANSI_MEMORY			1			/*ÊÇ·ñÊ¹ÓÃ ANSI ÄÚ´æ²Ù×÷¿â*/
#define	IV_ANSI_STRING			0			/*ÊÇ·ñÊ¹ÓÃ ANSI ×Ö·û´®²Ù×÷¿â*/

#define IV_ASSERT(exp)						/*¶ÏÑÔ²Ù×÷(¿ÉÒÔÎª¿Õ)*/
#define IV_YIELD							/*¿ÕÏÐ²Ù×÷(ÔÚÐ×÷Ê½µ÷¶ÈÏµÍ³ÖÐÓ¦¶¨ÒåÎªÈÎÎñÇÐ»»µ÷ÓÃ, ¿ÉÒÔÎª¿Õ)*/
#define IV_TTS_ARM_CODECACHE	0			/*ÊÖ¹¤´úÂëCache*/

/*TTS¿ª·¢°ü²»Ö§³Öµ÷ÊÔ*/
#define IV_DEBUG				0			/*ÊÇ·ñÖ§³Öµ÷ÊÔ*/

/*µ÷ÊÔ·½Ê½ÏÂÔòÊä³öÈÕÖ¾*/
#ifndef IV_LOG
	#define IV_LOG				IV_DEBUG	/*ÊÇ·ñÊä³öÈÕÖ¾*/
#endif

/*ÄÚºËÖ§³ÖUnicode´úÂëÒ³²»ÐèÒª±àÒëÎªUnicode°æ*/
#if defined(UNICODE) || defined(_UNICODE)
	#define IV_UNICODE			1			/*ÊÇ·ñÒÔ Unicode ·½Ê½¹¹½¨*/
#else
	#define IV_UNICODE			0			/*ÊÇ·ñÒÔ Unicode ·½Ê½¹¹½¨*/
#endif

