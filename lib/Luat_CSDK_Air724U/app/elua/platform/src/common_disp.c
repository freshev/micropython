/**************************************************************************
 *              Copyright (C), AirM2M Tech. Co., Ltd.
 *
 * Name:    common_disp.c
 * Author:  liweiqiang
 * Version: V0.1
 * Date:    2013/3/26
 *
 * Description:
 *          Display's API
 * History:
 *     panjun 2015.06.06 Fix a display's flaw for mono-LCD.
 **************************************************************************/
#ifdef LUA_DISP_LIB
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "type.h"
#include "lplatform.h"
#include "platform_disp.h"
#include "platform_lcd.h"
#include "LzmaLib.h"
/*+\NEW\zhuth\2014.2.16\Ö§³ÖpngÍ¼Æ¬µÄÏÔÊ¾*/
#ifdef AM_LPNG_SUPPORT
#include "elua_png.h"
#endif
/*-\NEW\zhuth\2014.2.16\Ö§³ÖpngÍ¼Æ¬µÄÏÔÊ¾*/
#ifdef LUA_LVGL_SUPPORT
#include "lvgl.h"
#endif

//#include "us_timer.h"
//#include "qrencode.h"
// µoâ€™ÀYâ€™ÂY (GB2312) â€™s É (UTF-8×´Unid)
#define ON  1  
#define OFF 0 

#define MAX_FONTS       10

/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
//#define MAX_LCD_WIDTH_SUPPORT 240
//#define MAX_LCD_HEIGH_SUPPORT 240
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/

#define MAX_LAYER_SUPPORT 3
#define MAX_USER_LAYER_SUPPORT 2

#define UNCACHED(addr) ((kal_uint32)(addr) & 0x0fffffff)
#define CACHED(addr)   ((kal_uint32)(addr) | 0xf0000000)


#if defined (__PROJECT_AM_WATCHER__)
#define HZ_FONT_WIDTH  (40)
#define HZ_FONT_HEIGHT (40)
#define ASC_FONT_WIDTH  (20)
#define ASC_FONT_HEIGHT (40)
#define ASC_FONT_FILE_NAME "font_40.dat"
/*+New \ abysses \ 2016.05.18 \ '*/  
#define PINYIN_FONT_FILE_NAME "font_pinyin_40.dat"
/*-New \ abysses \ 2016.05.18 \ æòôºnsicnèn¬n*/  
#ifdef FONT_HZ_COMPRESS
#define HZ_FONT_ZIP_FILE_NAME "fonthz_40.zip.dat"
#else
#define HZ_FONT_FILE_NAME "fonthz_40.dat"
#define HZ_EXT_FONT_FILE_NAME "fonthzext_40.dat"
#endif
#elif defined (__PROJECT_RUIHENG_WATCHER__)
#define HZ_FONT_WIDTH  (32)
#define HZ_FONT_HEIGHT (32)
#define ASC_FONT_WIDTH  (16)
#define ASC_FONT_HEIGHT (32)
#define ASC_FONT_FILE_NAME "font_rh_30.dat"
#ifdef FONT_HZ_COMPRESS
#define HZ_FONT_ZIP_FILE_NAME "font_rh_30.zip.dat"
#else
#define HZ_FONT_FILE_NAME "fonthz_rh_30.dat"
#define HZ_EXT_FONT_FILE_NAME "fonthzext_rh_30.dat"
#endif
#elif defined (__PROJECT_BD_WATCHER__)
#define HZ_FONT_WIDTH  (32)
#define HZ_FONT_HEIGHT (32)
#define ASC_FONT_WIDTH  (16)
#define ASC_FONT_HEIGHT (32)
#define ASC_FONT_FILE_NAME "font_bd_32.dat"
#ifdef FONT_HZ_COMPRESS
#define HZ_FONT_ZIP_FILE_NAME "font_bd_32.zip.dat"
#else
#define HZ_FONT_FILE_NAME "fonthz_bd_32.dat"
#define HZ_EXT_FONT_FILE_NAME "fonthzext_bd_32.dat"
#endif
#elif defined(__PROJECT_KUER__)
#define HZ_FONT_WIDTH  (40)
#define HZ_FONT_HEIGHT (40)
#define ASC_FONT_WIDTH  (20)
#define ASC_FONT_HEIGHT (40)
#define ASC_FONT_FILE_NAME "font.dat"

#ifdef FONT_HZ_COMPRESS
#define HZ_FONT_ZIP_FILE_NAME "fonthzkuer.zip.dat"
#else
#define HZ_FONT_FILE_NAME "fonthzkuer.dat"
#define HZ_EXT_FONT_FILE_NAME "fonthzkuerext_40.dat"
#endif

#else
#define HZ_FONT_WIDTH  (16)
#define HZ_FONT_HEIGHT (16)
#define ASC_FONT_WIDTH  (8)
#define ASC_FONT_HEIGHT (16)
#define ASC_FONT_FILE_NAME "font.dat"

#ifdef FONT_HZ_COMPRESS
#define HZ_FONT_ZIP_FILE_NAME "fonthz.zip.dat"
#else
/*+\BUG\wgyuan\2020.04.15\BUG_1542:UICYâ¯´¸¸µ2gOUµ¼µµµóóóó.*/
#if defined(AM_FONTHZ_TWO_LEVEL_SUPPORT)
#define HZ_FONT_FILE_NAME "fonthzTwoLevel.dat"
#else
#define HZ_FONT_FILE_NAME "fonthz.dat"
#endif
/*-\BUG\wgyuan\2020.04.15\BUG_1542:UICYâ¼¸¸µ2gOUµµµµµµóó×*/
#define HZ_EXT_FONT_FILE_NAME "fonthzext.dat"
#endif

#endif

#define FONT_SIZE(WIDTH, HEIGHT)   (((WIDTH) + 7) / 8 * (HEIGHT))

#define HZ_FONT_SIZE FONT_SIZE(HZ_FONT_WIDTH, HZ_FONT_HEIGHT)
#define ASC_FONT_SIZE FONT_SIZE(ASC_FONT_WIDTH, ASC_FONT_HEIGHT)
// èðºãfonts ==============================================================================================================
typedef struct gui_font_header{
    unsigned char  magic[4];   //'U'('S', 'M'), 'F', 'L', X---Unicode(Simple or MBCS) Font Library, X: ±nÊ¾°æ±¾ºÅ. ·Ö¸ßµÍ4Î»¡£Èç 0x12±nÊ¾ Ver 1.2
    unsigned char  nSection; // ¹²·Ö¼¸¶ÎÊý¾Ý¡£
    unsigned char  YSize;    /* height of font  */     
    unsigned short wCpFlag;    // Codepageflg: bit0~bJÿ´´´öbehaº´ú±nnO”»µ»µ´´èäö´â ±±±BodePage ±”â¶ugug´ ugh¨¶¶¶¶
    unsigned short nTotalChars;  // × üµä × ö · û
    unsigned char  ScanMode;   // É¨ÃèÄ£Ê½
}GUI_FONT_HEADER;

typedef struct gui_font_section{
    unsigned short first;  // μ μ. ç ° ¶Îμúò »¸Ö Î ö ö Û
    unsigned short last;   // μ ± Én ° ¶ itali »¸Ö × öÖ__
    unsigned short startIndex; // ¼çâ¼µ ± Ç ° ¶ÎµúÒ »¸Ö × Ö · ÛµÄæÐÊ¼Î» ÖÃ £ ¨¼´Ë ÷ ÒýÖµ £ ©
}GUI_FONT_SECTION;

typedef struct gui_font_index{
    unsigned char width;  // ¼çâ ± ± ° ° ° ö · û ·
    unsigned int  offset;  // PRESPECT // ± ¡狗 öμμö province û ¨ âВ ¨ μ £ ¨ μ £ ·μμ £ ©
}GUI_FONT_INDEX;

#if VERSION_CHINESE == ON
//****************** ÎÄ¼þÍ·header info *****************
static GUI_FONT_HEADER fontHeader_CJK = {
    'M', 'F', 'L', 0x11,     // magic
    0,         // section number
    32,         // ySize (Height)
    0x2,         // flag of codepage
    8178,         // total chars
    0,         // scan mode 
};

static GUI_FONT_HEADER fontHeader_Latin = {
    'M', 'F', 'L', 0x11,     // magic
    1,         // section number
    32,         // ySize (Height)
    0x80,         // flag of codepage
    122,         // total chars
    0,         // scan mode 
};

//****************** ¼ìË÷±nindex ***********************
static const GUI_FONT_INDEX fonts_index_Latin[256]= {
    #include "rh_gb2312_fonts_index_latin.dat"
};

// ***************** μ μ μå ¢ date *******************
static const unsigned char fonts_data_CJK[]= {
    #include "rh_gb2312_fonts_data_cjk.dat"
};

static const unsigned char fonts_data_Latin[]= {
    #include "rh_gb2312_fonts_data_latin.dat"
};

#elif VERSION_ENGLISH == ON
//****************** ÎÄ¼þÍ·header info *****************
static GUI_FONT_HEADER fontHeader = {
	'U', 'F', 'L', 0x11,     // magic
	1,         // section number
	32,         // ySize (Height)
	0x82,         // flag of codepage
	7575,         // total chars
	0,         // scan mode 
};

//****************** ¶ÎÐÅÏ¢sections ********************
static GUI_FONT_SECTION fonts_sections[1] = {
		0x0020, 0xffe5, 0x0,
};

//****************** ¼ìË÷±nindex ***********************
static const GUI_FONT_INDEX fonts_index[65478]= {
    #include "rh_unicode_fonts_index.dat"
};

// ***************** μ μ μå ¢ date *******************
static const unsigned char fonts_data[]= {
    #include "rh_unicode_fonts_data.dat"
};
#endif

/*+\ New \ 2013.4.10 \ ° ° ° °*/
// 1bitÎ»Í¼Êý¾Ý×éÖ¯·½Ê½Í¬win32 1bit bmp
typedef struct DispBitmapTag
{
    u16 width;
    u16 height;
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  	
    u16 orgWidth; // Ö§³ö × Öìåëõ · to
    u16 orgHeight;
    float zoomRatio;
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  	
    u8 bpp;
    const u8 *data;
}DispBitmap;
/*-\ New \ 2013.4.10 \ ° ° ° °*/

typedef struct FontInfoTag
{
    u8        width;
    u8        height;
    u8        size;
    u16       start;
    u16       end;
    const u8       *data;
}FontInfo;

static const u8 blankChar[HZ_FONT_SIZE] = {0};

// font 
// ËÎÌå16 ascii 0x20~0x7e
static const u8 sansFont16Data[]=
{
    #include ASC_FONT_FILE_NAME
};

static const FontInfo sansFont16 = 
{
    ASC_FONT_WIDTH,
    ASC_FONT_HEIGHT,
    ASC_FONT_SIZE,
    0x20,
    0x7E,
    sansFont16Data,
};

//ºº × öëëire (6
#if defined(FONT_HZ_COMPRESS)
static const u8 sansHzFont16DataZip[] =
{
    #include HZ_FONT_ZIP_FILE_NAME
};

static const u8 *sansHzFont16Data = NULL;
#else
static const u8 sansHzFont16Data[] =
{
    #include HZ_FONT_FILE_NAME
};

/*+ \ New \ liWeiqianG \ 2013.12.18 \ Ôö¼öðîä ± Û ºå ÛºåμÄÊ¾Ö§ôÖï*/
static const u8 sansHzFont16ExtData[] = 
{
    #include HZ_EXT_FONT_FILE_NAME
};
// [B0A1, F7FE] ² »° UºD7FA-D7FE
/*°´ÄÚÂëÓÉÐ¡µ½´óÅÅÁÐ*/
static const u16 sansHzFont16ExtOffset[] =
{
// "¡¢ ¡¡¡¡¡¡¡¡¡¡£ £ d ours ¬ o ¢ o¡ ¡£ £ £ £ £ £ º £ £ £ £"
    0xA1A2,0xA1A3,0xA1AA,0xA1AD,0xA1AE,0xA1AF,0xA1B0,0xA1B1,
    0xA1B2,0xA1B3,0xA1B4,0xA1B5,0xA1B6,0xA1B7,0xA1B8,0xA1B9,
    0xA1BA,0xA1BB,0xA1BE,0xA1BF,0xA3A1,0xA3A8,0xA3A9,0xA3AC,
    0xA3AD,0xA3AE,0xA3BA,0xA3BB,0xA3BF,0xE0C5
};

/*- \ new \ LIWEIQIANG \ 2013.12.18 \ ÔÖ¼öðÎÄ ± Ûμã ÛºåμÄïÊ¾Ö¾§³Ö*/

#ifdef __PROJECT_AM_WATCHER__
/*+New \ abysses \ 2016.05.18 \ '*/  
static const u8 sansHzFontPinyiData[] =
{
    #include PINYIN_FONT_FILE_NAME
};
/*-New \ abysses \ 2016.05.18 \ æòôºnsicnèn¬n*/  
#endif
#endif

static FontInfo sansHzFont16 =
{
    HZ_FONT_WIDTH,
    HZ_FONT_HEIGHT,
    HZ_FONT_SIZE,
    0,
    0,
    NULL
};

#ifdef __PROJECT_AM_WATCHER__
/*+New \ abysses \ 2016.05.18 \ '*/  
static FontInfo sansHzFontPinyin =
{
    ASC_FONT_WIDTH,
    HZ_FONT_HEIGHT,
    ASC_FONT_SIZE,
    0xa8a1,
    0xa8bf,
    NULL
};
/*-New \ abysses \ 2016.05.18 \ æòôºnsicnèn¬n*/  
#endif

static FontInfo dispFonts[MAX_FONTS];
static u8 curr_font_id = 0;
static FontInfo *dispHzFont;
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  
// ¸ × × × öìå¶¶ · · · · å ± ± èA
static u16 g_s_fontZoomSize = HZ_FONT_HEIGHT;
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  
// ± ³¾ ° is «Óëïôê¾ñõé«
static int disp_bkcolor = COLOR_WHITE_1;
static int disp_color = COLOR_BLACK_1;
/*+\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
static int lcd_hwfillcolor = COLOR_BLACK_1;// lcdîïànìî³äé «Éè¶¨, ä¿ç ° Ö» Ö§³öºú ° × æe, ´ÆÔÚºúµ   × ° × ÖµäæÁò²´ÆÔÚ ° × µ × ºÚ × ÖµÄæe
/*-\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/

// lcd éè ± ¸²ÎA
u16 lua_lcd_height;
u16 lua_lcd_width;
u8 lua_lcd_bpp;

PlatformLcdBus lcd_bus;

// ïôê¾ »º³åçø
/*+\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
//#define MAX_LCD_PIXEL_BYTES   3
//#define MAX_LCD_BUFF_SIZE (MAX_LCD_WIDTH_SUPPORT*MAX_LCD_HEIGH_SUPPORT* MAX_LCD_PIXEL_BYTES)
/*-\NEW\zhuwangbin\2020.05.01\Ìn¼Ódisp camera¹¦ÄÜ*/
#ifdef __MTK_TARGET__
    #pragma arm section zidata = "DYNAMICCACHEABLEZI_NC"
#endif

kal_uint8 framebuffer[MAX_LCD_BUFF_SIZE];

