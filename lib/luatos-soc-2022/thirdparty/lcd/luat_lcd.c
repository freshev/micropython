#include "luat_base.h"
#include "luat_lcd.h"
#include "luat_gpio.h"
#include "luat_spi.h"
#include "luat_mem.h"
#include "common_api.h"


luat_color_t BACK_COLOR = WHITE, FORE_COLOR = BLACK;
static luat_lcd_conf_t *default_conf = NULL;

#define LUAT_LCD_CONF_COUNT (1)
static luat_lcd_conf_t* confs[LUAT_LCD_CONF_COUNT] = {0};

luat_color_t color_swap(luat_color_t color) {
    luat_color_t tmp = (color >> 8) + ((color & 0xFF) << 8);
    return tmp;
}

void luat_lcd_execute_cmds(luat_lcd_conf_t* conf, uint32_t* cmds, uint32_t count) {
    uint32_t cmd = 0;
    for (size_t i = 0; i < count; i++)
    {
        cmd = cmds[i];
        switch(((cmd >> 16) & 0xFFFF)) {
            case 0x0000 :
                lcd_write_cmd(conf, (const uint8_t)(cmd & 0xFF));
                break;
            case 0x0001 :
                luat_rtos_task_sleep(cmd & 0xFF);
                break;
            case 0x0002 :
                lcd_write_cmd(conf, (const uint8_t)(cmd & 0xFF));
                break;
            case 0x0003 :
                lcd_write_data(conf, (const uint8_t)(cmd & 0xFF));
                break;
            default:
                break;
        }
    }
}


int lcd_write_cmd(luat_lcd_conf_t* conf, const uint8_t cmd){
    size_t len;
    luat_gpio_set(conf->pin_dc, Luat_GPIO_LOW);
#ifdef LUAT_LCD_CMD_DELAY_US
    if (conf->dc_delay_us){
    	luat_timer_us_delay(conf->dc_delay_us);
    }
#endif
    if (conf->port == LUAT_LCD_SPI_DEVICE){
        len = luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device),  (const char*)&cmd, 1);
    }else{
        len = luat_spi_send(conf->port, (const char*)&cmd, 1);
    }
    luat_gpio_set(conf->pin_dc, Luat_GPIO_HIGH);
    if (len != 1){
        DBG("lcd_write_cmd error. %d", len);
        return -1;
    }else{
        #ifdef LUAT_LCD_CMD_DELAY_US
        if (conf->dc_delay_us){
        	luat_timer_us_delay(conf->dc_delay_us);
        }
        #endif
        return 0;
    }
}

int lcd_write_data(luat_lcd_conf_t* conf, const uint8_t data){
    size_t len;
    if (conf->port == LUAT_LCD_SPI_DEVICE){
        len = luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device),  (const char*)&data, 1);
    }else{
        len = luat_spi_send(conf->port,  (const char*)&data, 1);
    }
    if (len != 1){
        DBG("lcd_write_data error. %d", len);
        return -1;
    }else{
        return 0;
    }
}

luat_lcd_conf_t* luat_lcd_get_default(void) {
    for (size_t i = 0; i < LUAT_LCD_CONF_COUNT; i++){
        if (confs[i] != NULL) {
            return confs[i];
        }
    }
    return NULL;
}

const char* luat_lcd_name(luat_lcd_conf_t* conf) {
    return conf->opts->name;
}

int luat_lcd_init(luat_lcd_conf_t* conf) {
	conf->is_init_done = 0;
    int ret = conf->opts->init(conf);
    if (ret == 0) {
    	conf->is_init_done = 1;
        for (size_t i = 0; i < LUAT_LCD_CONF_COUNT; i++)
        {
            if (confs[i] == NULL) {
                confs[i] = conf;
                break;
            }
        }
    }
    default_conf = conf;
    u8g2_SetFontMode(&(conf->luat_lcd_u8g2), 0);
    u8g2_SetFontDirection(&(conf->luat_lcd_u8g2), 0);
    return ret;
}

int luat_lcd_close(luat_lcd_conf_t* conf) {
    if (conf->pin_pwr != 255)
        luat_gpio_set(conf->pin_pwr, Luat_GPIO_LOW);
    return 0;
}

