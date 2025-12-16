#ifndef __SSL_LIB_H__
#define __SSL_LIB_H__

/** SSLLIB.H½Ó¿Ú´Ó²¿ · ÖÊÇ½ «ÒÆÖ²¹ýà´µäAXTLSÔ´ÂË²¿ · Ö½Ó¿úÔÙ´Î · Â × ° £ ¬Ö ÷ òªêçê¹óãeëò» Ð © Ä¬ÈÏÊÄÈË
 * ³Ýeë ± øðëµ ÷ óssl_regSocketcallbacknâ £ ¬æäëû½ó¿ú¾Ù¿éòô² »ê¹óã £ ¬¶ØÖ» ê¹ósl.hàïµä½ó¿ú¡ £
 "
 * Ìøðô £ º¼û http://axtls.sourceforge.net/*/

#include "os_port.h"
#include "ssl.h"
typedef int(*SocketAPI)(int SocketFd, void *Buf, uint16_t Len);
extern EXP_FUNC int STDCALL new_pem_obj(SSL_CTX *ssl_ctx, int is_cacert, char *where,
        int remain, const char *password);
extern EXP_FUNC int STDCALL do_obj(SSL_CTX *ssl_ctx, int obj_type,
        SSLObjLoader *ssl_obj, const char *password);
/**
 * @Brief × ¢ ²esslµäêýther · ¢ ° § £ ¬óéóúä¬èïê¹óãasocketðîê½ £ ¬îª · ½ ± ãatöçeîóã »§ £ ¬ìe¹ ´ë½ó £ ¬½ ¬½« ê¾êõ · · ½ »Óéatöçeînê³é³é³é³
 * @param Sendfun · ¢ ënº thousand
 * @param Receivefun ½óêõºrseº
 * @return îþ*/
void SSL_RegSocketCallback(SocketAPI SendFun, SocketAPI ReceiveFun);

/**
 * @bref AND» ANDSSL.
 * @param NumSisions
 Ò'SSL*/
SSL_CTX * SSL_CreateCtrl(uint16_t NumSessions);

/**
 * @brief É¾³ýÒ»¸öSSL¿ØÖÆ½e¹¹Ìå
 * @param SSLCtrl ¿ØÖÆ½e¹¹ÌåµÄµØÖ·Ö¸Õë
 * @return ÎÞ*/
void SSL_FreeCtrl(SSL_CTX *SSLCtrl);

/**
 * @rrief ´´¨¨²´««ug¹¹¹²²SAµSSLYSLYµ
 * @param SSL SLĀ²-SSL£ SL.
 * @param ClientID [in] Èce«ColeetetetetetetetetetetetetetID£ugd´´èÂ²²´´´´ug0´¶gÂ¶gâ₵ug´µugg.
 * @param SessionID [in] Or”ovovugâ€™tÀµµµ´µughâ«So´So´´À²§´´´´´Sï´´cult ¼M.
 * @param Sessions SessionID³´è¶
 * @param Hostname [in] ÃâéÑéµ²´´ó³²²²²²²¼¼¼âNULL
 * @param MaxFramSize [in] × ³³µµ³µ³µ³³²²²²²´culèçÐ´»²²µuggµµµ 
 * @return ·Ø”´´´««ug¹¹µÖµÖ¸Ö*/
SSL * SSL_NewLink(
		SSL_CTX *SSLCtrl,
		int32_t ClientID,
		const uint8_t *SessionID,
		uint8_t SessionIDSize,
		const char **Hostname,
		uint16_t *MaxFragmentSize);

/**
 * @rrief OÉ¿¼¼SSLYSLYS³³¡£
 * @param SSLink [in] SSLÁ½½e¹¹¹¹¹ become.
 * @return 0¾³â€ uggâ ²â*/
int32_t SSL_ReHandshake(SSL *SSLLink);

/**
 * @brief · Øs»ð«ughush.
 * @param SSLink [in] SSLÁ´«ug¹¹µÖµÖ¸èè.
 * @return 0¾³â€ uggâ ²â*/
int32_t SSL_HandshakeStatus(const SSL *SSLLink);