kal_uint8* workingbuffer = framebuffer;
/*+\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
int frameBufferSize = 0;
/*-\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
#ifdef __MTK_TARGET__
    #pragma arm section zidata
#endif 


static int active_layer_id = 0;
OPENAT_LAYER_INFO layer_info[MAX_LAYER_SUPPORT];

static platform_layer_head_t layers = SLIST_HEAD_INITIALIZER(layers);

int platform_disp_create_layer(platform_layer_t *layer)
{
	static int id = 0;
	layer->id = id++;
	SLIST_INSERT_HEAD(&layers, layer, iter);
	return layer->id;
}

int platform_disp_set_act_layer(int id)
{
	int retid = id;
	platform_layer_t *l = NULL;
    platform_layer_head_t *head = &layers;
    SLIST_FOREACH(l, head, iter)
    {
        if (l->id == id)
        {
        	lua_lcd_bpp = l->bpp;
			lua_lcd_width = l->width;
			lua_lcd_height = l->height;
			workingbuffer = l->buf;
			retid = active_layer_id;
			active_layer_id = id;
            break;
        }
    }
	return retid;
}

void platform_disp_destroy_layer(int id)
{
	if (!id)
		return;
	platform_layer_t *l = NULL;
    platform_layer_head_t *head = &layers;
    SLIST_FOREACH(l, head, iter)
    {
        if (l->id == id)
        {
            break;
        }
    }
	if (l)
	{
		SLIST_REMOVE(head, l, platform_layer, iter);
		if (l->destroy)
			l->destroy(l);
	}
}

void platform_disp_refr_act_layer(PlatformRect *area)
{
	platform_layer_t *l = NULL;
    platform_layer_head_t *head = &layers;
    SLIST_FOREACH(l, head, iter)
    {
        if (l->id == active_layer_id)
        {
            break;
        }
    }

	if (l->update)
		l->update(area, l);
}

static void update_default_layer(PlatformRect *area, platform_layer_t *layer)
{
	platform_lcd_update(area, layer->buf);
}

static void fontInit(void)
{
    static BOOL inited = FALSE;

    if(inited)
        return;

    inited = TRUE;

#if defined(LUA_DISP_SUPPORT_HZ)
#if defined(FONT_HZ_COMPRESS)
    LzmaDecodeBufToBuf(sansHzFont16DataZip, sizeof(sansHzFont16DataZip), &sansHzFont16Data);
#endif

    sansHzFont16.data = sansHzFont16Data;
#else
    sansHzFont16.data = NULL;
#endif

#ifdef __PROJECT_AM_WATCHER__
    /*+New \ abysses \ 2016.05.18 \ '*/  
    sansHzFontPinyin.data = sansHzFontPinyiData;
    /*-New \ abysses \ 2016.05.18 \ æòôºnsicnèn¬n*/  
#endif
    
    memset(dispFonts, 0, sizeof(dispFonts));

    dispFonts[0] = sansFont16;
#ifdef LUA_LVGL_SUPPORT
	lv_font_luat.line_height = sansFont16.height;
#endif
    dispHzFont = &sansHzFont16;
}

