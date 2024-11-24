#ifndef LUAT_CASIC_GNSS_H
#define LUAT_CASIC_GNSS_H

#include "luat_base.h"

// Position structure
typedef struct
{
	double lat; //Latitude, positive number indicates north latitude, negative number indicates south latitude
	double lon; // Longitude, positive numbers represent east longitude, negative numbers represent west longitude
	double alt; //Height, if the height cannot be obtained, it can be set to 0
	int valid;

} POS_LLA_STR;

// Time structure (Note: This is UTC time!!! There is an 8-hour difference from Beijing time, do not use Beijing time directly!!!)
// For example, Beijing time 2016.5.8,10:34:23, then UTC time should be 2016.5.8,02:34:23
// For example, Beijing time 2016.5.8,03:34:23, then UTC time should be 2016.5.7,19:34:23
typedef struct
{
	int valid; //Time valid flag, 1=valid, otherwise invalid
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	float ms;

} DATETIME_STR;

// Auxiliary information (location, time, frequency)
typedef struct
{
	double xOrLat, yOrLon, zOrAlt;
	double tow;
	float df;
	float posAcc;
	float tAcc;
	float fAcc;
	unsigned int res;
	unsigned short int wn;
	unsigned char timeSource;
	unsigned char flags;

} AID_INI_STR;

void casicAgnssAidIni(DATETIME_STR *dateTime, POS_LLA_STR* lla, char aidIniMsg[66]);

#endif
