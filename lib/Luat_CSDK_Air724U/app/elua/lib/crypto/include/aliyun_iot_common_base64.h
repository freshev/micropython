/*****************************************************************************************.
 * Ãµü³T: alyun_common_base 64.h
 * ÕÄß Õß:
 *   ±ugg:
 * ÆÕ ÆÚ: 2016-05-30
 * Ãâ€™s B.
 * Waters Gelo:
 * Atú ough ough:
 **************************************************************************************.*/

#ifndef ALIYUN_IOT_COMMON_BASE64_H
#define ALIYUN_IOT_COMMON_BASE64_H

#include "aliyun_iot_common_datatype.h"
#include "aliyun_iot_common_error.h"

IOT_RETURN_CODES_E aliyun_iot_common_base64encode(const UINT8 *data, UINT32 inputLength, UINT32 outputLenMax,UINT8 *encodedData,UINT32 *outputLength);
IOT_RETURN_CODES_E aliyun_iot_common_base64decode(const UINT8 *data, UINT32 inputLength, UINT32 outputLenMax,UINT8 *decodedData,UINT32 *outputLength);

#endif
