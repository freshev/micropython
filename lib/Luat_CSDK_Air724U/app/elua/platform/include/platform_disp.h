/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    platform_disp.h
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/26
 *
 * Description:
 *          platform display ½Ó¿Ú
 **************************************************************************/

#ifndef _PLATFORM_DISP_H_
#define _PLATFORM_DISP_H_

#include <sys/queue.h>

typedef struct PlatformRectTag
{
    u16 ltx;        //left top x y
    u16 lty;
    u16 rbx;        //right bottom x y
    u16 rby;
}PlatformRect;

// «« ¶ ¨ ¨ ¨ ¨ ¨ ¨ ¨ ¨ ¨ ¨ RGB (5,6,5)
#define COLOR_WHITE_16 0xffff
#define COLOR_BLACK_16 0x0000
#define COLOR_WHITE_24 0xffffff
#define COLOR_BLACK_24 0x000000

#define COLOR_WHITE_1 0xff
#define COLOR_BLACK_1  0x00

#define BASIC_LAYER_ID  0
#define USER_LAYER_1_ID  1
#define USER_LAYER_2_ID  2
#define INVALID_LAYER_ID  -1


typedef enum PlatformLcdBusTag
{
    PLATFORM_LCD_BUS_SPI4LINE,
    PLATFORM_LCD_BUS_PARALLEL,

/*+\ New \ lowiiqiaqiang \ 2014.10.11 \ ďelcd i2C spi½ó¿ú*/
    PLATFORM_LCD_BUS_I2C,
    PLATFORM_LCD_BUS_SPI,
/*-\ new \ liweiqiaqiang \ 2014.10.11 \ Án¼ólcd i2C spi½ó¿ú*/
    
    PLATFORM_LCD_BUS_QTY,
}PlatformLcdBus;

/*+\ New \ lowiiqiaqiang \ 2014.10.11 \ ďelcd i2C spi½ó¿ú*/
typedef union {
    struct {
        int bus_id;
        int pin_rs;
        int pin_cs;
        int freq;
    } bus_spi;
    
    struct {
        int bus_id;
        int freq;
        int slave_addr;
        int cmd_addr;
        int data_addr;
    } bus_i2c;
} lcd_itf_t;
/*-\ new \ liweiqiaqiang \ 2014.10.11 \ Án¼ólcd i2C spi½ó¿ú*/

typedef struct PlatformDispInitParamTag
{
    u16 width;  // lcdÉè±¸¿n¶È
    u16 height; // lcdÉè±¸¸ß¶È
    u8  bpp; // bits per pixel lcdéè ± é «éî 1: º ° × 16: 16î» is «²êæeeee
    u16 x_offset;
    u16 y_offset;
    u32 *pLcdCmdTable;    //lcd³õÊ¼»¯Ö¸Áî±n
    u16 tableSize;         //lcd³õÊ¼»¯Ö¸¶¨±n´óÐ¡
/*+\NEW\liweiqiang\2013.12.18\Ôö¼ÓlcdË¯ÃßÃüÁîÖ§³Ö*/
    u32 *pLcdSleepCmd;  // lcd sleepÖ¸Áî±n
    u16 sleepCmdSize;
    u32 *pLcdWakeCmd;   // lcd wakeÖ¸Áî±n
    u16 wakeCmdSize;
/*-\NEW\liweiqiang\2013.12.18\Ôö¼ÓlcdË¯ÃßÃüÁîÖ§³Ö*/
    PlatformLcdBus bus;
/*+\new\liweiqiang\2014.10.11\Ìn¼Ólcd i2c½Ó¿Ú*/
    lcd_itf_t lcd_itf;
/*-\new\liweiqiang\2014.10.11\Ìn¼Ólcd i2c½Ó¿Ú*/
    int pin_rst; //reset pin
    /*+\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
    int hwfillcolor; // lcdîïànìî³äé «
    /*-\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
/*+\ New \ 2013.4.10 \ ° ° ° °*/
    int pin_cs; // cs pin
/*+ + + + + + + + + + + + + + + + + +while aff these.*/
    int pin_rs; // rs pin
/*- \nw \nw.9.9 \g:2964 Mono_s7571.lua îî·è·¹ ¹ùùããããããe -SD ¶25720.*/
	/*what*/
    u8 camera_preview_no_update_screen;
	/*-\bug2406\zensually · Author has £28\\\'IÉki World comes in more*/
    u8 *framebuffer;
/*-\ New \ 2013.4.10 \ ° ° ° °*/

    /*+\BUG:3316\cm\2020.16\LCD_SPI*/  
    u8 Driving;// lcd_spiμäçý¶äe| × î'óölî
    /*-\BUG:3316\cmm\2020.16\LCD_SPI*/  
}PlatformDispInitParam;

