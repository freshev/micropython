/*@Modules usbapp
@summary USB function operation
@version 1.0
@date 2022.01.17
@demo usb_hid
@tagLUAT_USE_USB*/
#include "luat_base.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"

#define USB_ID0 0

enum
{
	USB_HID_NOT_READY,
	USB_HID_READY,
	USB_HID_SEND_DONE,
	USB_HID_NEW_DATA,
};

static int l_usb_app_vhid_cb(lua_State *L, void* ptr) {
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    lua_getglobal(L, "sys_pub");
    if (lua_isfunction(L, -1)) {
        lua_pushstring(L, "USB_HID_INC");
        lua_pushinteger(L, msg->arg1);
        lua_call(L, 2, 0);
    }
    return 0;
}

int32_t luat_usb_app_vhid_cb(void *pData, void *pParam)
{
    rtos_msg_t msg;
    msg.handler = l_usb_app_vhid_cb;
	switch((uint32_t)pParam)
	{
	case USB_HID_NOT_READY:
	case USB_HID_READY:
	case USB_HID_SEND_DONE:
	case USB_HID_NEW_DATA:
	    msg.arg1 = (uint32_t)pParam;
        luat_msgbus_put(&msg, 0);
		break;
	}
    return 0;
}
/*USB set VID and PID
@api usbapp.set_id(id, vid, pid)
@int device id, default is 0
@int vid little endian format
@int pid little endian format
@usage
usbapp.set_id(0, 0x1234, 0x5678)*/
static int l_usb_set_id(lua_State* L) {
    luat_usb_app_set_vid_pid(luaL_optinteger(L, 1, 0), luaL_optinteger(L, 2, 0x1234), luaL_optinteger(L, 3, 0x5678));
    return 0;
}

/*USB HID device mode
@api usbapp.hid_mode(id, mode, buff_size)
@int device id, default is 0
@int mode, currently 0 is the keyboard and 1 is custom
@int buff_size can only be 8, 16, 32, 64. If it is in keyboard mode or other values   are filled in, it will automatically be 8
@usage
usbapp.hid_mode(0, 0) -- usb hid keyboard mode
usbapp.hid_mode(0, 1) -- usb hid custom mode, used for driver-free USB interaction*/
static int l_usb_set_hid_mode(lua_State* L) {
	luat_usb_app_set_hid_mode(luaL_optinteger(L, 1, 0), luaL_optinteger(L, 2, 0), luaL_optinteger(L, 3, 8));
	return 0;
}

/*Start USB device
@api usbapp.start(id)
@int device id, default is 0
@return bool returns true if successful, otherwise returns false
@usage
-- Start USB
usbapp.start(0)*/
static int l_usb_start(lua_State* L) {
    luat_usb_app_start(USB_ID0);
    lua_pushboolean(L, 1);
    return 1;
}

/*Turn off USB device
@api usbapp.stop(id)
@int device id, default is 0
@return bool returns true if successful, otherwise returns false
@usage
-- Turn off USB
usbapp.stop(0)*/
static int l_usb_stop(lua_State* L) {
    luat_usb_app_stop(USB_ID0);
    lua_pushboolean(L, 1);
    return 1;
}

