/*@Modules gtfont
@summary Qualcomm font chip
@version 1.0
@date 2021.11.11
@tag LUAT_USE_GTFONT
@usage
--Tested font chip model GT5SLCD1E-1A
-- If you need to support other models, please report an issue*/

#include "luat_base.h"
#include "luat_spi.h"
#include "luat_lcd.h"
#include "luat_mem.h"

#include "epdpaint.h"

#include "GT5SLCD2E_1A.h"
#define LUAT_LOG_TAG "gt"
#include "luat_log.h"

#ifdef LUAT_USE_LCD
extern luat_color_t lcd_str_fg_color,lcd_str_bg_color;
#else
static luat_color_t lcd_str_fg_color  = LCD_BLACK ,lcd_str_bg_color  = LCD_WHITE ;
#endif

extern luat_spi_device_t* gt_spi_dev;

//Display horizontally and horizontally
unsigned int gtfont_draw_w(unsigned char *pBits,unsigned int x,unsigned int y,unsigned int size,unsigned int widt,unsigned int high,int(*point)(void*,uint16_t, uint16_t, uint32_t),void* userdata,int mode){
	unsigned int i=0,j=0,k=0,n=0,dw=0;
	unsigned char temp;
	int w = ((widt+7)>> 3);
	for( i = 0;i < high; i++){
		for( j = 0;j < w;j++){
			temp = pBits[n++];
			for(k = 0;k < 8;k++){
				if (widt < size){
					if (j==(w-1) && k==widt%8){
						break;
					}
				}
				if(((temp << k)& 0x80) == 0 ){//Background color
					// /* Display a pixel */
					// if (mode == 0)point((luat_lcd_conf_t *)userdata, x+k+(j*8), y+i, lcd_str_bg_color);
					// else if (mode == 1)point((Paint *)userdata, x+k+(j*8), y+i, 0xFFFF);
				}else{
					/*Display a pixel*/
					if (dw<k+(j*8)) dw = k+(j*8);
					if (mode == 0)point((luat_lcd_conf_t *)userdata, x+k+(j*8), y+i, lcd_str_fg_color);
					else if (mode == 1)point((Paint *)userdata, x+k+(j*8), y+i, 0x0000);
					else if (mode == 2)point((u8g2_t *)userdata, x+k+(j*8), y+i, 0x0000);
				}
			}
		}
		if (widt < size){
			n += (size-widt)>>3;
		}
	}
	return ++dw;
}

/*-------------------------------------------------- -----------------------------------------------
 * Grayscale data display function 1st level grayscale/2nd level grayscale/4th level grayscale
 * Parameters:
 * data grayscale data; x,y=display starting coordinates; w width, h height, grade grayscale level [1st level/2nd level/4th level]
 * HB_par 1 Black text on white background 0 White text on black background
 *------------------------------------------------ ----------------------------------------*/
