#include "oled.h"
#include "oledfont.h"
#include "iot_i2c.h"
#include "iot_types.h"
#include "string.h"

//OLEDµÄÏÔ´æ
//´æ·Å¸ñÊ½ÈçÏÂ.
//[0]0 1 2 3 ... 127
//[1]0 1 2 3 ... 127
//[2]0 1 2 3 ... 127
//[3]0 1 2 3 ... 127
//[4]0 1 2 3 ... 127
//[5]0 1 2 3 ... 127
//[6]0 1 2 3 ... 127
//[7]0 1 2 3 ... 127

uint8_t OLED_GRAM[128][8];

//¸üÐÂÏÔ´æµ½LCD
void OLED_Refresh_Gram(void)
{
	uint8_t i, n;
	for (i = 0; i < 8; i++)
	{
		OLED_WR_Byte(0xb0 + i, OLED_CMD); // ost³ ³³tk / Â ore ¨0 ~ 7 £ ©
		OLED_WR_Byte(0x00, OLED_CMD);	  // éèöãïôê¾î »ÖÃ¡ªjóðµnµøö ·
		OLED_WR_Byte(0x10, OLED_CMD);	  // éènôôîî »
		for (n = 0; n < 128; n++)
			OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA);
	}
}

/**********************************************
// IIC Write Command
**********************************************/
void Write_IIC_Command(uint8_t IIC_Command)
{
	uint8_t regAddr = 0x00;
	iot_i2c_write(OLED_I2C_PORT, OLED_I2C_ADDRESS, &regAddr, &IIC_Command, 1);
}

/**********************************************
// IIC Write Data
**********************************************/
void Write_IIC_Data(uint8_t IIC_Data)
{
	uint8_t regAddr = 0x40;
	iot_i2c_write(OLED_I2C_PORT, OLED_I2C_ADDRESS, &regAddr, &IIC_Data, 1);
}

//ÏòSSD1306Ð´ÈëÒ»¸ö×Ö½Ú¡£
//dot:O´cult´â »²â/Wón
//cmd:Êý¾Ý/ÃüÁî±êÖ¾ 0,±nÊ¾ÃüÁî;1,±nÊ¾Êý¾Ý;
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
	if (cmd)
	{
		Write_IIC_Data(dat);
	}
	else
	{
		Write_IIC_Command(dat);
	}
}

//¿ªÆôOLEDÏÔÊ¾
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D, OLED_CMD); // set dcdcãüeî
	OLED_WR_Byte(0X14, OLED_CMD); //DCDC ON
	OLED_WR_Byte(0XAF, OLED_CMD); //DISPLAY ON
}
//¹Ø±ÕOLEDÏÔÊ¾
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D, OLED_CMD); // set dcdcãüeî
	OLED_WR_Byte(0X10, OLED_CMD); //DCDC OFF
	OLED_WR_Byte(0XAE, OLED_CMD); //DISPLAY OFF
}
// çåæeº¯êý, çånêæe, õû¸öæeä »êçºúé« µä! ºnÃ »µÃÁÁò» ñÙ !!!
void OLED_Clear(void)
{
	uint8_t i, n;
	for (i = 0; i < 8; i++)
		for (n = 0; n < 128; n++)
			OLED_GRAM[n][i] = 0X00;
	OLED_Refresh_Gram(); //¸üÐÂÏÔÊ¾
}

