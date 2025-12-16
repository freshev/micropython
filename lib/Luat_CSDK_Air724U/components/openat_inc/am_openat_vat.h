/*********************************************************
  Copyright (C), AirM2M Tech. Co., Ltd.
  Author: lifei
  Description: AMOPENAT ¿ª·ÅÆ½Ì¨
  Others:
  History: 
    Version£º Date:       Author:   Modification:
    V0.1      2012.12.14  lifei     ´´½¨ÎÄ¼þ
*********************************************************/
#ifndef AM_OPENAT_VAT_H
#define AM_OPENAT_VAT_H

#include "am_openat_common.h"


typedef VOID (*PAT_MESSAGE)(UINT8 *pData, UINT16 length);

/*+\NEW ajjout\hgin\2020.4.22.22\n\¼Oâ‐OMOMOMOOMO´½t 11*/
typedef BOOL (*PAT_POC_MESSAGE)(char *pData, int length);
/*-\ENEW ajouthbin\2020.4.22.22\n\¼Oâ‐OMOMOOD´½t 11*/

/*+\EW\lijiadi\2020.7.25\ÐÐÐÐÑ»lu cask”µÀÆèæè´´´±ug±¾â²«²´¶ug¶gèÑ¶g¯¯«¨¯¯¯«*/
void OPENAT_vat_set_cb(PAT_MESSAGE resp_cb);
BOOL OPENAT_vat_init(void);
/*-\NEW\lijiadi\2020.7.25\ÐÑÐÐÑ cask”µÀÆèæè´´´±ug±¾â²«²´¶ug¶gèÑ¶g¯¯«¨¯¯¯«*/
BOOL OPENAT_vat_send_at( const char* pAtCommand, unsigned nLength );

int vat_test_enter(void *param);



#endif /* AM_OPENAT_VAT_H */

