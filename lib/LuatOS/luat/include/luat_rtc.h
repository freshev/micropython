#ifndef LUAT_RTC_H
#define LUAT_RTC_H
#include "luat_base.h"
#include "time.h"
/**
 * @defgroup luatos_RTC clock interface (RTC)
 * @{*/

/**
 * @brief Set system time
 *
 * @param tblock
 * @return int =0 success, others failure*/
int luat_rtc_set(struct tm *tblock);
/**
 * @brief Get system time
 *
 * @param tblock
 * @return int =0 success, others failure*/
int luat_rtc_get(struct tm *tblock);
/** @}*/
int luat_rtc_timer_start(int id, struct tm *tblock);
int luat_rtc_timer_stop(int id);
void luat_rtc_set_tamp32(uint32_t tamp);
/**
 * @brief Set the time zone, or synchronize the underlying time zone to the application layer
 *
 * @param timezone pointer to the time zone value. If it is empty, synchronize the underlying time zone to the application layer.
 * @return int current time zone*/
int luat_rtc_timezone(int* timezone);

#endif