int luat_lcd_display_off(luat_lcd_conf_t* conf) {
    if (conf->pin_pwr != 255)
        luat_gpio_set(conf->pin_pwr, Luat_GPIO_LOW);
    lcd_write_cmd(conf,0x28);
    return 0;
}

int luat_lcd_display_on(luat_lcd_conf_t* conf) {
    if (conf->pin_pwr != 255)
        luat_gpio_set(conf->pin_pwr, Luat_GPIO_HIGH);
    lcd_write_cmd(conf,0x29);
    return 0;
}

int luat_lcd_sleep(luat_lcd_conf_t* conf) {
    if (conf->pin_pwr != 255)
        luat_gpio_set(conf->pin_pwr, Luat_GPIO_LOW);
    luat_rtos_task_sleep(5);
    lcd_write_cmd(conf,0x10);
    return 0;
}

int luat_lcd_wakeup(luat_lcd_conf_t* conf) {
    if (conf->pin_pwr != 255)
        luat_gpio_set(conf->pin_pwr, Luat_GPIO_HIGH);
    luat_rtos_task_sleep(5);
    lcd_write_cmd(conf,0x11);
    return 0;
}

int luat_lcd_inv_off(luat_lcd_conf_t* conf) {
    lcd_write_cmd(conf,0x20);
    return 0;
}

int luat_lcd_inv_on(luat_lcd_conf_t* conf) {
    lcd_write_cmd(conf,0x21);
    return 0;
}

int luat_lcd_set_address(luat_lcd_conf_t* conf,uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    lcd_write_cmd(conf,0x2a);
    lcd_write_data(conf,(x1+conf->xoffset)>>8);
    lcd_write_data(conf,x1+conf->xoffset);
    lcd_write_data(conf,(x2+conf->xoffset)>>8);
    lcd_write_data(conf,x2+conf->xoffset);
    lcd_write_cmd(conf,0x2b);
    lcd_write_data(conf,(y1+conf->yoffset)>>8);
    lcd_write_data(conf,y1+conf->yoffset);
    lcd_write_data(conf,(y2+conf->yoffset)>>8);
    lcd_write_data(conf,y2+conf->yoffset);
    lcd_write_cmd(conf,0x2C);
    return 0;
}

int luat_lcd_set_color(luat_color_t back, luat_color_t fore){
    BACK_COLOR = back;
    FORE_COLOR = fore;
    return 0;
}

#ifndef LUAT_USE_LCD_CUSTOM_DRAW
int luat_lcd_flush(luat_lcd_conf_t* conf) {
    if (conf->buff == NULL) {
        return 0;
    }
    //DBG("luat_lcd_flush range %d %d", conf->flush_y_min, conf->flush_y_max);
    if (conf->flush_y_max < conf->flush_y_min) {
        // There is no content that needs to be refreshed, skip it directly
        //DBG("luat_lcd_flush no need");
        return 0;
    }
    uint32_t size = conf->w * (conf->flush_y_max - conf->flush_y_min + 1) * 2;
    luat_lcd_set_address(conf, 0, conf->flush_y_min, conf->w - 1, conf->flush_y_max);
    const char* tmp = (const char*)(conf->buff + conf->flush_y_min * conf->w);
	if (conf->port == LUAT_LCD_SPI_DEVICE){
		luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), tmp, size);
	}else{
		luat_spi_send(conf->port, tmp, size);
	}

    //Reset to a state that does not require refreshing
    conf->flush_y_max = 0;
    conf->flush_y_min = conf->h;
    
    return 0;
}

int luat_lcd_draw(luat_lcd_conf_t* conf, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t* color) {
    //Direct swipe mode
    if (conf->buff == NULL) {
        uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
        luat_lcd_set_address(conf, x1, y1, x2, y2);
	    if (conf->port == LUAT_LCD_SPI_DEVICE){
		    luat_spi_device_send((luat_spi_device_t*)(conf->lcd_spi_device), (const char*)color, size);
	    }else{
		    luat_spi_send(conf->port, (const char*)color, size);
	    }
        return 0;
    }
    // buff mode
    if (x1 > conf->w || y1 > conf->h) {
        DBG("out of lcd range");
        return -1;
    }
    uint16_t x_end = x2 > conf->w?conf->w:x2;
    uint16_t y_end = y2 > conf->h?conf->h:y2;
    luat_color_t* dst = (conf->buff + x1 + conf->w * y1);
    luat_color_t* src = (color);
    size_t lsize = (x_end - x1 + 1);
    for (size_t i = y1; i <= y_end; i++) {
        memcpy(dst, src, lsize * sizeof(luat_color_t));
        dst += conf->w;  //Move to next line
        src += lsize;    //Move data
        if (x2 > conf->w){
            src+=x2 - conf->w;
        }
    }
    //Storage the area that needs to be refreshed
    if (y1 < conf->flush_y_min)
        conf->flush_y_min = y1;
    if (y_end > conf->flush_y_max)
        conf->flush_y_max = y_end;
    return 0;
}
#endif

