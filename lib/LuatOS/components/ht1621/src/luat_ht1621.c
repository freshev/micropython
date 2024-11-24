
#include "luat_base.h"
#include "luat_ht1621.h"
#include "luat_gpio.h"
#include "luat_timer.h"

static void luat_ht1621_SendBit(luat_ht1621_conf_t *conf, unsigned char sdat,unsigned char cnt) //The high cnt bit of data is written to HT1621, high bit first
{ 
	unsigned char i;
	int pin_wr = conf->pin_wr;
	int pin_data = conf->pin_data;
	for(i=0;i<cnt;i++) 
	{ 
		luat_gpio_set(pin_wr, 0);
		luat_timer_us_delay(20); 
		if(sdat&0x80)
		{
			luat_gpio_set(pin_data, 1);
		}
		else
		{
			luat_gpio_set(pin_data, 0);
		}
		luat_timer_us_delay(20);
		luat_gpio_set(pin_wr, 1);
		luat_timer_us_delay(20);
		sdat<<=1;
	} 
	luat_timer_us_delay(20);  
}


void luat_ht1621_init(luat_ht1621_conf_t *conf) {
	luat_gpio_mode(conf->pin_cs, LUAT_GPIO_OUTPUT, LUAT_GPIO_PULLUP, 0);
	luat_gpio_mode(conf->pin_data, LUAT_GPIO_OUTPUT, LUAT_GPIO_PULLUP, 0);
	luat_gpio_mode(conf->pin_wr, LUAT_GPIO_OUTPUT, LUAT_GPIO_PULLUP, 1);
	luat_ht1621_lcd(conf, 0);
	luat_ht1621_write_cmd(conf, conf->cmd_com_mode);
	luat_ht1621_write_cmd(conf, conf->cmd_rc);
	luat_ht1621_write_cmd(conf, conf->cmd_sysen);
	luat_ht1621_lcd(conf, 1);
}

void luat_ht1621_write_cmd(luat_ht1621_conf_t *conf, uint8_t cmd) {
	luat_gpio_set(conf->pin_cs, 0);
	luat_ht1621_SendBit(conf, 0x80,4);    		//Write the flag code "100" and the 9-digit command command, because
	luat_ht1621_SendBit(conf, cmd,8); 		    //There is no command to change the clock output, for the convenience of programming
	luat_gpio_set(conf->pin_cs, 1); //Directly write "0" to the highest bit of command
}

void luat_ht1621_write_data(luat_ht1621_conf_t *conf, uint8_t addr, uint8_t sdat) {
	addr<<=2; 
	luat_gpio_set(conf->pin_cs, 0);
	luat_ht1621_SendBit(conf, 0xa0,3);     //Write the flag code "101"
	luat_ht1621_SendBit(conf, addr,6);     //Write the high 6 bits of addr
	luat_ht1621_SendBit(conf, sdat,8);    //Write 8 bits of data
	luat_gpio_set(conf->pin_cs, 1);
}

void luat_ht1621_lcd(luat_ht1621_conf_t *conf, int onoff) {
	luat_ht1621_write_cmd(conf, onoff ? LCD_on : LCD_off);
}