void platform_disp_init(PlatformDispInitParam *pParam)
{
    // OK§³èä16-Exce” ISOURY” orÆènè¯Æâ
    ASSERT(pParam->bpp == 16 || pParam->bpp == 1 || pParam->bpp == 24);

	static platform_layer_t default_layer;

    // lua_lcd_bpp = pParam->bpp;
    default_layer.bpp = pParam->bpp;

    // lua_lcd_width = pParam->width;
    default_layer.width = pParam->width;

    if(default_layer.bpp == 1) 
    {
        default_layer.height = (pParam->height%8 == 0) ? pParam->height : (pParam->height/8 + 1) * 8;
    } 
    else 
    {
        default_layer.height = pParam->height;
    }

    // · Öåäïôê¾ »º³åçø
#if 0
    framebuffer = L_MALLOC(lua_lcd_width*lua_lcd_height*lua_lcd_bpp/8);
#endif
    
/*+\ New \ 2013.4.10 \ ° ° ° °*/
    pParam->framebuffer = workingbuffer;
    default_layer.buf = (void *)workingbuffer;
	default_layer.update = update_default_layer;
/*+\czm\2020.9.11\initÖ»³õÊ¼»¯Ò»´ÎÍ¼²ã*/
    static bool disp_init_one = FALSE;
    if(disp_init_one == FALSE)
    {
        int l_id = platform_disp_create_layer(&default_layer);
        ASSERT(l_id == 0);
        disp_init_one = TRUE;
    }
    platform_disp_set_act_layer(0);
/*-\czm\2020.9.11\initÖ»³õÊ¼»¯Ò»´ÎÍ¼²ã*/
	/*+\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
	frameBufferSize = lua_lcd_width*lua_lcd_height*lua_lcd_bpp/8;
	/*-\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
	
	lcd_bus = pParam->bus;

/*-\ New \ 2013.4.10 \ ° ° ° °*/

/*+\ BUG0 \ ZHY \ 2014.10.14 \ ºÚ ° × æÁä¬ÈÏÎª ºÚµ × ° × æÁ*/
    if(lua_lcd_bpp == 1)
    {
        disp_bkcolor = COLOR_WHITE_1;
        disp_color = COLOR_BLACK_1;

/*+\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
        if(pParam->hwfillcolor != -1){
            lcd_hwfillcolor = pParam->hwfillcolor;
            if(lcd_hwfillcolor > COLOR_WHITE_1)
                lcd_hwfillcolor = COLOR_WHITE_1;
        }
/*-\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
    }
    else if(lua_lcd_bpp == 16)
    {
        disp_bkcolor = COLOR_WHITE_16;
        disp_color = COLOR_BLACK_16;
    }
    else if(lua_lcd_bpp == 24)
    {
        disp_bkcolor = COLOR_WHITE_24;
        disp_color = COLOR_BLACK_24;
    }/*-\ BUG0 \ ZHY \ 2014.10.14 \ ºÚ ° × æÁä¬ÈÏÎª ºÚµ × ° × æÁ*/

    fontInit();

    // ³õÊ¼»¯lcdÉè±¸
    platform_lcd_init(pParam);

    layer_info[BASIC_LAYER_ID].buffer = workingbuffer;
    layer_info[BASIC_LAYER_ID].src_rect.ltX = 0;
    layer_info[BASIC_LAYER_ID].src_rect.ltY = 0;
    layer_info[BASIC_LAYER_ID].src_rect.rbX = lua_lcd_width - 1;
    layer_info[BASIC_LAYER_ID].src_rect.rbY = lua_lcd_height - 1;
    layer_info[BASIC_LAYER_ID].disp_rect = layer_info[BASIC_LAYER_ID].src_rect;
    layer_info[BASIC_LAYER_ID].clip_rect = layer_info[BASIC_LAYER_ID].src_rect;

    layer_info[BASIC_LAYER_ID].color_format = OPENAT_COLOR_FORMAT_24;
    
    layer_info[USER_LAYER_1_ID].buffer = NULL;
    layer_info[USER_LAYER_2_ID].buffer = NULL;

    active_layer_id = BASIC_LAYER_ID;
}

void platform_disp_close(void)
{
/*+\bug2958\czm\2020.9.1\disp.close() Ö®ºóÔÙÖ´ÐÐdisp.init ÎÞÌeÊ¾Ö±½ÓÖØÆô*/
    // if(framebuffer != NULL)
    // {
    //     L_FREE(framebuffer);
    // }

    platform_lcd_close();
/*-\bug2958\czm\2020.9.1\disp.close() Ö®ºóÔÙÖ´ÐÐdisp.init ÎÞÌeÊ¾Ö±½ÓÖØÆô*/
}

void platform_disp_clear(void)
{
/*+\ New \ 2013.4.10 \ ° ° ° °*/
    if(lua_lcd_bpp == 1)
    {
/*+\ bug0 \ zhy \ 2014.10.14 \ ° ° × æeéèöã ± ³¾ ° ° ðþ*/
/*+\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
        if(disp_bkcolor^lcd_hwfillcolor) // óëìî³äé «²» ò »ñù¾n²» ìî³ä
/*-\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
            memset(workingbuffer, 0x00, lua_lcd_width*lua_lcd_height*lua_lcd_bpp/8);
        else // óëìî³äé «ò» Öâ¾nìî³ä
            memset(workingbuffer, 0xFF, lua_lcd_width*lua_lcd_height*lua_lcd_bpp/8);
/*-\ bug0 \ zhy \ 2014.10.14 \ ° ° × æeéèöã ± ³¾ ° ° ðþ*/
    }
    else if(lua_lcd_bpp == 16)
/*-\ New \ 2013.4.10 \ ° ° ° °*/
    {
        u16 *pPixel16;
        u16 row,col;

        
        pPixel16 = (u16*)workingbuffer;
        
        for(col = 0; col < lua_lcd_width; col++)
        {
            for(row = 0; row < lua_lcd_height; row++)
            {
                pPixel16[row*lua_lcd_width + col] = disp_bkcolor;
            }
        }
    }
    else if(lua_lcd_bpp == 24)
    {
        u8 *pPixel24;
        u16 row,col;
        
        pPixel24 = (u8*)workingbuffer;

				
        for(col = 0; col < lua_lcd_width; col++)
        {
            for(row = 0; row < lua_lcd_height; row++)
            {
                memcpy(&pPixel24[(row*lua_lcd_width + col) * 3], &disp_bkcolor, 3);
            }
        }
    }
}

/*+\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 
void platform_disp_new(PlatformDispInitParam *pParam)
{
    // OK§³èä16-Exce” ISOURY” orÆènè¯Æâ
    ASSERT(pParam->bpp == 16 || pParam->bpp == 24 || pParam->bpp == 1);

	lua_lcd_bpp = pParam->bpp;
    lua_lcd_width = pParam->width;
     if(lua_lcd_bpp == 1) 
    {
        lua_lcd_height = (pParam->height%8 == 0) ? pParam->height : (pParam->height/8 + 1) * 8;
    } 
    else 
    {
       lua_lcd_height = pParam->height;
    }
    

#ifndef AM_LAYER_SUPPORT
    // · Öåäïôê¾ »º³åçø
    //framebuffer = (u8*)((u32)malloc(lcd_width*lcd_height*lcd_bpp/8) | 0xa0000000);
    frameBufferSize = lua_lcd_width*lua_lcd_height*lua_lcd_bpp/8;
#endif

	if(frameBufferSize > MAX_LCD_BUFF_SIZE)
	{
		platform_assert(__FUNCTION__, __LINE__);
	}

/*+\ New \ 2013.4.10 \ ° ° ° °*/
    pParam->framebuffer = workingbuffer;
/*-\ New \ 2013.4.10 \ ° ° ° °*/

/*+\ BUG0 \ ZHY \ 2014.10.14 \ ºÚ ° × æÁä¬ÈÏÎª ºÚµ × ° × æÁ*/
    if(lua_lcd_bpp == 1)
    {
        disp_bkcolor = COLOR_WHITE_1;
        disp_color = COLOR_BLACK_1;

/*+\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
        if(pParam->hwfillcolor != -1){
            lcd_hwfillcolor = pParam->hwfillcolor;
        }
/*-\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
    }
    else if(lua_lcd_bpp == 16)
    {
        disp_bkcolor = COLOR_WHITE_16;
        disp_color = COLOR_BLACK_16;
    }
    else if(lua_lcd_bpp == 24)
    {
        disp_bkcolor = COLOR_WHITE_24;
        disp_color = COLOR_BLACK_24;
    }/*-\ BUG0 \ ZHY \ 2014.10.14 \ ºÚ ° × æÁä¬ÈÏÎª ºÚµ × ° × æÁ*/

    fontInit();

    // ³õÊ¼»¯lcdÉè±¸
    //platform_lcd_init(pParam);
}

int platform_disp_get()
{
    //buf = framebuffer;
	return frameBufferSize;
}
/*-\BUG2739\lijiaodi\2020.08.06\Ìn¼Ódisp.new disp.getframe½Ó¿ Ú\*/ 

void platform_disp_update(void)
{
    PlatformRect rect;
    
    rect.ltx = 0;
    rect.lty = 0;
    rect.rbx = lua_lcd_width-1;
    rect.rby = lua_lcd_height-1;
    
    platform_disp_refr_act_layer(&rect);
}

/*+\ New \ 2013.4.10 \ ° ° ° °*/
static void disp_bitmap_bpp1(const DispBitmap *pBitmap, u16 startX, u16 startY)
{
    u16 bx,by,x,y,page,bwbytes;
    u16 endX, endY;
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  	
    u16 srcbx, srcby;

    if(pBitmap->bpp != 1)
    {
        printf("[disp_bitmap_bpp1]: not support bpp %d", pBitmap->bpp);
        return;
    }
    
    endX = MIN(lua_lcd_width,startX + pBitmap->width);
    endY = MIN(lua_lcd_height,startY + pBitmap->height);

    //bwbytes = (pBitmap->width+7)/8;
    bwbytes = (pBitmap->orgWidth+7)/8;

    for(x = startX,bx = 0; x < endX; x++,bx++)
    {
        srcbx = pBitmap->zoomRatio*bx;
        for(y = startY,by = 0; y < endY; y++,by++)
        {
            srcby = pBitmap->zoomRatio*by;
            page = y/8;
/*+\ bug0 \ zhy \ 2014.10.14 \ ° ° × æeéèöãç ° ¾ ° ° ðþção*/
            if((disp_color^lcd_hwfillcolor) == 0) /*\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
            {
                        if(pBitmap->data[bwbytes*srcby+srcbx/8]&(0x80>>(srcbx%8)))
                        {
                            workingbuffer[page*lua_lcd_width+x] |= 1<<(y%8);
                        }
                        else
                        {
                            //framebuffer[page*lcd_width+x] &= ~(1<<(y%8));
                        }
            }
            else
            {
                        if(pBitmap->data[bwbytes*srcby+srcbx/8]&(0x80>>(srcbx%8)))
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  						
                        {
                             workingbuffer[page*lua_lcd_width+x] &= ~(1<<(y%8));
                        }
                        else
                        {                  
                            //framebuffer[page*lcd_width+x] |= 1<<(y%8);
                        }
            }
/*-\ bug0 \ zhy \ 2014.10.14 \ ° ° × æeéèöãç ° ¾ ° ° ðþção*/
        }
    }
}

static void disp_1bitbmp_bpp16(const DispBitmap *pBitmap, u16 startX, u16 startY)
{
    u16 bx,by,x,y,bwbytes;
    u16 endX, endY;
    u16 *buffer16 = (u16*)workingbuffer;
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  	
    u16 srcbx, srcby;

    ASSERT(pBitmap->bpp == 1);
    
    endX = MIN(lua_lcd_width,startX + pBitmap->width);
    endY = MIN(lua_lcd_height,startY + pBitmap->height);
    
    //bwbytes = (pBitmap->width+7)/8;
    bwbytes = (pBitmap->orgWidth+7)/8;
    
    for(x = startX,bx = 0; x < endX; x++,bx++)
    {
        srcbx = pBitmap->zoomRatio*bx;
        for(y = startY,by = 0; y < endY; y++,by++)
        {            
            srcby = pBitmap->zoomRatio*by;
            if(pBitmap->data[bwbytes*srcby+srcbx/8]&(0x80>>(srcbx%8)))
            {
                // ³õé «
                buffer16[y*lua_lcd_width + x] = disp_color;
            }
            else
            {
                // Ìî³ä ± ³¾ ° ° «
                // Ö ± ½óµþ¼óïôêcci, ôý² »Ö§³ö ± ³¾ °« Éèöã
                //buffer16[y*lcd_width + x] = disp_bkcolor;
            }
        }
    }
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  	
}
static void disp_1bitbmp_bpp24(const DispBitmap *pBitmap, u16 startX, u16 startY)
{
    u16 bx,by,x,y,bwbytes;
    u16 endX, endY;
    u8 *buffer24 = (u8*)workingbuffer;
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  	
    u16 srcbx,srcby;

    ASSERT(pBitmap->bpp == 1);
    
    endX = MIN(lua_lcd_width,startX + pBitmap->width);
    endY = MIN(lua_lcd_height,startY + pBitmap->height);
    
    bwbytes = (pBitmap->orgWidth+7)/8;
    
    for(x = startX,bx = 0; x < endX; x++,bx++)
    {
        srcbx = pBitmap->zoomRatio*bx;
        for(y = startY,by = 0; y < endY; y++,by++)
        {            
            srcby = pBitmap->zoomRatio*by;
            if(pBitmap->data[bwbytes*srcby+srcbx/8]&(0x80>>(srcbx%8)))
            {
                // ³õé «
                memcpy(&buffer24[(y*lua_lcd_width + x) * 3], &disp_color, 3);
            }
            else
            {
                // Ìî³ä ± ³¾ ° ° «
                // Ö ± ½óµþ¼óïôêcci, ôý² »Ö§³ö ± ³¾ °« Éèöã
                //buffer16[y*lcd_width + x] = disp_bkcolor;
            }
        }
    }
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  	
}

/*-\ New \ 2013.4.10 \ ° ° ° °*/

static void getFontBitmap(DispBitmap *pBitmap, u16 charcode)
{
    const FontInfo *pInfo = &dispFonts[curr_font_id];

    pBitmap->bpp = 1;
    pBitmap->width = pInfo->width;
    pBitmap->height = pInfo->height;

    if(pInfo->data)
    {
        if(charcode >= pInfo->start && charcode <= pInfo->end)
        {
            u32 index = charcode - pInfo->start;

            pBitmap->data = pInfo->data + index*pInfo->size;            
        }
        else
        {
            pBitmap->data = blankChar;
        }
    }
    else
    {
        pBitmap->data = blankChar;
    }
}

#ifdef __PROJECT_AM_WATCHER__
/*+New \ abysses \ 2016.05.18 \ '*/  
static void getPinyinFontBitmap(DispBitmap *pBitmap, u16 charcode)
{
    const FontInfo *pInfo = &sansHzFontPinyin;

    pBitmap->bpp = 1;
    pBitmap->width = pInfo->width;
    pBitmap->height = pInfo->height;

    if(pInfo->data)
    {
        if(charcode >= pInfo->start && charcode <= pInfo->end)
        {
            u32 index = charcode - pInfo->start;

            pBitmap->data = pInfo->data + index*pInfo->size;            
        }
        else
        {
            pBitmap->data = blankChar;
        }
    }
    else
    {
        pBitmap->data = blankChar;
    }
}
/*-New \ abysses \ 2016.05.18 \ æòôºnsicnèn¬n*/  
#endif

static void getHzBitmap(DispBitmap *pBitmap, u16 charcode)
{
    const FontInfo *pInfo = dispHzFont;

    pBitmap->bpp = 1;
    pBitmap->width = pInfo->width;
    pBitmap->height = pInfo->height;

    if(pInfo->data)
    {
        u8 byte1, byte2;
        u32 index;

        byte1 = charcode>>8;
        byte2 = charcode&0x00ff;

        if(byte1 >= 0xB0 && byte1 <= 0xF7 &&
            byte2 >= 0xA1 && byte2 <= 0xFE)
        {
            index = (byte1 - 0xB0)*(0xFE - 0xA1 + 1) + byte2 - 0xA1;
			/*+\BUG\wangyuan\2020.07.24\BUG_2657£ºui×Ö¿â´nÎ»£¬ÏÔÊ¾³öÀ´µÄÎÄ×Ö²»ÕýÈ·*/
			#if 0
            if(byte1 > 0xD7)
            {
                index -= 5; /*D7FA-D7FE*/
            }
            #endif
			/*-\BUG\wangyuan\2020.07.24\BUG_2657£ºui×Ö¿â´nÎ»£¬ÏÔÊ¾³öÀ´µÄÎÄ×Ö²»ÕýÈ·*/
            pBitmap->data = pInfo->data + index*pInfo->size;
        }
        else
        {
            pBitmap->data = blankChar;

        /*+ \ New \ liWeiqianG \ 2013.12.18 \ Ôö¼öðîä ± Û ºå ÛºåμÄÊ¾Ö§ôÖï*/
            for(index = 0; index < sizeof(sansHzFont16ExtOffset)/sizeof(u16); index++)
            {
                if(charcode < sansHzFont16ExtOffset[index])
                {
                    break;
                }

                if(charcode == sansHzFont16ExtOffset[index])
                {
                    pBitmap->data = sansHzFont16ExtData + index*pInfo->size;
                    break;
                }
            }
        /*- \ new \ LIWEIQIANG \ 2013.12.18 \ ÔÖ¼öðÎÄ ± Ûμã ÛºåμÄïÊ¾Ö¾§³Ö*/
        }
    }
    else
    {
        pBitmap->data = blankChar;
    }
}

static void getCharBitmap(DispBitmap *pBitmap, u16 charcode)
{
#ifdef __PROJECT_AM_WATCHER__
/*+NEW\2016.05.05.*/  
    if(charcode >= 0xa8a1 && charcode <= 0xa8bf)    
	{        //pinyin
        getPinyinFontBitmap(pBitmap, charcode);
    }
    else
#endif
        if(charcode >= 0x80A0)
/*-NEW\05.05.05.18.18.1*/  	
    {
        getHzBitmap(pBitmap, charcode);
    }
    else
    {
        getFontBitmap(pBitmap, charcode);
    }
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/
    pBitmap->orgHeight = pBitmap->height;
    pBitmap->orgWidth  = pBitmap->width;
    pBitmap->zoomRatio = (float)pBitmap->height/g_s_fontZoomSize;

    pBitmap->height = g_s_fontZoomSize;
    pBitmap->width = (u16)((float)pBitmap->width/pBitmap->zoomRatio);
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/      
}

#if 0 // çå³ý¾øðîçø½ó
static void disp_clear_rect(int ltx, int lty, int rbx, int rby)
{
    u16 *buffer16 = (u16*)framebuffer;
    int x_pos, y_pos;

    ltx = ltx >= lcd_width ? ltx : lcd_width;
    lty = lty >= lcd_height ? lty : lcd_height;
    rbx = rbx >= lcd_width ? rbx : lcd_width;
    rby = rby >= lcd_height ? rby : lcd_height;

    for(x_pos = ltx; x_pos < rbx; x_pos++)
        for(y_pos = lty; y_pos < rby; y_pos++)
            buffer16[y_pos*lcd_width + x_pos] = disp_bkcolor;
}
#endif

// èðºãfonts ==============================================================================================================

/************************* û tñèón ¢μèón ¢ *********************/
#if VERSION_CHINESE == ON
/*»Luxury¡ ñègb2312 ¥ μčó μåño ¢*/
static u32 rh_getGB2312Index(u16 gb2312code)
{
    u32 index = -1;
    u8 region = (gb2312code >> 8) & 0xFF; // çkâ
    u8 posit = gb2312code & 0xFF; //L ours âу
    if((region >= 0xA1 && region <= 0xFE) && (posit >= 0xA1 && posit <= 0xFE))
    //if((region >= 0xB0 && region <= 0xF7) && (posit >= 0xA1 && posit <= 0xFE))
    {
        index = (region - 0xA1) * (0xFE - 0xA1 + 1) + posit - 0xA1;
        //index = (region - 0xB0) * (0xFE - 0xA1 + 1) + posit - 0xA1;
        //if(region > 0xD7) index -= 5; // D7FA-D7FE Õâ5¸öÊÇ¿ÕµÄ
        //index = sizeof(GUI_FONT_HEADER) + index * (32/8*32);
        index *= 128;
        //index *= (((32) + 7) / 8 * (32));
    }
    return index;
}
static void rh_getGB2312Bitmap(DispBitmap *pBitmap, u16 gb2312code)
{
    if(gb2312code >= 0xA0A0) // GB2312µÄÖµ´óÓÚ0xA0A0
    {
        const GUI_FONT_HEADER *pHeader_CJK = &fontHeader_CJK;
        pBitmap->bpp = 1; // ×ÖÏñËØÉî¶È
        pBitmap->width = pHeader_CJK->YSize; // ×Ö¿n
        pBitmap->height = pHeader_CJK->YSize; // ×Ö¸ß
        pBitmap->data = &fonts_data_CJK[rh_getGB2312Index(gb2312code)]; // ×ÖµãÕó
    }
    else
    {
        const GUI_FONT_HEADER *pHeader_Latin = &fontHeader_Latin;
        const GUI_FONT_INDEX *pIndex_Latin = &fonts_index_Latin[0];
        pBitmap->bpp = 1; // ×ÖÏñËØÉî¶È
        pBitmap->width = (pIndex_Latin+gb2312code)->width; // ×Ö¿n
        pBitmap->height = pHeader_Latin->YSize; // ×Ö¸ß
        pBitmap->data = &fonts_data_Latin[(pIndex_Latin+gb2312code)->offset]; // ×ÖµãÕó
    }
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  	
    // ôý ±
    pBitmap->orgHeight = pBitmap->height;
    pBitmap->orgWidth = pBitmap->width;
    pBitmap->zoomRatio = 1;
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  	
}
#elif VERSION_ENGLISH == ON
/*»Avoè¡μWelling × öμï*/
static void rh_getUnicodeBitmap(DispBitmap *pBitmap, unsigned long unicode)
{
    //const GUI_FONT_HEADER *pHeader = unicode_header;
    const GUI_FONT_HEADER *pHeader = &fontHeader;
    const GUI_FONT_INDEX *pIndex = &fonts_index[0];
    //const GUI_FONT_INDEX *pIndex = &unicode_index[0];
    pBitmap->bpp = 1; // ×ÖÏñËØÉî¶È
    pBitmap->width = (pIndex+unicode)->width; // ×Ö¿n
    pBitmap->height = pHeader->YSize; // ×Ö¸ß
    pBitmap->data = &fonts_data[(pIndex+unicode)->offset]; // ×ÖµãÕó
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  
    // ôý ±
    pBitmap->orgHeight = pBitmap->height;
    pBitmap->orgWidth = pBitmap->width;
    pBitmap->zoomRatio = 1;
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  	
}
#endif



/*************************************************************************************/

/******************************************************************************************.
 * zzy¦â€™s
 * » «I”µ×èÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂÂâÂ³”ÉÉ (UCS-2«UCS-4)±ÂÂ
 * ²-AS:
 * pIYâ öïâ »³âóâ, OLEUTF-8±aÂ
 * Unic Ö¸èÏoughâ³”³âó prior, Æüugñµ²´ïuggâ »nicdeµ²â€™tâ€™t, Augmented long.
 *
 * ³Éug´´µµµØµÃÃÃµÂÂÂÂÂÂÂÂÂÂÂÂânelÂâÂâÂèÃµnÕïïÕ½; â€ deassed”Øµ”Ø0.
 * ×¢give:
 *TF8Ã”çoñÐçÐculè½çoöçoöçoçoçoçoçoçoçoçUçUçUçUçUCUÐñÐcult«OWçoIcult.
 ×èço‟cult‟cult‟ço³³´ó³(s)´èov(Little Endian)Ágram;
 * ÔUTYug´¦²²´¶âS´uggn«uggn´, NA´è²¶â ¼¨uggnâ. (µÖ ´ earlyµë”)
 ********************************************************************************************.*/
static int rh_getUTF8Size(const unsigned char pInput)
{
    unsigned char c = pInput;
    if(c< 0x80) return 0;// 0xxxxxxx ·µ»Ø0
    if(c>=0x80 && c<0xC0) return -1;// 10xxxxxx ²»´æÔÚ
    if(c>=0xC0 && c<0xE0) return 2;// 110xxxxx ·µ»Ø2
    if(c>=0xE0 && c<0xF0) return 3;// 1110xxxx · M »Ø3
    if(c>=0xF0 && c<0xF8) return 4;// 11110xxx ·µ»Ø4
    if(c>=0xF8 && c<0xFC) return 5;// 111110xx · µ »Ø5
    if(c>=0xFC) return 6;// 1111110x; µ »Ø6
}
static int rh_convertUTF8toUnicode(const unsigned char *pInput, unsigned long *Unic)
{
    //assert(pInput != NULL && Unic != NULL);
    // b1 ± nê¾utf-8 ± àâëµäpinputöðµä¸ß x ö½ú, b2 ± nê¾´î¸ß x ö½ú, ...
    char b1, b2, b3, b4, b5, b6;
    int utfbytes = rh_getUTF8Size(*pInput);
    unsigned char *pOutput = (unsigned char *) Unic;
    *Unic = 0x0; // ° ñ * unic big »`îªè« Áãa

    switch ( utfbytes )
    {
        case 0:
            *pOutput     = *pInput;
            utfbytes    += 1;
            break;
        case 2:
            b1 = *pInput;
            b2 = *(pInput + 1); //  (b2 & 0xC0) != 0x80 
            if ( (b2 & 0xC0) != 0x80 )
			{
				*pOutput = 63; // {Îþ · Î½ÂÎ Û½ Û½ Î½ Ûóã? 'Ìæïôê¾
                return 1;
            }
			*pOutput     = (b1 << 6) + (b2 & 0x3F);
            *(pOutput+1) = (b1 >> 2) & 0x07;
            break;
        case 3:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) )
			{
				*pOutput = 63; // {Îþ · Î½ÂÎ Û½ Û½ Î½ Ûóã? 'Ìæïôê¾
                return 1;
            }
            *pOutput     = (b2 << 6) + (b3 & 0x3F);
            *(pOutput+1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
            break;
        case 4:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            b4 = *(pInput + 3);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) )
			{
				*pOutput = 63; // {Îþ · Î½ÂÎ Û½ Û½ Î½ Ûóã? 'Ìæïôê¾
                return 1;
            }
            *pOutput     = (b3 << 6) + (b4 & 0x3F);
            *(pOutput+1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
            *(pOutput+2) = ((b1 << 2) & 0x1C)  + ((b2 >> 4) & 0x03);
            break;
        case 5:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            b4 = *(pInput + 3);
            b5 = *(pInput + 4);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) )
			{
				*pOutput = 63; // {Îþ · Î½ÂÎ Û½ Û½ Î½ Ûóã? 'Ìæïôê¾
                return 1;
            }
            *pOutput     = (b4 << 6) + (b5 & 0x3F);
            *(pOutput+1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
            *(pOutput+2) = (b2 << 2) + ((b3 >> 4) & 0x03);
            *(pOutput+3) = (b1 << 6);
            break;
        case 6:
            b1 = *pInput;
            b2 = *(pInput + 1);
            b3 = *(pInput + 2);
            b4 = *(pInput + 3);
            b5 = *(pInput + 4);
            b6 = *(pInput + 5);
            if ( ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) || ((b6 & 0xC0) != 0x80) )
			{
				*pOutput = 63; // {Îþ · Î½ÂÎ Û½ Û½ Î½ Ûóã? 'Ìæïôê¾
                return 1;
            }
            *pOutput     = (b5 << 6) + (b6 & 0x3F);
            *(pOutput+1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
            *(pOutput+2) = (b3 << 2) + ((b4 >> 4) & 0x03);
            *(pOutput+3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
            break;
        default:
            *pOutput = 63; // {Îþ · Î½ÂÎ Û½ Û½ Î½ Ûóã? 'Ìæïôê¾
            utfbytes = 1;
            break;
    }
    return utfbytes;
}