int luat_lcd_draw_point(luat_lcd_conf_t* conf, uint16_t x, uint16_t y, luat_color_t color) {
    // Note that the color needs to be swapped here.
    luat_color_t tmp = color_swap(color);
    return luat_lcd_draw(conf, x, y, x, y, &tmp);
}

int luat_lcd_clear(luat_lcd_conf_t* conf, luat_color_t color){
    luat_lcd_draw_fill(conf, 0, 0, conf->w, conf->h, color);
    return 0;
}

int luat_lcd_draw_fill(luat_lcd_conf_t* conf,uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2, luat_color_t color) {          
	uint16_t i;
	for(i=y1;i<=y2;i++)
	{
		luat_lcd_draw_line(conf, x1, i, x2, i, color);
	}
    return 0;			  	    
}

int luat_lcd_draw_vline(luat_lcd_conf_t* conf, uint16_t x, uint16_t y,uint16_t h, luat_color_t color) {
    if (h==0) return 0;
    return luat_lcd_draw_line(conf, x, y, x, y + h - 1, color);
}

int luat_lcd_draw_hline(luat_lcd_conf_t* conf, uint16_t x, uint16_t y,uint16_t w, luat_color_t color) {
    if (w==0) return 0;
    return luat_lcd_draw_line(conf, x, y, x + w - 1, y, color);
}

int luat_lcd_draw_line(luat_lcd_conf_t* conf,uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,luat_color_t color) {
    uint16_t t;
    uint32_t i = 0;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, row, col;
    if (x1 == x2 || y1 == y2) // straight line
    {
        size_t dots = (x2 - x1 + 1) * (y2 - y1 + 1);//number of points
        luat_color_t* line_buf = (luat_color_t*) luat_heap_malloc(dots * sizeof(luat_color_t));
        // color swap
        luat_color_t tmp = color_swap(color);
        if (line_buf) {
            for (i = 0; i < dots; i++)
            {
                line_buf[i] = tmp;
            }
            luat_lcd_draw(conf, x1, y1, x2, y2, line_buf);
            luat_heap_free(line_buf);
            return 0;
        }
    }

    delta_x = x2 - x1;
    delta_y = y2 - y1;
    row = x1;
    col = y1;
    if (delta_x > 0)incx = 1;
    else if (delta_x == 0)incx = 0;
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)incy = 1;
    else if (delta_y == 0)incy = 0;
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)distance = delta_x;
    else distance = delta_y;
    for (t = 0; t <= distance + 1; t++)
    {
        luat_lcd_draw_point(conf,row, col,color);
        xerr += delta_x ;
        yerr += delta_y ;
        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
    return 0;
}

int luat_lcd_draw_rectangle(luat_lcd_conf_t* conf,uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, luat_color_t color){
    luat_lcd_draw_line(conf,x1, y1, x2, y1, color);
    luat_lcd_draw_line(conf,x1, y1, x1, y2, color);
    luat_lcd_draw_line(conf,x1, y2, x2, y2, color);
    luat_lcd_draw_line(conf,x2, y1, x2, y2, color);
    return 0;
}

