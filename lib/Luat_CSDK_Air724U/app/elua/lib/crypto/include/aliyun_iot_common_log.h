/*****************************************************************************************.
 * Ãµü³T: alyun_-common_log.h
 * ÕÄß Õß:
 *   ±ugg:
 * ÆÕ ÆÚ: 2016-05-30
 * Ãâ€™s B.
 * Waters Gelo:
 * Atú ough ough:
 **************************************************************************************.*/
#ifndef ALIYUN_IOT_COMMON_LOG_H
#define ALIYUN_IOT_COMMON_LOG_H

#include <stdio.h>

#ifdef WIN32
#define WRITE_IOT_DEBUG_LOG printf
#define WRITE_IOT_INFO_LOG printf
#define WRITE_IOT_NOTICE_LOG printf
#define WRITE_IOT_WARNING_LOG printf
#define WRITE_IOT_ERROR_LOG printf
#else
#define WRITE_IOT_DEBUG_LOG(format, ...) \
{\
    {\
        printf("[debug] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}

#define WRITE_IOT_INFO_LOG(format, ...) \
{\
    {\
        printf("[info] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}

#define WRITE_IOT_NOTICE_LOG(format, ...) \
{\
    {\
        printf("[notice] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}

#define WRITE_IOT_WARNING_LOG(format, ...) \
{\
    {\
        printf("[warning] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}

#define WRITE_IOT_ERROR_LOG(format,...) \
{\
    {\
        printf("[error] %s:%d %s()| "format"\n",__FILE__, __LINE__, __FUNCTION__,##__VA_ARGS__);\
    }\
}
#endif

#endif