/*GB2312תUnicode
static int UnicodeBinarySearch(unsigned int key,const unsigned int lookup_table[][2],unsigned int start,unsigned int end)
{
   while(start <= end)
   {
       unsigned int mid = (start + end)/2;
       if( key >  lookup_table[mid][0])
       {
           start = mid+1;
       }
       else if (key < lookup_table[mid][0])
       {
           end = mid - 1;
       }
       else //found
       {
          return mid;
       }
   }
   return -1;
}
static unsigned int GetUnicodeByGB2312(unsigned int gb2312)
{
   int index = UnicodeBinarySearch(gb2312,GB2312_Unicode,0,GB2312_UNICODE_LEN-1);
   if (index != -1)
   {
       return GB2312_Unicode[index][1];
   }
   return 0;
}*/
/**************** а× `,¡ d'e → _ ¡_Â 'UR URNE. _ Òö.*/
/*
static unsigned char rh_CharWidthToBytes(unsigned char num)
{
    if(num <= 8)  return 1;
    if(num <= 16) return 2;
    if(num <= 24) return 3;
    if(num <= 32) return 4;
    if(num <= 40) return 5;
    if(num <= 48) return 6;
    if(num <= 56) return 7;
    if(num <= 64) return 8;
    //return (num+7)/8;
}
*/
#if VERSION_CHINESE == ON
static unsigned char rh_getGB2312Width(u16 gb2312code)
{
    if(gb2312code >= 0xA0A0) // GB2312ÖÐÎÄµÄÖµ´óÓÚ0xA0A0
    {
        const GUI_FONT_HEADER *pHeader_CJK = &fontHeader_CJK;
        //return rh_CharWidthToBytes(pHeader_CJK->YSize); // ×Ö¿n
        return pHeader_CJK->YSize;
    }
    else
    {
        const GUI_FONT_INDEX *pIndex_Latin = &fonts_index_Latin[0];
        //return rh_CharWidthToBytes((pIndex_Latin+gb2312code)->width); // ×Ö¿n
        return (pIndex_Latin+gb2312code)->width;
    }
}
#elif VERSION_ENGLISH == ON
static unsigned char rh_getUnicodeWidth(unsigned long unicode)
{
    const GUI_FONT_INDEX *pIndex = &fonts_index[0];
    //return rh_CharWidthToBytes((pIndex+unicode)->width); // ×Ö¿n
    return (pIndex+unicode)->width;
}
#endif
u16 disp_getcharwidth(const char *string, u8 mode)
{
    int i;
    int len = strlen(string); // ×Ü×Ö½Ú³¤¶È
    u16 rst = 0;
    const u8 *pText = (const u8 *)string; // ´ýÏÔÊ¾µÄÎÄ×Ö
    u16 rh_GB2312code;
    unsigned long rh_unicode;

    for(i = 0; i < len; ) // öð × ö · Û Û
    {
#if VERSION_CHINESE == ON
        if(pText[i]&0x80)
        {
            if(pText[i+1]&0x80) // GB2312¼òÖÐ
            {
                rh_GB2312code = pText[i]<<8 | pText[i+1];
                i += 2;
            }
            else
            {
                rh_GB2312code = '?';
                i += 1;
            }
        }
        else // ASCII
        {
            rh_GB2312code = pText[i];
            i += 1;
        }
        rst += rh_getGB2312Width(rh_GB2312code);
#elif VERSION_ENGLISH == ON
        i = i + rh_convertUTF8toUnicode(&pText[i],&rh_unicode);
        rh_unicode -= 0x20; // ASCIIDIDIDIC.
        rst += rh_getUnicodeWidth(rh_unicode);
#endif
    }

    return rst;
}
/***************************************************************************/
// ÏÔÊ¾ÎÄ×Ö

#ifdef __PROJECT_RUIHENG_WATCHER__
u16* platform_disp_puttext(const char *string, u16 x, u16 y)
{
    int i;
    DispBitmap bitmap;
    u16 display_x, display_y;
    int len = strlen(string); // ×Ü×Ö½Ú³¤¶È
    const u8 *pText = (const u8 *)string; // ´ýÏÔÊ¾µÄÎÄ×Ö
    u16 rh_GB2312code, cutlr=0, offset[2]={0};
    unsigned long rh_unicode; //

    // ÏÔÊ¾×ø±ê³ö½ç
    if(x >= lua_lcd_width)  x = 0;
    if(y >= lua_lcd_height) y = 0;

    for(i = 0; i < len; ) // Öð×Ö·ûÏÔÊ¾
    {
        /*»»ÐÐ´¦Àn
          0x0D -- '\r'
          0x0A -- '\n'*/
        if(pText[i] == 0x0D && pText[i+1] == 0x0A) {
            i+=2; offset[1]++;
            if(cutlr == 1) {
              cutlr++; goto next_char;
            } else {
              goto next_line;
            }
        } else if(pText[i] == 0x0D || pText[i+1] == 0x0A) {
            i++;
            if(cutlr == 1) {
              cutlr++; goto next_char;
            } else {
              goto next_line;
            }
        }
        cutlr=0;
        /*¶ÁÈ¡×Ö·ûÊý¾Ý*/
#if VERSION_CHINESE == ON
        /* GB2312 */
        if(pText[i]&0x80)
        {
            if(pText[i+1]&0x80) // GB2312¼òÌåÖÐÎÄ
            {
                rh_GB2312code = pText[i]<<8 | pText[i+1];
                i += 2;
            }
            else
            {
                rh_GB2312code = '?';
                i += 1;
            }
            //rh_unicode = GetUnicodeByGB2312(rh_GB2312code);
        }
        else // ASCII
        {
            rh_GB2312code = pText[i];
            i += 1;
            //i = i + rh_convertUTF8toUnicode(&pText[i],&rh_unicode);
        }
        rh_getGB2312Bitmap(&bitmap,rh_GB2312code);
        //mmi_chset_convert(MMI_CHSET_GB2312,MMI_CHSET_UCS2,(char *)(&rh_GB2312code),(char *)(&rh_unicode),sizeof(&rh_unicode));
        //rh_unicode = GetUnicodeByGB2312(rh_GB2312code);
#elif VERSION_ENGLISH == ON
        /*UTF-8תUnicode*/
        //mmi_chset_convert(MMI_CHSET_UTF8,MMI_CHSET_UCS2,(char *)(&pText[i]),(char *)(&rh_unicode),sizeof(&rh_unicode));
        i = i + rh_convertUTF8toUnicode(&pText[i],&rh_unicode);
        rh_unicode -= 0x20; // ASCIIDIDIDIC.
        rh_getUnicodeBitmap(&bitmap,rh_unicode);
#endif
        /*ÎÄ×ÖÏÔÊ¾×ø±ê´¦Àn*/
        display_x = x;
        display_y = y;
        if(lua_lcd_bpp == 1)
        {
            if(bitmap.width == 14 && bitmap.height == 14)
            {
                bitmap.width = 16;
                bitmap.height = 16;
                display_x = x + 1;
                display_y = y + 1;
            }
        }
        if(display_x + bitmap.width > lua_lcd_width)
        {
            display_y += bitmap.height;
            display_x = 0;        
        }
        /*¸ù¾ÝÏñËØÉî¶ÈÏÔÊ¾ÎÄ×Ö*/
        if(lua_lcd_bpp == 1)
        	disp_bitmap_bpp1(&bitmap, display_x, display_y);
        else if(lua_lcd_bpp == 16)
        	disp_1bitbmp_bpp16(&bitmap, display_x, display_y);
        else if(lua_lcd_bpp == 24)
        	disp_1bitbmp_bpp24(&bitmap, display_x, display_y);
        /*×ø±ê¸üÐÂ*/
        x = display_x;
        y = display_y;

        if(x + bitmap.width >= lua_lcd_width) {
           cutlr=1;
           goto next_line;
        } else {
           x += bitmap.width;
           goto next_char;
        }
        
next_line:
        y += bitmap.height;
        x = 0;

next_char:
        // × ¯ ¯ »
        if( y >= lua_lcd_height)
        {
            y = 0;
            break;
        }
    }

    offset[0] = i;

    return offset;
}
void platform_disp_puttext_rh(const char *string, u16 x, u16 y , u16 charset)
{
	//
}
#else
u16* platform_disp_puttext(const char *string, u16 x, u16 y)
{
    int len = strlen(string);
    const u8 *pText = (const u8 *)string;
    int i;
    DispBitmap bitmap;
    u16 charcode,cutlr=0, offset[2]={0};
    u16 display_x, display_y;
    
    if(x >= lua_lcd_width)
        x = 0;

    if(y >= lua_lcd_height)
        y = 0;

    for(i = 0; i < len; )
    {
        /*
          0x0D -- '\r'
          0x0A -- '\n'
        */
        if(pText[i] == 0x0D && pText[i+1] == 0x0A){
            i+=2; offset[1]++;
            if(cutlr == 1)
              { cutlr++; goto next_char; }
            else
              { goto next_line; }
        } else if(pText[i] == 0x0D  || pText[i] == 0x0A) {
            i++;
            if(cutlr == 1)
              { cutlr++; goto next_char; }
            else
              { goto next_line; }
        }
        cutlr=0;
/*+NEW\2016.05.05.*/          
        if(pText[i] > 0x81)
        {
            if(1)//(pText[i+1]&0x80)
/*-NEW\05.05.05.18.18.1*/  			
            {
                // gb2312 chinese char
                charcode = pText[i]<<8 | pText[i+1];
                i += 2;
            }
            else
            {
                charcode = '?';
                i += 1;
            }
        }
        else
        {
            // ascii char
            charcode = pText[i];
            i += 1;
        }

        getCharBitmap(&bitmap, charcode);
        
        display_x = x;
        display_y = y;
  
        if(lua_lcd_bpp == 1)
        {
            if(bitmap.width == 14 && bitmap.height == 14)
            {
                bitmap.width = 16;
                bitmap.height = 16;
                display_x = x+1;
                display_y = y+1;
            }
        }

        if(display_x + bitmap.width > lua_lcd_width)
        {
            display_y += bitmap.height;
            display_x = 0;        
        }

        if(lua_lcd_bpp == 1)
            disp_bitmap_bpp1(&bitmap, display_x, display_y);
        else if(lua_lcd_bpp == 16)
            disp_1bitbmp_bpp16(&bitmap, display_x, display_y);
        else if(lua_lcd_bpp == 24)
            disp_1bitbmp_bpp24(&bitmap, display_x, display_y);

        x = display_x;
        y = display_y;
        
        if(x + bitmap.width >= lua_lcd_width) {
           cutlr=1;
           goto next_line;
        } else {
           x += bitmap.width;
           goto next_char;
        }
next_line:   
        // ×Ô¶¯»»ÐÐÏÔÊ¾
        y += bitmap.height;
        x = 0;
      
next_char:
        // × ¯ ¯ »
        if( y >= lua_lcd_height) {
            y = 0;
            break;
        }
    }
    offset[0] = i;

    return offset;
}

#endif

/*+\NEW\liweiqiang\2013.11.4\Ôö¼ÓBMPÍ¼Æ¬ÏÔÊ¾Ö§³Ö*/
/*	Support for BMP files		*/
#ifdef WIN32
#define __attribute__(x)
#endif

#ifdef WIN32 
#pragma pack(push,pack1,1)
#endif
typedef struct _bitmap_file_header
{
    u16 file_type;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 bitmap_offset;
} __attribute__((packed)) bitmap_file_header ;

typedef struct _bitmap_info_header
{
    u32 header_size;
    u32 width;
    u32 height;
    u16 number_of_planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 bitmap_size;
    u32 device_width;
    u32 device_height;
    u32 number_of_colors;
    u32 number_of_important_colors;
} __attribute__((packed)) bitmap_info_header ;
#ifdef WIN32 
#pragma pack(pop,pack1)
#endif

#define RGB24ToRGB16(r,g,b) (((r&0x00f8)<<8)|((g&0x00fc)<<3)|((b&0xf8)>>3))
#define RGB16ToRGB24(rgb16) (( ((rgb16>>8)&0x00f8)<<16)|( ((rgb16>>3)&0x00fc)<<8)|((rgb16<<3)&0x00f8))

/*+\NEW\liweiqiang\2013.12.6\Ö§³Öbpp16µÄbmpÏÔÊ¾*/
/*+\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
int put_bmp_file_buff(const u8 *bitmap_buffer, int x, int y, int transcolor, int left, int top, int right, int bottom, T_AMOPENAT_IMAGE_INFO *info)
/*-\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
{
    bitmap_file_header *p_fileHeader = (bitmap_file_header *)bitmap_buffer;
    bitmap_info_header *p_infoHeader = (bitmap_info_header *)(bitmap_buffer+sizeof(bitmap_file_header));
    const u8 *data_buf;
    u16 data_r,data_g,data_b;
    u16 width, height;
    u16 rowIndex,colIndex;
    u16 bitmapRowBytes;
    u16 rgb16;
    u16 real_width;
/*+\NEW\liweiqiang\2013.11.12\ÐÞÕýÏÔÊ¾Í¼Æ¬x,y×ø±êÎÞ·¨ÉèÖÃ*/
    int bitmapRowIndex,bmpColIndex;       
    u32 data_index;

    u16 *buffer16 = (u16*)workingbuffer;
    u16 bmp_bpp = p_infoHeader->bits_per_pixel;
	u16 lcd_width = lua_lcd_width;

	if (!info)
	{
    	/*+\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
    	if((left > right) || (top > bottom))
    	{
        	printf("put_bmp_buffer: rect error\n");
        	return PLATFORM_ERR;
    	}

    	if((left == 0) && (top == 0) && (right == 0) && (bottom == 0))
    	{
        	width = MIN(p_infoHeader->width + x, lua_lcd_width);
        	height = MIN(p_infoHeader->height + y, lua_lcd_height);  
        	bottom = p_infoHeader->height - 1;
        	real_width = MIN(p_infoHeader->width, lua_lcd_width); 
    	}
    	else
    	{
        	width = MIN(right - left + 1 + x, lua_lcd_width);
        	height = MIN(bottom - top + 1 + y, lua_lcd_height); 
        	real_width = MIN(right - left + 1, lua_lcd_width);
    	}
    	/*-\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
	}
	else
	{
		width = p_infoHeader->width;
		height = p_infoHeader->height;
		info->width = width;
		info->height = height;
		u32 bsize = width * height * lua_lcd_bpp / 8;
		info->buffer = L_MALLOC(bsize);
		if (!info->buffer)
		{
			return PLATFORM_ERR;
		}
		buffer16 = info->buffer;
		memset(buffer16, 0xff, bsize);
		real_width = width;
		bottom = height - 1;
		lcd_width = width;
	}

    data_buf = bitmap_buffer + p_fileHeader->bitmap_offset;
    
    bitmapRowBytes = ((p_infoHeader->width*bmp_bpp + 31)&~31)>>3; //4×Ö½Ú¶ÔÆë
    
    /*+\ New \ lowiiqiaqiang \ 2014.9.9 \ a ôö¼ó ° × imageôl ° ° × æEEZY*/
    if (lua_lcd_bpp == 1 && bmp_bpp == 1)
    {
        int page;
        u8 lcdfill = lcd_hwfillcolor == COLOR_WHITE_1 ? 1:0;
        u8 bmpfill = 1;
        u8 finalfill;

        if (p_infoHeader->number_of_planes == 1)
        {
            const u8 *fill_plate_p = bitmap_buffer \
                + sizeof(bitmap_file_header) /*Ì ì î · ·*/ \
                + sizeof(bitmap_info_header) /*Ìø¹ýðåï ¢ n ·*/ \
                + 4 /*Ì ì ì ÷ ÷ é «° å ¢ ¢*/;

            if(fill_plate_p[0] == 0xff && fill_plate_p[1] == 0xff && fill_plate_p[2] == 0xff){
                // 1Öµµ×É ‘°â€™Ââ«f ±BORâ™ ¼µ¼µug´¶
                bmpfill = 1;
            } else {
                bmpfill = 0;
            }
        }

        // lectedμäìî di «ão «é у½« "τäoòo öа½¹
        finalfill = ((lcdfill^bmpfill) == 0) ? 0x80 : 0x00;

        for(rowIndex = y, bitmapRowIndex = p_infoHeader->height - top - 1; 
            rowIndex < height && bitmapRowIndex >= p_infoHeader->height - bottom - 1;
            rowIndex++, bitmapRowIndex--)
        {
            page = rowIndex/8;

            for(colIndex = x, bmpColIndex = left; colIndex < width; colIndex++, bmpColIndex++)
            {
                /*Èç¹û¸ãµãîªðèòªìî³ä ² ¢ çòî »n¼ìî³äé« óëlcdìî³äé «ò» Öâ ôòìî³ä*/
                if(0 == ((data_buf[bitmapRowBytes*bitmapRowIndex+bmpColIndex/8]&(0x80>>(bmpColIndex%8)))^(finalfill>>(bmpColIndex%8))))
                    workingbuffer[page*lua_lcd_width+colIndex] |= 1<<(rowIndex%8);
                /*· Ñ ìî³*/
                else
                    workingbuffer[page*lua_lcd_width+colIndex] &= ~(1<<(rowIndex%8));
            }
        }

        return PLATFORM_OK;    
    }
    

    if(bmp_bpp != 24 && bmp_bpp != 16)
    {
        printf("put_bmp_buffer: bmp not support bpp %d\n", bmp_bpp);
        return PLATFORM_ERR;
    }
    /*-\ New \ liweiqiaqiang \ 2014.9.9 \ a ôö¼ó ° × imageôl ° ° × æEEZY*/

    if(lua_lcd_bpp == 16)
    {
        /*+\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
        for(rowIndex = y, bitmapRowIndex = p_infoHeader->height - top - 1; 
            rowIndex < height && bitmapRowIndex >= p_infoHeader->height - bottom - 1;
            rowIndex++, bitmapRowIndex--)
        {
            for(colIndex = x, bmpColIndex = left; colIndex < width; colIndex++, bmpColIndex++)
        /*-\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
            {
                data_index = bitmapRowIndex*bitmapRowBytes + bmpColIndex*bmp_bpp/8;

                if(24 == bmp_bpp)
                {
                    data_r = data_buf[data_index+2];
                    data_g = data_buf[data_index+1];
                    data_b = data_buf[data_index];
                    /*+\NEW\liweiqiang\2013.11.12\°×É«ÇøÓòÍ¸Ã÷ÏÔÊ¾*/
                    rgb16 = RGB24ToRGB16(data_r,data_g,data_b);
                }
                else
                {
                    rgb16 = *(u16*)&data_buf[data_index];
                }

    /*+ \ NEW \ LIWEIQIANG \ 2013.12.6 {Urinkink toæ2 «« Luxemburn Search*/
                if(-1 == transcolor || rgb16 != transcolor)
    /*-\ NEW \ Lirowei Qiagang \ 2013.12.6 {Urinkink toæ2 «« Luxemburc sofi ie "*/
                {
                    buffer16[rowIndex*lcd_width+colIndex] = rgb16;
                }
                /*-\NEW\liweiqiang\2013.11.12\°×É«ÇøÓòÍ¸Ã÷ÏÔÊ¾*/
            }
        }
    }
    else
    {
        for(rowIndex = y, bitmapRowIndex = p_infoHeader->height - top - 1; 
            rowIndex < height && bitmapRowIndex >= p_infoHeader->height - bottom - 1;
            rowIndex++, bitmapRowIndex--)
        {
            memcpy(&workingbuffer[(rowIndex *lua_lcd_width + x)* 3],
                &data_buf[bitmapRowIndex * bitmapRowBytes  + left * 3],
                real_width * 3);
        }
        
    }
