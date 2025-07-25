/*
 * This file is part of the MicroPython ESP32 project, https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 LoBo (https://github.com/loboris)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task.h"

#include "py/mpstate.h"
#include "py/runtime.h"
#include "py/obj.h"

#include "modmachine.h"
#include "mphalport.h"

#define I2C_ACK_CHECK_EN            (1)
#define I2C_RX_MAX_BUFF_LEN         2048  // maximum low level commands receive buffer length
#define I2C_SLAVE_DEFAULT_BUFF_LEN  256   // default slave buffer length
#define I2C_SLAVE_ADDR_DEFAULT      44    // default slave address
#define I2C_SLAVE_MUTEX_TIMEOUT     (500 / portTICK_PERIOD_MS)
#define I2C_SLAVE_READ_TIMEOUT      (10 / portTICK_PERIOD_MS)

#define I2C_SLAVE_TASK_STACK_SIZE   832

#define CONFIG_MICROPY_TASK_PRIORITY (ESP_TASK_PRIO_MIN + 2)
#define I2C_SLAVE_MIN_BUFFER_LENGTH I2C_SLAVE_DEFAULT_BUFF_LEN
#define I2C_SLAVE_MAX_BUFFER_LENGTH 0x1000
const char * I2C_DEBUG_TAG ="[I2C]";
static const char *I2C_TAG = "i2c";

typedef struct _mp_machine_i2c_obj_t {
    mp_obj_base_t base;
    uint32_t speed;
    uint8_t mode;
    uint8_t scl;
    uint8_t sda;
    int8_t bus_id;
    i2c_cmd_handle_t cmd;
    uint16_t rx_buflen;      // low level commands receive buffer length
    uint16_t rx_bufidx;      // low level commands receive buffer index
    uint8_t *rx_data;        // low level commands receive buffer
    int8_t slave_addr;       // slave only, slave 8-bit address
    uint16_t slave_rbuflen;  // slave only, read buffer length
    uint16_t slave_wbuflen;  // slave only, write buffer length
    uint32_t *slave_cb;      // slave only, slave callback function
    uint8_t *cbrx_data;      // callback rx buffer
    uint16_t cbrx_len;       // callback rx buffer length
    bool slave_busy;
} mp_machine_i2c_obj_t;


const mp_obj_type_t machine_hw_i2c_type;

static int i2c_used[I2C_MODE_MAX] = { -1, -1 };
static QueueHandle_t slave_mutex[I2C_MODE_MAX] = { NULL, NULL };
static TaskHandle_t i2c_slave_task_handle = NULL;

// ============================================================================================
// === Low level I2C functions using esp-idf i2c-master driver ================================
// ============================================================================================

void machine_i2c_init0() {
    for (int p = 0; p < I2C_MODE_MAX; p++) {
        i2c_reset_rx_fifo(p);
        i2c_reset_tx_fifo(p);
    }
}

//--------------------------------------------------------------
STATIC esp_err_t i2c_init_master (mp_machine_i2c_obj_t *i2c_obj)
{
    i2c_config_t conf;

    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = i2c_obj->sda;
    conf.scl_io_num = i2c_obj->scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = i2c_obj->speed;
    conf.clk_flags = 0;

    i2c_param_config(i2c_obj->bus_id, &conf);
    return i2c_driver_install(i2c_obj->bus_id, I2C_MODE_MASTER, 0, 0, ESP_INTR_FLAG_IRAM);
}

//------------------------------------------------------------------------
STATIC esp_err_t i2c_init_slave (mp_machine_i2c_obj_t *i2c_obj, bool busy)
{
    i2c_config_t conf;

    conf.mode = I2C_MODE_SLAVE;
    conf.sda_io_num = i2c_obj->sda;
    conf.scl_io_num = i2c_obj->scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.slave.addr_10bit_en = 0;
    conf.slave.slave_addr = i2c_obj->slave_addr;
    conf.slave.maximum_speed = i2c_obj->speed;
    conf.clk_flags = 0;

    i2c_param_config(i2c_obj->bus_id, &conf);
    return i2c_driver_install(i2c_obj->bus_id, conf.mode, i2c_obj->slave_rbuflen, i2c_obj->slave_wbuflen, ESP_INTR_FLAG_IRAM);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------------
STATIC int mp_i2c_master_write(mp_machine_i2c_obj_t *i2c_obj, uint16_t slave_addr, uint8_t memwrite, uint32_t memaddr, uint8_t *data, uint16_t len, bool stop)
{
    esp_err_t ret = ESP_FAIL;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if (i2c_master_start(cmd) != ESP_OK) {ret=1; goto error;};
    // send slave address
    if (i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN) != ESP_OK) {ret=2; goto error;};

    while (memwrite > 0) {
        // send memory address, MSByte first
        memwrite--;
        if (i2c_master_write_byte(cmd, (memaddr >> (memwrite*8)), I2C_ACK_CHECK_EN) != ESP_OK) {ret=3; goto error;};
    }

    // send data
    if ((data) && (len > 0)) {
        if (i2c_master_write(cmd, data, len, I2C_ACK_CHECK_EN) != ESP_OK) {ret=5; goto error;};
    }
    if (stop) {
        if (i2c_master_stop(cmd) != ESP_OK) {ret=6; goto error;};
    }

    ret = i2c_master_cmd_begin(i2c_obj->bus_id, cmd, (5000 + (1000 * len)) / portTICK_PERIOD_MS);

error:
    i2c_cmd_link_delete(cmd);

    return ret;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
STATIC int mp_i2c_master_read(mp_machine_i2c_obj_t *i2c_obj, uint16_t slave_addr, uint8_t memread, uint32_t memaddr, uint8_t *data, uint16_t len, bool stop)
{
    esp_err_t ret = ESP_FAIL;

    memset(data, 0xFF, len);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if (i2c_master_start(cmd) != ESP_OK) {ret=1; goto error;};

    if (memread) {
        // send slave address
        if (i2c_master_write_byte(cmd, ( slave_addr << 1 ) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN) != ESP_OK) {ret=2; goto error;};

        if (memread > 0) {
            // send memory address, MSByte first
            while (memread > 0) {
                // send memory address, MSByte first
                memread--;
                if (i2c_master_write_byte(cmd, (memaddr >> (memread*8)), I2C_ACK_CHECK_EN) != ESP_OK) {ret=3; goto error;};
            }

            if (stop) {
                // Finish the write transaction
                if (i2c_master_stop(cmd) != ESP_OK) {ret=5; goto error;};
                    if (i2c_master_cmd_begin(i2c_obj->bus_id, cmd, 100 / portTICK_PERIOD_MS) != ESP_OK) {ret=6; goto error;};
                    i2c_cmd_link_delete(cmd);
                    // Start the read transaction
                    cmd = i2c_cmd_link_create();
                    if (i2c_master_start(cmd) != ESP_OK) {ret=7; goto error;};
            } else {
                // repeated start, generate START signal, slave address will be send next
                if (i2c_master_start(cmd) != ESP_OK) {ret=4; goto error;};
            }
        }
    }

    // READ, send slave address
    if (i2c_master_write_byte(cmd, ( slave_addr << 1 ) | I2C_MASTER_READ, I2C_ACK_CHECK_EN) != ESP_OK) {ret=8; goto error;};

    if (len > 1) {
        if (i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK) != ESP_OK) {ret=9; goto error;};
    }

    if (i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK) != ESP_OK) {ret=10; goto error;};
    if (i2c_master_stop(cmd) != ESP_OK) {ret=11; goto error;};

    ret = i2c_master_cmd_begin(i2c_obj->bus_id, cmd, (5000 + (1000 * len)) / portTICK_PERIOD_MS);

error:
    i2c_cmd_link_delete(cmd);

    return ret;
}

//----------------------------------------------------------------------------------
STATIC bool hw_i2c_slave_detect (mp_machine_i2c_obj_t *i2c_obj, uint16_t slave_addr)
{
    esp_err_t ret = ESP_FAIL;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    if (i2c_master_start(cmd) != ESP_OK) goto error;
    if (i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN) != ESP_OK) goto error;
    if (i2c_master_stop(cmd) != ESP_OK) goto error;
    ret = i2c_master_cmd_begin(i2c_obj->bus_id, cmd, 500 / portTICK_PERIOD_MS);

error:
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? true : false;
}

//-----------------------------------------------------------------------------------
// Attention: using ESP_LOGX(...) corrupts stack. Do not use it in regular operation
// Also avoid any printf and scanf functions
//-----------------------------------------------------------------------------------
STATIC void i2c_slave_task(void *self_in)
{
    int res;
    bool to_exit = false;

    mp_machine_i2c_obj_t *i2c_obj = (mp_machine_i2c_obj_t *)self_in;
    if (i2c_obj->slave_rbuflen == 0) goto exit;

    if (i2c_obj->cbrx_data != NULL) free(i2c_obj->cbrx_data);
    i2c_obj->cbrx_data = malloc(i2c_obj->slave_rbuflen);
    if (i2c_obj->cbrx_data == NULL) goto exit;

    while (1) {
        if (i2c_obj != NULL && i2c_obj->bus_id < I2C_NUM_MAX) {
            //if (slave_mutex[i2c_obj->bus_id]) xSemaphoreTake(slave_mutex[i2c_obj->bus_id], I2C_SLAVE_MUTEX_TIMEOUT); // was I2C_SLAVE_MUTEX_TIMEOUT
            if (slave_mutex[i2c_obj->bus_id]) xSemaphoreTake(slave_mutex[i2c_obj->bus_id], I2C_SLAVE_READ_TIMEOUT); // was I2C_SLAVE_MUTEX_TIMEOUT

            if (i2c_obj->cbrx_data != NULL) {
                int log_level = esp_log_level_get(I2C_TAG);
                esp_log_level_set(I2C_TAG, 0);
                res = i2c_slave_read_buffer(i2c_obj->bus_id, i2c_obj->cbrx_data, i2c_obj->slave_rbuflen, I2C_SLAVE_READ_TIMEOUT);
                esp_log_level_set(I2C_TAG, log_level);

                if (res > 0) {
                    i2c_obj->cbrx_len = res;
                    //ESP_LOGI(I2C_DEBUG_TAG, "Received %d B", res);
                    if (i2c_obj->slave_cb) {
                        mp_sched_schedule(i2c_obj->slave_cb, MP_OBJ_FROM_PTR(i2c_obj));
                    }
                } else if(res < 0) {
                    //ESP_LOGI(I2C_DEBUG_TAG, "Slave read buffer returns %d", res);
                    to_exit = true;
                }
            } else {
               ESP_LOGW(I2C_DEBUG_TAG, "Slave callback buffer is NULL!");
            }
            if (slave_mutex[i2c_obj->bus_id]) xSemaphoreGive(slave_mutex[i2c_obj->bus_id]);

        } else {
            ESP_LOGW(I2C_DEBUG_TAG, "I2C object deleted");
            to_exit = true;
        }

        vTaskDelay(I2C_SLAVE_READ_TIMEOUT);
        if(to_exit) break;
    }
exit:
    if(i2c_obj->cbrx_data != NULL) free(i2c_obj->cbrx_data);
    i2c_obj->cbrx_data = NULL;
    i2c_slave_task_handle = NULL;
    vTaskDelete(NULL);
}

// ============================================================================================
// === I2C MicroPython bindings ===============================================================
// ============================================================================================

enum { ARG_id, ARG_mode, ARG_speed, ARG_freq, ARG_sda, ARG_scl, ARG_slaveaddr, ARG_rbuflen, ARG_wbuflen, ARG_busy };

// Arguments for new object and init method
//----------------------------------------------------------
STATIC const mp_arg_t mp_machine_i2c_init_allowed_args[] = {
    { MP_QSTR_id,    MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_mode,  MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_speed, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_freq,  MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_sda,   MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_scl,   MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_slave_addr,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_slave_rbuflen, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_slave_wbuflen, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    { MP_QSTR_slave_busy,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
};

//----------------------------------------------
static uint8_t * get_buffer(void *buff, int len)
{
    if (len > 0) {
        uint8_t *buf = heap_caps_malloc(len, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        if (buf == NULL) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error allocating I2C data buffer"));
        }
        memcpy(buf, buff, len);
        return buf;
    }
    return NULL;
}

//----------------------------------
static void i2c_check_error(int err)
{
    if (err != ESP_OK) {
        char errs[32];
        sprintf(errs, "I2C bus error (%d)", err);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT(errs));
    }
}

//----------------------------------
STATIC void _checkAddr(uint8_t addr)
{
    if ((addr < 0x08) || (addr > 0x77)) {
        mp_raise_ValueError("Wrong i2c address (0x08 - 0x77 allowed)");
    }
}

//--------------------------------------------------
STATIC void _checkMaster(mp_machine_i2c_obj_t *self)
{
    if (self->mode != I2C_MODE_MASTER) {
        mp_raise_ValueError("I2C not in MASTER mode)");
    }
}

//-------------------------------------------------
STATIC void _checkSlave(mp_machine_i2c_obj_t *self)
{
    if (self->mode != I2C_MODE_SLAVE) {
        mp_raise_ValueError("I2C not in SLAVE mode)");
   }
}

//-----------------------------------------------------------------------------------------------
STATIC void mp_machine_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_printf(print, "I2C start");
    mp_machine_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (i2c_used[self->bus_id] >= 0) {
        if (self->mode == I2C_MODE_MASTER)
            mp_printf(print, "I2C (Port=%u, Mode=MASTER, Speed=%u Hz, sda=%d, scl=%d)", self->bus_id, self->speed, self->sda, self->scl);
        else {
            mp_printf(print, "I2C (Port=%u, Mode=SLAVE, Speed=%u Hz, sda=%d, scl=%d, addr=%d, read_buffer=%d B, write_buffer=%d B)",
                      self->bus_id, self->speed, self->sda, self->scl, self->slave_addr, self->slave_rbuflen, self->slave_wbuflen);

            mp_printf(print, "\n     Callback=%s Data=%d B", self->slave_cb ? "True" : "False", self->cbrx_len);
            if (i2c_slave_task_handle) {
                 mp_printf(print, "\n     I2C task minimum free stack: %u", uxTaskGetStackHighWaterMark(i2c_slave_task_handle));
            }
        }
    }
    else {
        mp_printf(print, "I2C (Deinitialized)");
    }
}

//---------------------------------------------------------------------------------------------------------------
mp_obj_t mp_machine_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_val_t args[MP_ARRAY_SIZE(mp_machine_i2c_init_allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(mp_machine_i2c_init_allowed_args), mp_machine_i2c_init_allowed_args, args);

    int8_t sda;
    int8_t scl;
    int32_t speed;
    int8_t bus_id = args[ARG_id].u_int;
    if (bus_id < 0) bus_id = I2C_NUM_0;

    if (args[ARG_freq].u_int > 0) speed = args[ARG_freq].u_int;
    else speed = args[ARG_speed].u_int;
    if (speed < 0) speed = 100000;

    // Check the peripheral id
    if (bus_id < 0 || bus_id > 1) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C bus not available"));
    }
    if (i2c_used[bus_id] >= 0) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C bus already used"));
    }
    // Check mode
    int mode = args[ARG_mode].u_int;
    if (mode < 0) mode = I2C_MODE_MASTER;
    if ((mode != I2C_MODE_MASTER) && (mode != I2C_MODE_SLAVE)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("MASTER or SLAVE mode must be selected"));
    }

    if ((args[ARG_sda].u_obj == MP_OBJ_NULL) || (args[ARG_sda].u_obj == MP_OBJ_NULL)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("sda & scl must be given"));
    }
    sda = machine_pin_get_id(args[ARG_sda].u_obj);
    scl = machine_pin_get_id(args[ARG_scl].u_obj);

    // Create I2C object
    mp_machine_i2c_obj_t *self = m_new_obj(mp_machine_i2c_obj_t );
    self->base.type = &machine_hw_i2c_type;
    self->mode = mode;
    self->bus_id = bus_id;
    self->speed = speed;
    self->scl = scl;
    self->sda = sda;
    self->cmd = NULL;
    self->rx_buflen = 0;
    self->rx_bufidx = 0;
    self->rx_data = NULL;
    self->slave_rbuflen = 0;
    self->slave_wbuflen = 0;
    self->slave_addr = 0;
    self->slave_cb = NULL;
    self->slave_busy = false;
    self->cbrx_data = NULL; // allocated in callback task
    self->cbrx_len = 0;

    if (mode == I2C_MODE_MASTER) {
        // Setup I2C master
        if (i2c_init_master(self) != ESP_OK) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error installing I2C driver (init master failed)"));
        }
    }
    else {
        if (args[ARG_busy].u_int == 1) self->slave_busy = true;

        if (slave_mutex[self->bus_id] == NULL) {
            slave_mutex[self->bus_id] = xSemaphoreCreateMutex();
        }
        // Set I2C slave address
        if (args[ARG_slaveaddr].u_int > 0) {
            _checkAddr(args[ARG_slaveaddr].u_int);
            self->slave_addr = args[ARG_slaveaddr].u_int;
        }
        else self->slave_addr = I2C_SLAVE_ADDR_DEFAULT;

        // Set I2C slave buffers
        if ((args[ARG_rbuflen].u_int >= I2C_SLAVE_MIN_BUFFER_LENGTH) && (args[ARG_rbuflen].u_int <= I2C_SLAVE_MAX_BUFFER_LENGTH)) 
            self->slave_rbuflen = args[ARG_rbuflen].u_int;
        else self->slave_rbuflen = I2C_SLAVE_DEFAULT_BUFF_LEN;


        //if ((args[ARG_wbuflen].u_int > 0) && (args[ARG_wbuflen].u_int < (self->slave_rbuflen / 2)))
        //    self->slave_wbuflen = args[ARG_wbuflen].u_int;
        //else self->slave_wbuflen = 0;
        if (args[ARG_wbuflen].u_int > 0) self->slave_wbuflen = args[ARG_wbuflen].u_int;
        else self->slave_wbuflen = 0;

        if ((self->slave_busy) && (self->slave_wbuflen == 0)) self->slave_wbuflen = 1;

        if (i2c_init_slave(self, self->slave_busy) != ESP_OK) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error installing I2C driver (init slave failed)"));
        }

        //if (i2c_slave_task_handle == NULL) {
        //    int res = xTaskCreate(i2c_slave_task, "i2c_slave_task", I2C_SLAVE_TASK_STACK_SIZE, (void *)self, CONFIG_MICROPY_TASK_PRIORITY, &i2c_slave_task_handle);
        //    if (res != pdPASS) {
        //        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error installing I2C driver (slave task handle)"));
        //    }
        //    vTaskDelay(100 / portTICK_PERIOD_MS);
        //}
    }

    i2c_used[bus_id] = mode;

    return MP_OBJ_FROM_PTR(self);
}

//------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_machine_i2c_obj_t *self = pos_args[0];

    mp_arg_val_t args[MP_ARRAY_SIZE(mp_machine_i2c_init_allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(mp_machine_i2c_init_allowed_args), mp_machine_i2c_init_allowed_args, args);

    int8_t sda, scl, slave_addr;
    int32_t speed;
    uint8_t changed = 0;
    int buff_len, ro_len;

    int8_t old_bus_id = self->bus_id;
    int8_t bus_id = args[ARG_id].u_int;
    if (bus_id < 0) bus_id = self->bus_id;

    if (args[ARG_freq].u_int > 0) speed = args[ARG_freq].u_int;
    else speed = args[ARG_speed].u_int;
    if (speed < 0) speed = self->speed;

    // Check the peripheral id
    if (bus_id < 0 || bus_id > 1) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C bus not available"));
    }
    // Check mode
    int mode = args[ARG_mode].u_int;
    if ((mode >= 0) && (mode != self->mode)) {
        if ((mode != I2C_MODE_MASTER) && (mode != I2C_MODE_SLAVE)) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("MASTER or SLAVE mode must be selected"));
        }
        changed++;
    }
    else mode = self->mode;

    scl = self->scl;
    sda = self->sda;
    buff_len = self->slave_rbuflen;
    ro_len = self->slave_wbuflen;
    slave_addr = self->slave_addr;

    if (args[ARG_sda].u_obj != MP_OBJ_NULL) {
        sda = machine_pin_get_id(args[ARG_sda].u_obj);
    }
    if (args[ARG_scl].u_obj != MP_OBJ_NULL) {
        scl = machine_pin_get_id(args[ARG_scl].u_obj);
    }

    // Check if the configuration changed
    if (old_bus_id != bus_id) changed++;
    if (self->speed != speed) changed++;
    if (self->scl != scl) changed++;
    if (self->sda != sda) changed++;

    if (mode == I2C_MODE_SLAVE) {
        if (args[ARG_busy].u_int >= 0) {
            if (self->slave_busy != ((args[ARG_busy].u_int == 1) ? true : false)) {
                self->slave_busy = (args[ARG_busy].u_int == 1) ? true : false;
                if ((self->slave_busy) && (self->slave_wbuflen == 0)) self->slave_wbuflen = 1;
                changed++;
            }
        }
        if (args[ARG_slaveaddr].u_int > 0) {
            _checkAddr(args[ARG_slaveaddr].u_int);
            if (args[ARG_slaveaddr].u_int != slave_addr) {
                slave_addr = args[ARG_slaveaddr].u_int;
                changed++;
            }
        }
        if ((args[ARG_rbuflen].u_int >= I2C_SLAVE_MIN_BUFFER_LENGTH) && (args[ARG_rbuflen].u_int <= I2C_SLAVE_MAX_BUFFER_LENGTH)) {
            if (args[ARG_rbuflen].u_int != buff_len) {
                buff_len = args[ARG_rbuflen].u_int;
                changed++;
            }
        }
        if ((args[ARG_wbuflen].u_int > 0) && (args[ARG_wbuflen].u_int < (buff_len/2))) {
            if (args[ARG_wbuflen].u_int != ro_len) {
                ro_len = args[ARG_wbuflen].u_int;
                if ((self->slave_busy) && (self->slave_wbuflen == 0)) self->slave_wbuflen = 1;
                changed++;
            }
        }
    }

    if (changed) {
        if (i2c_used[old_bus_id] >= 0) {
            // Delete old driver, if it was a slave, the task will be stopped
            i2c_used[old_bus_id] = -1;
            i2c_driver_delete(old_bus_id);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        if (self->mode == I2C_MODE_SLAVE) {
            if (slave_mutex[self->bus_id] == NULL) {
                slave_mutex[self->bus_id] = xSemaphoreCreateMutex();
            }
            if (slave_mutex[self->bus_id]) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);
        }
        if (i2c_used[bus_id] >= 0) {
            if ((self->mode == I2C_MODE_SLAVE) && (slave_mutex[self->bus_id])) xSemaphoreGive(slave_mutex[self->bus_id]);
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("MASTER or SLAVE mode must be selected"));
        }

        self->bus_id = bus_id;
        self->speed = speed;
        self->scl = scl;
        self->sda = sda;
        if (mode == I2C_MODE_MASTER) {
            if (i2c_init_master(self) != ESP_OK) {
                mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error installing I2C driver (init master failed)"));
            }
        } 
        else {
            // Setup I2C slave
            self->slave_addr = slave_addr;
            self->slave_rbuflen = buff_len;
            self->slave_wbuflen = ro_len;
            if (i2c_init_slave(self, self->slave_busy) != ESP_OK) {
                if (slave_mutex[self->bus_id]) xSemaphoreGive(slave_mutex[self->bus_id]);
                mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error installing I2C driver (init slave failed)"));
            }
        }
        i2c_used[bus_id] = mode;
        if ((self->mode == I2C_MODE_SLAVE) && (slave_mutex[self->bus_id])) xSemaphoreGive(slave_mutex[self->bus_id]);
        self->mode = mode;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_init_obj, 1, mp_machine_i2c_init);

//---------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_scan(mp_obj_t self_in)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkMaster(self);

    mp_obj_t list = mp_obj_new_list(0, NULL);

    // don't include in scan the reserved 7-bit addresses: 0x00-0x07 & 0x78-0x7F

    for (int addr = 0x08; addr < 0x78; ++addr) {
        if (hw_i2c_slave_detect(self, addr)) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }
    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_scan_obj, mp_machine_i2c_scan);

//-------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_is_ready(mp_obj_t self_in, mp_obj_t addr_in)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkMaster(self);

    int addr = mp_obj_get_int(addr_in);
    _checkAddr(addr);

    return hw_i2c_slave_detect(self, addr) ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_i2c_is_ready_obj, mp_machine_i2c_is_ready);

//----------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_readfrom(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    STATIC const mp_arg_t machine_i2c_readfrom_args[] = {
        { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_nbytes,  MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
    };

    mp_machine_i2c_obj_t *self = pos_args[0];
    _checkMaster(self);

    // parse arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_readfrom_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), machine_i2c_readfrom_args, args);

    _checkAddr(args[0].u_int);

    if (args[1].u_int > 0) {
        uint8_t *buf = heap_caps_malloc(args[1].u_int, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        if (buf == NULL) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error allocating I2C data buffer"));
        }
        int ret = mp_i2c_master_read(self, args[0].u_int, false, 0, buf, args[1].u_int, false);
        if (ret != ESP_OK) {
            free(buf);
            i2c_check_error(ret);
        }
        vstr_t vstr;
        vstr_init_len(&vstr, args[1].u_int);
        memcpy(vstr.buf, buf, args[1].u_int);
        free(buf);
        // Return read data as string

        //return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
        return mp_obj_new_str_from_vstr(&vstr);
    }
    return mp_const_empty_bytes;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_readfrom_obj, 1, mp_machine_i2c_readfrom);

//---------------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_readfrom_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_machine_i2c_obj_t *self = pos_args[0];
    _checkMaster(self);

    STATIC const mp_arg_t machine_i2c_readfrom_into_args[] = {
        { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_buf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    // parse arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_readfrom_into_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), machine_i2c_readfrom_into_args, args);

    _checkAddr(args[0].u_int);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1].u_obj, &bufinfo, MP_BUFFER_WRITE);

    uint8_t *buf = get_buffer(bufinfo.buf, bufinfo.len);
    if (buf) {
        int ret = mp_i2c_master_read(self, args[0].u_int, false, 0, buf, bufinfo.len, false);
        if (ret != ESP_OK) {
            free(buf);
            i2c_check_error(ret);
        }
        memcpy(bufinfo.buf, buf, bufinfo.len);
        free(buf);
    }
    // Return read length
    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_readfrom_into_obj, 1, mp_machine_i2c_readfrom_into);

//---------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_writeto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    mp_machine_i2c_obj_t *self = pos_args[0];
    _checkMaster(self);

    STATIC const mp_arg_t machine_i2c_writeto_args[] = {
        { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_buf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop,    MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
    };

    // parse arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_writeto_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), machine_i2c_writeto_args, args);

    _checkAddr(args[0].u_int);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1].u_obj, &bufinfo, MP_BUFFER_READ);

    uint8_t *buf = get_buffer(bufinfo.buf, bufinfo.len);
    if (buf) {
        int ret = mp_i2c_master_write(self, args[0].u_int, 0, 0, buf, bufinfo.len, args[2].u_bool);
        free(buf);
        i2c_check_error(ret);
    }

    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_writeto_obj, 1, mp_machine_i2c_writeto);

// Arguments for memory read/write methods
//---------------------------------------------------------
STATIC const mp_arg_t mp_machine_i2c_mem_allowed_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    { MP_QSTR_arg,     MP_ARG_REQUIRED | MP_ARG_OBJ,  {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_adrlen,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 1} },
    { MP_QSTR_stop,    MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
};

//---------------------------------------------
uint8_t getMemAdrLen(int memlen, uint32_t addr)
{
    if ((memlen < 1) || (memlen > 4)) {
        mp_raise_ValueError("Memory address length error, 1 - 4 allowed");
    }
    uint8_t len = 1;
    if (addr > 0xFF) len++;
    if (addr > 0xFFFF) len++;
    if (addr > 0xFFFFFF) len++;
    if (memlen > len) len = memlen;
    return len;
}

//-----------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_addr, ARG_memaddr, ARG_n, ARG_memlen, ARG_stop };
    mp_machine_i2c_obj_t *self = pos_args[0];
    _checkMaster(self);

    // parse arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(mp_machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(mp_machine_i2c_mem_allowed_args), mp_machine_i2c_mem_allowed_args, args);

    _checkAddr(args[ARG_addr].u_int);

    // Get read length
    int n = mp_obj_get_int(args[ARG_n].u_obj);
    if (n > 0) {
        uint32_t addr = (uint32_t)mp_obj_get_int(args[ARG_memaddr].u_obj);
        uint8_t memlen = getMemAdrLen(args[ARG_memlen].u_int, addr);

        uint8_t *buf = heap_caps_malloc(n, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        if (buf == NULL) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error allocating I2C data buffer"));
        }
        int ret = mp_i2c_master_read(self, args[ARG_addr].u_int, memlen, addr, buf, n, args[ARG_stop].u_bool);
        if (ret != ESP_OK) {
            free(buf);
            i2c_check_error(ret);
        }
        vstr_t vstr;
        vstr_init_len(&vstr, n);
        memcpy(vstr.buf, buf, n);
        free(buf);

        // Return read data as string
        //return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
        return mp_obj_new_str_from_vstr(&vstr);
    }
    // Return empty string
    return mp_const_empty_bytes;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_readfrom_mem_obj, 1, mp_machine_i2c_readfrom_mem);

//----------------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_memlen, ARG_stop };
    mp_machine_i2c_obj_t *self = pos_args[0];
    _checkMaster(self);

    // parse arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(mp_machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(mp_machine_i2c_mem_allowed_args), mp_machine_i2c_mem_allowed_args, args);

    _checkAddr(args[ARG_addr].u_int);

    // Get the output data buffer
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);

    if (bufinfo.len > 0) {
        uint32_t addr = (uint32_t)mp_obj_get_int(args[ARG_memaddr].u_obj);
        uint8_t memlen = getMemAdrLen(args[ARG_memlen].u_int, addr);

        uint8_t *buf = get_buffer(bufinfo.buf, bufinfo.len);
        if (buf) {
            // Transfer data into buffer
            int ret = mp_i2c_master_read(self, args[ARG_addr].u_int, memlen, addr, buf, bufinfo.len, args[ARG_stop].u_bool);
            if (ret != ESP_OK) {
                free(buf);
                i2c_check_error(ret);
            }
            memcpy(bufinfo.buf, buf, bufinfo.len);
            free(buf);
        }
    }
    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_readfrom_mem_into_obj, 1, mp_machine_i2c_readfrom_mem_into);

//----------------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_memlen };
    mp_machine_i2c_obj_t *self = pos_args[0];
    _checkMaster(self);

    // parse arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(mp_machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(mp_machine_i2c_mem_allowed_args), mp_machine_i2c_mem_allowed_args, args);

    _checkAddr(args[ARG_addr].u_int);

    uint32_t addr = (uint32_t)mp_obj_get_int(args[ARG_memaddr].u_obj);
    uint8_t memlen = getMemAdrLen(args[ARG_memlen].u_int, addr);

    uint8_t *buf = NULL;
    int len = 0;
    if (args[ARG_buf].u_obj != mp_const_none) {
        // Get the input data buffer
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);
        buf = get_buffer(bufinfo.buf, bufinfo.len);
        len = bufinfo.len;
    }

    // Transfer address and data
    int ret = mp_i2c_master_write(self, args[ARG_addr].u_int, memlen, addr, buf, len, true);
    if (buf) free(buf);
    i2c_check_error(ret);
    return mp_obj_new_int(len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_machine_i2c_writeto_mem_obj, 1, mp_machine_i2c_writeto_mem);

//-------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_deinit(mp_obj_t self_in) {
    mp_machine_i2c_obj_t *self = self_in;

    if (i2c_used[self->bus_id] >= 0) {
        if ((self->mode == I2C_MODE_SLAVE) && (slave_mutex[self->bus_id])) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);

        i2c_used[self->bus_id] = -1;
        i2c_driver_delete(self->bus_id);

        if ((self->mode == I2C_MODE_SLAVE) && (slave_mutex[self->bus_id])) xSemaphoreGive(slave_mutex[self->bus_id]);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_deinit_obj, mp_machine_i2c_deinit);

//-------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_deinit_all() {

    for(int i = 0; i < I2C_MODE_MAX; i++)
    if (i2c_used[i] >= 0) {
        if (slave_mutex[i]) xSemaphoreTake(slave_mutex[i], I2C_SLAVE_MUTEX_TIMEOUT);

        i2c_used[i] = -1;
        i2c_driver_delete(i);

        if (slave_mutex[i]) xSemaphoreGive(slave_mutex[i]);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_machine_i2c_deinit_all_fun_obj, mp_machine_i2c_deinit_all);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(mp_machine_i2c_deinit_all_obj, MP_ROM_PTR(&mp_machine_i2c_deinit_all_fun_obj));


// ============================================================================================
// ==== Low level MicroPython I2C commands ====================================================
// ============================================================================================

//-------------------------------------------------
static void _checkBegin(mp_machine_i2c_obj_t *self)
{
    _checkMaster(self);
    if (self->cmd == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C: command before begin!"));
    }
}

// Begin i2c transaction
//-------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_begin(mp_obj_t self_in, mp_obj_t rxlen_in) {
    mp_machine_i2c_obj_t *self = self_in;
    _checkMaster(self);

    int rxbuflen = mp_obj_get_int(rxlen_in);
    if ((rxbuflen < 0) || (rxbuflen > I2C_RX_MAX_BUFF_LEN)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Buffer length error"));
    }

    if (self->cmd != NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C: 2nd begin before end not allowed!"));
    }

    if (self->rx_data) free(self->rx_data);
    self->rx_data = NULL;
    if (rxbuflen) {
        //self->rx_data = malloc(rxbuflen);
        self->rx_data = heap_caps_malloc(rxbuflen, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
        if (self->rx_data == NULL) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error allocating rx buffer"));
        }
    }
    self->rx_buflen = rxbuflen;
    self->rx_bufidx = 0;

    self->cmd = i2c_cmd_link_create();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_i2c_begin_obj, mp_machine_i2c_begin);

// Queue start command
//------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_start(mp_obj_t self_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    // Queue start command
    if (i2c_master_start(self->cmd) != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Start command error"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_start_obj, mp_machine_i2c_start);

// Queue slave address
//------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_address(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t rw_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    uint8_t slave_addr = mp_obj_get_int(addr_in);
    uint8_t rw = mp_obj_get_int(rw_in) & 1; // 0 -> write, 1 -> read

    _checkAddr(slave_addr);

    // Queue slave address
    if (i2c_master_write_byte(self->cmd, (slave_addr << 1) | rw, I2C_ACK_CHECK_EN) != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Address write error"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_machine_i2c_address_obj, mp_machine_i2c_address);

// Queue stop command
//-----------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_stop(mp_obj_t self_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    if (i2c_master_stop(self->cmd) != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Stop command error"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_stop_obj, mp_machine_i2c_stop);

//----------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_write_byte(mp_obj_t self_in, mp_obj_t val_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    uint8_t val = mp_obj_get_int(val_in);

    if (i2c_master_write_byte(self->cmd, val, I2C_ACK_CHECK_EN) != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Write error"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_i2c_write_byte_obj, mp_machine_i2c_write_byte);

//-----------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_write_bytes(mp_obj_t self_in, mp_obj_t buf_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (i2c_master_write(self->cmd, bufinfo.buf, bufinfo.len, I2C_ACK_CHECK_EN) != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Write error"));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_i2c_write_bytes_obj, mp_machine_i2c_write_bytes);

//----------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_read_byte(mp_obj_t self_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    if ((self->rx_data == NULL) || ((self->rx_bufidx + 1) >= self->rx_buflen)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Rx buffer overflow"));
    }

    if (i2c_master_read_byte(self->cmd, &(self->rx_data[self->rx_bufidx]), I2C_MASTER_NACK) != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Read error"));
    }
    self->rx_bufidx++;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_read_byte_obj, mp_machine_i2c_read_byte);

//----------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_read_bytes(mp_obj_t self_in, mp_obj_t len_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    esp_err_t ret = ESP_OK;
    int len = mp_obj_get_int(len_in);
    if ((len < 1) || (len > self->rx_buflen)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Rx buffer overflow"));
    }
    if ((self->rx_data == NULL) || ((self->rx_bufidx + len) >= self->rx_buflen)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Rx buffer overflow"));
    }

    if (len > 1) {
        ret = i2c_master_read(self->cmd, &(self->rx_data[self->rx_bufidx]), len-1, I2C_MASTER_ACK);
    }
    if (ret == ESP_OK) ret = i2c_master_read_byte(self->cmd, &(self->rx_data[self->rx_bufidx + len - 1]), I2C_MASTER_NACK);

    if (ret != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Read error"));
    }
    self->rx_bufidx += len;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_i2c_read_bytes_obj, mp_machine_i2c_read_bytes);

// End i2c transaction
//----------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_end(mp_obj_t self_in) {
    mp_machine_i2c_obj_t *self = self_in;

    _checkBegin(self);

    esp_err_t res = i2c_master_cmd_begin(self->bus_id, self->cmd, (5000 / portTICK_PERIOD_MS));

    i2c_cmd_link_delete(self->cmd);
    self->cmd = NULL;

    mp_obj_t ret = mp_const_none;
    if ((res == ESP_OK) && (self->rx_data) && (self->rx_bufidx > 0)) {
        vstr_t vstr;
        vstr_init_len(&vstr, self->rx_bufidx);
        memcpy(vstr.buf, self->rx_data, self->rx_bufidx);
        // Return read data as string
        // ret = mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
        ret = mp_obj_new_str_from_vstr(&vstr);
    }

    self->rx_buflen = 0;
    self->rx_bufidx = 0;
    if (self->rx_data) free(self->rx_data);
    self->rx_data = NULL;

    if (res != ESP_OK) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C bus error"));
    }
    return ret;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_end_obj, mp_machine_i2c_end);


// ============================================================================================
// ==== I2C slave functions ===================================================================
// ============================================================================================

//-----------------------------------------------------------------
void _check_addr_len(mp_machine_i2c_obj_t *self, int addr, int len)
{
    if ((len < 1) || (len > self->slave_rbuflen)) {
        mp_raise_ValueError("Length out of range");
    }
    if (addr >= self->slave_rbuflen) {
        mp_raise_ValueError("Address not in slave data buffer");
    }
    if ((addr + len) > self->slave_rbuflen) {
        mp_raise_ValueError("Data outside buffer");
    }
}

//-----------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_slave_set_data(mp_obj_t self_in, mp_obj_t buf_in, mp_obj_t addr_in)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkSlave(self);

    if(self->slave_wbuflen == 0) mp_raise_ValueError("Write buffer length = 0. Use 'slave_wbuflen = value' in I2C constructor.");

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (slave_mutex[self->bus_id]) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);

    ESP_LOGI(I2C_DEBUG_TAG, "Writing [%d] = %p (%d)", self->bus_id, bufinfo.buf, bufinfo.len);
    int res = i2c_slave_write_buffer(self->bus_id, bufinfo.buf, bufinfo.len, I2C_SLAVE_MUTEX_TIMEOUT);

    if (slave_mutex[self->bus_id]) xSemaphoreGive(slave_mutex[self->bus_id]);

    if (res <= 0) return mp_const_false;
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_machine_i2c_slave_set_data_obj, mp_machine_i2c_slave_set_data);

//---------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_slave_reset_busy(mp_obj_t self_in)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkSlave(self);

    if (slave_mutex[self->bus_id]) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);

    //Depricated
    //int res = i2c_slave_reset_busy(self->bus_id, I2C_SLAVE_MUTEX_TIMEOUT);

    if (slave_mutex[self->bus_id]) xSemaphoreGive(slave_mutex[self->bus_id]);

    //if (res <= 0) return mp_const_false;
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_slave_reset_busy_obj, mp_machine_i2c_slave_reset_busy);
//-----------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_slave_get_data(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t len_in)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkSlave(self);

    int addr = mp_obj_get_int(addr_in);
    int len = mp_obj_get_int(len_in);
    _check_addr_len(self, addr, len);

    uint8_t *databuf = malloc(len);
    if (databuf == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Error allocating data buffer"));
    }

    mp_obj_t data;
    if (slave_mutex[self->bus_id]) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);

    int res = i2c_slave_read_buffer(self->bus_id, databuf, len, I2C_SLAVE_MUTEX_TIMEOUT);
    if (res > 0) data = mp_obj_new_bytes(databuf, res);
    else data = mp_const_empty_bytes;

    if (slave_mutex[self->bus_id]) xSemaphoreGive(slave_mutex[self->bus_id]);

    free(databuf);
    // Return read data as byte array
    return data;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_machine_i2c_slave_get_data_obj, mp_machine_i2c_slave_get_data);

//-----------------------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_slave_get_cbdata(mp_obj_t self_in)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkSlave(self);

    mp_obj_t data;
    if (slave_mutex[self->bus_id]) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);

    int res = self->cbrx_len;
    if (res > 0) data = mp_obj_new_bytes(self->cbrx_data, res);
    else data = mp_const_empty_bytes;

    if (slave_mutex[self->bus_id]) xSemaphoreGive(slave_mutex[self->bus_id]);

    // Return read data as byte array
    return data;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_machine_i2c_slave_get_cbdata_obj, mp_machine_i2c_slave_get_cbdata);

//----------------------------------------------------------------------------------------------
#define MP_OBJ_IS_METH(o) (MP_OBJ_IS_OBJ(o) && (((mp_obj_base_t*)MP_OBJ_TO_PTR(o))->type->name == MP_QSTR_bound_method))

STATIC mp_obj_t mp_machine_i2c_slave_callback(mp_obj_t self_in, mp_obj_t func)
{
    mp_machine_i2c_obj_t *self = self_in;
    _checkSlave(self);

    if ((!MP_OBJ_IS_FUN(func)) && (!MP_OBJ_IS_METH(func)) && (func != mp_const_none)) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Function argument required"));
    }

    if (slave_mutex[self->bus_id]) xSemaphoreTake(slave_mutex[self->bus_id], I2C_SLAVE_MUTEX_TIMEOUT);

    if (func == mp_const_none) self->slave_cb = NULL;
    else self->slave_cb = func;

    if (i2c_slave_task_handle == NULL) {
        int res = xTaskCreate(i2c_slave_task, "i2c_slave_task", I2C_SLAVE_TASK_STACK_SIZE, (void *)self, CONFIG_MICROPY_TASK_PRIORITY, &i2c_slave_task_handle);
        if (res != pdPASS) {
            ESP_LOGE(I2C_DEBUG_TAG, "Error creating slave task");
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } else ESP_LOGW(I2C_DEBUG_TAG, "Error adding slave task");

    if (slave_mutex[self->bus_id]) xSemaphoreGive(slave_mutex[self->bus_id]);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_i2c_slave_callback_obj, mp_machine_i2c_slave_callback);

//----------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_timing(size_t n_args, const mp_obj_t *args, int type)
{
    mp_machine_i2c_obj_t *self = args[0];

    int setup_time = -1;
    int hold_time = -1;
    if (n_args == 3) {
        setup_time = mp_obj_get_int(args[1]);
        hold_time = mp_obj_get_int(args[2]);
        if (type == 1) i2c_set_start_timing(self->bus_id, setup_time, hold_time);
        else if (type == 2) i2c_set_stop_timing(self->bus_id, setup_time, hold_time);
        else if (type == 3) i2c_set_data_timing(self->bus_id, setup_time, hold_time);
        else if (type == 4) i2c_set_period(self->bus_id, setup_time, hold_time);
    }
    if (type == 1) i2c_get_start_timing(self->bus_id, &setup_time, &hold_time);
    else if (type == 2) i2c_get_stop_timing(self->bus_id, &setup_time, &hold_time);
    else if (type == 3) i2c_get_data_timing(self->bus_id, &setup_time, &hold_time);
    else if (type == 4) i2c_get_period(self->bus_id, &setup_time, &hold_time);

    mp_obj_t tuple[2];

    tuple[0] = mp_obj_new_int(setup_time);
    tuple[1] = mp_obj_new_int(hold_time);
    return mp_obj_new_tuple(2, tuple);
}

//------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_start_timing(size_t n_args, const mp_obj_t *args)
{
    mp_obj_t res = mp_machine_i2c_timing(n_args, args, 1);
    return res;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_i2c_start_timing_obj, 1, 3, mp_machine_i2c_start_timing);

//-----------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_stop_timing(size_t n_args, const mp_obj_t *args)
{
    mp_obj_t res = mp_machine_i2c_timing(n_args, args, 2);
    return res;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_i2c_stop_timing_obj, 1, 3, mp_machine_i2c_stop_timing);

//-----------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_data_timing(size_t n_args, const mp_obj_t *args)
{
    mp_obj_t res = mp_machine_i2c_timing(n_args, args, 3);
    return res;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_i2c_data_timing_obj, 1, 3, mp_machine_i2c_data_timing);

//------------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_clock_timing(size_t n_args, const mp_obj_t *args)
{
    mp_obj_t res = mp_machine_i2c_timing(n_args, args, 4);
    return res;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_i2c_clock_timing_obj, 1, 3, mp_machine_i2c_clock_timing);

//-------------------------------------------------------------------------
STATIC mp_obj_t mp_machine_i2c_timeout(size_t n_args, const mp_obj_t *args)
{
    mp_machine_i2c_obj_t *self = args[0];
    _checkMaster(self);

    int tmo = -1;
    if (n_args == 2) {
        tmo = mp_obj_get_int(args[1]);
        i2c_set_timeout(self->bus_id, tmo * 80);
    }
    i2c_get_timeout(self->bus_id, &tmo);
    tmo /= 80;

    return mp_obj_new_int(tmo);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_i2c_timeout_obj, 1, 2, mp_machine_i2c_timeout);

//===================================================================
STATIC const mp_rom_map_elem_t mp_machine_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),                (mp_obj_t)&mp_machine_i2c_init_obj },
    { MP_ROM_QSTR(MP_QSTR_deinit),              (mp_obj_t)&mp_machine_i2c_deinit_obj },
    { MP_ROM_QSTR(MP_QSTR_deinit_all),          (mp_obj_t)&mp_machine_i2c_deinit_all_obj },
    { MP_ROM_QSTR(MP_QSTR_scan),                (mp_obj_t)&mp_machine_i2c_scan_obj },
    { MP_ROM_QSTR(MP_QSTR_is_ready),            (mp_obj_t)&mp_machine_i2c_is_ready_obj },
    { MP_ROM_QSTR(MP_QSTR_start_timing),        (mp_obj_t)&mp_machine_i2c_start_timing_obj },
    { MP_ROM_QSTR(MP_QSTR_stop_timing),         (mp_obj_t)&mp_machine_i2c_stop_timing_obj },
    { MP_ROM_QSTR(MP_QSTR_data_timing),         (mp_obj_t)&mp_machine_i2c_data_timing_obj },
    { MP_ROM_QSTR(MP_QSTR_clock_timing),        (mp_obj_t)&mp_machine_i2c_clock_timing_obj },
    { MP_ROM_QSTR(MP_QSTR_timeout),             (mp_obj_t)&mp_machine_i2c_timeout_obj },

    // Standard methods
    { MP_ROM_QSTR(MP_QSTR_readfrom),            (mp_obj_t)&mp_machine_i2c_readfrom_obj },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),       (mp_obj_t)&mp_machine_i2c_readfrom_into_obj },
    { MP_ROM_QSTR(MP_QSTR_writeto),             (mp_obj_t)&mp_machine_i2c_writeto_obj },
    { MP_ROM_QSTR(MP_QSTR_setdata),             (mp_obj_t)&mp_machine_i2c_slave_set_data_obj },
    { MP_ROM_QSTR(MP_QSTR_getdata),             (mp_obj_t)&mp_machine_i2c_slave_get_data_obj },
    { MP_ROM_QSTR(MP_QSTR_getcbdata),           (mp_obj_t)&mp_machine_i2c_slave_get_cbdata_obj },
    { MP_ROM_QSTR(MP_QSTR_resetbusy),           (mp_obj_t)&mp_machine_i2c_slave_reset_busy_obj },
    { MP_ROM_QSTR(MP_QSTR_callback),            (mp_obj_t)&mp_machine_i2c_slave_callback_obj },

    // Memory methods
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem),        (mp_obj_t)&mp_machine_i2c_readfrom_mem_obj },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into),   (mp_obj_t)&mp_machine_i2c_readfrom_mem_into_obj },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),         (mp_obj_t)&mp_machine_i2c_writeto_mem_obj },

    // Low level MicroPython I2C methods
    { MP_ROM_QSTR(MP_QSTR_begin),               (mp_obj_t)&mp_machine_i2c_begin_obj },
    { MP_ROM_QSTR(MP_QSTR_start),               (mp_obj_t)&mp_machine_i2c_start_obj },
    { MP_ROM_QSTR(MP_QSTR_address),             (mp_obj_t)&mp_machine_i2c_address_obj },
    { MP_ROM_QSTR(MP_QSTR_stop),                (mp_obj_t)&mp_machine_i2c_stop_obj },
    { MP_ROM_QSTR(MP_QSTR_read_byte),           (mp_obj_t)&mp_machine_i2c_read_byte_obj },
    { MP_ROM_QSTR(MP_QSTR_read_bytes),          (mp_obj_t)&mp_machine_i2c_read_bytes_obj },
    { MP_ROM_QSTR(MP_QSTR_write_byte),          (mp_obj_t)&mp_machine_i2c_write_byte_obj },
    { MP_ROM_QSTR(MP_QSTR_write_bytes),         (mp_obj_t)&mp_machine_i2c_write_bytes_obj },
    { MP_ROM_QSTR(MP_QSTR_end),                 (mp_obj_t)&mp_machine_i2c_end_obj },

    // Constants
    { MP_OBJ_NEW_QSTR(MP_QSTR_MASTER),          MP_OBJ_NEW_SMALL_INT(I2C_MODE_MASTER) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SLAVE),           MP_OBJ_NEW_SMALL_INT(I2C_MODE_SLAVE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_READ),            MP_OBJ_NEW_SMALL_INT(I2C_MASTER_READ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WRITE),           MP_OBJ_NEW_SMALL_INT(I2C_MASTER_WRITE) },
};

STATIC MP_DEFINE_CONST_DICT(mp_machine_i2c_locals_dict, mp_machine_i2c_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_i2c_make_new,
    print, mp_machine_i2c_print,
    //protocol, &machine_hw_i2c_p,
    locals_dict, &mp_machine_i2c_locals_dict
    );

