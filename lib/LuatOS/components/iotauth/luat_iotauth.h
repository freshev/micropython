#ifndef LUAT_LCD
#define LUAT_LCD

#define CLIENT_ID_LEN 192
#define USER_NAME_LEN 192
#define PASSWORD_LEN 256

typedef struct iotauth_ctx
{
    char client_id[CLIENT_ID_LEN];
    char user_name[USER_NAME_LEN];
    char password[PASSWORD_LEN];
}iotauth_ctx_t;

typedef struct iotauth_onenet {
    const char* product_id;
    const char* device_name;
    const char* device_secret;
    long long cur_timestamp;
    const char* method;
    const char* version;
    const char* res;
}iotauth_onenet_t;

/**
 *@brief Alibaba Cloud obtains triplet information
 *@param ctx iotauth_ctx_t Triplet information obtained
 *@param product_key product key
 *@param device_name device name
 *@param device_secret device secret key
 *@param cur_timestamp timestamp
 *@param method encryption method
 *@param is_tls whether tls
 *@return success is 0, other values   fail*/
int luat_aliyun_token(iotauth_ctx_t* ctx,const char* product_key,const char* device_name,const char* device_secret,long long cur_timestamp,const char* method,uint8_t is_tls);
/**
 *@brief onenet gets triplet information
 *@param ctx iotauth_ctx_t Triplet information obtained
 *@param onenet onenet incoming structure information
 *@return success is 0, other values   fail*/
int luat_onenet_token(iotauth_ctx_t* ctx,const iotauth_onenet_t* onenet);
/**
 *@brief Huawei IoT obtains triplet information
 *@param ctx iotauth_ctx_t Triplet information obtained
 *@param device_id device id
 *@param device_secret device secret key
 *@param cur_timestamp timestamp
 *@param ins_timestamp whether to verify the timestamp
 *@return success is 0, other values   fail*/
int luat_iotda_token(iotauth_ctx_t* ctx,const char* device_id,const char* device_secret,long long cur_timestamp,int ins_timestamp);
/**
 *@brief Tencent obtains triplet information
 *@param ctx iotauth_ctx_t Triplet information obtained
 *@param product_id product id
 *@param device_name device name
 *@param device_secret device secret key
 *@param cur_timestamp timestamp
 *@param method encryption method
 *@param sdk_appid appid
 *@return success is 0, other values   fail*/
int luat_qcloud_token(iotauth_ctx_t* ctx,const char* product_id,const char* device_name,const char* device_secret,long long cur_timestamp,const char* method,const char* sdk_appid);
/**
 *@brief Tuya gets triplet information
 *@param ctx iotauth_ctx_t Triplet information obtained
 *@param device_id device id
 *@param device_secret device secret key
 *@param cur_timestamp timestamp
 *@return success is 0, other values   fail*/
int luat_tuya_token(iotauth_ctx_t* ctx,const char* device_id,const char* device_secret,long long cur_timestamp);
/**
 *@brief Tencent obtains triplet information
 *@param ctx iotauth_ctx_t Triplet information obtained
 *@param iot_core_id iot_core_idid
 *@param device_key device key
 *@param device_secret device secret key
 *@param method encryption method
 *@param cur_timestamp timestamp
 *@return success is 0, other values   fail*/
int luat_baidu_token(iotauth_ctx_t* ctx,const char* iot_core_id,const char* device_key,const char* device_secret,const char* method,long long cur_timestamp);

#endif



