/*-\NEW\liweiqiang\2013.11.12\ÐÞÕýÏÔÊ¾Í¼Æ¬y×ø±êÎÞ·¨ÉèÖÃ*/

    return PLATFORM_OK;
}
/*-\NEW\liweiqiang\2013.12.6\Ö§³Öbpp16µÄbmpÏÔÊ¾*/

void put_qr_code_buff(unsigned char* buff, int width, int dispHeight)
{
    int i, j;

    u32 margin;
    u32 size;
    u32 pixel_size = lua_lcd_bpp / 8;

    unsigned char *row, *p, *q;
    int x, y, xx, yy, bit;
    int realwidth;

    int height = 0;

    if(dispHeight == 0)
    {
        height = lua_lcd_width;
    }
    
    size = dispHeight  / width;
    margin = (dispHeight % width ) + lua_lcd_height - dispHeight ;

    OPENAT_print("qr %d %d %d", lua_lcd_height, dispHeight, width, margin);
    realwidth = lua_lcd_width;
    
    row = (unsigned char *)L_MALLOC(realwidth * pixel_size);

    if(lua_lcd_height == dispHeight)
    {
        /* top margin */
        memset(row, 0xff, realwidth * pixel_size );
        
        for(y=0; y< margin /2 ; y++) {
        	memcpy(&workingbuffer[height * lua_lcd_width * pixel_size], row, realwidth * pixel_size);
        	height++;
        }
    }
    else
    {
        height = margin;
    }
    /* data */
    p = buff;
    for(y=0; y< width; y++) {

    	memset(row, 0xff, (realwidth* pixel_size));
    	q = row;
    	q += (margin /2 )* pixel_size ;
        
    	for(x=0; x < width; x++) {
    		for(xx=0; xx<size; xx++) {
               if((*p & 1))
                       memset(q, 0, pixel_size);
    			q += pixel_size;
    		}
    		p++;
    	}
    	for(yy=0; yy<size; yy++) {
    		memcpy(&workingbuffer[height * lua_lcd_width* pixel_size], row, realwidth * pixel_size);
    		height++;
    	}
    }

    if(lua_lcd_height == dispHeight)
     {
        /* bottom margin */
        memset(row, 0xff, (realwidth* pixel_size));
        
        for(y=0; y<margin / 2; y++) {
        	memcpy(&workingbuffer[height * lua_lcd_width* pixel_size], row, realwidth * pixel_size);
        	height++;
        }
    }

    L_FREE(row);
}

#ifdef AM_LPNG_SUPPORT
static int preload_png_file_buff(const char *filename, 
                                        int layer_id,
                                        u8* buffer, 
                                        kal_uint32* width, 
                                        kal_uint32* height,
                                        kal_uint32* channel)
{
    png_FILE_p fp;
    png_structp read_ptr;
    png_infop read_info_ptr;
    png_bytep row_buf = NULL;
    png_uint_32  row_idx;
    png_byte color_type;
    png_size_t row_bytes;
    int nChannel;
    int write_index = 0;
    

    fp = fopen(filename, "rb");
    if(NULL == fp)
    {
        printf("[preload_png_file_buff]: %s file not exist.\n", filename);
        return PLATFORM_ERR;
    }

    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    read_info_ptr = png_create_info_struct(read_ptr);    

    #ifdef PNG_SETJMP_SUPPORTED
    if (setjmp(png_jmpbuf(read_ptr)))
    {
       printf("[preload_png_file_buff]: %s libpng read error\n", filename);
       png_free(read_ptr, row_buf);
       row_buf = NULL;
       png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
       fclose(fp);
       return PLATFORM_ERR;
    } 
    #endif

    png_init_io(read_ptr, fp);
    png_set_bgr(read_ptr);
    
    png_read_info(read_ptr, read_info_ptr);
    *width = png_get_image_width(read_ptr, read_info_ptr);
    *height = png_get_image_height(read_ptr, read_info_ptr);
    color_type = png_get_color_type(read_ptr, read_info_ptr);
    row_bytes = png_get_rowbytes(read_ptr, read_info_ptr);

    if(channel != NULL)
    {
        *channel == png_get_channels(read_ptr, read_info_ptr);
        
        if(layer_id == BASIC_LAYER_ID && *channel == 4)
        {
            printf("[preload_png_file_buff]: basic layer can not load png with alpha\n");
            return PLATFORM_ERR;
        }
    }

    
    for(row_idx=0; row_idx< *height; row_idx++)
    {
        png_read_row(read_ptr, (png_bytep)&buffer[write_index], NULL);
        write_index += row_bytes;
    }

    #ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
      #ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
            png_free_data(read_ptr, read_info_ptr, PNG_FREE_UNKN, -1);
      #endif
    #endif

    png_free(read_ptr, row_buf);
    row_buf = NULL;
    png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
    fclose(fp);
    
    return PLATFORM_OK;
}



int platform_get_png_file_resolution(const char *filename, png_uint_32* width, png_uint_32* height)
{

    png_FILE_p fp;
    png_structp read_ptr;
    png_infop read_info_ptr;
    png_size_t row_bytes;



    fp = fopen(filename, "rb");
    if(NULL == fp)
    {
        printf("[put_png_file_buff]: %s file not exist.\n", filename);
        return PLATFORM_ERR;
    }

    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    read_info_ptr = png_create_info_struct(read_ptr);    

    #ifdef PNG_SETJMP_SUPPORTED
    if (setjmp(png_jmpbuf(read_ptr)))
    {
       printf("[put_png_file_buff]: %s libpng read error\n", filename);
       png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
       fclose(fp);
       return PLATFORM_ERR;
    } 
    #endif

    png_init_io(read_ptr, fp);
    
    png_read_info(read_ptr, read_info_ptr);
    *width = png_get_image_width(read_ptr, read_info_ptr);
    *height = png_get_image_height(read_ptr, read_info_ptr);

    png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
    fclose(fp);    
}
#endif

/*+\NEW\zhuth\2014.2.14\Ö§³ÖpngÍ¼Æ¬µÄÏÔÊ¾*/
#ifdef AM_LPNG_SUPPORT
int put_png_file_buff(const char *filename, int x, int y, int transcolor, int left, int top, int right, int bottom, int transtype, const char *dstFileName, T_AMOPENAT_IMAGE_INFO *info)
{
	if (info != NULL && lua_lcd_bpp != 16)
		return PLATFORM_ERR;

    png_FILE_p fp;
    png_structp read_ptr;
    png_infop read_info_ptr;
    png_bytep row_buf = NULL;
    png_uint_32 width, height, row_idx, tmp_idx;
    png_byte color_type,channel;
    png_size_t row_bytes;

    u16 *buffer16 = (u16*)workingbuffer;
    u16 data_r,data_g,data_b;
    u8 data_alpha,proc;
    u16 rgb16;
    u32 bgrgb888,fgrgb888,dstrgb888;
    short fr,fg,fb;
    short br,bg,bb;
    u8 dr,dg,db;
    u8 fpercent, bpercent;
    u32 rgb888;
    u8 *buffer24 = (u8*)workingbuffer;
    u32 layer_width = lua_lcd_width;
	/*+\BUG\wgyuan\2020.07,23,2562:u ±Sâ MYS«320" and*/
	u32 layer_height = lua_lcd_height;
	/*-\BUG\wgyuan\2020.07.23.2562:u ±SâçoâS«320" E*/
    u16 temp;

    FILE *file = NULL;

    if((left > right) || (top > bottom))
    {
        printf("[put_png_file_buff]: rect error\n");
        return PLATFORM_ERR;
    }

    fp = fopen(filename, "rb");
    if(NULL == fp)
    {
        printf("[put_png_file_buff]: %s file not exist\n", filename);
        return PLATFORM_ERR;
    }

    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    read_info_ptr = png_create_info_struct(read_ptr);

    #ifdef PNG_SETJMP_SUPPORTED
    if (setjmp(png_jmpbuf(read_ptr)))
    {
       printf("[put_png_file_buff]: %s libpng read error\n", filename);
       png_free(read_ptr, row_buf);
       row_buf = NULL;
       png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
       fclose(fp);
       return PLATFORM_ERR;
    }
    #endif

    png_init_io(read_ptr, fp);
    png_set_bgr(read_ptr);

    png_read_info(read_ptr, read_info_ptr);
    width = png_get_image_width(read_ptr, read_info_ptr);
    height = png_get_image_height(read_ptr, read_info_ptr);
    color_type = png_get_color_type(read_ptr, read_info_ptr);
    channel = png_get_channels(read_ptr, read_info_ptr);
    row_bytes = png_get_rowbytes(read_ptr, read_info_ptr);

	if (info != NULL)
	{
		u32 bsize = width * height * lua_lcd_bpp / 8;
		buffer16 = L_MALLOC(bsize);
		if (!buffer16)
		{
			png_free(read_ptr, row_buf);
			row_buf = NULL;
			png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
			fclose(fp);
			return PLATFORM_ERR;
		}
		memset(buffer16, 0xff, bsize);
		info->width = width;
		info->height = height;
		info->buffer = buffer16;
		info->format = 0;

		layer_height = height;
		layer_width = width;
	}
    /*if(strcmp(filename,"BAT.png") == 0)
    {
        printf("[put_png_file_buff]: width=%d,height=%d,color_type=%d,channel=%d,row_bytes=%d\n", width, height, color_type, channel,row_bytes);
    }*/
    if((3 != channel) && (4 != channel))
    {
        printf("[put_png_file_buff]: channel %d error\n", channel);
        png_free(read_ptr, row_buf);
        row_buf = NULL;
        png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
        fclose(fp);
        return PLATFORM_ERR;
    }

    row_buf = (png_bytep)png_malloc(read_ptr, row_bytes);
    if(NULL == row_buf)
    {
        printf("[put_png_file_buff]: %d row no buf error\n", row_bytes);
        png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
        fclose(fp);
        return PLATFORM_ERR;
    }

    if((left == 0) && (top == 0) && (right == 0) && (bottom == 0))
    {
        left = 0;
        top = 0;
        right = width - 1;
        bottom = height - 1;

        if(lua_lcd_bpp == 24 && channel == 3 && dstFileName==NULL)
        {
            int write_index = ((y)*layer_width + x) * 3;

            printf("[put_png_file_buff]: FAST READ\n");

            for(row_idx=0; row_idx< height; row_idx++)
            {
                png_read_row(read_ptr, row_buf, NULL);
                memcpy(&buffer24[write_index],
                        row_buf,
                        row_bytes);

                write_index += layer_width * 3;
            }

            #ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
              #ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
                    png_free_data(read_ptr, read_info_ptr, PNG_FREE_UNKN, -1);
              #endif
            #endif

            png_free(read_ptr, row_buf);
            row_buf = NULL;
            png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
            fclose(fp);

            return PLATFORM_OK;

        }
    }

    for(row_idx=0; row_idx<height; row_idx++)
    {
    	 if(row_idx > bottom)
    	 {
    	     break;
    	 }

        png_read_row(read_ptr, row_buf, NULL);
        if(row_idx < top)
    	 {
    	     continue;
    	 }

        for(tmp_idx=left; tmp_idx<=right; tmp_idx++)
        {
            proc = 0;

            if(channel==4)
            {
               data_alpha = row_buf[tmp_idx*channel + 3];
            }

            if(lua_lcd_bpp == 16 || dstFileName)
            {
            	/*+\BUG\wangyuan\2020.08.19\BUG_2877:dispÏÔÊ¾bug*/
				if(((y+row_idx-top)*layer_width+(x+tmp_idx-left)) >= (MAX_LCD_BUFF_SIZE/2))
					break;
				/*-\BUG\wangyuan\2020.08.19\BUG_2877:dispÏÔÊ¾bug*/
                fr = row_buf[tmp_idx*channel + 2];
                fg = row_buf[tmp_idx*channel + 1];
                fb = row_buf[tmp_idx*channel + 0];

                rgb16 = RGB24ToRGB16(fr,fg,fb);

                if((-1 == transcolor || rgb16 != transcolor || transtype == 2) )
                {
                    if(channel==4 && !dstFileName)
                    {
                        // Ì «¸τä Winery £ ÷¸ ÷ ÷ ÷ ÷ ÷ ÷ ±¸ ÷ ÷¸ ÷¸ ÷÷ ÷÷ ÷÷ ÷÷ ÷÷ ÷ ÷¸ ÷÷ ÷ ÷¸ ÷÷ ÷÷ ÷÷ ÷÷ ÷÷ ÷÷ ÷ ÷¸ ÷÷ ÷ ÷¸ ÷¸ ÷÷ ÷÷ ÷÷ ÷ ÷¸ ÷¸ ÷ ÷¸ ÷¸ ÷÷ ÷÷
                        // 1:
                        // »n¸μälänμè« (Д³ ب الäμ ÷¶÷¶'ãoène
                        if((transtype==0 && data_alpha==0)
                            || (transtype==1)
                            || (transtype==2)
                            )
                        {
                            proc = 1;

				            if(lcd_bus == PLATFORM_LCD_BUS_SPI)
                            {
			                	temp = (buffer16[(y+row_idx-top)*layer_width+(x+tmp_idx-left)] >> 8 ) |
                                    ((buffer16[(y+row_idx-top)*layer_width+(x+tmp_idx-left)] & 0xff) << 8);
                            }
                            else
                            {
                                temp = buffer16[(y+row_idx-top)*layer_width+(x+tmp_idx-left)];
                            }

                            br = (temp >> 8) & 0xf8;
                            bg = (temp >> 3) & 0xfc;
                            bb = (temp << 3) & 0xf8;


                           if(transtype==2 && data_alpha != 0)
                            {
                                data_alpha = transcolor;
                            }

                            dr = (fr * data_alpha + br * (255 - data_alpha))/255;
                            dg = (fg * data_alpha + bg * (255 - data_alpha))/255;
                            db = (fb * data_alpha + bb * (255 - data_alpha))/255;

				            if(lcd_bus != PLATFORM_LCD_BUS_SPI)
				                temp = RGB24ToRGB16(dr, dg, db);
				            else
			                	temp = (RGB24ToRGB16(dr, dg, db) >> 8 ) | ((RGB24ToRGB16(dr, dg, db) & 0xff) << 8);

                            buffer16[(y+row_idx-top)*layer_width+(x+tmp_idx-left)] = temp;

                        }
                    }

                    if(proc==0)
                    {
                        if(dstFileName==NULL)
                        {
                            if(lcd_bus != PLATFORM_LCD_BUS_SPI)
			                temp = rgb16;
			         else
		                	temp = (rgb16 >> 8 ) | ((rgb16 & 0xff) << 8);

		               buffer16[(y+row_idx-top)*layer_width+(x+tmp_idx-left)] = temp;
                        }
                        else
                        {
                            if(file == NULL)
                            {
                                file = fopen(dstFileName, "wb");
                                if(NULL == file)
                                {
                                    printf("[put_png_file_buff]: %s file not exist\n", dstFileName);
                                    goto exit_fnc;
                                }
                            }
            
                            if(fwrite(&rgb16, 2, 1, file) != 1)
                            {
                                fclose(file);
                                file = NULL;
                                printf("[put_bmp_file_buff]: write file error\n");
                                remove(dstFileName);
                                file = NULL;
                                goto exit_fnc;
                            }
                        }
                    }
                }
            }
            else  if(lua_lcd_bpp == 24)
            {

                fr = row_buf[tmp_idx*channel + 2];
                fg = row_buf[tmp_idx*channel + 1];
                fb = row_buf[tmp_idx*channel + 0];

                if(channel==4)
                {
                    // Ì «¸τä Winery £ ÷¸ ÷ ÷ ÷ ÷ ÷ ÷ ±¸ ÷ ÷¸ ÷¸ ÷÷ ÷÷ ÷÷ ÷÷ ÷÷ ÷ ÷¸ ÷÷ ÷ ÷¸ ÷÷ ÷÷ ÷÷ ÷÷ ÷÷ ÷÷ ÷ ÷¸ ÷÷ ÷ ÷¸ ÷¸ ÷÷ ÷÷ ÷÷ ÷ ÷¸ ÷¸ ÷ ÷¸ ÷¸ ÷÷ ÷÷
                    // 1:
                    // »n¸μälänμè« (Д³ ب الäμ ÷¶÷¶'ãoène
                    if(data_alpha == 0xff)
                    {
                        buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 2] = fr;// dstrgb888 >> 16;
                        buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 1] = fg;//(dstrgb888    & 0xff00)>> 8;
                        buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 0] = fb;// dstrgb888 &0xff;
						continue;
                    }
                    else if(data_alpha == 0)
                    {
                        continue;
                    }
                    else
                    {
					    if(transtype== 2 && data_alpha != 0)
						{
						    fpercent = transcolor;
						}
						else
						{
						    fpercent = data_alpha;
						}
                        br = buffer24[((y+row_idx-top)*layer_width + (x+tmp_idx-left)) * 3 + 2] ;
                        bg = buffer24[((y+row_idx-top)*layer_width + (x+tmp_idx-left)) * 3 + 1] ;
                        bb = buffer24[((y+row_idx-top)*layer_width + (x+tmp_idx-left)) * 3 + 0] ;

                        dr = ((((fr - br)* fpercent) >> 8) ) + br;
                        dg = ((((fg - bg)* fpercent) >> 8) ) + bg;
                        db = ((((fb - bb)* fpercent) >> 8) ) + bb;

                        buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 2] = dr;// dstrgb888 >> 16;
                        buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 1] = dg;//(dstrgb888  & 0xff00)>> 8;
                        buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 0] = db;// dstrgb888 &0xff;
                    }
                }
				else
				{
                    buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 2] = fr;// dstrgb888 >> 16;
                    buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 1] = fg;//(dstrgb888    & 0xff00)>> 8;
                    buffer24[((y+row_idx-top)*layer_width+(x+tmp_idx-left))*3 + 0] = fb;// dstrgb888 &0xff;
				}


            }
        }
    }