int luat_lcd_draw_circle(luat_lcd_conf_t* conf,uint16_t x0, uint16_t y0, uint8_t r, luat_color_t color){
    int a, b;
    int di;
    a = 0;
    b = r;
    di = 3 - (r << 1);
    while (a <= b)
    {
        luat_lcd_draw_point(conf,x0 - b, y0 - a,color);
        luat_lcd_draw_point(conf,x0 + b, y0 - a,color);
        luat_lcd_draw_point(conf,x0 - a, y0 + b,color);
        luat_lcd_draw_point(conf,x0 - b, y0 - a,color);
        luat_lcd_draw_point(conf,x0 - a, y0 - b,color);
        luat_lcd_draw_point(conf,x0 + b, y0 + a,color);
        luat_lcd_draw_point(conf,x0 + a, y0 - b,color);
        luat_lcd_draw_point(conf,x0 + a, y0 + b,color);
        luat_lcd_draw_point(conf,x0 - b, y0 + a,color);
        a++;
        //Bresenham
        if (di < 0)di += 4 * a + 6;
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
        luat_lcd_draw_point(conf,x0 + a, y0 + b,color);
    }
    return 0;
}

int luat_lcd_set_font(luat_lcd_conf_t* conf,const uint8_t *font) {
    u8g2_SetFont(&(conf->luat_lcd_u8g2), font);
    return 0;
}

static uint8_t utf8_state;
static uint16_t encoding;
static uint16_t utf8_next(uint8_t b)
{
  if ( b == 0 )  /* '\n' terminates the string to support the string list procedures */
    return 0x0ffff; /* end of string detected, pending UTF8 is discarded */
  if ( utf8_state == 0 )
  {
    if ( b >= 0xfc )  /* 6 byte sequence */
    {
      utf8_state = 5;
      b &= 1;
    }
    else if ( b >= 0xf8 )
    {
      utf8_state = 4;
      b &= 3;
    }
    else if ( b >= 0xf0 )
    {
      utf8_state = 3;
      b &= 7;
    }
    else if ( b >= 0xe0 )
    {
      utf8_state = 2;
      b &= 15;
    }
    else if ( b >= 0xc0 )
    {
      utf8_state = 1;
      b &= 0x01f;
    }
    else
    {
      /* do nothing, just use the value as encoding */
      return b;
    }
    encoding = b;
    return 0x0fffe;
  }
  else
  {
    utf8_state--;
    /* The case b < 0x080 (an illegal UTF8 encoding) is not checked here. */
    encoding<<=6;
    b &= 0x03f;
    encoding |= b;
    if ( utf8_state != 0 )
      return 0x0fffe; /* nothing to do yet */
  }
  return encoding;
}

static void u8g2_draw_hv_line(u8g2_t *u8g2, int16_t x, int16_t y, int16_t len, uint8_t dir, uint16_t color){
  switch(dir)
  {
    case 0:
      luat_lcd_draw_hline(default_conf,x,y,len,color);
      break;
    case 1:
      luat_lcd_draw_vline(default_conf,x,y,len,color);
      break;
    case 2:
        luat_lcd_draw_hline(default_conf,x-len+1,y,len,color);
      break;
    case 3:
      luat_lcd_draw_vline(default_conf,x,y-len+1,len,color);
      break;
  }
}

