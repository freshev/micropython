#ifndef GPSMAG_H
#define GPSMAG_H

//UTC time information
typedef struct
{										    
 	uint16_t year;	//years
	uint8_t month;	//month
	uint8_t date;	//date
	uint8_t hour; 	//Hour
	uint8_t min; 	//minute
	uint8_t sec; 	//Seconds
}nmea_utc_time;

//NMEA 0183 data storage structure after protocol analysis
typedef struct  
{										    
	nmea_utc_time utc;			    //UTC time
	uint32_t latitude;				//The latitude minute is expanded by 100000 times, which actually needs to be divided by 100000
	uint8_t nshemi;					//North/South latitude, N: North latitude; S: South latitude
	uint32_t longitude;			    //The longitude minute is expanded by 100000 times, which actually needs to be divided by 100000
	uint8_t ewhemi;					//East longitude/West longitude, E: East longitude; W: West longitude
	uint8_t gpssta;					//GPS status: 0, not positioned; 1, positioned successfully
	int altitude;			 	    //Altitude, magnified 10 times, actually divided by 10. Unit: 0.1m
	uint16_t speed;					//Ground speed, magnified 1000 times, actually divided by 1000. Unit: 0.001 km/h
    uint16_t course;                //azimuth angle
    uint8_t posslnum;				//The number of GPS satellites used for positioning, 0~12.
}nmea_msg;


void gps_service_init(void);
void agps_service_init(void);
void agps_start_timer(void);

#endif