void gtfont_draw_gray_hz (unsigned char *data,unsigned short x,unsigned short y,unsigned short w ,unsigned short h,unsigned char grade, unsigned char HB_par,int(*point)(void*,uint16_t, uint16_t, uint32_t),void* userdata,int mode){
	unsigned int temp=0,gray,x_temp=x;
	unsigned int i=0,j=0,t;
	unsigned char c,c2,*p;
	unsigned long color4bit,color3bit[8],color2bit,color;
	t=(w+7)/8*grade;//
	p=data;
	if(grade==2){
		for(i=0;i<t*h;i++){
			c=*p++;
			for(j=0;j<4;j++){
				color2bit=(c>>6);//Get the 2-bit color value of the pixel
				if(HB_par==1)color2bit= (3-color2bit)*250/3;//Black text on white background
				else color2bit= color2bit*250/3;//white text on black background
				gray=color2bit/8;
				color=(0x001f&gray)<<11;							//r-5
				color=color|(((0x003f)&(gray*2))<<5);	//g-6
				color=color|(0x001f&gray);						//b-5
				temp=color;
				temp=temp;
				c<<=2;
				if(x<(x_temp+w)){
					if (mode == 0)point((luat_lcd_conf_t *)userdata,x,y,temp);
					else if (mode == 1)point((Paint *)userdata, x,y,temp);
				}
				x++;
				if(x>=x_temp+(w+7)/8*8) {x=x_temp; y++;}
			}
		}
	}
	else if(grade==3){
		for(i=0;i<t*h;i+=3){
			c=*p; c2=*(p+1);
			color3bit[0]=(c>>5)&0x07;
			color3bit[1]=(c>>2)&0x07;
			color3bit[2]=((c<<1)|(c2>>7))&0x07;
			p++;
			c=*p; c2=*(p+1);
			color3bit[3]=(c>>4)&0x07;
			color3bit[4]=(c>>1)&0x07;
			color3bit[5]=((c<<2)|(c2>>6))&0x07;
			p++;
			c=*p;
			color3bit[6]=(c>>3)&0x07;
			color3bit[7]=(c>>0)&0x07;
			p++;
			for(j=0;j<8;j++){
				if(HB_par==1)color3bit[j]= (7-color3bit[j])*255/7;//Black text on white background
				else color3bit[j]=color3bit[j]*255/7;//white text on black background
				gray =color3bit[j]/8;
				color=(0x001f&gray)<<11;							//r-5
				color=color|(((0x003f)&(gray*2))<<5);	//g-6
				color=color|(0x001f&gray);						//b-5
				temp =color;
				if(x<(x_temp+w)){
					if (mode == 0)point((luat_lcd_conf_t *)userdata,x,y,temp);
					else if (mode == 1)point((Paint *)userdata, x,y,temp);
				}
				x++;
				if(x>=x_temp+(w+7)/8*8) {x=x_temp; y++;}
			}
		}
	}
	else if(grade==4){
		for(i=0;i<t*h;i++){
			c=*p++;
			for(j=0;j<2;j++){
				color4bit=(c>>4);
				if(HB_par==1)color4bit= (15-color4bit)*255/15;//Black text on white background
				else color4bit= color4bit*255/15;//white text on black background
				gray=color4bit/8;
				color=(0x001f&gray)<<11;				//r-5
				color=color|(((0x003f)&(gray*2))<<5);	//g-6
				color=color|(0x001f&gray);				//b-5
				temp=color;
				c<<=4;
				if(x<(x_temp+w)){
					if (mode == 0)point((luat_lcd_conf_t *)userdata,x,y,temp);
					else if (mode == 1)point((Paint *)userdata, x,y,temp);
				}
				x++;
				if(x>=x_temp+(w+7)/8*8) {x=x_temp; y++;}
			}
		}
	}
	else{
		for(i=0;i<t*h;i++){
			c=*p++;
			for(j=0;j<8;j++){
				if(c&0x80) color=0x0000;
				else color=0xffff;
				c<<=1;
				if(x<(x_temp+w)){
					if(color == 0x0000 && HB_par == 1){
						if (mode == 0)point((luat_lcd_conf_t *)userdata,x,y,color);
						else if (mode == 1)point((Paint *)userdata, x,y,color);
					}else if(HB_par == 0 && color == 0x0000){
						if (mode == 0)point((luat_lcd_conf_t *)userdata,x,y,~color);
						else if (mode == 1)point((Paint *)userdata, x,y,~color);
					}
				}
				x++;
				if(x>=x_temp+(w+7)/8*8) {x=x_temp; y++;}
			}
		}
	}
}

#ifndef LUAT_COMPILER_NOWEAK
LUAT_WEAK int GT_Font_Init(void) {
    return 1;
}
#endif

/**
Initialize Qualcomm font chip
@api gtfont.init(spi_device)
@userdata only supports pointer data generated by spi device
@return boolean returns true if successful, otherwise returns false
@usage
-- Special reminder: Any code using this library requires an additional Qualcomm font chip!!
--Can't run without extra chips!!
gtfont.init(spi_device)*/
static int l_gtfont_init(lua_State* L) {
    if (gt_spi_dev == NULL) {
        gt_spi_dev = lua_touserdata(L, 1);
    }
	const char data = 0xff;
	luat_spi_device_send(gt_spi_dev, &data, 1);
	int font_init = GT_Font_Init();
	lua_pushboolean(L, font_init == 0 ? 1 : 0);
    return 1;
}

#include "rotable2.h"
static const rotable_Reg_t reg_gtfont[] =
{
    { "init" ,          ROREG_FUNC(l_gtfont_init)},
	{ NULL,             ROREG_INT(0)}
};

LUAMOD_API int luaopen_gtfont( lua_State *L ) {
    luat_newlib2(L, reg_gtfont);
    return 1;
}
