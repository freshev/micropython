
/*@Modules camera
@summary camera
@version 1.0
@date 2022.01.11
@democamera
@tag LUAT_USE_CAMERA*/
#include "luat_base.h"
#include "luat_camera.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include "luat_mem.h"
#include "luat_uart.h"
#include "luat_zbuff.h"
#define LUAT_LOG_TAG "camera"
#include "luat_log.h"

#define MAX_DEVICE_COUNT 2

typedef struct luat_camera_cb {
    int scanned;
} luat_camera_cb_t;
static luat_camera_cb_t camera_cbs[MAX_DEVICE_COUNT];

int l_camera_handler(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_pop(L, 1);
    int camera_id = msg->arg1;
    if (camera_cbs[camera_id].scanned) {
        lua_geti(L, LUA_REGISTRYINDEX, camera_cbs[camera_id].scanned);
        if (lua_isfunction(L, -1)) {
            lua_pushinteger(L, camera_id);
            if (msg->ptr)
            {
                lua_pushlstring(L, (char *)msg->ptr,msg->arg2);
            }
            else if (msg->arg2 > 1)
            {
            	lua_pushinteger(L, msg->arg2);
            }
            else
            {
            	lua_pushboolean(L, msg->arg2);
            }
            lua_call(L, 2, 0);
        }
    }
    lua_pushinteger(L, 0);
    return 1;
}

/*Initialize camera
@api camera.init(InitReg_or_cspi_id, cspi_speed, mode, is_msb, rx_bit, seq_type, is_ddr, only_y, scan_mode, w, h)
@table/integer If it is a table, it is the configuration of the DVP camera, see demo/camera/AIR105, and subsequent parameters are ignored; if it is a number, it is the camera spi bus serial number
@int camera spi bus speed
@int camera spi mode, 0~3
@int Whether the bit order of the bytes is msb, 0 no 1 yes
@int receives the number of bits at the same time, 1, 2, 4
@int byte sequence, 0~1
@int Dual edge sampling configuration, 0 is not enabled, other values   are determined according to the actual SOC
@int only receives Y component, 0 is not enabled, 1 is enabled, code scanning must be enabled, otherwise it will fail
@int working mode, camera.AUTO automatic, camera.SCAN code scanning
@int camera width
@int camera height
@return int/false Returns camera_id successfully, returns false if failed
@usage
camera_id = camera.init(GC032A_InitReg)--screen output rgb image
--After initialization, start is required to start output/scan code
camera.start(camera_id)--Start the specified camera*/