static void u8g2_font_decode_len(u8g2_t *u8g2, uint8_t len, uint8_t is_foreground){
  uint8_t cnt;  /* total number of remaining pixels, which have to be drawn */
  uint8_t rem;  /* remaining pixel to the right edge of the glyph */
  uint8_t current;  /* number of pixels, which need to be drawn for the draw procedure */
    /* current is either equal to cnt or equal to rem */
  /* local coordinates of the glyph */
  uint8_t lx,ly;
  /* target position on the screen */
  int16_t x, y;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  cnt = len;
  /* get the local position */
  lx = decode->x;
  ly = decode->y;
  for(;;){
    /* calculate the number of pixel to the right edge of the glyph */
    rem = decode->glyph_width;
    rem -= lx;
    /* calculate how many pixel to draw. This is either to the right edge */
    /* or lesser, if not enough pixel are left */
    current = rem;
    if ( cnt < rem )
      current = cnt;
    /* now draw the line, but apply the rotation around the glyph target position */
    //u8g2_font_decode_draw_pixel(u8g2, lx,ly,current, is_foreground);
    // printf("lx:%d,ly:%d,current:%d, is_foreground:%d \r\n",lx,ly,current, is_foreground);
    /* get target position */
    x = decode->target_x;
    y = decode->target_y;
    /* apply rotation */
    x = u8g2_add_vector_x(x, lx, ly, decode->dir);
    y = u8g2_add_vector_y(y, lx, ly, decode->dir);
    /* draw foreground and background (if required) */
    if ( current > 0 )		/* avoid drawing zero length lines, issue #4 */
    {
      if ( is_foreground )
      {
	    u8g2_draw_hv_line(u8g2, x, y, current, decode->dir, FORE_COLOR);
      }
      // else if ( decode->is_transparent == 0 )
      // {
	    // u8g2_draw_hv_line(u8g2, x, y, current, decode->dir, lcd_str_bg_color);
      // }
    }
    /* check, whether the end of the run length code has been reached */
    if ( cnt < rem )
      break;
    cnt -= rem;
    lx = 0;
    ly++;
  }
  lx += cnt;
  decode->x = lx;
  decode->y = ly;
}
static void u8g2_font_setup_decode(u8g2_t *u8g2, const uint8_t *glyph_data)
{
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  decode->decode_ptr = glyph_data;
  decode->decode_bit_pos = 0;

  /* 8 Nov 2015, this is already done in the glyph data search procedure */
  /*
  decode->decode_ptr += 1;
  decode->decode_ptr += 1;
  */

  decode->glyph_width = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
  decode->glyph_height = u8g2_font_decode_get_unsigned_bits(decode,u8g2->font_info.bits_per_char_height);

}
static int8_t u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data){
  uint8_t a, b;
  int8_t x, y;
  int8_t d;
  int8_t h;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  u8g2_font_setup_decode(u8g2, glyph_data);
  h = u8g2->font_decode.glyph_height;
  x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
  y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
  d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);

  if ( decode->glyph_width > 0 )
  {
    decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h+y), decode->dir);
    decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h+y), decode->dir);
    //u8g2_add_vector(&(decode->target_x), &(decode->target_y), x, -(h+y), decode->dir);
    /* reset local x/y position */
    decode->x = 0;
    decode->y = 0;
    /* decode glyph */
    for(;;){
      a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
      b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
      do{
        u8g2_font_decode_len(u8g2, a, 0);
        u8g2_font_decode_len(u8g2, b, 1);
      } while( u8g2_font_decode_get_unsigned_bits(decode, 1) != 0 );
      if ( decode->y >= h )
        break;
    }
  }
  return d;
}
const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding);
static int16_t u8g2_font_draw_glyph(u8g2_t *u8g2, int16_t x, int16_t y, uint16_t encoding){
  int16_t dx = 0;
  u8g2->font_decode.target_x = x;
  u8g2->font_decode.target_y = y;
  const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
  if ( glyph_data != NULL ){
    dx = u8g2_font_decode_glyph(u8g2, glyph_data);
  }
  return dx;
}

int luat_lcd_draw_str(luat_lcd_conf_t* conf,uint16_t x, uint16_t y,const uint8_t* str) {
    uint16_t e;
    int16_t delta;
    utf8_state = 0;

    for(;;){
        e = utf8_next((uint8_t)*str);
        if ( e == 0x0ffff )
        break;
        str++;
        if ( e != 0x0fffe ){
        delta = u8g2_font_draw_glyph(&(conf->luat_lcd_u8g2), x, y, e);
        switch(conf->luat_lcd_u8g2.font_decode.dir){
            case 0:
            x += delta;
            break;
            case 1:
            y += delta;
            break;
            case 2:
            x -= delta;
            break;
            case 3:
            y -= delta;
            break;
        }
        }
    }
    return 0;
}


#include "qrcodegen.h"

int luat_lcd_drawQrcode(luat_lcd_conf_t *conf,uint16_t x,uint16_t y,const char* text,int size){
    uint8_t *qrcode = luat_heap_malloc(qrcodegen_BUFFER_LEN_MAX);
    uint8_t *tempBuffer = luat_heap_malloc(qrcodegen_BUFFER_LEN_MAX);
    if (qrcode == NULL || tempBuffer == NULL) {
        if (qrcode)
            luat_heap_free(qrcode);
        if (tempBuffer)
            luat_heap_free(tempBuffer);
        DBG("qrcode out of memory");
        return 0;
    }
    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
        qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (ok){
        int qr_size = qrcodegen_getSize(qrcode);
        if (size < qr_size){
            DBG("size must be greater than size:%d qr_size:%d",size,qr_size);
            goto end;
        }
        int scale = size / qr_size ;
        if (!scale)scale = 1;
        int margin = (size - qr_size * scale) / 2;
        luat_lcd_draw_fill(conf,x,y,x+size,y+size,BACK_COLOR);
        x+=margin;
        y+=margin;
        for (int j = 0; j < qr_size; j++) {
            for (int i = 0; i < qr_size; i++) {
                if (qrcodegen_getModule(qrcode, i, j))
                    luat_lcd_draw_fill(conf,x+i*scale,y+j*scale,x+(i+1)*scale,y+(j+1)*scale,FORE_COLOR);
            }
        }
    }else{
        DBG("qrcodegen_encodeText false");
    }
end:
    if (qrcode)
        luat_heap_free(qrcode);
    if (tempBuffer)
        luat_heap_free(tempBuffer);
    return 0;
}

