/*********************************************************
  Copyright (C), AirM2M Tech. Co., Ltd.
  Author: lifei
  Description: AMOPENAT ¿ª·ÅÆ½Ì¨
  Others:
  History: 
    Version£º Date:       Author:   Modification:
    V0.1      2012.12.14  lifei     ´´½¨ÎÄ¼þ
*********************************************************/
//#ifdef AM_OPENAT_CRYPTO_FUNC_SUPPORT
#ifndef AM_OPENAT_CRYPTO_H
#define AM_OPENAT_CRYPTO_H

#include "am_openat_common.h"

typedef struct T_AMOPENAT_SHA256_CTX_TAG
{
    UINT32 total[2];
    UINT32 state[8];
    UINT8 buffer[64];
} AMOPENAT_SHA256_CTX;

//#ifdef AM_POLARSSL_RSA_FUNC_SUPPORT
typedef enum E_AMOPENAT_RSA_KEY_MODE_TAG
{
    OPENAT_RSA_PUBLIC_KEY,        /*«« O¿*/
    OPENAT_RSA_PRIVATE_KEY,     /*˽Կ*/
    OPENAT_RSA_KEY_MAX
}E_AMOPENAT_RSA_KEY_MODE;

typedef enum E_AMOPENAT_RSA_CRYPT_MODE_TAG
{
    OPENAT_RSA_PUBLIC_KEY_CRYPT,        /*«« «¼ ¼¿ »ü ò ò ò*/
    OPENAT_RSA_PRIVATE_KEY_CRYPT,     /*Ë½Ô¿¼ÓÃÜ»òÕß½âÃÜ*/
    OPENAT_RSA_CRYPT_MODE_MAX
}E_AMOPENAT_RSA_CRYPT_MODE;

//#endif


#endif /* AM_OPENAT_CRYPTO_H */
//#endif

