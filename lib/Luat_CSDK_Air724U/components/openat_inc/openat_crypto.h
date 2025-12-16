#ifndef __OPENAT_CRYPTO_H__
#define __OPENAT_CRYPTO_H__

/*+\ New \ Wangyuan \ 2020.04.14 \ bug_1445: RSAAAKOóâãü½ú¿ú¿ú - ¿éóã*/
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

int openat_rsa_encrypt(int nKeyMode, unsigned char *pKeyBuf, int nKeyLen,
                                             unsigned char *pPswd, int nPswdLen,
      								  int nEncryptMode,
      								  unsigned char *pInBuf, int nInLen,
      								  unsigned char *pOutBuf);


int openat_rsa_decrypt(int nKeyMode, unsigned char *pKeyBuf, int nKeyLen,
                                             unsigned char *pPswd, int nPswdLen,
      								  int nDecryptMode,
      								  unsigned char *pInBuf, int nInLen,
      								  unsigned char *pOutBuf, int *pOutLen, int nOutBufSize);


int openat_rsa_sha256_sign(int nKeyMode, unsigned char *pKeyBuf, int nKeyLen,
                                             unsigned char *pPswd, int nPswdLen,
      								  int nEncryptMode,
      								  unsigned char *pInBuf, int nInLen,
      								  unsigned char *pOutBuf);


int openat_rsa_sha256_verify(int nKeyMode, unsigned char *pKeyBuf, int nKeyLen,
                                                     unsigned char *pPswd, int nPswdLen,
      								  int nEncryptMode,
      								  unsigned char *pInBuf, int nInLen,
      								  unsigned char *pInPlainBuf, int nInPlainLen);

int openat_rsa_encrypt_ne(int nKeyMode, unsigned char *pKeyNBuf, int nKeyNLen,
								unsigned char *pKeyEBuf, int nKeyELen,
                                             unsigned char *pPswd, int nPswdLen,
      								  int nEncryptMode,
      								  unsigned char *pInBuf, int nInLen,
      								  unsigned char *pOutBuf);

int openat_rsa_decrypt_ne(int nKeyMode, unsigned char *pKeyNBuf, int nKeyNLen,
								unsigned char *pKeyEBuf, int nKeyELen,
                                             unsigned char *pPswd, int nPswdLen,
      								  int nDecryptMode,
      								  unsigned char *pInBuf, int nInLen,
      								  unsigned char *pOutBuf, int *pOutLen, int nOutBufSize);


#endif
/*-\ New \ Wangyuan \ 2020.04.14 \ bug_1445: RSAAAKó½â € ½úú¿ú¿ ¿éóã*/