#include "tjpgd.h"
#include "tjpgdcnf.h"

#define N_BPP (3 - JD_FORMAT)

/* Session identifier for input/output functions (name, members and usage are as user defined) */
typedef struct {
    FILE *fp;               /* Input stream */
    int x;
    int y;
    // int width;
    // int height;
    uint16_t buff[16*16]
} IODEV;

static unsigned int file_in_func (JDEC* jd, uint8_t* buff, unsigned int nbyte){
    IODEV *dev = (IODEV*)jd->device;   /* Device identifier for the session (5th argument of jd_prepare function) */
    if (buff) {
        /* Read bytes from input stream */
        return luat_fs_fread(buff, 1, nbyte, dev->fp);
    } else {
        /* Remove bytes from input stream */
        return luat_fs_fseek(dev->fp, nbyte, SEEK_CUR) ? 0 : nbyte;
    }
}

static unsigned int lcd_out_func (JDEC* jd, void* bitmap, JRECT* rect){
    IODEV *dev = (IODEV*)jd->device;
    uint16_t* tmp = (uint16_t*)bitmap;

    // rgb high and low swap
    uint16_t count = (rect->right - rect->left + 1) * (rect->bottom - rect->top + 1);
    for (size_t i = 0; i < count; i++)
    {
      dev->buff[i] = ((tmp[i] >> 8) & 0xFF)+ ((tmp[i] << 8) & 0xFF00);
    }

    // LLOGD("jpeg seg %dx%d %dx%d", rect->left, rect->top, rect->right, rect->bottom);
    // LLOGD("jpeg seg size %d %d %d", rect->right - rect->left + 1, rect->bottom - rect->top + 1, (rect->right - rect->left + 1) * (rect->bottom - rect->top + 1));
    luat_lcd_draw(default_conf, dev->x + rect->left, dev->y + rect->top,
                                dev->x + rect->right, dev->y + rect->bottom,
                                dev->buff);
    return 1;    /* Continue to decompress */
}

int lcd_draw_jpeg(const char* path, int xpos, int ypos) {
  JRESULT res;      /* Result code of TJpgDec API */
  JDEC jdec;        /* Decompression object */
  void *work;       /* Pointer to the decompressor work area */
#if JD_FASTDECODE == 2
  size_t sz_work = 3500 * 3; /* Size of work area */
#else
  size_t sz_work = 3500; /* Size of work area */
#endif
  IODEV devid;      /* User defined device identifier */

  FILE* fd = luat_fs_fopen(path, "r");
  if (fd == NULL) {
    DBG("no such file %s", path);
    return -1;
  }

  devid.fp = fd;
  work = luat_heap_malloc(sz_work);
  if (work == NULL) {
    DBG("out of memory when malloc jpeg decode workbuff");
    return -3;
  }
  res = jd_prepare(&jdec, file_in_func, work, sz_work, &devid);
  if (res != JDR_OK) {
    luat_heap_free(work);
    luat_fs_fclose(fd);
    DBG("jd_prepare file %s error %d", path, res);
    return -2;
  }
  devid.x = xpos;
  devid.y = ypos;
  // devid.width = jdec.width;
  // devid.height = jdec.height;
  res = jd_decomp(&jdec, lcd_out_func, 0);
  luat_heap_free(work);
  luat_fs_fclose(fd);
  if (res != JDR_OK) {
    DBG("jd_decomp file %s error %d", path, res);
    return -2;
  }
  else {
    luat_lcd_flush(default_conf);
    return 0;
  }
}