static int l_camera_init(lua_State *L){
	int result;
    if (lua_istable(L, 1)) {
    	luat_camera_conf_t conf = {0};
    	conf.lcd_conf = luat_lcd_get_default();
        lua_pushliteral(L, "zbar_scan");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.zbar_scan = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "draw_lcd");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.draw_lcd = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "i2c_id");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.i2c_id = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "i2c_addr");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.i2c_addr = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "pwm_id");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.pwm_id = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "pwm_period");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.pwm_period = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "pwm_pulse");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.pwm_pulse = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "sensor_width");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.sensor_width = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "sensor_height");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.sensor_height = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "color_bit");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.color_bit = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "id_reg");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.id_reg = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "id_value");
        lua_gettable(L, 1);
        if (lua_isinteger(L, -1)) {
            conf.id_value = luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "init_cmd");
        lua_gettable(L, 1);
        if (lua_istable(L, -1)) {
            conf.init_cmd_size = lua_rawlen(L, -1);
            conf.init_cmd = luat_heap_malloc(conf.init_cmd_size * sizeof(uint8_t));
            for (size_t i = 1; i <= conf.init_cmd_size; i++){
                lua_geti(L, -1, i);
                conf.init_cmd[i-1] = luaL_checkinteger(L, -1);
                lua_pop(L, 1);
            }
        }else if(lua_isstring(L, -1)){
            int len,cmd;
            const char *fail_name = luaL_checklstring(L, -1, &len);
            FILE* fd = luat_fs_fopen(fail_name, "rb");
            conf.init_cmd_size = 0;
            if (fd){
                #define INITCMD_BUFF_SIZE 128
                char init_cmd_buff[INITCMD_BUFF_SIZE] ;
                conf.init_cmd = luat_heap_malloc(sizeof(uint8_t));
                while (1) {
                    memset(init_cmd_buff, 0, INITCMD_BUFF_SIZE);
                    int readline_len = luat_fs_readline(init_cmd_buff, INITCMD_BUFF_SIZE-1, fd);
                    if (readline_len < 1)
                        break;
                    if (memcmp(init_cmd_buff, "#", 1)==0){
                        continue;
                    }
                    char *token = strtok(init_cmd_buff, ",");
                    if (sscanf(token,"%x",&cmd) < 1){
                        continue;
                    }
                    conf.init_cmd_size = conf.init_cmd_size + 1;
                    conf.init_cmd = luat_heap_realloc(conf.init_cmd,conf.init_cmd_size * sizeof(uint8_t));
                    conf.init_cmd[conf.init_cmd_size-1]=cmd;
                    while( token != NULL ) {
                        token = strtok(NULL, ",");
                        if (sscanf(token,"%x",&cmd) < 1){
                            break;
                        }
                        conf.init_cmd_size = conf.init_cmd_size + 1;
                        conf.init_cmd = luat_heap_realloc(conf.init_cmd,conf.init_cmd_size * sizeof(uint8_t));
                        conf.init_cmd[conf.init_cmd_size-1]=cmd;
                    }
                }
                conf.init_cmd[conf.init_cmd_size]= 0;
                luat_fs_fclose(fd);
            }else{
                LLOGE("init_cmd fail open error");
            }
        }
        lua_pop(L, 1);
        result = luat_camera_init(&conf);
        if (result < 0) {
        	lua_pushboolean(L, 0);
        } else {
        	lua_pushinteger(L, result);
        }

    } else if (lua_isinteger(L, 1)) {
    	luat_spi_camera_t conf = {0};
    	conf.lcd_conf = luat_lcd_get_default();
    	int cspi_id = lua_tointeger(L, 1);
    	int default_value = 24000000;
    	conf.camera_speed = lua_tointegerx(L, 2, &default_value);
    	default_value = 0;
    	conf.spi_mode = lua_tointegerx(L, 3, &default_value);
    	conf.is_msb = lua_tointegerx(L, 4, &default_value);
    	conf.is_two_line_rx = lua_tointegerx(L, 5, &default_value) - 1;
    	conf.seq_type = lua_tointegerx(L, 6, &default_value);
    	result = lua_tointegerx(L, 7, &default_value);
    	memcpy(conf.plat_param, &result, 4);
    	conf.only_y = lua_tointegerx(L, 8, &default_value);
    	int mode = lua_tointegerx(L, 9, &default_value);
    	default_value = 240;
    	conf.sensor_width = lua_tointegerx(L, 10, &default_value);
    	default_value = 320;
    	conf.sensor_height = lua_tointegerx(L, 11, &default_value);
    	luat_camera_init(NULL);
    	luat_camera_work_mode(cspi_id, mode);
    	result = luat_camera_setup(cspi_id, &conf, NULL, 0);

        if (result < 0) {
        	lua_pushboolean(L, 0);
        } else {
        	lua_pushinteger(L, result);
        }
    } else {
    	lua_pushboolean(L, 0);
    }

    return 1;
}

/**
Register camera event callback
@api camera.on(id, event, func)
@int camera id, write 0 for camera 0, write 1 for camera 1
@string event name
@function callback method
@return nil no return value
@usage
camera.on(0, "scanned", function(id, str)
--id int camera id
--str Multiple types false The camera is not working properly, true The photo is luat successfully and saved in the photo mode, int The size of the data returned this time in the original data mode, string The decoded value after the code scan is successful in the scan mode
    print(id, str)
end)*/
static int l_camera_on(lua_State *L) {
    int camera_id = luaL_checkinteger(L, 1);
    const char* event = luaL_checkstring(L, 2);
    if (!strcmp("scanned", event)) {
        if (camera_cbs[camera_id].scanned != 0) {
            luaL_unref(L, LUA_REGISTRYINDEX, camera_cbs[camera_id].scanned);
            camera_cbs[camera_id].scanned = 0;
        }
        if (lua_isfunction(L, 3)) {
            lua_pushvalue(L, 3);
            camera_cbs[camera_id].scanned = luaL_ref(L, LUA_REGISTRYINDEX);
        }
    }
    return 0;
}

/**
Start the specified camera
@api camera.start(id)
@int camera id, for example 0
@return boolean returns true if successful, otherwise returns false
@usage
camera.start(0)*/
static int l_camera_start(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, luat_camera_start(id) == 0 ? 1 : 0);
    return 1;
}

/**
Stop the specified camera
@api camera.stop(id)
@int camera id, for example 0
@return boolean returns true if successful, otherwise returns false
@usage
camera.stop(0)*/
static int l_camera_stop(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, luat_camera_stop(id) == 0 ? 1 : 0);
    return 1;
}

/**
Close the specified camera and release the corresponding IO resources
@api camera.close(id)
@int camera id, for example 0
@return boolean returns true if successful, otherwise returns false
@usage
camera.close(0)*/
static int l_camera_close(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, luat_camera_close(id) == 0 ? 1 : 0);
    return 1;
}