/**
 * @BRIef É¾³Ýò »¸öSSLÁ¬½Ó½e¹ìÅ £ ¬ÔÚÉ¾³ÝÇ ° £ ¬ÈÇ¹ÛÃ» ÓÐ · ¢ ËÍ¹Ý ± ÕÍ¨Öª £ ¬ »E × Ô¶¯¶ÔÁ¬½Ó × ÅµÄ · þÎÑæ
 * @param SSLLINK SSLÁ¬½Ó½E¹ÌÅµÄµøÖ · Ö¸õË
 * @return îþ*/
void SSL_FreeLink(SSL *SSLLink);

/**
 * @brief ÔYèèèèèèóóó³³²»²ï´´ä´S´è´´øï³´óµµµµµµµµ
 * @param SSLink [in] SSLÁ´«ug¹¹µÖµÖ¸èè.
 * @param InData [out] ½èóµ³µ°µè¸è×è×ó »²²²²²²²²²²3²²²²²²²¼²²»¼.
 *
 * @return >0 ½ÀïÀïèÀ³³µµµµµ´¾â»º´²â çâ’´èÑugh«uggó*/
int32_t SSL_Read(SSL *SSLLink, uint8_t **InData);

/**
 * @brief ALEâ â µ¼µµ²»²¢²¢¢¢¢¢¢¢¢«¿ï«¿è××××óâ ¢â
 * @param SSLink [in] SSLÁ´«ug¹¹µÖµÖ¸èè.
 * @param OutData [in] ÐââCââöâââô´´´ï²µè¸èè
 * @param OutLen [in] ÐèÍ´´âââôè´ug´´cul³µ³µµµ³.
 * @return >0Wµµ²n³µóµ³´ó³*/
int32_t SSL_Write(SSL *SSLLink, const uint8_t *OutData, uint16_t OutLen);

/**
 * @brief µBERÉ¤²” orâ ughçµçµçµçµçughö is O¤¤²ugh´öli¶À²¶Àugâ‼²²¾ï«´ï´
 * @param SSLCtrl [in] SSL should ¿Éug¹¹µÖµÖ¸ÕÖ
 * @param Type [in] µWâ »½¾â‟futhâ §    
 * - SSL_OBJ_X509_CERT (not password) ¿Í»§¶ËµÄÖ¤Êé£¬ÓÃÓÚ·þÎñÆ÷ÑéÖ¤¿Í»§¶Ë£¬´ó²¿· Ð²²”Rcult´«ug¼C°µµµ²ÐÐÑ»²è»±¿Òç
 * - SSL_OBJ_X509_CECT (not required password) ug µâ‟ï¤Yâ µ¤²±±±ug
 * - SSL_OBJ_KEY (AES128/AES256 Surded PEM)
 * - SSL_OBJ_PKCS8 (RC4-128 snaps date supped)
 * - SSL_OBJ_PKCS12 (RC4-128 snare data supped).
 * @param Data [in] µWYµ²»è²¸¤B16«C16«C16²²²²²¼²¤op MOWIC.
 * @param Len [in] â ºµ³¶¶
 * @param Passwords [in] Èce«CµWÃµèÂäèÂ´â′´´´â »NULLLLL
 * @return 0³É¹´´*/
int32_t SSL_LoadKey(SSL_CTX *SSLCtrl, int32_t Type, const uint8_t *Data, int32_t Len, const int8_t *Password);

/**
 * @BRIEF ÑÉÖ¤ · Þîñæ ÷ Ö¤ÊÉ £ ¬ ± Èô ° æ¸Ä½ØÔÚÓÚ¿ÉÒÔÊ¹ÓÃ´Î¼¶¸ÙÖ¤ÊÉÀ´ÑÉÖ¤ £ ¬ò »° Ãä¯ààæ ÷ ä ÚÖÃÁË´óÁ¿´Î¼¶¸ùÖ¤Êé£¬¿ÉÒÔµ¼³öÊ¹ÓÃ£¬ÔÚ²âÊÔÊµ¼ÊÍøÒ³Ê±£¬ÊÊºÏÊ¹ÓÃ±¾º¯Êý
 * @param SSLLINK SSLÁ¬½Ó½E¹ÌÅµÄµøÖ · Ö¸õË
 * @return 0³é¹¦ æäëûê§ ° ü*/
int32_t SSL_VerifyCert(SSL *SSLLink);
#endif
