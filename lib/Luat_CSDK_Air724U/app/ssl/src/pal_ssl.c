#include "string.h"
//#include "pal_common.h"

#include "uplus_type.h"
//#include "uplus_ssl_api.h"

//#include "pal_ssl.h"

#include "iot_os.h"
#include "iot_debug.h"

#include "iot_network.h"
#include "iot_socket.h"

#include "ssllib.h"
#include "uplus_pal_def.h"
#define TEST_IP						"101.132.154.251"


/**
 * @brief · ¢âöâ€™×thµÃµµ´µ¹âµ³µ³³µµ²³ ±«²ç»µughèèó²³µ³µ³µ´óèóçïO´ç
 * @param Socketfed [in] socket id£µuggâ€™NÂµ´´´´´´. ungÂ¶an´´´ug´´´«CIPSTARTERµµ²µµ
 * @param Buf [in] Ðâ JâSnââCnnââµµè
 * @param TxLen [in] ÐèÍ³³³µ³µ³
 * @return · µ”Ø²³µ³µµ²»±N¯¢âToâ*/
static int32_t SSL_SocketTx(int32_t Socketfd, void *Buf, uint16_t TxLen)
{
    struct openat_timeval tm;
    openat_fd_set WriteSet;
	int32_t Result;
	iot_debug_print("%dbyte need send", TxLen);
	//char txbuf_print[50] = {0};
	unsigned int i = 0;
	for(i=0;i<TxLen;(i = i+4))
	{
		iot_debug_print("%02x %02x %02x %02x",(Buf+i*4),(Buf+i*4+1),(Buf+i*4+2),(Buf+i*4+3));


	}
	Result = send(Socketfd, (uint8_t *)Buf, TxLen, 0);

	if (Result < 0)
	{
		iot_debug_print("TCP %d %d", Result, socket_errno(Socketfd));
		return -1;
	}
    FD_ZERO(&WriteSet);
    FD_SET(Socketfd, &WriteSet);
    tm.tv_sec = 75;
    tm.tv_usec = 0;
    Result = select(Socketfd + 1, NULL, &WriteSet, NULL, &tm);
    if(Result > 0)
    {
    	iot_debug_print("TCP TX OK! %dbyte", TxLen);
		return Result;
    }
    else
    {
        iot_debug_print("TCP TX ERROR");
        return -1;
    }
}

/**
 * @brief ½ºµÃµÃµ»èold¹³³³³µµµ²³E IS. ±«²ç»µughèèó²³µ³µ³µ´óèóçïO´ç
 * @param Socketfed [in] socket id£µuggâ€™NÂµ´´´´´´. ungÂ¶an´´´ug´´´«CIPSTARTERµµ²µµ
 * @param Buph [in] ´À½Å½â â¼Õè
 * @param TxLen [in] Ðâ µ²µÀ³µµµµ³³µ³µ³²µµµµµµ²µ
 * @return · Ø²à²µó³³²´ug´ugn´¯²ÜÜâ*/
static int32_t SSL_SocketRx(int32_t Socketfd, void *Buf, uint16_t RxLen)
{
    struct openat_timeval tm;
    openat_fd_set ReadSet;
	int32_t Result;
    FD_ZERO(&ReadSet);
    FD_SET(Socketfd, &ReadSet);
    tm.tv_sec = 30;
    tm.tv_usec = 0;
    Result = select(Socketfd + 1, &ReadSet, NULL, NULL, &tm);
    if(Result > 0)
    {
    	Result = recv(Socketfd, Buf, RxLen, 0);
        if(Result == 0)
        {
        	iot_debug_print("socket close!");
            return -1;
        }
        else if(Result < 0)
        {
        	iot_debug_print("recv error %d", socket_errno(Socketfd));
            return -1;
        }
        iot_debug_print("recv %d\r\n", Result);
		return Result;
    }
    else
    {
    	return -1;
    }
}