typedef SLIST_ENTRY(platform_layer) platform_layer_iter_t;
typedef SLIST_HEAD(platform_layer_head, platform_layer) platform_layer_head_t;

typedef struct platform_layer
{
    platform_layer_iter_t iter;
	int id;
	int height;
	int width;
	int bpp;
	void *buf;
	void (*update) (PlatformRect *area, struct platform_layer *layer);
    void (*destroy) (struct platform_layer *layer);
} platform_layer_t;

int platform_disp_create_layer(platform_layer_t *layer);

int platform_disp_set_act_layer(int id);

void platform_disp_destroy_layer(int id);

void platform_disp_init(PlatformDispInitParam *pParam);

void platform_disp_close(void);

void platform_disp_clear(void);

void platform_disp_update(void);

u16* platform_disp_puttext(const char *string, u16 x, u16 y);

/*+ \ NEW \ LIWEIQIANG \ 2013.12.6 {Urinkink toæ2 «« Luxemburn Search*/
/*+\NEW\liweiqiang\2013.11.4\Ôö¼ÓBMPÍ¼Æ¬ÏÔÊ¾Ö§³Ö*/
/*+\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
int platform_disp_putimage(const char *filename, u16 x, u16 y, int transcolor, u16 left, u16 top, u16 right, u16 bottom,int transtype);
/*-\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
/*-\NEW\liweiqiang\2013.11.4\Ôö¼ÓBMPÍ¼Æ¬ÏÔÊ¾Ö§³Ö*/
/*-\ NEW \ Lirowei Qiagang \ 2013.12.6 {Urinkink toæ2 «« Luxemburc sofi ie "*/

void platform_layer_start_move(int layer_id1, int layer_id2,int layer_id3,int delay_ms,int x_inc,int y_inc);

void platform_disp_playgif(const char* gif_file_name, int x, int y,  int times);


void platform_disp_stopgif(void);


int platform_disp_preload_png_foreground(const char *filename, int index);

int platform_disp_preload_png_background(const char *filename);

int platform_disp_move_foreimg(int x1, int y1, int x2, int y2);


/*+\NEW\liweiqiang\2013.12.7\Ôö¼Ó¾ØÐÎÏÔÊ¾Ö§³Ö*/
int platform_disp_drawrect(int x1, int y1, int x2, int y2, int color);
/*-\NEW\liweiqiang\2013.12.7\Ôö¼Ó¾ØÐÎÏÔÊ¾Ö§³Ö*/

/*+\ New \ liweiqiang \ 2013.12.9 \ ôö¼óç ° ° ° ° \ ± ³ ° ° ° «Éèöã*/
int platform_disp_setcolor(int color);
int platform_disp_setbkcolor(int color);
/*-\ new \ liweiqiang \ 2013.12.9 \ ôö¼óç ° ¾ ° \ \ ± ³¾ ° «Éèöã*/

/*+\NEW\liweiqiang\2013.12.9\Ôö¼Ó·ÇÖÐÎÄ×ÖÌåÉèÖÃ*/
int platform_disp_loadfont(const char *name);
int platform_disp_setfont(int id);
/*-\NEW\liweiqiang\2013.12.9\Ôö¼Ó·ÇÖÐÎÄ×ÖÌåÉèÖÃ*/
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  
int platform_disp_setfontHeight(int height);
int platform_disp_getfontHeight(void);
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  
/*NEW TO THE entities NEW TO THE entities ****** The I shall.
****fns of nam---- platform_disp_png_png_to_yer
*** --fide: Ò×°4×õçØµçüÆ¬µüâsâm
                 wing_ad: ân¬¬Æ¬× °4Øµ µ » µ µ µS «» »way
2
**Sereon ---- BC´É»»
*****The     
NEW TO THE entities NEW TO THE entities ****** The I shall.*/
int platform_disp_preload_png_to_layer(const char *filename, int layer_id);





/********* of*******lable**********s***lable********slable********lain******lain*******lain*******lainly********lainly********lainly********lainly********lainly*******lainly)
****f if Is--- the site of theform_lap___________
****pas ----away_id:Ò ´´´½£´½½
            ----are luxury: óã¬nnã¶¶
            ----energy_he's lust:ÍjóóøŸøøø
****desc ---- ´´´´´´±£ stage»´´´½
***etr -----a ó 4 views·â€™t
2***n     
---1.Ôó´´´½´´ point
----2.dðOðONO»O»
----3.»âùé»²ã²ûâé´½â½ ½´½´
----4.â£»´´´©´£´¬KO´´º´´½´©´ level£££»
******** of*******lable**********s***lable********slable********lain******lain*******lain*******lainly********lainly********lainly********lainly********lainly*******lainly)*/
int platform_create_user_layer(int layer_id, int start_x, int start_y, int layer_width, int layer_height);




