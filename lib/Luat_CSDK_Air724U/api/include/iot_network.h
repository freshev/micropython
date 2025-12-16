#ifndef __IOT_NETWORK_H__
#define __IOT_NETWORK_H__

#include "am_openat.h"

// #define OPENAT_NETWORK_ISP_LENGTH (64)
// #define OPENAT_NETWORK_IMSI_LENGHT (64)
#define OPENAT_NETWORK_APN_LENGTH (64)
#define OPENAT_NETWORK_PASSWORD_LENGTH	(64)
#define OPENAT_NETWORK_USER_NAME_LENGTH (64)


typedef enum
{
    /*!*/
	OPENAT_NETWORK_DISCONNECT            		= 0x00,
    /*!*/
	OPENAT_NETWORK_READY,
	/*!< Á´Â·ÕýÔÚ¼¤»î*/
	OPENAT_NETWORK_LINKING,
    /*!< Á´Â·ÒÑ¾­¼¤»î PDPÒÑ¾­¼¤»î£¬¿ÉÒÔÍ¨¹ýsocket½Ó¿Ú½¨Á¢Êý¾ÝÁ¬½Ó*/
	OPENAT_NETWORK_LINKED,
	/*! <Á´â õýôúè ¥ ¼¤ »*/
	OPENAT_NETWORK_GOING_DOWN,
}E_OPENAT_NETWORK_STATE;

typedef VOID(*F_OPENAT_NETWORK_IND_CB)(E_OPENAT_NETWORK_STATE state);



typedef enum
{
    /*!*/
	OPENAT_NETWORK_UNKNOWN=0,
    /*!< sim¿¨×´Ì¬ÓÐÐ§*/
	OPENAT_NETWORK_TRUE,
    /*! <Weeks'eë »loot ipothers ½ ½āac*/
	OPENAT_NETWORK_FALSE=255,
}E_OPENAT_SIM_STATE;

typedef struct
{
	/*!< ÍøÂç×´Ì¬*/
	E_OPENAT_NETWORK_STATE state;
	/*! <Íøâçðåºå £ º0-31 (Öµô½´Ó £ ¬ðåºåô½ºÃ)*/
	UINT8 csq;
	/*!< SIM¿¨×´Ì¬*/
	E_OPENAT_SIM_STATE  simpresent;
}T_OPENAT_NETWORK_STATUS;

typedef struct
{
	char apn[OPENAT_NETWORK_APN_LENGTH];
	char username[OPENAT_NETWORK_USER_NAME_LENGTH];
	char password[OPENAT_NETWORK_PASSWORD_LENGTH];
}T_OPENAT_NETWORK_CONNECT;




/**
 * @defgroup iot_sdk_network ÍøÂç½Ó¿Ú
 * @{*/
/** »Ñè¡nøÂç ×'ì¬
* @ param Status: · μ »ønøâç ×'ì¬
* @ Return True: ³³¹ |
            Flay: ê§ ° ü            
**/                                
BOOL iot_network_get_status (
                            T_OPENAT_NETWORK_STATUS* status
                            );
/**Éèöãnøâç ´ ´ì¬ »Øµ ÷ ° º thousand
*@param INDCB: »Øµ ÷ º thousand
*@Return True: ³é¹¦
            Flase: § ° ü
**/                            
BOOL iot_network_set_cb(F_OPENAT_NETWORK_IND_CB indCb);
/**½¨e ¢ Øâçe¬ £ ¬ê µ¼êîªPDPmptoke
*@param connectparam: Øâçe¬~ £ £ ¬ðèòªéöãapn £ ¬username £ ¬passwrdðåï ¢
*@Return True: ³é¹¦
            Flase: § ° ü
@Note Ção °rseîªòì² ° £ ¬ ¬ µ »µ» Øº ° ± ± ± Nnøâçe¬ ° £ £ £ ¬indcb »enªéï² À € · ñ³éÁ
           ´´~ ½ ± ± Øðëòª ¢ ¢ Øâçe®
           ½ ¢ ¢ ° ° ° ´ ´ì¬ðªîªOopenat_network_ready × ´ì¬ ¬ ¬ · ñôò »Ee ° ^ ° ü
**/                          
BOOL iot_network_connect(T_OPENAT_NETWORK_CONNECT* connectParam);
/**®´øøäøÀºµµ¼¼¼µ¼µµ¤¤”
*@param flymode: I’m â₱§”Ö³´³BWHY«FLY
*@return TRUE: ³É¹´
            FLASE: SE§°Ü
@note ¯¯¯´à²²¯¯²¯´µ³³³²´ú´øøøÂøÂøÂ »ug´´´´´®´¯ugdCb” andèè¨è‟«²¦
           Á´¶¶¶ led»³óøÂóóóóóóóóóóóðºµµ´SWORK_READY×´àâ
           ugCern ´´´²²²²²²²²²²´culseµ.
**/                                        
BOOL iot_network_disconnect(BOOL flymode);

/** @}*/

#endif

