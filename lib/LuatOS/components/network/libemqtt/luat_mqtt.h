#ifndef LUAT_MQTT_H
#define LUAT_MQTT_H
/**
 * @defgroup luatos_MQTT MQTT related interface
 * @{*/
#define MQTT_MSG_RELEASE 		0	/**< mqtt callback message before releasing resources*/
#define MQTT_MSG_TCP_TX_DONE 	1	/**< mqtt tcp sending completed*/
#define MQTT_MSG_TIMER_PING 	2	/**< mqtt callback message before ping*/
#define MQTT_MSG_RECONNECT  	3	/**< mqtt callback message before reconnection*/
#define MQTT_MSG_CLOSE 			4	/**< mqtt closes callback messages (will not reconnect)*/

#define MQTT_ERROR_STATE_SOCKET		-1
#define MQTT_ERROR_STATE_DISCONNECT	-2

#ifndef MQTT_RECV_BUF_LEN_MAX
#if defined(CHIP_EC618) || defined(CHIP_EC718)|| defined(CHIP_EC716)
#define MQTT_RECV_BUF_LEN_MAX (32*1024) ///< MQTT receiving BUFF size
#else
#define MQTT_RECV_BUF_LEN_MAX 4096 ///< MQTT receiving BUFF size
#endif
#endif


/**
 * @brief mqtt status*/
typedef enum {
	MQTT_STATE_DISCONNECT 			,	/**< mqtt disconnect*/
	MQTT_STATE_SCONNECT 			,	/**<mqtt socket connecting*/
	MQTT_STATE_MQTT					,	/**< mqtt socket is connected mqtt is connecting*/
	MQTT_STATE_READY 					/**< mqtt mqtt is connected*/
}LUAT_MQTT_STATE_E;

/**
 * @brief Set the configuration parameters of the MQTT client*/
typedef struct{
	mqtt_broker_handle_t broker;/**< mqtt broker*/
	network_ctrl_t *netc;		/**< mqtt netc*/
	luat_ip_addr_t ip_addr;		/**<mqtt ip*/
	char host[192]; 			/**<mqtt host*/
	uint32_t buffer_offset; 	/**< is used to identify how much data mqtt_packet_buffer currently has*/
	uint32_t rxbuff_size; 		/**<length of mqtt_packet_buffer*/
	uint8_t *mqtt_packet_buffer;/**<Receive BUFF*/
	void* mqtt_cb;			/**< mqtt callback function*/
	int8_t error_state;    		/**< mqtt error status*/
	uint16_t remote_port; 		/**<Remote port number*/
	uint32_t keepalive;   		/**<Heartbeat duration unit s*/
	uint8_t adapter_index; 		/**<Adapter index number, seems to be of no use*/
	LUAT_MQTT_STATE_E mqtt_state;    		/**< mqtt status*/
	uint8_t reconnect;    		/**< mqtt whether to reconnect*/
	uint32_t reconnect_time;    /**< mqtt reconnection time unit ms*/
	void* reconnect_timer;		/**< mqtt reconnect timer*/
	void* ping_timer;			/**< mqtt_ping timer*/
	int mqtt_ref;				/**< forces a reference to itself to avoid GC*/
	void* userdata;				/**< userdata */
}luat_mqtt_ctrl_t;

typedef struct{
	uint16_t topic_len;
	uint16_t dummy;
    uint32_t payload_len;
	uint8_t data[];
}luat_mqtt_msg_t;

/**
 * @brief Set the MQTT server server information and encryption information*/
typedef struct luat_mqtt_connopts
{
    const char* host;/**< Server HOST*/
    uint16_t port;/**<server port*/
    uint8_t is_tls;/**< Whether to use tls encryption*/
	uint8_t is_ipv6;
	uint8_t verify;
    const char* server_cert;
    size_t server_cert_len;
    const char* client_cert;
    size_t client_cert_len;
    const char* client_key;
    size_t client_key_len;
    const char* client_password;
    size_t client_password_len;
}luat_mqtt_connopts_t;

typedef void (*luat_mqtt_cb_t)(luat_mqtt_ctrl_t *luat_mqtt_ctrl, uint16_t event);

/**
 *@brief initiates MQTT connection
 *@param mqtt_ctrl luatos_mqtt object instance
 *@return success is 0, other values   fail*/
int luat_mqtt_connect(luat_mqtt_ctrl_t *mqtt_ctrl);
// static int luat_mqtt_msg_cb(luat_mqtt_ctrl_t *mqtt_ctrl);
// int l_mqtt_callback(lua_State *L, void* ptr);
/**
 *@brief MQTT internal callback (users do not need to care)
 *@param ctrl luatos_mqtt object instance
 *@param arg1 parameter 1
 *@param arg2 parameter 2
 *@return success is 0, other values   fail*/
