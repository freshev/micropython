/**Creation time: 2020-03-27
 *Creator: yang
**/
#ifndef _GT5SLCD2E1A_H_
#define _GT5SLCD2E1A_H_

/*External function declaration*/
extern unsigned long r_dat_bat(unsigned long address,unsigned long DataLen,unsigned char *pBuff);
extern unsigned char gt_read_data(unsigned char* sendbuf , unsigned char sendlen , unsigned char* receivebuf, unsigned int receivelen);

extern unsigned char CheckID(unsigned char CMD, unsigned long address,unsigned long byte_long,unsigned char *p_arr);
/* ----------------------------------------------------------- */
//font library initialization
int GT_Font_Init(void);

/********************* Vector common part *********************/
//Chinese
#define VEC_BLACK_STY		1		//Bold body

//ASCII code
#define VEC_FT_ASCII_STY 	3
#define VEC_HZ_ASCII_STY 	4
#define VEC_CH_ASCII_STY 	5
#define VEC_TIMES_STY		6

/******************* Two calling mode configurations *******************/

/**
 * Method 1 VEC_ST_MODE: Configure structure information by declaring VECFONT_ST structure variables,
 * Get the lattice data into the zk_buffer[] array.
 * Method 2 VEC_PARM_MODE: Call by specifying parameters to obtain the dot matrix data into the pBits[] array.
 * ps: Both methods can be configured and used at the same time, or you can choose one to use.*/
#define VEC_ST_MODE
#define VEC_PARM_MODE

/*********************** Dividing line ***********************/

#ifdef VEC_ST_MODE

    #define ZK_BUFFER_LEN   4608    //The size can be modified, which is approximately equal to font size*font size/8.

    typedef struct vecFont
    {
        unsigned long fontCode;		//Character encoding Chinese: GB18030, ASCII/Foreign language: unicode
        unsigned char type;			//Font
        unsigned char size;			//Text size
        unsigned char thick;		//Text thickness
        unsigned char zkBuffer[ZK_BUFFER_LEN];	//data storage
    }VECFONT_ST;

    unsigned int get_font_st(VECFONT_ST * font_st);
#endif

#ifdef VEC_PARM_MODE
	/**Function name: get_font()
*Function: Vector text reading function
*Parameter: pBits data storage
* sty text font selection
* fontCode text encoding
* width text width
* height text height
* thick text thickness
*Return value: text display width
**/
    unsigned int get_font(unsigned char *pBits,unsigned char sty,unsigned long fontCode,unsigned char width,unsigned char height, unsigned char thick);
#endif
/*********************** End of vector area *************************/


/**Function name: get_Font_Gray()
 *Function Grayscale vector text reading function
 *Parameter: pBits data storage
 * sty text font selection @vector common part
 * fontCode character encoding Chinese: GB18030, ASCII/foreign language: unicode
 * fontSize text size
 * thick text thickness
 *Return value: re_buff[0] character display width, re_buff[1] character gray level [1st level/2nd level/3rd level/4th level]
**/
unsigned int* get_Font_Gray(unsigned char *pBits,unsigned char sty,unsigned long fontCode,unsigned char fontSize, unsigned char thick);

//Unicode to GBK
unsigned long  U2G(unsigned int  unicode);	
//BIG5 to GBK
unsigned int BIG52GBK( unsigned char h,unsigned char l );

/*-------------------------------------------------- -----------------------------------------------
 * Grayscale data conversion function 2-level grayscale/4-level grayscale
 * Description: Convert dot matrix data to grayscale data [eg: 32 dot matrix data converted to 2-level grayscale data will be converted to 16 dot matrix grayscale data]
 * Parameters:
 * OutPutData grayscale data; width width; High height; grade grayscale level [1st level/2nd level/3rd level/4th level]
 *------------------------------------------------ ----------------------------------------*/
void Gray_Process(unsigned char *OutPutData ,int width,int High,unsigned char Grade);

/*-------------------------------------------------- -----------------------------------------------
 * Grayscale text color settings
 * BmpDst target image data
 * BmpSrc icon image data
 * WORD x, WORD y, the icon is at the X, Y position of the target image.
 * WORD src_w, WORD src_h, the width and height of the icon
 * WORD dst_w, WORD dst_h width and height of the target image
 * SrcGray grayscale text data
 *Grade grayscale [2nd level/4th level]
 *------------------------------------------------ ----------------------------------------*/
void AlphaBlend_whiteBC(unsigned char *BmpDst,unsigned char *BmpSrc, int x, int y,
	int src_w, int src_h, int dst_w, int dst_h,unsigned char *SrcGray,unsigned char Grade);

/*-------------------------------------------------- -----------------------------------------------
 * Grayscale text blended with background
 * BmpDst target image data
 * BmpSrc icon image data
 * WORD x, WORD y, the icon is at the X, Y position of the target image.
 * WORD src_w, WORD src_h, the width and height of the icon
 * WORD dst_w, WORD dst_h width and height of the target image
 * SrcGray grayscale text data
 *Grade grayscale [2nd level/4th level]
 *------------------------------------------------ ----------------------------------------*/
void AlphaBlend_blackBC(unsigned char *BmpDst,unsigned char *BmpSrc, int x, int y,
	int src_w, int src_h, int dst_w, int dst_h,unsigned char *SrcGray,unsigned char Grade);


#endif

/*--------------------------------------- end of file ---------------------------------------------*/