/*USB HID device upload data
@api usbapp.vhid_upload(id, data)
@int device id, default is 0
@string data. Note that the available characters of HID are limited. Basically only visible characters are supported, and unsupported characters will be replaced with spaces.
@return bool returns true if successful, otherwise returns false
@usage
-- HID upload data
usbapp.vhid_upload(0, "1234") -- usb hid will simulate 1234*/
static int l_usb_vhid_upload(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    if (len > 0) {
        luat_usb_app_vhid_upload(USB_ID0, data, len);
        lua_pushboolean(L, 1);
    }
    else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

/*USB HID device uploads user-defined data
@api usbapp.hid_tx(id, data, start, len)
@int device id, default is 0
@string or zbuff Note that 0 will be automatically filled when the amount of data is insufficient.
@int Optional, valid only when data is zbuff. The starting position of the data to be sent, the default is 0
@int Optional, data is valid only if it is zbuff. The length of the data to be sent defaults to the valid data in zbuff. The maximum value does not exceed the maximum space of zbuff.
@return bool returns true if successful, otherwise returns false
@usage
-- HID upload data
usbapp.hid_tx(0, "1234") -- usb hid upload 0x31 0x32 0x33 0x34 + N zeros*/
static int l_usb_hid_tx(lua_State* L) {
    size_t start, len;
    const char *buf;
    luat_zbuff_t *buff = NULL;
    if(lua_isuserdata(L, 2)) {
        buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        start = luaL_optinteger(L, 3, 0);
        len = luaL_optinteger(L, 4, buff->used);
        if (start >= buff->len) {
        	lua_pushboolean(L, 0);
        	return 1;
        }
        if ((start + len)>= buff->len) {
        	len = buff->len - start;
        }
        if (len > 0) {
        	luat_usb_app_vhid_tx(USB_ID0, buff->addr + start, len);
            lua_pushboolean(L, 1);
        }
        else {
            lua_pushboolean(L, 0);
        }
    } else {
    	buf = luaL_checklstring(L, 2, &len);
    	luat_usb_app_vhid_tx(USB_ID0, buf, len);
        lua_pushboolean(L, 1);
    }
    return 1;
}

/*Read the received data in buff form, and read all the data at once and store it in the buff. If the buff space is not enough, it will automatically expand. Currently, only air105 supports this operation.
@api usbapp.hid_rx(id, buff)
@int device id, default is 0
@zbuff zbuff object
@return int Returns the read length and moves the zbuff pointer back
@usage
usbapp.hid_rx(0, buff)*/
static int l_usb_hid_rx(lua_State *L)
{
    uint8_t id = luaL_checkinteger(L, 1);

    if(lua_isuserdata(L, 2)){//Special handling of zbuff objects
    	luat_zbuff_t *buff = ((luat_zbuff_t *)luaL_checkudata(L, 2, LUAT_ZBUFF_TYPE));
        int result = luat_usb_app_vhid_rx(id, NULL, 0);	//Read the length of the current cache. Currently, only 105 supports this operation.
        if (result > (buff->len - buff->used))
        {
        	__zbuff_resize(buff, buff->len + result);
        }
        luat_usb_app_vhid_rx(id, buff->addr + buff->used, result);
        lua_pushinteger(L, result);
        buff->used += result;
        return 1;
    }
    else
    {
        lua_pushinteger(L, 0);
        return 1;
    }
    return 1;
}

/*USB HID device cancels upload data
@api usbapp.vhid_cancel_upload(id)
@int device id, default is 0
@return nil no return value
@usage
-- Cancel uploading data, usually not needed
usbapp.vhid_cancel_upload(0)*/
static int l_usb_vhid_cancel_upload(lua_State* L) {
    luat_usb_app_vhid_cancel_upload(USB_ID0);
    return 0;
}

/*USB U disk device mounts SDHC, TF card
@api usbapp.udisk_attach_sdhc(id)
@int device id, default is 0
@return nil no return value
@usage
usbapp.udisk_attach_sdhc(0)*/
static int l_usb_udisk_attach_sdhc(lua_State* L) {
	luat_usb_udisk_attach_sdhc(USB_ID0);
    return 0;
}

/*Remove USB U disk device from mounting SDHC and TF card
@api usbapp.udisk_detach_sdhc(id)
@int device id, default is 0
@return nil no return value
@usage
usbapp.udisk_detach_sdhc(0)*/
static int l_usb_udisk_detach_sdhc(lua_State* L) {
	luat_usb_udisk_detach_sdhc(USB_ID0);
    return 0;
}


#include "rotable2.h"
static const rotable_Reg_t reg_usbapp[] =
{
	{ "set_id" ,         ROREG_FUNC(l_usb_set_id)},
	{ "hid_mode" ,         ROREG_FUNC(l_usb_set_hid_mode)},
    { "start" ,         ROREG_FUNC(l_usb_start)},
    { "stop" ,       ROREG_FUNC(l_usb_stop)},
	{ "hid_tx", ROREG_FUNC(l_usb_hid_tx)},
	{ "hid_rx", ROREG_FUNC(l_usb_hid_rx)},
    { "vhid_upload",        ROREG_FUNC(l_usb_vhid_upload)},
    { "vhid_cancel_upload", ROREG_FUNC(l_usb_vhid_cancel_upload)},
	{ "udisk_attach_sdhc", ROREG_FUNC(l_usb_udisk_attach_sdhc)},
	{ "udisk_detach_sdhc", ROREG_FUNC(l_usb_udisk_detach_sdhc)},
    //@const NO_READY number NO_READY事件
    { "NO_READY",      ROREG_INT(USB_HID_NOT_READY)},
    //@const READY number READY事件
    { "READY",         ROREG_INT(USB_HID_READY)},
    //@const SEND_OK number SEND_OK事件
    { "SEND_OK",      ROREG_INT(USB_HID_SEND_DONE)},
    //@const NEW_DATA number NEW_DATA incident
	{ "NEW_DATA",      ROREG_INT(USB_HID_NEW_DATA)},
	{ NULL,            ROREG_INT(0)},
};

LUAMOD_API int luaopen_usbapp( lua_State *L ) {
	luat_newlib2(L, reg_usbapp);
    return 1;
}