int l_luat_mqtt_msg_cb(luat_mqtt_ctrl_t * ctrl, int arg1, int arg2);
/**
 *@brief MQTT message parsing internal callback (users do not need to care)
 *@param data data
 *@param param parameter
 *@return success is 0, other values   fail*/
int32_t luat_mqtt_callback(void *data, void *param);
/**
 *@brief MQTT timer internal callback (users do not need to care)*/
LUAT_RT_RET_TYPE luat_mqtt_timer_callback(LUAT_RT_CB_PARAM);
// int luat_mqtt_read_packet(luat_mqtt_ctrl_t *mqtt_ctrl);
/**
 *@brief MQTT message sending (users do not need to care)
 *@param socket_info socket
 *@param buf data
 *@param count data size
 *@return success is 0, other values   fail*/
int luat_mqtt_send_packet(void* socket_info, const void* buf, unsigned int count);

/**
 *@brief Close the MQTT connection. If automatic reconnection is set, it will automatically reconnect.
 *@param mqtt_ctrl luatos_mqtt object instance
 *@return success is 0, other values   fail*/
void luat_mqtt_close_socket(luat_mqtt_ctrl_t *mqtt_ctrl);

/**
 *@brief Get MQTT connection status
 *@param mqtt_ctrl luatos_mqtt object instance
 *@return LUAT_MQTT_STATE_E*/
LUAT_MQTT_STATE_E luat_mqtt_state_get(luat_mqtt_ctrl_t *mqtt_ctrl);

/**
 *@brief Releases MQTT resources. The luatos_mqtt object is unavailable after release.
 *@param mqtt_ctrl luatos_mqtt object instance
 *@return success is 0, other values   fail*/
void luat_mqtt_release_socket(luat_mqtt_ctrl_t *mqtt_ctrl);

/**
 *@brief initialize luatos_mqtt (initialize MQTT)
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param adapter_index network card type (unique value NW_ADAPTER_INDEX_LWIP_GPRS)
 *@return success is 0, other values   fail*/
int luat_mqtt_init(luat_mqtt_ctrl_t *mqtt_ctrl, int adapter_index);

/**
 *@brief Set MQTT server information and encryption information function
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param opts structure MQTT server information, encryption information function
 *@return success is 0, other values   fail*/
int luat_mqtt_set_connopts(luat_mqtt_ctrl_t *mqtt_ctrl, luat_mqtt_connopts_t *opts);

/**
 *@brief Set MQTT server triplet information
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param clientid clientid
 *@param username username
 *@param password password
 *@return success is 0, other values   fail*/
int luat_mqtt_set_triad(luat_mqtt_ctrl_t *mqtt_ctrl, const char* clientid, const char* username, const char* password);

/**
 *@brief Set the MQTT server receiving buff size
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param rxbuff_size receives buff size
 *@return success is 0, other values   fail*/
int luat_mqtt_set_rxbuff_size(luat_mqtt_ctrl_t *mqtt_ctrl, uint32_t rxbuff_size);

/**
 *@brief Set MQTT server heartbeat duration
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param keepalive keepalive unit s
 *@return success is 0, other values   fail*/
int luat_mqtt_set_keepalive(luat_mqtt_ctrl_t *mqtt_ctrl, uint32_t keepalive);

/**
 *@brief Set whether the MQTT server will automatically reconnect
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param auto_connect whether to automatically reconnect
 *@param reconnect_time automatic reconnection time unit ms
 *@return success is 0, other values   fail*/
int luat_mqtt_set_auto_connect(luat_mqtt_ctrl_t *mqtt_ctrl, uint8_t auto_connect,uint32_t reconnect_time);

/**
 *@brief Manually initiate reconnection
 *@param mqtt_ctrl luatos_mqtt object instance
 *@return success is 0, other values   fail*/
int luat_mqtt_reconnect(luat_mqtt_ctrl_t *mqtt_ctrl);

/**
 *@brief Send ping packet
 *@param mqtt_ctrl luatos_mqtt object instance
 *@return success is 0, other values   fail*/
int luat_mqtt_ping(luat_mqtt_ctrl_t *mqtt_ctrl);

/**
 *@brief Set will message
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param topic topic of will message
 *@param payload payload of will message
 *@param payload_len The length of the will message payload
 *@param qos qos of will message
 *@param retain retain of will message
 *@return success is 0, other values   fail*/
int luat_mqtt_set_will(luat_mqtt_ctrl_t *mqtt_ctrl, const char* topic, const char* payload, size_t payload_len, uint8_t qos, size_t retain);
/**
 *@brief Set MQTT event callback function
 *@param mqtt_ctrl luatos_mqtt object instance
 *@param mqtt_cb callback function
 *@return success is 0, other values   fail*/
int luat_mqtt_set_cb(luat_mqtt_ctrl_t *mqtt_ctrl, luat_mqtt_cb_t mqtt_cb);
/** @}*/
#endif
