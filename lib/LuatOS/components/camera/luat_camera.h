/**************************************************** ***************************
 * CAMERA device operation abstraction layer
 * @author Dozingfiretruck
 * @since 0.0.1
 *************************************************** ****************************/
#ifndef Luat_CAMERA_H
#define Luat_CAMERA_H

#include "luat_base.h"
#ifdef __LUATOS__
#include "luat_lcd.h"
#endif
/**
 * @defgroup CAMERA CAMERA device (CAMERA)
 * @{*/
enum
{
	LUAT_CAMERA_FRAME_START = 0,
	LUAT_CAMERA_FRAME_END,
	LUAT_CAMERA_FRAME_RX_DONE,
	LUAT_CAMERA_FRAME_ERROR,

	LUAT_CAMERA_MODE_AUTO = 0,
	LUAT_CAMERA_MODE_SCAN,
};

typedef struct luat_camera_conf
{
    uint8_t id;
    uint8_t zbar_scan;
    uint8_t draw_lcd;
    uint8_t i2c_id;
    uint8_t i2c_addr;
    uint8_t pwm_id;
    size_t pwm_period;
    uint8_t pwm_pulse;
    uint16_t sensor_width;
    uint16_t sensor_height;
    uint8_t color_bit;
    uint8_t id_reg;
	uint8_t id_value;
    size_t init_cmd_size;
    uint8_t *init_cmd;
#ifdef __LUATOS__
    luat_lcd_conf_t* lcd_conf;
#else
    void *lcd_conf;
#endif
} luat_camera_conf_t;

typedef struct
{
	size_t  camera_speed;			//Provide camera clock frequency
	uint16_t sensor_width;			//Maximum width of camera
    uint16_t sensor_height;			//The maximum height of the camera
    uint8_t only_y;
	uint8_t rowScaleRatio;
	uint8_t colScaleRatio;
	uint8_t scaleBytes;
	uint8_t spi_mode;
	uint8_t is_msb;	//0 or 1;
	uint8_t is_two_line_rx; //0 or 1;
	uint8_t seq_type;	//0 or 1
	uint8_t plat_param[4];
#ifdef __LUATOS__
    luat_lcd_conf_t* lcd_conf;
#else
    void *lcd_conf;
#endif
} luat_spi_camera_t;
#ifdef __LUATOS__
int l_camera_handler(lua_State *L, void* ptr);
#endif
/**
 * @brief configure spi camera and initialize camera
 * @param id camera receives data bus ID, there are 2 on ec618, 0 and 1
 * @param conf camera related configuration
 * @param callback camera receives interrupt callback, note that this is the callback in the interrupt
 * @param param User parameters when interrupting callback
 * @return >=0 success, others failure*/
int luat_camera_setup(int id, luat_spi_camera_t *conf, void* callback, void *param);

/**
 * @brief configure image size
 * @param id camera receives data bus ID
 * @param w image width
 * @param h image height
 * @return >=0 success, others failure*/
int luat_camera_set_image_w_h(int id, uint16_t w, uint16_t h);

/**
 * @brief configure the camera and initialize the camera, do not use this for spi camera
 * @param conf camera related configuration
 * @return 0 success, others failure*/
int luat_camera_init(luat_camera_conf_t *conf);

/**
 * @brief Close camera and release resources
 * @param id camera receives data bus ID
 * @return 0 success, others failure*/
int luat_camera_close(int id);

/**
 * @brief The camera starts to receive data, dedicated to csdk
 * @param id camera receives data bus ID
 * @param buf user area address, if it is NULL, it means not to store it in the user area
 * @return 0 success, others failure*/
int luat_camera_start_with_buffer(int id, void *buf);
/**
 * @brief Camera switching receiving data buffer, dedicated to csdk
 * @param id camera receives data bus ID
 * @param buf user area address, if it is NULL, it means not to store it in the user area
 * @return 0 success, others failure*/
void luat_camera_continue_with_buffer(int id, void *buf);
/**
 * @brief Pause receiving camera data
 * @param id camera receives data bus ID
 * @param is_pause non-zero pause, 0 resume
 * @return 0 success, others failure*/
int luat_camera_pause(int id, uint8_t is_pause);
/** @brief Initialization of code scanning library
 * @param type Scan code library model, currently only supports 0
 * @param stack The stack address of the scan code library task
 * @param stack_length The stack depth of the code scanning library task. When type=0, at least 220KB is required.
 * @param priority Scanning library task priority
 * @return 0 success, others failure*/
int luat_camera_image_decode_init(uint8_t type, void *stack, uint32_t stack_length, uint32_t priority);
/** @brief Scan code library for one-time decoding
 * @param data buffer
 * @param image_w image width
 * @param image_h image height
 * @param timeout timeout
 * @param callback callback function
 * @param param callback parameter
 * @return 0 success, others failure*/
int luat_camera_image_decode_once(uint8_t *data, uint16_t image_w, uint16_t image_h, uint32_t timeout, void *callback, void *param);

/** @brief Scanning library de-initialization*/
void luat_camera_image_decode_deinit(void);
/** @brief Get the decoding result
 * @param buf buffer
 * @return 1 success, others failure*/
int luat_camera_image_decode_get_result(uint8_t *buf);

/**********The following is for luatos, do not use csdk*************/
/**
 * @brief Start receiving camera data
 * @param id camera receives data bus ID
 * @return 0 success, others failure*/
int luat_camera_start(int id);
/**
 * @brief Stop receiving camera data
 * @param id camera receives data bus ID
 * @return 0 success, others failure*/
int luat_camera_stop(int id);

int luat_camera_preview(int id, uint8_t on_off);

int luat_camera_work_mode(int id, int mode);

int luat_camera_capture(int id, uint8_t quality, const char *path);

int luat_camera_capture_in_ram(int id, uint8_t quality, void *buffer);
/** @}*/
#endif