exit_fnc:

    #ifdef PNG_STORE_UNKNOWN_CHUNKS_SUPPORTED
      #ifdef PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
            png_free_data(read_ptr, read_info_ptr, PNG_FREE_UNKN, -1);
      #endif
    #endif

    png_free(read_ptr, row_buf);
    row_buf = NULL;
    png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
    fclose(fp);

    if(file != NULL)
    {
        fclose(file);
    }

    if(dstFileName && file==NULL)
    {
        return PLATFORM_ERR;
    }

    return PLATFORM_OK;
}
#endif
/*-\NEW\zhuth\2014.2.14\Ö§³ÖpngÍ¼Æ¬µÄÏÔÊ¾*/
#ifdef AM_JPG_SUPPORT
/*+\NEW\zhuwangbin\2020.3.25\Ìn¼ÓjpgÎÄ¼þµÄ½âÂëºÍÏÔÊ¾*/
static int put_jpg_file_buff(const char *filename, int x, int y, int transcolor, int left, int top, int right, int bottom)
{
	T_AMOPENAT_IMAGE_INFO imageinfo;
	FILE *fp;
	u32 len;
	u16* buffer = (u16*)workingbuffer;
	u8 *buff;

	/**1. "*/
	fp = fopen(filename, "rb");

    if(NULL == fp)
    {
        printf("[putimage]: %s file not exist\n", filename);
        return PLATFORM_ERR;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buff = L_MALLOC(len);
    if(NULL == buff)
    {
        printf("[putimage]:not enough memory filename=%s, len=%d.\n", len);
        fclose(fp);
        return PLATFORM_ERR;
    }
    
    fread(buff, 1, len, fp);
    fclose(fp);

	/**2. jpeg½âÎö**/
	if (OPENAT_ImgsDecodeJpeg(buff, len, &imageinfo) >= 0)
	{
		/** 3. Ìî³äoîºóêêêý back **/
		int startPos;
        int i;
        int j;
		if((left == 0) && (top == 0) && (right == 0) && (bottom == 0))
	    {
	        left = 0;
	        top = 0;
	        right = imageinfo.width - 1;
	        bottom = imageinfo.height - 1;
	    }

	    startPos = top *imageinfo.width + left;

	    for(i = top;  i <= bottom; i ++)
	    {
	        memcpy(&buffer[y*lua_lcd_width + x],
	            &imageinfo.buffer[startPos],
	            (right - left + 1) * 2);
	        startPos += imageinfo.width;
	        y++;
	    }

		/**4. Ên · å. jpeg½âîöbuff**/
		OPENAT_ImgsFreeJpegDecodedata(&imageinfo);
	}

	L_FREE(buff);
    return PLATFORM_OK;
}
/*-\NEW\zhuwangbin\2020.3.25\Ìn¼ÓjpgÎÄ¼þµÄ½âÂëºÍÏÔÊ¾*/
#endif

/*+\bug NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
int platform_disp_putimage(const char *filename, u16 x, u16 y, int transcolor, u16 left, u16 top, u16 right, u16 bottom,int transtype)
/*-\bug NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
{
    u8 *buff;
    FILE *fp;
    u32 len;
    int result;

    /*+\NEW\zhuth\2014.2.16\Ö§³ÖpngÍ¼Æ¬µÄÏÔÊ¾*/
    if(strstr(filename, ".bmp") || strstr(filename, ".BMP"))
    {
        fp = fopen(filename, "rb");

        if(NULL == fp)
        {
            printf("[putimage]: %s file not exist\n", filename);
            return PLATFORM_ERR;
        }
    
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        buff = L_MALLOC(len);
        if(NULL == buff)
        {
            printf("[putimage]:not enough memory filename=%s, len=%d.\n", len);
            fclose(fp);
            return PLATFORM_ERR;
        }
        
        fread(buff, 1, len, fp);
        fclose(fp);
    
        /*+\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
        result = put_bmp_file_buff(buff, x, y, transcolor, left, top, right, bottom, NULL);
        /*-\NewReq NEW\zhutianhua\2013.12.24\ÏÔÊ¾Í¼Æ¬µÄÖ¸¸¶¨ÇøÓò*/
        L_FREE(buff);
    }
    #ifdef AM_LPNG_SUPPORT
    else if(strstr(filename, ".png") || strstr(filename, ".PNG"))
    {
        result = put_png_file_buff(filename, x, y, transcolor, left, top, right, bottom, transtype, NULL, NULL);
    }
    #endif
	#ifdef AM_JPG_SUPPORT
	/*+\NEW\zhuwangbin\2020.3.25\Ìn¼ÓjpgÎÄ¼þµÄ½âÂëºÍÏÔÊ¾*/
	else if(strstr(filename, ".jpg") || strstr(filename, ".JPG"))
    {
        result = put_jpg_file_buff(filename, x, y, transcolor, left, top, right, bottom);
    }
	/*-\NEW\zhuwangbin\2020.3.25\Ìn¼ÓjpgÎÄ¼þµÄ½âÂëºÍÏÔÊ¾*/
	#endif
    else
    {
        printf("[putimage]:only support bmp and png!\n");
        return PLATFORM_ERR;
    }   
    /*-\NEW\zhuth\2014.2.16\Ö§³ÖpngÍ¼Æ¬µÄÏÔÊ¾*/

    return result;
}
/*-\NEW\liweiqiang\2013.11.4\Ôö¼ÓBMPÍ¼Æ¬ÏÔÊ¾Ö§³Ö*/








void platform_disp_merge_layer(u8* dst_buff, 
                                       u8* src_buff, 
                                       int display_x,
                                       int display_y,
                                       T_AMOPENAT_LCD_RECT_T* src_rect,
                                       u8 foreground_cf)
{
    int x;
    int y;
    short fr, fg, fb, br, bg, bb;
    u32 tmp_idx;
    u8 channel;
    int src_width = src_rect->rbX - src_rect->ltX + 1;
    int top = src_rect->ltY;
    int left = src_rect->ltX;
    u8 dr, dg, db;
    u8 data_alpha;
    
    channel = (foreground_cf == OPENAT_COLOR_FORMAT_24) ? 3 : 4;

    if(channel == 4)
    {
        for(y = top; y <= src_rect->rbY; y++)
        {
            for(x = left; x <= src_rect->rbX; x++)
            {   
                tmp_idx = (y * src_width) + x;
                
                fr = src_buff[tmp_idx*channel];
                fg = src_buff[tmp_idx*channel + 1];
                fb = src_buff[tmp_idx*channel + 2];

                data_alpha = src_buff[tmp_idx * channel + 3];
                
                if(data_alpha == 0xff)
                {
                    dst_buff[((display_y + y - top)*lua_lcd_width+(x+display_x-left))*3 + 2] = fr;// dstrgb888 >> 16;
                    dst_buff[((display_y + y - top)*lua_lcd_width+(display_x+x-left))*3 + 1] = fg;//(dstrgb888    & 0xff00)>> 8;
                    dst_buff[((display_y + y - top)*lua_lcd_width+(display_x+x-left))*3 + 0] = fb;// dstrgb888 &0xff;
                    continue;
                }
                else if(data_alpha == 0)
                {
                    continue;
                }
                else
                {
                    br = dst_buff[((display_y + y - top)*lua_lcd_width+(x+display_x-left))*3 + 2];
                    bg = dst_buff[((display_y + y - top)*lua_lcd_width+(display_x+x-left))*3 + 1] ;
                    bb = dst_buff[((display_y + y - top)*lua_lcd_width+(display_x+x-left))*3 + 0] ;
                                                
                    dr = ((((fr - br)* data_alpha) >> 8)) + br; 
                    dg = ((((fg - bg)* data_alpha) >> 8)) + bg; 
                    db = ((((fb - bb)* data_alpha) >> 8)) + br; 

                    dst_buff[((display_y + y - top)*lua_lcd_width+(x+display_x-left))*3 + 2] = dr;// dstrgb888 >> 16;
                    dst_buff[((display_y + y - top)*lua_lcd_width+(display_x+x-left))*3 + 1] = dg;//(dstrgb888    & 0xff00)>> 8;
                    dst_buff[((display_y + y - top)*lua_lcd_width+(display_x+x-left))*3 + 0] = db;// dstrgb888 &0xff;
                }
            }
        }
    }
    else
    {
        for(y = top; y <= src_rect->rbY; y++)
        {
            memcpy(&dst_buff[((display_y + y - top)*lua_lcd_width+(display_x))*3],
                   &src_buff[(y * src_width) * 3],
                   src_width * 3);
        }
    }
}



#ifdef GIF_DECODE
void platform_disp_stopgif(void)
{
    OPENAT_stop_gif();
}



