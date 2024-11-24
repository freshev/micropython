/*
 * Copyright (c) 2022 OpenLuat & AirM2M
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common_api.h"
#include "luat_rtos.h"
#include "luat_debug.h"
#include "luat_uart.h"
#include "luat_gpio.h"
#include "platform_define.h"
#include "gpsmsg.h"

#define UART_ID 2
luat_rtos_task_handle gps_led_task_handle;
nmea_msg gpsx = {0};

/**
 * @brief Get the position of the cxth comma from buf
 *
 * @param buf GPS data buffer address
 * @param cx cxth comma
 * @return 0~0xFE, represents the offset of the comma position; 0xFF, represents that the cxth comma does not exist*/					  
uint8_t nmea_comma_pos(uint8_t *buf,uint8_t cx)
{	 		    
	uint8_t *p = buf;
	while(cx)
	{		 
		if((*buf == '*')||(*buf < ' ')||(*buf > 'z'))
            return 0xFF;//When '*' or illegal character is encountered, the cxth comma does not exist
		if(*buf == ',')   
            cx--;
		buf++;
	}
	return buf-p;	 
}

/**
 * @brief m^n function
 *
 * @param m value
 * @param n value
 * @return m^n power*/
uint32_t nmea_pow(uint8_t m,uint8_t n)
{
	uint32_t result = 1;	 
	while(n--)  result*=m;    
	return result;
}

/**
 * @brief str is converted to a number, ending with ',' or '*'
 *
 * @param buf digital storage area
 * @param dx Number of decimal points, returned to the calling function
 * @return converted value*/
int nmea_str2num(uint8_t *buf,uint8_t*dx)
{
	uint8_t *p = buf;
	uint32_t ires = 0,fres = 0;
	uint8_t ilen = 0,flen = 0,i;
	uint8_t mask = 0;
	int res;
	while(1) //Get the length of integers and decimals
	{
		if(*p == '-')
        {
            mask |= 0x02;
            p++;
        }//is a negative number
		if((*p == ',')||(*p == '*'))
            break;//The encounter is over
		if(*p == '.')
        {
            mask |= 0x01;
            p++;
        }//Encountered a decimal point
		else if((*p > '9')||(*p < '0'))	//There are illegal characters
		{	
			ilen = 0;
			flen = 0;
			break;
		}	
		if(mask&0x01)
            flen++;
		else 
            ilen++;
		p++;
	}
	if(mask&0x02)
        buf++;	//Remove the negative sign
	for(i = 0;i < ilen;i++)	//Get the integer part data
	{  
		ires += nmea_pow(10,ilen-1-i)*(buf[i]-'0');
	}
	if(flen > 5)
        flen = 5;	//take up to 5 decimal places
	*dx = flen;	 		//number of decimal points
	for(i = 0;i < flen;i++)	//Get the decimal part data
	{  
		fres += nmea_pow(10,flen-1-i)*(buf[ilen+1+i]-'0');
	} 
	res = ires*nmea_pow(10,flen)+fres;
	if(mask&0x02)
        res = -res;		   
	return res;
}	

/**
 * @brief Analyze GNRMC information
 *
 * @param buf The first address of the received GPS data buffer*/
void gps_nmea_gnrmc_parse(uint8_t *buf)
{
	uint8_t *p1,dx;			 
	uint8_t posx;     
	uint32_t temp;	   
	float rs;  
	p1 = (uint8_t*)strstr((const char *)buf,"$GNRMC");//"$GNRMC", & and GNRMC are often separated, so only GPRMC is judged.

    posx = nmea_comma_pos(p1,2);								//Get GPS status
    if(posx != 0xFF)
	{
		if((*(p1+posx)) == 'A')
        {
            //gpsx.gpssta = 1;

            posx = nmea_comma_pos(p1,1);								//Get UTC time
            if(posx != 0xFF)
            {
                temp = nmea_str2num(p1+posx,&dx)/nmea_pow(10,dx);	 	//Get UTC time, remove ms
                gpsx.utc.hour = temp/10000;
                gpsx.utc.min = (temp/100)%100;
                gpsx.utc.sec = temp%100;	 	 
            }

            posx = nmea_comma_pos(p1,3);								//get latitude
            if(posx != 0xFF)
            {
                temp = nmea_str2num(p1+posx,&dx);	 
                gpsx.latitude = temp/nmea_pow(10,dx+2);
                rs = temp%nmea_pow(10,dx+2);
                gpsx.latitude = gpsx.latitude*nmea_pow(10,6) + (rs*nmea_pow(10,6-dx))/60;
            }

            posx = nmea_comma_pos(p1,4);								//Southern latitude or northern latitude
            if(posx != 0xFF)
                gpsx.nshemi = *(p1+posx);	

            posx = nmea_comma_pos(p1,5);								//get longitude
            if(posx != 0xFF)
            {												  
                temp = nmea_str2num(p1+posx,&dx);	 	 
                gpsx.longitude = temp/nmea_pow(10,dx+2);
                rs = temp%nmea_pow(10,dx+2);
                gpsx.longitude = gpsx.longitude*nmea_pow(10,6) + (rs*nmea_pow(10,6-dx))/60;
            }

            posx = nmea_comma_pos(p1,6);								//East longitude or west longitude
            if(posx != 0xFF)
                gpsx.ewhemi = *(p1+posx);

            posx = nmea_comma_pos(p1,7);								//speed
            if(posx != 0xFF)
            {
                int speed = nmea_str2num(p1+posx,&dx) / 1000;
                LUAT_DEBUG_PRINT("this is gps speed 1 %d", speed);
                gpsx.speed = (speed*1852 - (speed*1852 %1000))/1000;
                LUAT_DEBUG_PRINT("this is gps speed 2 %d", gpsx.speed);
            }
                

            posx = nmea_comma_pos(p1,8);								//direction angle
            if(posx != 0xFF)
                gpsx.course = nmea_str2num(p1+posx,&dx)/100;

            posx = nmea_comma_pos(p1,9);								//Get UTC date
            if(posx != 0xFF)
            {
                temp = nmea_str2num(p1+posx,&dx);		 				//Get UTC date
                gpsx.utc.date = temp/10000;
                gpsx.utc.month = (temp/100)%100;
                gpsx.utc.year = 2000+temp%100;	 	 
            } 
            	
        }
        else
            gpsx.gpssta = 0;

	}
	 
}