//»­µã
//x:0~127
//y:0~63
// T: 1. Çi³ä 0, Ç.
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
	uint8_t pos, bx, temp = 0;
	if (x > 127 || y > 63)
		return; //³¬³ö·¶Î§ÁË.
	pos = 7 - y / 8;
	bx = y % 8;
	temp = 1 << (7 - bx);
	if (t)
		OLED_GRAM[x][pos] |= temp;
	else
		OLED_GRAM[x][pos] &= ~temp;
}
// x1, y1, x2, y2 ìî³äççóòµä¶ô¶ô½ç x Ø
// is · ± £ x1 <= x2; y1 <= y2 0 <= x1 <= 127 0 <= y1 <= 63
//dot:0,Cå²
void OLED_Fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot)
{
	uint8_t x, y;
	for (x = x1; x <= x2; x++)
	{
		for (y = y1; y <= y2; y++)
			OLED_DrawPoint(x, y, dot);
	}
	OLED_Refresh_Gram(); //¸üÐÂÏÔÊ¾
}
// ÔúÖ¸¸¶¨Î »öÃïê¾ò» ¸ö × ö · Û, ° ÜÀ¨²¿ · ö × Ö Î
//x:0~127
//y:0~63
// Mode: 0, · ´ ° × ïôê¾;
//size:Ñ¡Ôñ×ÖÌå 12/16/24
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode)
{
	uint8_t temp, t, t1;
	uint8_t y0 = y;
	uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); //µÃµ½×ÖÌåÒ»¸ö×Ö·û¶ÔÓ¦µãÕó¼¯ËùÕ¼µÄ×Ö½ÚÊý
	chr = chr - ' ';												// μÃль½æro òæºóμäöμäöμ
	for (t = 0; t < csize; t++)
	{
		if (size == 12)
			temp = asc2_1206[chr][t]; //µ÷ÓÃ1206×ÖÌå
		else if (size == 16)
			temp = asc2_1608[chr][t]; //µ÷ÓÃ1608×ÖÌå
		else if (size == 24)
			temp = asc2_2412[chr][t]; //µ÷ÓÃ2412×ÖÌå
		else
			return; //Ã»ÓÐµÄ×Ö¿â
		for (t1 = 0; t1 < 8; t1++)
		{
			if (temp & 0x80)
				OLED_DrawPoint(x, y, mode);
			else
				OLED_DrawPoint(x, y, !mode);
			temp <<= 1;
			y++;
			if ((y - y0) == size)
			{
				y = y0;
				x++;
				break;
			}
		}
	}
}
//m^nº¯Êý
uint32_t mypow(uint8_t m, uint8_t n)
{
	uint32_t result = 1;
	while (n--)
		result *= m;
	return result;
}
//ÏÔÊ¾2¸öÊý×Ö
// x, y: æðµã × Ø ± ê
// only: × ö m »êý
//size:×ÖÌå´óÐ¡
//hanced: AM£é½ 0 0, Jusî»»»®»»»Ö»»®»
// num: êýöµ (0 ~ 4294967295);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
	uint8_t t, temp;
	uint8_t enshow = 0;
	for (t = 0; t < len; t++)
	{
		temp = (num / mypow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1))
		{
			if (temp == 0)
			{
				OLED_ShowChar(x + (size / 2) * t, y, ' ', size, 1);
				continue;
			}
			else
				enshow = 1;
		}
		OLED_ShowChar(x + (size / 2) * t, y, temp + '0', size, 1);
	}
}
//ÏÔÊ¾×Ö·û´®
// x, y: æðµã × Ø ± ê
//size:×ÖÌå´óÐ¡
//*p:×Ö·û´®ÆðÊ¼µØÖ·
void OLED_ShowString(uint8_t x, uint8_t y, const uint8_t *p, uint8_t size)
{
	while ((*p <= '~') && (*p >= ' ')) // Åð¶ïÊÇ² »ÊÇ · Ç · ¨ × Ö · Û!
	{
		if (x > (128 - (size / 2)))
		{
			x = 0;
			y += size;
		}
		if (y > (64 - size))
		{
			y = x = 0;
			OLED_Clear();
		}
		OLED_ShowChar(x, y, *p, size, 1);
		x += size / 2;
		p++;
	}
}
T_AMOPENAT_I2C_PARAM i2cCfg = {0};
//³õÊ¼»¯SSD1306
bool OLED_Init(void)
{
	i2cCfg.freq = 1000000;
	if (iot_i2c_open(OLED_I2C_PORT, &i2cCfg) == FALSE)
	{
		return FALSE;
	}
	iot_os_sleep(1000);

	OLED_WR_Byte(0xAE, OLED_CMD); //¹Ø±ÕÏÔÊ¾
	OLED_WR_Byte(0xD5, OLED_CMD); //ÉèÖÃÊ±ÖÓ·ÖÆµÒò×Ó,Õðµ´ÆµÂÊ
	OLED_WR_Byte(80, OLED_CMD);	  //[3:0],·ÖÆµÒò×Ó;[7:4],Õðµ´ÆµÂÊ
	OLED_WR_Byte(0xA8, OLED_CMD); // éèm · · · ·
	OLED_WR_Byte(0X3F, OLED_CMD); //Ä¬ÈÏ0X3F(1/64)
	OLED_WR_Byte(0xD3, OLED_CMD); // éèm «òn
	OLED_WR_Byte(0X00, OLED_CMD); //Ä¬ÈÏÎª0

	OLED_WR_Byte(0x40, OLED_CMD); // éèm ¾ ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ª ªu ª ªu ª ªu.

	OLED_WR_Byte(0x8D, OLED_CMD); // μçºÉ Éééeöão
	OLED_WR_Byte(0x14, OLED_CMD); // bit2 £ ¬ojehô / ¹ø ± õ
	OLED_WR_Byte(0x20, OLED_CMD); //ÉèÖÃÄÚ´æµØÖ·Ä£Ê½
	OLED_WR_Byte(0x02, OLED_CMD); //1/1:0,00£££´´K½O½O½ 10½1£´â€™t 10, 10½
	OLED_WR_Byte(0xA1, OLED_CMD); //¶à/¶ï¶¶¨¨ååèÖÖÖÖÖÖÖÖÖÃ, bits0,0;1,0;1.0->127;
	OLED_WR_Byte(0xC0, OLED_CMD); // Éèöãcomé · · ½ïò; Bit3: 0, æõn £ exi 0; 1, Örseåä £ ex * with [n-1]-> com0;
	OLED_WR_Byte(0xDA, OLED_CMD); // Musecasy must have been thirtybases *
	OLED_WR_Byte(0x12, OLED_CMD); // [5: 4]

	OLED_WR_Byte(0x81, OLED_CMD); //¶Ô±È¶ÈÉèÖÃ
	OLED_WR_Byte(0xEF, OLED_CMD); //1~255;Ä¬ÈÏ0X7F (ÁÁ¶ÈÉèÖÃ,Ô½´óÔ½ÁÁ)
	OLED_WR_Byte(0xD9, OLED_CMD); // ¤³¤³¤³¤³¤³¤³¤³¤³¤³öhanga
	OLED_WR_Byte(0xf1, OLED_CMD); //[3:0],PHASE 1;[7:4],PHASE 2;
	OLED_WR_Byte(0xDB, OLED_CMD); //● μöÃVCom μçover ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ± ±â
	OLED_WR_Byte(0x30, OLED_CMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

	OLED_WR_Byte(0xA4, OLED_CMD); // / od Paris: mythorine English: 1, → 1, → ô; (° ¹Ø ± ± ±; ¹Ø)
	OLED_WR_Byte(0xA6, OLED_CMD); //MAY FREED/TOURTERS, State, Language, lingu, minute languages: 1, [,? *) 0, Directories for £ · PATE LEET6ATION ·
	OLED_WR_Byte(0xAF, OLED_CMD); //¿ªÆôÏÔÊ¾
	OLED_Clear();
}