void platform_disp_playgif(const char* gif_file_name, int x, int y,  int times)
{
    u8 *buff;
    FILE *fp;
    u32 len;
    fp = fopen(gif_file_name, "rb");

    if(NULL == fp)
    {
        printf("[platform_disp_playgif]: %s file not exist\n", gif_file_name);
        return ;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buff = L_MALLOC(len);
    if(NULL == buff)
    {
        printf("[putimage]:not enough memory filename=%s, len=%d.\n", len);
        fclose(fp);
        return ;
    }
    
    fread(buff, 1, len, fp);
    fclose(fp);

    OPENAT_play_gif(workingbuffer, buff, len, x, y, times);

    L_FREE(buff);

}
#endif


/*+\NEW\liweiqiang\2013.12.7\Ôö¼Ó¾ØÐÎÏÔÊ¾Ö§³Ö*/
int platform_disp_drawrect(int x1, int y1, int x2, int y2, int color)
{
    int i,j;
    x1 = MIN(x1, lua_lcd_width-1);
    x2 = MIN(x2, lua_lcd_width-1);
    y1 = MIN(y1, lua_lcd_height-1);
    y2 = MIN(y2, lua_lcd_height-1);

    if(x1 > x2 || y1 > y2)
    {
        printf("[platform_disp_drawrect]: range error %d %d %d %d!\n", x1, y1, x2, y2);
        return PLATFORM_ERR;
    }

    if(lua_lcd_bpp == 16)
    {
        u16 *buf16 = (u16*)workingbuffer;

        if(-1 == color)
        {
            //»­¾ØÐÎ¿ò
            int height = y2 - y1;
            int pixels = height*lua_lcd_width;
            
            buf16 += y1*lua_lcd_width;

            for(i = x1; i <= x2; i++)
            {
                buf16[i] = disp_color;
                buf16[pixels+i] = disp_color;
            }

            for(j = y1; j <= y2; j++)
            {
                buf16[x1] = disp_color;
                buf16[x2] = disp_color;
                buf16 += lua_lcd_width;
            }
        }
        else
        {
            // òôìî³äé «ìî³ä¾øðî
            buf16 += y1*lua_lcd_width;

            for(j = y1; j <= y2; j++)
            {
                for(i = x1; i <= x2; i++)
                {
                    buf16[i] = color;
                }
                buf16 += lua_lcd_width;
            }
        }
    }
    else if(lua_lcd_bpp == 24)
    {
        u8 *buf24 = (u8*)workingbuffer;

        if(-1 == color)
        {
            //»­¾ØÐÎ¿ò
            int height = y2 - y1;
            int pixels = height*lua_lcd_width*3;
            
            buf24 += y1*lua_lcd_width*3;

            for(i = x1; i <= x2; i++)
            {
                memcpy(&buf24[i*3],&disp_color,3);
                memcpy(&buf24[pixels+i*3],&disp_color,3);
            }

            for(j = y1; j <= y2; j++)
            {
                memcpy(&buf24[x1*3],&disp_color,3);
                memcpy(&buf24[x2*3],&disp_color,3);
                buf24 += lua_lcd_width*3;
            }
        }
        else
        {
            // òôìî³äé «ìî³ä¾øðî
            buf24 += y1*lua_lcd_width*3;

            for(j = y1; j <= y2; j++)
            {
                for(i = x1; i <= x2; i++)
                {
                    memcpy(&buf24[i*3],&color,3);
                }
                buf24 += lua_lcd_width*3;
            }
        }
    }
/*+\ BUG0 \ ZHY \ 2014.10.15 \ ºÚ ° × æeìn¼ó*/
    else if(lua_lcd_bpp == 1)
    {
        if(color == COLOR_BLACK_1 || color == COLOR_WHITE_1)
        {
            u16 x,y,page;
            
            if((color^lcd_hwfillcolor) == 0) /*\ new \ liweiqiang \ 2014.10.21 \ ôö¼ó² »*/
            {

                for(x = x1; x < x2; x++)
                {
                    for(y = y1;y < y2; y++)
                    {
                        page = y/8;
                        workingbuffer[page*lua_lcd_width+x] |= 1<<(y%8);
                    }
                }
            }
            else
            {
                for(x = x1; x < x2; x++)
                {
                    for(y = y1;y < y2; y++)
                    {
                        page = y/8;
                        workingbuffer[page*lua_lcd_width+x] &= ~(1<<(y%8));
                    }
                }
            }
        }
        else
        {
            printf("[platform_disp_drawrect]: lcd_bpp = 1,color must be balck or white\n");
        }
    }
/*-\ BUG0 \ ZHY \ 2014.10.15 \ ºÚ ° × æeìn¼ó*/
    else
    {
        // ôý² »Ö§³öæäëûæe
        printf("[platform_disp_drawrect]: not support bpp %d\n", lua_lcd_bpp);
        return PLATFORM_ERR;
    }

    return PLATFORM_OK;
}
/*-\NEW\liweiqiang\2013.12.7\Ôö¼Ó¾ØÐÎÏÔÊ¾Ö§³Ö*/

/*+\NEW\NEW\senawoes\2020.31\¬·un άvicious · Vences of burdens*/
int platform_disp_qrcode(u8 *data, u16 w, u16 disp_w, u16 start_x, u16 start_y)
{
	int i, j;
	u16 color, end_x, end_y, bx, by, page;
	unsigned int write_len = 0;
	u16 *buf16 = (u16*)workingbuffer;
    int scale;
    int x_width, y_width;

	if ( disp_w < w || (start_x >= lua_lcd_width) || (start_y >= lua_lcd_width) )
	{
		goto __error;
	}

    x_width = lua_lcd_width - start_x;
    y_width = lua_lcd_height - start_y;

    x_width = MIN(x_width, y_width);
    disp_w = MIN(disp_w, x_width);

    if(disp_w < w) goto __error;

    scale = disp_w / w;
    disp_w = w*scale;

    end_x = disp_w + start_x;
    end_y = disp_w + start_y;

	switch (lua_lcd_bpp)
	{
	case 1:
		for (i = start_x, bx = 0; i < end_x; i++, bx++)
		{
			for (j = start_y, by = 0; j < end_y; j++, by++)
			{
				if (data[(bx/scale) + (by/scale) * w] & 1)
				{
					color = disp_color;
				}
				else
				{
					color = disp_bkcolor;
				}
				page = j/8;
	            if((color^lcd_hwfillcolor) == 0)
	            {
	            	workingbuffer[page*lua_lcd_width + i] |= 1<<(j%8);
	            }
	            else
	            {
	                workingbuffer[page*lua_lcd_width + i] &= ~(1<<(j%8));
	            }
	            write_len++;
			}
		}
		break;
	case 16:
		for (i = start_x, bx = 0; i < end_x; i++, bx++)
		{
            for (j = start_y, by = 0; j < end_y; j++, by++)
            {
                if (data[(bx/scale) + (by/scale) * w] & 1)
                {
                    color = disp_color;
                }
                else
                {
                    color = disp_bkcolor;
                }
                buf16[i + j * lua_lcd_width] = color;
                write_len++;
            }
		}
		break;
	default:
		goto __error;
	}
	return write_len;

__error:
	write_len = 0;
	return write_len;
}
/*-\NEW\NEW\senyafried\20*/

/*+\ New \ liweiqiang \ 2013.12.9 \ ôö¼óç ° ° ° ° \ ± ³ ° ° ° «Éèöã*/
int platform_disp_setcolor(int color)
{
    int old_color = disp_color;
    disp_color = color;
    return old_color;
}

int platform_disp_setbkcolor(int color)
{
    int old_color = disp_bkcolor;
    disp_bkcolor = color;
    return old_color;
}
/*-\ new \ liweiqiang \ 2013.12.9 \ ôö¼óç ° ¾ ° \ \ ± ³¾ ° «Éèöã*/

/*+\NEW\liweiqiang\2013.12.9\Ôö¼Ó·ÇÖÐÎÄ×ÖÌåÉèÖÃ*/
#ifdef WIN32 
#pragma pack(push,pack1,1)
#endif
typedef struct FontFileInfoTag
{
    u8        width;
    u8        height;
    u8        type;
    u16       start;
    u16       end;
}__attribute__((packed)) FontFileInfo;
#ifdef WIN32 
#pragma pack(pop,pack1)
#endif

static int load_file_data(const char *name, u8 **buf_pp)
{
    FILE *fp = NULL;
    int size;
    char *buf = NULL;
    
    fp = fopen(name, "rb");
    
    if(fp == NULL)
    {
        printf("[load_file_data]: file not exist!\n");
        goto load_error;
    }
    
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    buf = L_MALLOC(size);
    if(NULL ==  buf)
    {
        printf("[load_file_data]: no memory\n");
        goto load_error;
    }
    fseek(fp, 0, SEEK_SET);
    fread(buf, 1, size, fp);
    fclose(fp);
    
    *buf_pp = buf;
    return size;
    
load_error:
    if(fp)
        fclose(fp);
    
    if(buf)
        L_FREE(buf);
    return 0;
}

int platform_disp_loadfont(const char *name)
{
    u8 *buf = NULL;
    int size = 0;
    FontFileInfo *fileInfo_p;
    u32 charsize;
    BOOL found = FALSE;
    int i;
    int retcode = 0;

    size = load_file_data(name, &buf);
    
    if(size <= sizeof(FontFileInfo))
    {
        retcode = -1;
        goto load_font_error;
    }

    fileInfo_p = (FontFileInfo *)buf;

    if(fileInfo_p->type != 0)
    {
        // V ö »ö§³öe¬ðØ × ö Î Û · ½½μä × Ö¿q
        retcode = -2;
        goto load_font_error;
    }

    if(fileInfo_p->end < fileInfo_p->start)
    {
        retcode = -3;
        goto load_font_error;
    }
    
    charsize = (fileInfo_p->width+7)/8;
    charsize *= fileInfo_p->height;

    if(charsize*(fileInfo_p->end - fileInfo_p->start + 1) != size - sizeof(FontFileInfo))
    {
        retcode = -4;
        goto load_font_error;
    }

    for(i = 0; i < MAX_FONTS && !found; i++)
    {
        if(dispFonts[i].data == NULL)
        {
            found = TRUE;
            break;
        }
    }
    
    if(!found)
    {
        retcode = -5;
        goto load_font_error;
    }

    dispFonts[i].width = fileInfo_p->width;
    dispFonts[i].height = fileInfo_p->height;
    dispFonts[i].size = charsize;
    dispFonts[i].start = fileInfo_p->start;
    dispFonts[i].end = fileInfo_p->end;
    dispFonts[i].data = buf + sizeof(FontFileInfo);

    return i;

load_font_error:
    if(buf)
        L_FREE(buf);

    printf("[platform_disp_loadfont]:error code %d\n", retcode);

    return -1;
}

int platform_disp_setfont(int id)
{
    int old_font_id;

    /*+\NEW\liweiqiang\2013.12.10\ÐÞÕý¸º×ÖÌåidµ¼ÖÂ»¨ÆÁ*/
    if(id < 0 || id >= MAX_FONTS || NULL == dispFonts[id].data)
    /*-\NEW\liweiqiang\2013.12.10\ÐÞÕý¸º×ÖÌåidµ¼ÖÂ»¨ÆÁ*/
    {
        printf("[platform_disp_setfont]: error font id\n");
        return -1;
    }

    old_font_id = curr_font_id;
    curr_font_id = id;
#ifdef LUA_LVGL_SUPPORT
	lv_font_luat.line_height = dispFonts[curr_font_id].height;
#endif
    return old_font_id;
}
/*+New \ brezen \ 2016.05.13 \ × Öìåëõ · to*/  
int platform_disp_setfontHeight(int height)
{
    if(height < 10 || height > 60)
    {
        return -1;
    }
    else
    {
        g_s_fontZoomSize = height;
        return 0;
    }
}

int platform_disp_getfontHeight(void)
{
    return g_s_fontZoomSize;
}
/*-New \ Brezen \ 2016.05.13 \ × Öìåëõ · Oh to*/  

/*+\BUG\skinyuanyuan\2020.06.02\BUG_1983jÓµwrite()¿ï´»uggè¾´à³³â ²â*/
int platform_disp_wrire(int cmd)
{
	platform_lcd_wrire(cmd);
}
/*-\BUGUG\skinyuan\2020.06.02\BUG_1983\ölisp.writ()¿ï«´«²¢âLESà³²µµµ*/

/*+\BUG\BURUG\Singer's 2020.*/
void platform_get_lcd_info(u16 *pWidth, u16 *pHeight, u8 *pBpp)
{
    *pWidth = lua_lcd_width;
    *pHeight = lua_lcd_height;
    *pBpp = lua_lcd_bpp;
}
/*-\BURUG\SIVE*/

#ifdef AM_LAYER_SUPPORT
/*NEW TO THE entities NEW TO THE entities ****** The I shall.
****fns of nam---- platform_disp_png_png_to_yer
*** --fide: Ò×°4×õçØµçüÆ¬µüâsâm
                 wing_ad: ân¬¬Æ¬× °4Øµ µ » µ µ µS «» »way
2
**Sereon ---- BC´É»»
*****The     
NEW TO THE entities NEW TO THE entities ****** The I shall.*/
int platform_disp_preload_png_to_layer(const char *filename, int layer_id)
{
    kal_uint32 width;
    kal_uint32 height;
    kal_uint32 result;
    kal_uint32 channel;

    result = preload_png_file_buff(filename, layer_id, layer_info[layer_id].buffer, &width, &height, &channel);

    if(channel == 3)
    {
        layer_info[layer_id].color_format = OPENAT_COLOR_FORMAT_24;
    }
    else
    {
        layer_info[layer_id].color_format = OPENAT_COLOR_FORMAT_32;
    }

#if 0
    layer_info[layer_id].width = width;
    layer_info[layer_id].height = height;
#endif

    return result;
}




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
int platform_create_user_layer(int layer_id, int start_x, int start_y, int layer_width, int layer_height)
{
    if(layer_id != USER_LAYER_1_ID && layer_id != USER_LAYER_2_ID)
    {
        return PLATFORM_ERR;
    }

    OPENAT_print("create layer:%d %d %d %d %d", layer_id, start_x, start_y, layer_width, layer_height);
    OPENAT_print("blocksize%d",NOR_BlockSize(0));

    if (layer_info[layer_id].buffer == NULL)
    {
      layer_info[layer_id].buffer = (kal_uint8*)UNCACHED((OPENAT_malloc(MAX_LCD_WIDTH_SUPPORT*MAX_LCD_HEIGH_SUPPORT* 4)));
    }
    
    layer_info[layer_id].src_rect.ltX = 0;
    layer_info[layer_id].src_rect.ltY = 0;
    layer_info[layer_id].clip_rect.ltX = start_x;
    layer_info[layer_id].clip_rect.ltY = start_y;

    if(layer_width == 0 || layer_height == 0)
    {
        layer_info[layer_id].src_rect.rbX = lua_lcd_width - 1;
        layer_info[layer_id].src_rect.rbY = lua_lcd_height - 1;
        layer_info[layer_id].clip_rect.rbX = layer_info[layer_id].clip_rect.ltX + lua_lcd_width - 1;
        layer_info[layer_id].clip_rect.rbY = layer_info[layer_id].clip_rect.ltY + lua_lcd_height - 1;
    }
    else
    { 
        layer_info[layer_id].src_rect.rbX = layer_width - 1;
        layer_info[layer_id].src_rect.rbY = layer_height - 1;    

        layer_info[layer_id].clip_rect.rbX = layer_info[layer_id].clip_rect.ltX + layer_width - 1;
        layer_info[layer_id].clip_rect.rbY = layer_info[layer_id].clip_rect.ltY + layer_height - 1;
    }
    
    layer_info[layer_id].disp_rect = layer_info[layer_id].src_rect;
    layer_info[layer_id].color_format = OPENAT_COLOR_FORMAT_24;
    
    return PLATFORM_OK;
}



/*NEW TO THE entities NEW TO THE entities ****** The I shall.
****fns of nam-----destroy_stroy_ander_yer_layer
*777*-- winge_id:ÒêÙ»ÙµKüçÍõçÍüçÍ.
2
**Sereon ---- BC´É»»
*****The     
-----1. in_id_idÖ"ÄÇS_1_DID"
-----2. SónçâÃâÃâÃâÃâsses, judges theóânê º º ºóÃÃø»Sø¸¸» » the USK»Sûâæyne's between all.
NEW TO THE entities NEW TO THE entities ****** The I shall.*/
int platform_destroy_user_layer(int layer_id)
{
    if(layer_id != USER_LAYER_1_ID && layer_id != USER_LAYER_2_ID)
    {
        return PLATFORM_ERR;
    }
  
    if(layer_info[layer_id].buffer != NULL)
    {
        OPENAT_free((void*)CACHED(layer_info[layer_id].buffer));
        layer_info[layer_id].buffer = 0;
    }
    
    return PLATFORM_OK;
}

/*-\NEW\zhuwangbin\2016.7.14\lua Ìn¼Ótp »ÆÁËÀ»úµ÷ÊÔ*/
kal_bool platform_create_user_layer_check(int layer_id, int count)
{
	    if (layer_info[layer_id].buffer == NULL)
	    {
			OPENAT_print("workingbuffer layer_id = %d, count = %d", layer_id, count);
			return KAL_FALSE;
		}

		return KAL_TRUE;
}
/*-\NEW\zhuwangbin\2016.7.14\lua Ìn¼Ótp »ÆÁËÀ»úµ÷ÊÔ*/

/******************************************************************************
***func name---- platform_set_active_layer
***param    ---- layer_id:Òª¼¤»îµÄÍ¼²ãID
***desc     ---- ¼¤»îÍ¼²ã£¬¶ÔLCDµÄËùÓÐ»æÍ¼¶¯×÷¶¼»eÔÚ¼¤»îÍ¼²ãÉÏ½øÐÐ
***return   ---- NULL
***note     
******************************************************************************/
void platform_set_active_layer(int layer_id)
{
    active_layer_id = layer_id;
    workingbuffer = layer_info[layer_id].buffer;

/*-\NEW\zhuwangbin\2016.7.14\lua Ìn¼Ótp »ÆÁËÀ»úµ÷ÊÔ*/
	if (workingbuffer ==  NULL)
	{
		OPENAT_print("platform_set_active_layer workingbuffer == null");
		OPENAT_Delayms(2000);
	}
/*-\NEW\zhuwangbin\2016.7.14\lua Ìn¼Ótp »ÆÁËÀ»úµ÷ÊÔ*/
}


/******************************************************************************
***func name---- platform_swap_user_layer
***param    ---- NULL
***desc     ---- ¶Ô»»Á½¸öÓÃ»§Í¼²ãµÄÄÚÈÝ¡£´Ëº¯Êý²»Éæ¼°Í¼²ãµÄÊý¾Ý¿½±´£¬Òò´ËËÙ¶ÈºÜ¿ì¡£
                 ÊÊÓÃÓÚ¶ÔËÙ¶ÈÒªÇó¸ßµÄÇé¿öÏÂ¿ìËÙ½»»»Á½¸öÓÃ»§Í¼²ãµÄÄÚÈÝ.
***return   ---- NULL
***note     
******************************************************************************/
void platform_swap_user_layer(void)
{
    OPENAT_LAYER_INFO* layer;

    layer = &layer_info[USER_LAYER_1_ID];
    layer_info[USER_LAYER_1_ID] = layer_info[USER_LAYER_2_ID];
    layer_info[USER_LAYER_2_ID] = *layer;

    if(active_layer_id != BASIC_LAYER_ID)
    {
        workingbuffer = (active_layer_id == USER_LAYER_1_ID) ? 
                        layer_info[USER_LAYER_2_ID].buffer:
                        layer_info[USER_LAYER_1_ID].buffer;
    }

/*-\NEW\zhuwangbin\2016.7.14\lua Ìn¼Ótp »ÆÁËÀ»úµ÷ÊÔ*/
	if (workingbuffer ==  NULL)
	{
		OPENAT_print("platform_swap_user_layer workingbuffer == null");
		OPENAT_Delayms(2000);
	}
/*-\NEW\zhuwangbin\2016.7.14\lua Ìn¼Ótp »ÆÁËÀ»úµ÷ÊÔ*/
}



/********* of*******lable**********s***lable********slable********lain******lain*******lain*******lainly********lainly********lainly********lainly********lainly*******lainly)
**** million if it is---staged_____s_s___.
****pas ---- sc_ity_id1: Ò´OOPOOOO’s
            ----x1: Ô´´´´½´´½1ã stage1óéééééé player´OD´On ONLY ONE
            ----y1: Ô´´´´½´´½1ã stage1óéééééé leader´OD½OMENTOM
            ---- sc_ader_id2: ²OOPY´OnÍ´´½´½£2
            ----x2: Ô´´´´½´´½ 2éóéééééthé stage point
            ----y2: Ô´´´´½´´´ chapter2éé stage2óóóééóéé?            
1*** ****--'-- ½â ¼»´½´½ nué´OP£££££££££n
****--'--NLL
2***n
******** of*******lable**********s***lable********slable********lain******lain*******lain*******lainly********lainly********lainly********lainly********lainly*******lainly)*/
void platform_copy_layer(int dst_layer_id,
                                 int display_x,
                                 int display_y,
                                 int src_layer_id,
                                 T_AMOPENAT_LCD_RECT_T* src_rect)
{
    int x;
    int y;
    short fr, fg, fb, br, bg, bb;
    u32 tmp_idx;
    u8 channel;
    int src_width = src_rect->rbX - src_rect->ltX + 1;
    int top = src_rect->ltY;
    int left = src_rect->ltX;
    u8 dr, dg, db;
    u8 data_alpha;
    
    u8* dst_buff = layer_info[dst_layer_id].buffer;
    u8* src_buff = layer_info[src_layer_id].buffer;
    u8 src_cf = layer_info[src_layer_id].color_format;
    u8 dst_cf = layer_info[dst_layer_id].color_format;

    int src_layer_width = layer_info[src_layer_id].clip_rect.rbX - layer_info[src_layer_id].clip_rect.ltX + 1;
    int dst_layer_width = layer_info[dst_layer_id].clip_rect.rbX - layer_info[dst_layer_id].clip_rect.ltX + 1;
    
    channel = (src_cf == OPENAT_COLOR_FORMAT_24) ? 3 : 4;

    if(channel == 4)
    {
        for(y = top; y <= src_rect->rbY; y++)
        {
            for(x = left; x <= src_rect->rbX; x++)
            {   
                tmp_idx = (y * src_layer_width) + x;
                
                fr = src_buff[tmp_idx*channel];
                fg = src_buff[tmp_idx*channel + 1];
                fb = src_buff[tmp_idx*channel + 2];

                data_alpha = src_buff[tmp_idx * channel + 3];
                
                if(data_alpha == 0xff)
                {
                    dst_buff[((display_y + y - top)*dst_layer_width + (x+display_x-left))*3 + 2] = fr;// dstrgb888 >> 16;
                    dst_buff[((display_y + y - top)*dst_layer_width + (display_x+x-left))*3 + 1] = fg;//(dstrgb888    & 0xff00)>> 8;
                    dst_buff[((display_y + y - top)*dst_layer_width + (display_x+x-left))*3 + 0] = fb;// dstrgb888 &0xff;
                    continue;
                }
                else if(data_alpha == 0)
                {
                    continue;
                }
                else
                {
                    br = dst_buff[((display_y + y - top)*dst_layer_width+(x+display_x-left))*3 + 2];
                    bg = dst_buff[((display_y + y - top)*dst_layer_width+(display_x+x-left))*3 + 1] ;
                    bb = dst_buff[((display_y + y - top)*dst_layer_width+(display_x+x-left))*3 + 0] ;
                                                
                    dr = ((((fr - br)* data_alpha) >> 8)) + br; 
                    dg = ((((fg - bg)* data_alpha) >> 8)) + bg; 
                    db = ((((fb - bb)* data_alpha) >> 8)) + br; 

                    dst_buff[((display_y + y - top)*dst_layer_width+(x+display_x-left))*3 + 2] = dr;// dstrgb888 >> 16;
                    dst_buff[((display_y + y - top)*dst_layer_width+(display_x+x-left))*3 + 1] = dg;//(dstrgb888    & 0xff00)>> 8;
                    dst_buff[((display_y + y - top)*dst_layer_width+(display_x+x-left))*3 + 0] = db;// dstrgb888 &0xff;
                }
            }
        }
    }
    else
    {
        for(y = top; y <= src_rect->rbY; y++)
        {
            memcpy(&dst_buff[((display_y + y - top)*dst_layer_width+(display_x))*3],
                   &src_buff[(y * src_layer_width + left) * 3],
                   src_width * 3);
        }
    }
}




/*NEW TO THE entities NEW TO THE entities ****** The I shall.
****fn’
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
                                  int layer_id3, int x3, int y3)
{
    OPENAT_LAYER_INFO* layers_p[3]= {NULL, NULL, NULL};
    int layer_id[3];
    int x[3]; 
    int y[3];
    int i;
    int validLayerCount = 0;
    int clip_width;
    int clip_height;
    u32 begin = ust_get_current_time();

    OPENAT_print("platform_layer_flatten begin %x %x %x", layer_id1, layer_id2, layer_id3);
 
    x[0] = x1;
    x[1] = x2;
    x[2] = x3;
    y[0] = y1;
    y[1] = y2;    
    y[2] = y3;    

    layer_id[0] = layer_id1;
    layer_id[1] = layer_id2;
    layer_id[2] = layer_id3;

    for(i = 0; i < MAX_LAYER_SUPPORT; i++)
    {
        if(layer_id[i] == INVALID_LAYER_ID)
        {
            continue;
        }

        clip_width  = layer_info[layer_id[i]].clip_rect.rbX - layer_info[layer_id[i]].clip_rect.ltX + 1;
        clip_height = layer_info[layer_id[i]].clip_rect.rbY - layer_info[layer_id[i]].clip_rect.ltY + 1;
        
        layer_info[layer_id[i]].disp_rect.ltX = x[i] + layer_info[layer_id[i]].clip_rect.ltX;
        layer_info[layer_id[i]].disp_rect.ltY = y[i] + layer_info[layer_id[i]].clip_rect.ltY;
        layer_info[layer_id[i]].disp_rect.rbX = x[i] + layer_info[layer_id[i]].clip_rect.rbX;
        layer_info[layer_id[i]].disp_rect.rbY = y[i] + layer_info[layer_id[i]].clip_rect.rbY;

        layer_info[layer_id[i]].src_rect.ltY = 0;
        layer_info[layer_id[i]].src_rect.ltX = 0;
        layer_info[layer_id[i]].src_rect.rbX = clip_width - 1;
        layer_info[layer_id[i]].src_rect.rbY = clip_height - 1;

        /*Aiç¹æoðo is allow live to μää³ß.õoßAß £¬ßßns) Pronunciation d'îniotta*/
        if(layer_info[layer_id[i]].disp_rect.ltY >= (short)lua_lcd_height || 
            layer_info[layer_id[i]].disp_rect.ltX >= (short)lua_lcd_width)
        {
            continue;
        }

        /*These are-uzy »here'uenian» okææe »éææ*/
        if(layer_info[layer_id[i]].disp_rect.ltY < layer_info[layer_id[i]].clip_rect.ltY)
        {
           layer_info[layer_id[i]].disp_rect.ltY = layer_info[layer_id[i]].clip_rect.ltY;   /*Éokàé¾¾æææ¼¼omejä amenian0*/
           layer_info[layer_id[i]].src_rect.ltY  =  - y[i]; /*Étoãbufferμæreg) ânê*/
        }

        if(layer_info[layer_id[i]].disp_rect.rbY > layer_info[layer_id[i]].clip_rect.rbY)
        {
           layer_info[layer_id[i]].disp_rect.rbY = layer_info[layer_id[i]].clip_rect.rbY; 
           layer_info[layer_id[i]].src_rect.rbY -= y[i];
        }

        if(layer_info[layer_id[i]].disp_rect.ltX < layer_info[layer_id[i]].clip_rect.ltX)
        {
           layer_info[layer_id[i]].disp_rect.ltX = layer_info[layer_id[i]].clip_rect.ltX;
           layer_info[layer_id[i]].src_rect.ltX  = - x[i];
        }
        
        if(layer_info[layer_id[i]].disp_rect.rbX > layer_info[layer_id[i]].clip_rect.rbX)
        {
           layer_info[layer_id[i]].disp_rect.rbX = layer_info[layer_id[i]].clip_rect.rbX; 
           layer_info[layer_id[i]].src_rect.rbX -= x[i];
        }

        if(layer_info[layer_id[i]].disp_rect.ltX >= layer_info[layer_id[i]].disp_rect.rbX ||
            layer_info[layer_id[i]].disp_rect.ltY >= layer_info[layer_id[i]].disp_rect.rbY )
        {
            OPENAT_print("platform_layer_flatten ,CATCH YOU!");
        }
        layers_p[validLayerCount ++] = &layer_info[layer_id[i]];
        
    }


    OPENAT_print("platform_layer_flatten disp %d %d, %d %d, %d %d, src %d %d, %d %d, %d %d",
        layer_info[layer_id[0]].disp_rect.ltY,
        layer_info[layer_id[0]].disp_rect.rbY,
        layer_info[layer_id[1]].disp_rect.ltY,
        layer_info[layer_id[1]].disp_rect.rbY,
        layer_info[layer_id[2]].disp_rect.ltY,
        layer_info[layer_id[2]].disp_rect.rbY,
        layer_info[layer_id[0]].src_rect.ltY,
        layer_info[layer_id[0]].src_rect.rbY,
        layer_info[layer_id[1]].src_rect.ltY,
        layer_info[layer_id[1]].src_rect.rbY,
        layer_info[layer_id[2]].src_rect.ltY,
        layer_info[layer_id[2]].src_rect.rbY
        );


    if(layers_p[0] == NULL)
    {
        OPENAT_print("platform_layer_flatten ATTETION NO LAYER TO DISPLAY!!");
        return;
    }

    OPENAT_layer_flatten(layers_p[0], layers_p[1], layers_p[2]);
    OPENAT_print("platform_layer_flatten end %d", ust_get_current_time() - begin);
}