/**
 * @brief analyze GNGGA information
 *
 * @param buf The first address of the received GPS data buffer*/
void gps_nmea_gngga_parse(uint8_t *buf)
{
	uint8_t *p1,dx;			 
	uint8_t posx;    
    uint8_t gpsfind;    
	p1 = (uint8_t*)strstr((const char *)buf,"$GNGGA");
	posx = nmea_comma_pos(p1,6);								//Get GPS status
	if(posx != 0xFF)
    {
        gpsfind = nmea_str2num(p1+posx,&dx);	
        if((gpsfind == 1)||(gpsfind == 2)||(gpsfind == 4))
        {
            gpsx.gpssta = 1;

            posx = nmea_comma_pos(p1,7);								//Get the number of satellites used for positioning
            if(posx != 0xFF)
                gpsx.posslnum = nmea_str2num(p1+posx,&dx); 

            posx = nmea_comma_pos(p1,9);								//Get the altitude
            if(posx != 0xFF)
                gpsx.altitude = nmea_str2num(p1+posx,&dx)/10;

            agps_start_timer(); 
        }
        else
            gpsx.gpssta = 0;
    }
	
}

/**
 * @brief Analyze GPS information
 *
 * @param buf The first address of the received GPS data buffer*/
void gps_nmea_parse(uint8_t *buf)
{
    char *p;
    p = strtok(buf, "\r\n");
    while(p)
    {
        if(strstr((const char *)p,"$GNGGA"))
        {
            // LUAT_DEBUG_PRINT("nmea GNGGA data:%s",p);
            gps_nmea_gngga_parse(p);
        }
        else if(strstr((const char *)p,"$GNRMC"))
        {
            // LUAT_DEBUG_PRINT("nmea GNRMC data:%s",p);
            gps_nmea_gnrmc_parse(p);
        }
        p = strtok(NULL, "\r\n");
    }
}

void gps_uart_send_cb(int uart_id, uint32_t data_len){
    uint8_t* data_buff = malloc(data_len+1);
    memset(data_buff,0,data_len+1);
    luat_uart_read(uart_id, data_buff, data_len);
    LUAT_DEBUG_PRINT("luat_uart_cb uart_id:%d data:%s data_len:%d",uart_id,data_buff,data_len);
    gps_nmea_parse(data_buff);
    free(data_buff);
}

void gps_writedata(uint8_t* data, uint32_t length)
{
    luat_uart_write(UART_ID,data,length);
}

void gps_led_task(void *args)
{
    luat_gpio_cfg_t gpio_cfg;
	luat_gpio_set_default_cfg(&gpio_cfg);
	gpio_cfg.pin = HAL_GPIO_26;
	luat_gpio_open(&gpio_cfg);
    while (1)
    {
        if(0 == config_gps_get())
        {
            luat_gpio_set(HAL_GPIO_26, 0);
            luat_rtos_task_sleep(1000);
        }
        else
        {
            if(gpsx.gpssta == 1)
            {
                luat_gpio_set(HAL_GPIO_26, 1);
                luat_rtos_task_sleep(1000);
            }
            else
            {
                luat_gpio_set(HAL_GPIO_26, 1);
                luat_rtos_task_sleep(200);
                luat_gpio_set(HAL_GPIO_26, 0);
                luat_rtos_task_sleep(200);
            }
        }
    }
    
}

void gps_service_init(void)
{
    luat_uart_pre_setup(UART_ID, 1);
    config_gps_set(1);
    luat_uart_t uart = {
        .id = UART_ID,
        .baud_rate = 115200,
        .data_bits = 8,
        .stop_bits = 1,
        .parity    = 0
    };

    luat_uart_setup(&uart);
    luat_uart_ctrl(UART_ID, LUAT_UART_SET_RECV_CALLBACK, gps_uart_send_cb);
    luat_rtos_task_create(&gps_led_task_handle, 512, 20, "gps led task", gps_led_task, NULL, NULL);
}