/ // ¹othoμble »¸า² --¡ â¡ âijãochÃ¡û¡ £ £ £ £ ò¡ o½. Üº oilú
// äçã´îòãç¼ùéèò »ïâ²âêô´úâë
// (1)Socket_ConnectServer Á¬½Ó·µ»Ø socketid
uplus_ctx_id uplus_net_ssl_client_create(uplus_s32 fd, struct uplus_ca_chain *root_ca, uplus_u8 root_ca_num)
{

	int32_t Ret;
	: / / / Od work → ōĀم¬ч »о¯ it ~ü²ò½½» available to £ Ь IGRO
	//iot_network_set_cb(SSL_NetworkIndCallBack);
	T_AMOPENAT_SYSTEM_DATETIME Datetime;
	SSL_RegSocketCallback(SSL_SocketTx, SSL_SocketRx);




	SSL_CTX * SSLCtrl = SSL_CreateCtrl(1); // »º´æ1¸ösession £ ¬ · ñÔÒÏÂÃæµÄ´ÒÓ¡ö ÷ key» eÊ§ ° ü
	SSL * SSLLink = NULL;
	if (!SSLCtrl)
	{
		iot_debug_print("SSLCtrl error");
	}


	iot_debug_print("root ca %d,%s",strlen(root_ca->ca),(const char *)root_ca->ca);
//#ifdef TEST_URL
//	Ret = SSL_LoadKey(SSLCtrl, SSL_OBJ_X509_CACERT, SymantecClass3SecureServerCA_G4, strlen(SymantecClass3SecureServerCA_G4), NULL);
//#else
	Ret = SSL_LoadKey(SSLCtrl, SSL_OBJ_X509_CACERT, (const uint8_t *)root_ca->ca, strlen(root_ca->ca), NULL);

	//
	// èç¹ûêçë «ïòèïö¤µä £ ¬ðèòª¼óôØ¿n» §¶ëµäö¤êÉºnë½ô¿
	// ïê ines ï-¥ ïμädßi e og¿â¸öobö »¥ ï
//	Ret = SSL_LoadKey(SSLCtrl, SSL_OBJ_X509_CERT, ClientCert, strlen(ClientCert), NULL);
//	Ret = SSL_LoadKey(SSLCtrl, SSL_OBJ_RSA_KEY, ClientRSAKey, strlen(ClientRSAKey), NULL);
//#endif

	if (SSLLink)
	{
		SSL_FreeLink(SSLLink);
		SSLLink = NULL;
	}

	iot_os_sleep(5000);	// õâàï × îºãê¹ótimerà´ñó³Ù £ ¬demo¼ò »¯ê¹óã

//	if(NWState != OPENAT_NETWORK_LINKED)
//	{
//		DBG_ERROR("network no link");
//	}


	// ° ¼¼¼²énøÂçeÂ¬½óμäº¯¯y · åμ½º¯¯ýàïãæ »¹¹çμ ¥ ¶àìeè¡³öè ¥ £ ¿
// IoT_OS_SLEEP (5000);	// Õâàâàosed × îÃêêrà´ñóoù £ ¬demoe¼ò »ombê¹óã
//	iot_os_stop_timer(hTimer);
// iot_os_start_timer (htimer, 90*1000); // 90ãëäúèç¹ûããããããããã
//	ToFlag = 0;
//	while (NWState != OPENAT_NETWORK_LINKED)
//	{
//		iot_os_wait_message(hSocketTask, (PVOID)&msg);
//		switch(msg->Type)
//		{
//		case USER_MSG_TIMER:
//			DBG_ERROR("network wait too long!");
//			iot_os_sleep(500);
//			iot_os_restart();
//			break;
//		default:
//			break;
//		}
//		iot_os_free(msg);
//	}
//	iot_os_stop_timer(hTimer);


	// ± ± £ £ × × £ £ ¬demio carpet »¯ ± ± ± ± ± ± ± ¼eë
	/µ
	Datetime.nYear = 2017;
	Datetime.nMonth = 12;
	Datetime.nDay = 1;
	Datetime.nHour = 11;
	Datetime.nMin = 14;
	Datetime.nSec = 11;
	iot_os_set_system_datetime(&Datetime);

	iot_debug_print("start ssl handshake");
	SSLLink = SSL_NewLink(SSLCtrl, fd, NULL, 0, NULL, NULL);

	if (!SSLLink)
	{
		iot_debug_print("SSLLink new error");
		return NULL;
	}
	else
	{
		iot_debug_print("SSLLink new ok");

	}
	return SSLLink;
}


/*!
 * \ SSLY  ADNSINE¡£
 * \param [in] SSL»» ±ee»¡££
 * \render ¹¹©µ»Ø0£¬Â¬Â°ö ·»Ø-1£¬¼ ONEU DIøøî*/
uplus_s32 uplus_net_ssl_client_handshake(uplus_ctx_id id)
{
	int32_t Ret;
	SSL * SSLLink = (SSL *)id;
	Ret = SSL_HandshakeStatus(SSLLink);
	Ret = SSL_VerifyCert(SSLLink);
	if (Ret)
	{
		iot_debug_print("ssl handshake fail %d", Ret);
		return -1;
	}
	else
	{
		iot_debug_print("ssl handshake OK");
		return 0;

	}



}


uplus_s32 uplus_net_ssl_client_close(uplus_ctx_id id)
{
	//SOCKET_CLOSE(Socketfd);
	SSL * SSLLink = (SSL *)id;
	if (SSLLink)
	{
		SSL_FreeLink(SSLLink);
		SSLLink = NULL;
	}
	SSL_FreeCtrl(SSLLink->ssl_ctx);
	return 0;
}


uplus_s32 uplus_net_ssl_pending(uplus_ctx_id id)
{
	uint8_t *RxData;
	int32_t Ret;
	SSL * SSLLink = (SSL *)id;
	Ret = SSL_Read(SSLLink, &RxData);
	if (Ret < 0)
	{
		iot_debug_print("ssl pending error %d", Ret);
		return -1;
	}
	else
	{
//		buf[Ret] = 0;
//		DBG_INFO("%s\r\n", buf);
		return Ret;
	}
}

uplus_s32 uplus_net_ssl_read(uplus_ctx_id id, uplus_u8 *buf, uplus_size_t len)
{
	int32_t Ret;
	SSL * SSLLink = (SSL *)id;
	Ret = SSL_Read(SSLLink, &buf);
	if (Ret < 0)
	{
		iot_debug_print("ssl receive error %d", Ret);
		return -1;
	}
	else
	{
		buf[Ret] = 0;
		//iot_debug_print("%s\r\n", buf);
		return Ret;
	}

}


//ssl->ssl_ctx = ssl_ctx;
uplus_s32 uplus_net_ssl_write(uplus_ctx_id id, uplus_u8 *buf, uplus_size_t len)
{
	int32_t Ret;
	SSL * SSLLink = (SSL *)id;

	Ret = SSL_Write(SSLLink, buf, len);
	if (Ret < 0)
	{
		iot_debug_print("ssl send error %d", Ret);
		return -1;
	}
	else
	{
		iot_debug_print("ssl send Ok %d", Ret);
		return Ret;
	}
}