/*-\NEW\liweiqiang\2013.12.9\Ôö¼Ó·ÇÖÐÎÄ×ÖÌåÉèÖÃ*/


/*-\NEW\zhuwangbin\2015.2.23\½«lua¿ ØÖÆ½çÃæµÄÇÐ»»ÒÆµ½µ×²ã´¦Àn*/
void platform_layer_start_move(int layer_id1, 
                                  int layer_id2,
                                  int layer_id3,
                                  int delay_ms,
                                  int x_inc,
                                  int y_inc)
{
  OPENAT_layer_flatten_start_move(layer_id1, 
                                 layer_id2,
                                 layer_id3,
                                 delay_ms,
                                 x_inc,
                                 y_inc);
}
/*-\NEW\zhuwangbin\2015.2.23\½«lua¿ ØÖÆ½çÃæµÄÇÐ»»ÒÆµ½µ×²ã´¦Àn*/

/*-\NEW\zhuwangbin\2015.2.26\lua Í¼µüüüÍ£µÄµüµ××²ö²ö*/

void platform_layer_hang_QRcode(const unsigned char *url_string, int length, int disp_height)
{
  QRcode* code;

  code  = (QRcode *)qrencode(url_string, length);

  put_qr_code_buff(code->data, code->width, disp_height);

  qrencode_free(code);
}

#ifdef TOUCH_PANEL_SUPPORT

void platform_layer_hang_start(int layer_id1, int layer_id2, int layer_id3, 
                            int y_inc, unsigned int delay_ms, int move_config, int lost_dirction)
{
  OPENAT_layer_hang_start( layer_id1, layer_id2, layer_id3, y_inc, delay_ms, move_config, lost_dirction);
}

void platform_layer_hang_stop(void)
{
  OPENAT_layer_hang_stop();
}
#endif
#endif
/*-\NEW\zhuwangbin\2015.2.26\lua Í¼µüüüÍ£µÄµüµ××²ö²ö*/

#ifdef LUA_LVGL_SUPPORT

extern lv_font_t lv_font_roboto_16;

typedef struct
{
	s32 code;
	DispBitmap bitmap;
} platform_bitmap_cache_t;

static platform_bitmap_cache_t g_s_cache = 
{
	.code = -1,
	.bitmap = {0}
};

static uint16_t platform_unicode_to_gb2312(uint32_t unicode)
{
	uint16_t ucs2 = unicode;
	uint16_t gb = unicode_to_gb2312(ucs2);
	// osiTracePrintf(0, "%s code %x", __FUNCTION__, gb);
	return gb;
}

static bool platform_get_bitmap(const lv_font_t *font, DispBitmap *bitmap, u16 code)
{
	// osiTracePrintf(0, "%s code %x", __FUNCTION__, code);
	platform_bitmap_cache_t *cache = (platform_bitmap_cache_t *)font->dsc;
	if (cache->code == code)
	{
		*bitmap = cache->bitmap;
		return TRUE;
	}
	getCharBitmap(bitmap, code);
	if (bitmap->data != blankChar)
	{
		cache->code = code;
		cache->bitmap = *bitmap;
		return TRUE;
	}
	return FALSE;
}

static bool platform_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *out, uint32_t letter, uint32_t letter_next)
{
	DispBitmap bitmap;
	if (platform_get_bitmap(font, &bitmap, platform_unicode_to_gb2312(letter)))
	{
		out->bpp = bitmap.bpp;
		out->adv_w = bitmap.orgWidth;
		out->box_h = bitmap.orgHeight;
		out->box_w = bitmap.orgWidth;
		out->ofs_x = 0;
		out->ofs_y = 0;
		return TRUE;
	}
	return lv_font_roboto_16.get_glyph_dsc(&lv_font_roboto_16, out, letter, letter_next);
}

static const uint8_t *platform_get_glyph_bitmap(const struct lv_font_t *font, uint32_t unicode_letter)
{
	DispBitmap bitmap;
	if (platform_get_bitmap(font, &bitmap, platform_unicode_to_gb2312(unicode_letter)))
	{
		return bitmap.data;
	}
	return lv_font_roboto_16.get_glyph_bitmap(&lv_font_roboto_16, unicode_letter);
}

lv_font_t lv_font_luat = {
    .dsc = &g_s_cache,
    .get_glyph_bitmap = platform_get_glyph_bitmap,
    .get_glyph_dsc = platform_get_glyph_dsc,
    .line_height = 0,
    .base_line = 0,
};
#endif

#endif
