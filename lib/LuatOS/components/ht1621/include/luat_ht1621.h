#ifndef __SEGMENTLCD__H__
#define __SEGMENTLCD__H__

#include "luat_base.h"
#include "luat_gpio.h"

//Define the command of HT1621
#define  ComMode    0x52  //4COM,1/3bias  1000    010 1001  0  
#define  RCosc      0x30  //Internal RC oscillator (power-on default) 1000 0011 0000
#define  LCD_on     0x06  //Turn on the LCD bias generator 1000 0000 0 11 0
#define  LCD_off    0x04  //Close LCD display
#define  Sys_en     0x02  //System oscillator on 1000 0000 0010
#define  CTRl_cmd   0x80  //Write control command
#define  Data_cmd   0xa0  //write data command

//Set variable register function
// #define sbi(x, y) (x |= (1 << y)) /*Set the yth bit of register x*/
// #define cbi(x, y) (x &= ~(1 <<y )) /*Clear the yth bit of register x*/

//IO port definition
// #define LCD_DATA 4
// #define LCD_WR 5
// #define LCD_CS 6

#ifndef HIGH
#define HIGH 1
#endif

#ifndef LOW
#define LOW 1
#endif

typedef struct luat_ht1621_conf {
    uint8_t pin_data;
    uint8_t pin_wr;
    uint8_t pin_cs;
    uint8_t cmd_com_mode;
    uint8_t cmd_rc;
    uint8_t cmd_sysen;
}luat_ht1621_conf_t;

//Define port HT1621 data port
#define LCD_DATA1    luat_gpio_set(LCD_DATA,HIGH) 
#define LCD_DATA0    luat_gpio_set(LCD_DATA,LOW) 
#define LCD_WR1      luat_gpio_set(LCD_WR,HIGH)  
#define LCD_WR0      luat_gpio_set(LCD_WR,LOW)   
#define LCD_CS1      luat_gpio_set(LCD_CS,HIGH)  
#define LCD_CS0      luat_gpio_set(LCD_CS,LOW)


void luat_ht1621_init(luat_ht1621_conf_t *conf);
void luat_ht1621_write_cmd(luat_ht1621_conf_t *conf, uint8_t cmd);
void luat_ht1621_write_data(luat_ht1621_conf_t *conf, uint8_t addr, uint8_t data);
void luat_ht1621_lcd(luat_ht1621_conf_t *conf, int onoff);


#endif