LUAT_WEAK int luat_camera_setup(int id, luat_spi_camera_t *conf, void * callback, void *param) {
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_capture(int id, uint8_t quality, const char *path) {
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_capture_in_ram(int id, uint8_t quality, void *buffer) {
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_get_raw_start(int id, int w, int h, uint8_t *data, uint32_t max_len) {
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_get_raw_again(int id) {
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_video(int id, int w, int h, uint8_t uart_id) {
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_preview(int id, uint8_t on_off){
    LLOGD("not support yet");
    return -1;
}

LUAT_WEAK int luat_camera_work_mode(int id, int mode){
    LLOGD("not support yet");
    return -1;
}
/**
camera to take pictures
@api camera.capture(id, save_path, quality)
@int camera id, for example 0
@string/zbuff/nil save_path, the file saving path. If it is empty, it will be written in the last path. The default is /capture.jpg. If it is zbuff, the picture will be saved in the buff without writing to the file system.
@int quality, jpeg compression quality, 1 is the worst, takes up less space, 3 is the highest, takes up the most space and is time-consuming, the default is 1
@return boolean Returns true successfully, otherwise returns false. After the actual completion, the length received through the callback function set by camera.on
@usage
camera.capture(0)*/
static int l_camera_capture(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int quality = luaL_optinteger(L, 3, 1);
    if (lua_isstring(L, 2)){
    	const char* save_path = luaL_checkstring(L, 2);
    	lua_pushboolean(L, !luat_camera_capture(id, quality, save_path));
    } else {
    	luat_zbuff_t *buff = luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE);
    	lua_pushboolean(L, !luat_camera_capture_in_ram(id, quality, buff));
    }
    return 1;
}

/**
camera output video stream to USB
@api camera.video(id, w, h, out_path)
@int camera id, for example 0
@int width
@int height
@int output path, currently only virtual serial port 0 can be used
@return boolean returns true if successful, otherwise returns false
@usage
camera.video(0, 320, 240, uart.VUART_0)*/
static int l_camera_video(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int w = luaL_optinteger(L, 2, 320);
    int h = luaL_optinteger(L, 3, 240);
    int param = luaL_optinteger(L, 4, LUAT_VUART_ID_0);
    lua_pushboolean(L, !luat_camera_video(id, w, h, param));
    return 1;
}


/**
Start the camera to output raw data to the user's zbuff buffer area. It will stop after outputting 1fps, and call back the received length through the callback function set by camera.on. If you need to output again, please call camera.getRaw
@api camera.startRaw(id, w, h, buff)
@int camera id, for example 0
@int width
@int height
@zbuff The cache area used to store data must be no less than w X h X 2 byte
@return boolean returns true if successful, otherwise returns false
@usage
camera.startRaw(0, 320, 240, buff)*/
static int l_camera_start_raw(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    int w = luaL_optinteger(L, 2, 320);
    int h = luaL_optinteger(L, 3, 240);
    luat_zbuff_t *buff = luaL_checkudata(L, 4, LUAT_ZBUFF_TYPE);
    lua_pushboolean(L, !luat_camera_get_raw_start(id, w, h, buff->addr, buff->len));
    return 1;
}

/**
Start the camera again to output the original data to the user's zbuff cache. It will stop after outputting 1fps, and call back the received length through the callback function set by camera.on. If you need to output again, please continue to call this API.
@api camera.getRaw(id)
@int camera id, for example 0
@return boolean returns true if successful, otherwise returns false
@usage
camera.getRaw(0)*/
static int l_camera_get_raw(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    lua_pushboolean(L, !luat_camera_get_raw_again(id));
    return 1;
}

/**
Start and stop camera preview function, output directly to LCD, only SOC supported by hardware can run
@api camera.preview(id, onoff)
@int camera id, for example 0
@boolean true to enable, false to stop
@return boolean returns true if successful, otherwise returns false
@usage
camera.preview(1, true)*/
static int l_camera_preview(lua_State *L) {
    int id = luaL_checkinteger(L, 1);
    uint8_t onoff = lua_toboolean(L, 2);
    lua_pushboolean(L, !luat_camera_preview(id, onoff));
    return 1;
}


#include "rotable2.h"
static const rotable_Reg_t reg_camera[] =
{
    { "init" ,       ROREG_FUNC(l_camera_init )},
    { "start" ,      ROREG_FUNC(l_camera_start )},
	{ "preview",     ROREG_FUNC(l_camera_preview)},
    { "stop" ,       ROREG_FUNC(l_camera_stop)},
    { "capture",     ROREG_FUNC(l_camera_capture)},
	{ "video",     ROREG_FUNC(l_camera_video)},
	{ "startRaw",     ROREG_FUNC(l_camera_start_raw)},
	{ "getRaw",     ROREG_FUNC(l_camera_get_raw)},
	{ "close",		 ROREG_FUNC(l_camera_close)},
    { "on",          ROREG_FUNC(l_camera_on)},

    //@const AUTO number The camera works in automatic mode
	{ "AUTO",             ROREG_INT(LUAT_CAMERA_MODE_AUTO)},
    //@const SCAN number The camera works in scanning mode and only outputs the Y component.
	{ "SCAN",             ROREG_INT(LUAT_CAMERA_MODE_SCAN)},
	{ NULL,          {}}
};

LUAMOD_API int luaopen_camera( lua_State *L ) {
    luat_newlib2(L, reg_camera);
    return 1;
}