/*NEW TO THE entities NEW TO THE entities ****** The I shall.
****fns of nam-----destroy_stroy_ander_yer_layer
*777*-- winge_id:ÒêÙ»ÙµKüçÍõçÍüçÍ.
2
**Sereon ---- BC´É»»
*****The     
-----1. in_id_idÖ"ÄÇS_1_DID"
-----2. SónçâÃâÃâÃâÃâsses, judges theóânê º º ºóÃÃø»Sø¸¸» » the USK»Sûâæyne's between all.
NEW TO THE entities NEW TO THE entities ****** The I shall.*/
int platform_destroy_user_layer(int layer_id);



/******************************************************************************
***func name---- platform_set_active_layer
***param    ---- layer_id:Òª¼¤»îµÄÍ¼²ãID
***desc     ---- ¼¤»îÍ¼²ã£¬¶ÔLCDµÄËùÓÐ»æÍ¼¶¯×÷¶¼»eÔÚ¼¤»îÍ¼²ãÉÏ½øÐÐ
***return   ---- NULL
***note     
******************************************************************************/
void platform_set_active_layer(int layer_id);




/******************************************************************************
***func name---- platform_swap_user_layer
***param    ---- NULL
***desc     ---- ¶Ô»»Á½¸öÓÃ»§Í¼²ãµÄÄÚÈÝ¡£´Ëº¯Êý²»Éæ¼°Í¼²ãµÄÊý¾Ý¿½±´£¬Òò´ËËÙ¶ÈºÜ¿ì¡£
                 ÊÊÓÃÓÚ¶ÔËÙ¶ÈÒªÇó¸ßµÄÇé¿öÏÂ¿ìËÙ½»»»Á½¸öÓÃ»§Í¼²ãµÄÄÚÈÝ.
***return   ---- NULL
***note     
******************************************************************************/
void platform_swap_user_layer(void);





/********* of*******lable**********s***lable********slable********lain******lain*******lain*******lainly********lainly********lainly********lainly********lainly*******lainly)
****fun if it is---the site venue_ay_ay_lap_lap
****pas ---- dort_id: Ò²OOPOOM´´´½
            ----display: â´´´½âéé»OOO¿ answer
            -----display_y: â´´´´½âé»OO»O©´½½
            ---- sc_ader_id:  OUPO´´´½»´
            ----scr_rage: âÒ»»´OPOOPYOMOOKOOn
****nsc ----a--âóéããueã party´ basis
****--'--NLL
2***n
******** of*******lable**********s***lable********slable********lain******lain*******lain*******lainly********lainly********lainly********lainly********lainly*******lainly)*/
void platform_copy_layer(int dst_layer_id,
                                 int display_x,
                                 int display_y,
                                 int src_layer_id,
                                 T_AMOPENAT_LCD_RECT_T* src_rect);




/*NEW TO THE entities NEW TO THE entities ****** The I shall.
****fns of nam----to_to_to_to_to_basics
**** -- wing_id1: Òêâts" ¾µµµ» 1»1»1»1»1»1»1»1»1µ
            ------x1: à ô¼22ü2çüLCüçüLõüâmsüLµõõçüL »
            ----y-y1: à ô¼22ü2çüLCüõüâms1õمüLµõõçýõüHLµ »_×ùnic.
            ---- w this
            -----x2: à ô¼2üµ how Frenchü LearykaÔÚLCER »
            ----y-y2: à ô¼2üµH2çüLCüõõçüL »            
            ---- w this
            -----x2: à ô¼22ü2ü2çüLCüõõosâms.
            -----y2: à ô¼22ü2çüLCüõüçâxõõõçõüHER »9×ù´» since the truth.            
2
****return
c
NEW TO THE entities NEW TO THE entities ****** The I shall.*/
void platform_layer_flatten(int layer_id1, int x1, int y1, 
                                  int layer_id2, int x2, int y2,
                                  int layer_id3, int x3, int y3);



int platform_get_png_file_resolution(const char *filename, u32* width, u32* height);

/*+\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
void platform_disp_new(PlatformDispInitParam *pParam);

int platform_disp_get();
/*-\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
#define MAX_LCD_WIDTH_SUPPORT 240
#define MAX_LCD_HEIGH_SUPPORT 240

#define MAX_LCD_PIXEL_BYTES   3
#define MAX_LCD_BUFF_SIZE (MAX_LCD_WIDTH_SUPPORT*MAX_LCD_HEIGH_SUPPORT* MAX_LCD_PIXEL_BYTES)
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/

#endif//_PLATFORM_DISP_H_
