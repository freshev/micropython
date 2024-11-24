#include "luat_base.h"
#include "luat_casic_gnss.h"

#define LUAT_LOG_TAG "casic"
#include "luat_log.h"

/**************************************************** *********
Function name: isLeapYear
Function function: leap year judgment. Judgment rules: there is a leap every four years, there is no leap every hundred years, and there is another leap every four hundred years.
Function input: year, year to be determined
Function output: 1, leap year, 0, non-leap year (normal year)
*************************************************** **********/
static int isLeapYear(int year)
{
	if ((year & 0x3) != 0)
	{ // If year is not a multiple of 4, it must be an ordinary year
		return 0;
	}
	else if ((year % 400) == 0)
	{ // year is a multiple of 400
		return 1;
	}
	else if ((year % 100) == 0)
	{ // year is a multiple of 100
		return 0;
	}
	else
	{ // year is a multiple of 4
		return 1;
	}
}
/**************************************************** ************************
Function name: gregorian2SvTime
Function: Time format conversion, UTC jump second correction needs to be considered
The input time format is the time in the regular year, month, day, hour, minute, and second format;
The converted time format is GPS time format, expressed in week numbers and hours of the week. The starting point of GPS time is 1980.1.6
GPS time has no leap second correction and is a continuous time, while regular time is corrected by leap seconds.
The leap second correction for 2016 is 17 seconds
Function input: pDateTime, structure pointer, time in year, month, day, hour, minute and second format
Function output: pAidIni, structure pointer, time format of week number and time of week (or number of days and time of day)
*************************************************** *************************/
static void gregorian2SvTime(DATETIME_STR *pDateTime, AID_INI_STR *pAidIni)
{
	int DayMonthTable[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int i, dn, wn;
	uint64_t tod, tow;

	//Time within the day
	tod = pDateTime->hour * 3600 + pDateTime->minute * 60 + pDateTime->second;

	//Reference time: 1980.1.6
	dn = pDateTime->day;
	// year->day
	for (i = 1980; i < (pDateTime->year); i++)
	{
		if (isLeapYear(i))
		{
			dn += 366;
		}
		else
		{
			dn += 365;
		}
	}
	dn -= 6;
	//month->day
	if (isLeapYear(pDateTime->year))
	{
		DayMonthTable[1] = 29;
	}
	for (i = 1; i < pDateTime->month; i++)
	{
		dn += DayMonthTable[i - 1];
	}

	// Week number + time within the week
	wn = (dn / 7);					   //week number
	tow = (dn % 7) * 86400 + tod + 17; // Time of week, leap second correction

	if (tow >= 604800)
	{
		wn++;
		tow -= 604800;
	}

	pAidIni->wn = wn;
	pAidIni->tow = tow;
}
/**************************************************** ************************
Function name: casicAgnssAidIni
Function: Pack the auxiliary position and auxiliary time into a dedicated data format. Binary information is formatted and output
Function input: dateTime, date and time, including valid flag (1 valid)
lla, latitude and longitude flags, including valid flags (1 valid)
Function output: aidIniMsg[66], character array, auxiliary information packet, fixed length
*************************************************** *************************/
void casicAgnssAidIni(DATETIME_STR *dateTime, POS_LLA_STR *lla, char aidIniMsg[66])
{
	AID_INI_STR aidIni = {0};
	int ckSum, i;
	int *pDataBuff = (int *)&aidIni;

	gregorian2SvTime(dateTime, &aidIni);

	LLOGD("date time %d %d %d %d %d %d", dateTime->year, dateTime->month, dateTime->day, dateTime->hour, dateTime->minute, dateTime->second);
	LLOGD("lat %7f", lla->lat);
	LLOGD("lng %7f", lla->lon);
	LLOGD("lls %s, time %s", lla->valid ? "ok" : "no", dateTime->valid ? "ok" : "no");

	aidIni.df = 0;
	aidIni.xOrLat = lla->lat;
	aidIni.yOrLon = lla->lon;
	aidIni.zOrAlt = lla->alt;
	aidIni.fAcc = 0;
	aidIni.posAcc = 0;
	aidIni.tAcc = 0;
	aidIni.timeSource = 0;

	aidIni.flags = 0x20;										// The position format is LLA format, the height is invalid, and the frequency and position accuracy estimates are invalid
	aidIni.flags = aidIni.flags | ((lla->valid == 1) << 0);		// BIT0: Position valid flag
	aidIni.flags = aidIni.flags | ((dateTime->valid == 1) << 1); // BIT1: time valid flag

	// Auxiliary data packaging
	ckSum = 0x010B0038;
	for (i = 0; i < 14; i++)
	{
		ckSum += pDataBuff[i];
	}

	aidIniMsg[0] = 0xBA;
	aidIniMsg[1] = 0xCE;
	aidIniMsg[2] = 0x38; // LENGTH
	aidIniMsg[3] = 0x00;
	aidIniMsg[4] = 0x0B; // CLASS	ID
	aidIniMsg[5] = 0x01; // MESSAGE	ID

	memcpy(&aidIniMsg[6], (char *)(&aidIni), 56);
	memcpy(&aidIniMsg[62], (char *)(&ckSum), 4);

	return;
}
