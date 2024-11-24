
/*@Modules errDump
@summary Error reporting
@version 1.0
@date 2022.12.15
@demo errDump
@tag LUAT_USE_ERRDUMP
@usage
--Basic usage, report once every 10 minutes, if available
if errDump then
    errDump.config(true, 600)
end

-- Attached open source server: https://gitee.com/openLuat/luatos-devlog*/
#include "luat_base.h"
#include "luat_sys.h"
#include "luat_msgbus.h"
#include "luat_zbuff.h"
#include "ldebug.h"
#include "luat_rtos.h"
#ifdef LUAT_USE_MOBILE
#include "luat_mobile.h"
#endif
#include "luat_mcu.h"
#include "luat_fs.h"
#include "luat_pm.h"
#include "luat_mem.h"

#ifdef LUAT_USE_NETWORK

#include "luat_network_adapter.h"
#define LUAT_ERRDUMP_TAG "log"
#include "luat_errdump.h"

#define LUAT_LOG_TAG "errDump"
#include "luat_log.h"

#define ERR_DUMP_LEN_MAX	(4096)
#define LUAT_ERRDUMP_PORT	(12425)
const char luat_errdump_domain[] = "dev_msg1.openluat.com";
enum
{
	LUAT_ERRDUMP_RECORD_TYPE_SYS,
	LUAT_ERRDUMP_RECORD_TYPE_USR,
	LUAT_ERRDUMP_RECORD_TYPE_NONE,

	LUAT_ERRDUMP_CONNECT = 0,
	LUAT_ERRDUMP_TX,
	LUAT_ERRDUMP_RX,
	LUAT_ERRDUMP_CLOSE,
};

static const char sys_error_log_file_path[] = {'/',0xaa,'s','e','r','r',0};
static const char user_error_log_file_path[] = {'/',0xaa,'u','e','r','r',0};
typedef struct luat_errdump_conf
{
	Buffer_Struct tx_buf;
	network_ctrl_t *netc;
	char *user_string;
	uint32_t upload_period;
	uint32_t sys_error_r_cnt;
	uint32_t sys_error_w_cnt;
	uint32_t user_error_r_cnt;
	uint32_t user_error_w_cnt;
	luat_rtos_timer_t upload_timer;
	luat_rtos_timer_t network_timer;
	uint8_t is_uploading;
	uint8_t error_dump_enable;
	uint8_t upload_poweron_reason_done;
	char custom_id[49];
	char custom_domain_host[49];
	uint16_t custom_domain_port;
}luat_errdump_conf_t;

static luat_errdump_conf_t econf;

static void luat_errdump_load(const char *path, Buffer_Struct *buffer);
static void luat_errdump_clear(const char *path);
static int32_t l_errdump_callback(lua_State *L, void* ptr);
static LUAT_RT_RET_TYPE luat_errdump_timer_callback(LUAT_RT_CB_PARAM);
static LUAT_RT_RET_TYPE luat_errdump_rx_timer_callback(LUAT_RT_CB_PARAM);
static int luat_errdump_network_callback(void *data, void *param)
{
	OS_EVENT *event = (OS_EVENT *)data;
	// int ret = 0;
	rtos_msg_t msg = {0};
	(void)param;
	if (event->Param1)
	{
		LLOGE("errdump fail, after %d second retry", econf.upload_period);
		econf.is_uploading = 0;
		msg.handler = l_errdump_callback,
		msg.arg1 = LUAT_ERRDUMP_CLOSE,
		luat_msgbus_put(&msg, 0);
		luat_rtos_timer_start(econf.upload_timer, econf.upload_period * 1000, 0, luat_errdump_timer_callback, NULL);
		OS_DeInitBuffer(&econf.tx_buf);
		return 0;
	}
	switch(event->ID)
	{
	case EV_NW_RESULT_CONNECT:
		msg.handler = l_errdump_callback,
		msg.arg1 = LUAT_ERRDUMP_TX,
		luat_msgbus_put(&msg, 0);
		break;
	case EV_NW_RESULT_EVENT:
		msg.handler = l_errdump_callback,
		msg.arg1 = LUAT_ERRDUMP_RX,
		luat_msgbus_put(&msg, 0);
		network_wait_event(econf.netc, NULL, 0, 0);
		break;
	case EV_NW_RESULT_CLOSE:
		break;
	}
	return 0;
}

static void luat_errdump_make_data(lua_State *L)
{
	const char *project = "unkonw";
	const char *version = "";
#ifdef LUAT_USE_MOBILE
	char imei[16] = {0};
#endif
	char *selfid = econf.custom_id;
	const char *sn = version;
	FILE* fd = NULL;
	size_t len = 0;
	if (econf.custom_id[0] == 0) {
		#ifdef LUAT_USE_MOBILE
		luat_mobile_get_imei(0, imei, 15);
		selfid = imei;
		#else
    	const char* id = luat_mcu_unique_id(&len);
		if (id != NULL && len > 0 && len < 24) {
			for (size_t i = 0; i < len; i++)
			{
				sprintf_(econf.custom_id + i*2, "%02X", econf.custom_id[i]);
			}
		}
		#endif
	}
    lua_getglobal(L, "PROJECT");
    size_t version_len, project_len;
    if (LUA_TSTRING == lua_type(L, -1))
    {
    	project = luaL_tolstring(L, -1, &project_len);
    }
    lua_getglobal(L, "VERSION");
    if (LUA_TSTRING == lua_type(L, -1))
    {
    	version = luaL_tolstring(L, -1, &version_len);
    }
    int32_t file_len[LUAT_ERRDUMP_RECORD_TYPE_NONE];
    econf.sys_error_r_cnt = econf.sys_error_w_cnt;
    file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] = luat_fs_fsize(sys_error_log_file_path);
    econf.user_error_r_cnt = econf.user_error_w_cnt;
    file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] = luat_fs_fsize(user_error_log_file_path);
    if (file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] < 0)
    {
    	file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] = 0;
    }
    if (file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] > (ERR_DUMP_LEN_MAX * 2))
    {
    	LLOGE("sys record file len %d too much, drop", file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS]);
    	luat_fs_remove(sys_error_log_file_path);
    	file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] = 0;
    }
    if (file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] < 0)
    {
    	file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] = 0;
    }
    if (file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] > (ERR_DUMP_LEN_MAX * 2))
    {
    	LLOGE("user record file len %d too much, drop", file_len[LUAT_ERRDUMP_RECORD_TYPE_USR]);
    	luat_fs_remove(user_error_log_file_path);
    	file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] = 0;
    }

    if (econf.user_string)
    {
    	sn = econf.user_string;
    }
    OS_ReInitBuffer(&econf.tx_buf, file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] + file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] + 128);
    econf.tx_buf.Pos = sprintf_((char*)econf.tx_buf.Data, "%s_LuatOS-SoC_%s_%s,%s,%s,%s,\r\n", project, luat_version_str(), luat_os_bsp(), version, selfid, sn);
    if (!econf.upload_poweron_reason_done)
    {
    	econf.tx_buf.Pos += sprintf_((char*)(econf.tx_buf.Data + econf.tx_buf.Pos), "poweron reason:%d\r\n", luat_pm_get_poweron_reason());
    }
    if (file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS] > 0)
    {
    	fd = luat_fs_fopen(sys_error_log_file_path, "r");
    	len = luat_fs_fread((char*)(econf.tx_buf.Data + econf.tx_buf.Pos), file_len[LUAT_ERRDUMP_RECORD_TYPE_SYS], 1, fd);
    	if (len > 0)
    	{
    		econf.tx_buf.Pos += len;
    		econf.tx_buf.Data[econf.tx_buf.Pos] = '\r';
    		econf.tx_buf.Data[econf.tx_buf.Pos + 1] = '\n';
    		econf.tx_buf.Pos += 2;
    	}
    	luat_fs_fclose(fd);
    }
    if (file_len[LUAT_ERRDUMP_RECORD_TYPE_USR] > 0)
    {
    	fd = luat_fs_fopen(user_error_log_file_path, "r");
    	len = luat_fs_fread(econf.tx_buf.Data + econf.tx_buf.Pos, file_len[LUAT_ERRDUMP_RECORD_TYPE_USR], 1, fd);
    	if (len > 0)
    	{
    		econf.tx_buf.Pos += len;
    		econf.tx_buf.Data[econf.tx_buf.Pos] = '\r';
    		econf.tx_buf.Data[econf.tx_buf.Pos + 1] = '\n';
    		econf.tx_buf.Pos += 2;
    	}
    	luat_fs_fclose(fd);
    }
}

static int32_t l_errdump_callback(lua_State *L, void* ptr)
{
	(void)ptr;
	uint8_t response[16];
	luat_ip_addr_t remote_ip;
	uint16_t remote_port;
	uint32_t dummy_len;
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    const char *ok_result = "{\"r\": 1}";
    switch(msg->arg1)
    {
    case LUAT_ERRDUMP_CONNECT:
    	econf.netc = network_alloc_ctrl(network_get_last_register_adapter());
    	if (!econf.netc)
    	{
    		LLOGE("no socket, errdump fail, after %d second retry", econf.upload_period);
    		econf.is_uploading = 0;
    		luat_rtos_timer_start(econf.upload_timer, econf.upload_period * 1000, 0, luat_errdump_timer_callback, NULL);
    		OS_DeInitBuffer(&econf.tx_buf);

    	}
    	else
    	{
    		network_init_ctrl(econf.netc, NULL, luat_errdump_network_callback, NULL);
    		network_set_base_mode(econf.netc, 0, 0, 0, 0, 0, 0);
    		luat_rtos_timer_start(econf.network_timer, 30000, 0, luat_errdump_rx_timer_callback, NULL);
			if (econf.custom_domain_host[0]) {
				if (econf.custom_domain_port < 1) {
					econf.custom_domain_port = LUAT_ERRDUMP_PORT;
				}
				// LLOGD("Uplink to custom server %s %d", econf.custom_domain_host, econf.custom_domain_port);
				network_connect(econf.netc, econf.custom_domain_host, strlen(econf.custom_domain_host), NULL, econf.custom_domain_port, 0);
			}
			else {
				network_connect(econf.netc, luat_errdump_domain, sizeof(luat_errdump_domain), NULL, LUAT_ERRDUMP_PORT, 0);
			}
    		
    	}
    	break;
    case LUAT_ERRDUMP_TX:
    	if (!econf.tx_buf.Data)
    	{
    		luat_errdump_make_data(L);
    	}
    	else
    	{
    		if (econf.sys_error_r_cnt != econf.sys_error_w_cnt || econf.user_error_r_cnt != econf.user_error_w_cnt)
    		{
    			msg->arg2 = 0;
    			luat_errdump_make_data(L);
    		}
    	}

    	if (network_tx(econf.netc, econf.tx_buf.Data, econf.tx_buf.Pos, 0, NULL, 0, &dummy_len, 0) < 0)
    	{
    		LLOGE("socket tx error, errdump fail, after %d second retry", econf.upload_period);
    		luat_rtos_timer_start(econf.upload_timer, econf.upload_period * 1000, 0, luat_errdump_timer_callback, NULL);
			goto SOCKET_CLOSE;
    	}
    	network_wait_event(econf.netc, NULL, 0, 0);
    	luat_rtos_timer_start(econf.network_timer, 10000, 0, luat_errdump_rx_timer_callback, (void *)(msg->arg2 + 1));
    	break;
    case LUAT_ERRDUMP_RX:

    	if (network_rx(econf.netc, response, 16, 0, &remote_ip, &remote_port, &dummy_len))
    	{
    		LLOGE("socket rx error, errdump fail, after %d second retry", econf.upload_period);
    		luat_rtos_timer_start(econf.upload_timer, econf.upload_period * 1000, 0, luat_errdump_timer_callback, NULL);
			goto SOCKET_CLOSE;
    	}
    	if (8 == dummy_len)
    	{
    		if (memcmp(response, ok_result, 8))
    		{
    			LLOGD("errdump response error %.*s", dummy_len, response);
    			break;
    		}
    	}
    	else if (2 == dummy_len)
    	{
    		if (memcmp(response, "OK", 2))
    		{
    			LLOGD("errdump response error %.*s", dummy_len, response);
    			break;
    		}
    	}
    	else
    	{
    		LLOGD("errdump response maybe new %.*s", dummy_len, response);
    	}
		if (econf.sys_error_r_cnt != econf.sys_error_w_cnt || econf.user_error_r_cnt != econf.user_error_w_cnt)
		{
			LLOGD("errdump need retry!");
			luat_errdump_make_data(L);
			if (network_tx(econf.netc, econf.tx_buf.Data, econf.tx_buf.Pos, 0, NULL, 0, &dummy_len, 0))
			{
				LLOGE("socket tx error, errdump fail, after %d second retry", econf.upload_period);
				luat_rtos_timer_start(econf.upload_timer, econf.upload_period * 1000, 0, luat_errdump_timer_callback, NULL);
				goto SOCKET_CLOSE;
			}
			network_wait_event(econf.netc, NULL, 0, 0);
			luat_rtos_timer_start(econf.network_timer, 10000, 0, luat_errdump_rx_timer_callback, (void *)1);
		}
		else
		{
			LLOGD("errdump ok!");
			luat_errdump_clear(sys_error_log_file_path);
			luat_errdump_clear(user_error_log_file_path);
			econf.upload_poweron_reason_done = 1;
			goto SOCKET_CLOSE;
		}
    	break;
    case LUAT_ERRDUMP_CLOSE:
    	goto SOCKET_CLOSE;
    	break;
    }
    return 0;
SOCKET_CLOSE:
	luat_rtos_timer_stop(econf.network_timer);
	econf.is_uploading = 0;
	if (econf.netc)
	{
		network_close(econf.netc, 0);
		network_release_ctrl(econf.netc);
		OS_DeInitBuffer(&econf.tx_buf);
		econf.netc = NULL;
	}
	return 0;
}

static LUAT_RT_RET_TYPE luat_errdump_rx_timer_callback(LUAT_RT_CB_PARAM)
{
	uint32_t retry = (uint32_t)param;
	if (retry && (retry < 3))
	{
		LLOGE("errdump tx fail %d cnt, retry", retry);
		rtos_msg_t msg = {
			.handler = l_errdump_callback,
			.ptr = NULL,
			.arg1 = LUAT_ERRDUMP_TX,
			.arg2 = retry,
		};
		luat_msgbus_put(&msg, 0);
	}
	else
	{
		LLOGE("errdump server connect or tx fail, after %d second retry", econf.upload_period);
		rtos_msg_t msg = {
			.handler = l_errdump_callback,
			.ptr = NULL,
			.arg1 = LUAT_ERRDUMP_CLOSE,
			.arg2 = 0,
		};
		luat_msgbus_put(&msg, 0);
		econf.is_uploading = 0;
		luat_rtos_timer_start(econf.upload_timer, econf.upload_period * 1000, 0, luat_errdump_timer_callback, NULL);
	}
}



static LUAT_RT_RET_TYPE luat_errdump_timer_callback(LUAT_RT_CB_PARAM)
{
	(void)param;
	if (!econf.upload_poweron_reason_done || luat_fs_fsize(sys_error_log_file_path) > 0 || luat_fs_fsize(user_error_log_file_path) > 0)
	{
		econf.is_uploading = 1;
		rtos_msg_t msg = {
			.handler = l_errdump_callback,
			.ptr = NULL,
			.arg1 = LUAT_ERRDUMP_CONNECT,
			.arg2 = 0,
		};
		luat_msgbus_put(&msg, 0);
	}
	else
	{
		LLOGD("no info errdump stop");
	}
}

static void luat_errdump_save(const char *path, const uint8_t *data, uint32_t len)
{
	if (!econf.error_dump_enable) return;
	size_t now_len = luat_fs_fsize(path);
	FILE* fd = NULL;
	// Handle on a case-by-case basis
	if (len >= ERR_DUMP_LEN_MAX) {
		// The new data directly exceeds the maximum length, so the old data is meaningless.
		// And new data must be truncated
		data += (len - ERR_DUMP_LEN_MAX);
		len = ERR_DUMP_LEN_MAX;
		fd = luat_fs_fopen(path, "w+"); // This will be truncated
	}
	else if (now_len + len > ERR_DUMP_LEN_MAX) {
		// New data is less than ERR_DUMP_LEN_MAX, but new data + old data <ERR_DUMP_LEN_MAX
		size_t keep_len = ERR_DUMP_LEN_MAX - len;// Old data retention measures
		uint8_t *buffer = luat_heap_malloc(keep_len); 
		if (buffer) {
			//Open the original file and read the existing data
			fd = luat_fs_fopen(path, "r");
			if (fd) {
				luat_fs_fseek(fd, now_len - keep_len, SEEK_SET);
				luat_fs_fread(buffer, keep_len, 1, fd);
				luat_fs_fclose(fd);
				//Open the file again for writing
				fd = luat_fs_fopen(path, "w+");
				if (fd) {
					luat_fs_fwrite(buffer, keep_len, 1, fd);
				} // If the old data cannot be read, just ignore it.
			} // If the old data cannot be read, just ignore it.
			luat_heap_free(buffer);
		}
	}
	else {
		// Just append the data directly
		fd = luat_fs_fopen(path, "a+");
	}
	if (fd == NULL) {
		//last try
		luat_fs_remove(path);
		fd = luat_fs_fopen(path, "w+"); 
	}

	// Finally, write new data
	if (fd) {
		luat_fs_fwrite(data, len, 1, fd);
		luat_fs_fclose(fd);
	}
	else {
		// TODO Do I need to return here?
	}

	if (!econf.is_uploading  && econf.upload_period)
	{
		luat_rtos_timer_start(econf.upload_timer, 2000, 0, luat_errdump_timer_callback, NULL);
	}

}

static void luat_errdump_clear(const char *path)
{
	if (luat_fs_fexist(path)) luat_fs_remove(path);
}

static void luat_errdump_load(const char *path, Buffer_Struct *buffer)
{
	if (!econf.error_dump_enable) return;
	size_t len = luat_fs_fsize(path);
	if (!len)
	{
		return;
	}
	FILE* fd = luat_fs_fopen(path, "rb");
	if (buffer->MaxLen < len)
	{
		OS_ReInitBuffer(buffer, len);
	}
	len = luat_fs_fread(buffer->Data, len, 1, fd);
	if (len > 0)
	{
		buffer->Pos = len;
	}
	luat_fs_fclose(fd);
}

void luat_errdump_save_file(const uint8_t *data, uint32_t len)
{
	econf.sys_error_w_cnt++;
	luat_errdump_save(sys_error_log_file_path, data, len);
	econf.sys_error_w_cnt++;
}

void luat_errdump_record_init(uint8_t enable, uint32_t upload_period)
{
	econf.error_dump_enable = enable;
	if (econf.error_dump_enable)
	{
		if (upload_period)
		{
			if (!econf.upload_timer)
			{
				luat_rtos_timer_create(&econf.upload_timer);
			}
			if (!econf.network_timer)
			{
				luat_rtos_timer_create(&econf.network_timer);
			}
			econf.upload_period = upload_period;
			luat_rtos_timer_start(econf.upload_timer, 2000, 0, luat_errdump_timer_callback, NULL);
			luat_rtos_timer_stop(econf.network_timer);
		}
		else
		{
			econf.upload_period = 0;
			luat_rtos_timer_delete(econf.upload_timer);
			luat_rtos_timer_delete(econf.network_timer);
		}
	}
	else
	{
		luat_errdump_clear(sys_error_log_file_path);
		luat_errdump_clear(user_error_log_file_path);
		luat_rtos_timer_delete(econf.upload_timer);
		luat_rtos_timer_delete(econf.network_timer);
		if (econf.user_string)
		{
			luat_heap_free(econf.user_string);
			econf.user_string = NULL;
		}
	}
}

/*Manually reading exception logs is mainly used for users to send logs to their own servers instead of the IOT platform. If periodic upload is configured in errDump.config, this function cannot be used.
@api errDump.dump(zbuff, type, isDelete)
@zbuff log information cache, if it is nil, it will not be read, usually when
@int log type, currently only errDump.TYPE_SYS and errDump.TYPE_USR
@boolean whether to delete the log
@return boolean true means that no data was written before this reading, false on the contrary, before deleting the log, it is best to read it again to ensure that no new data has been written.
@usage
local result = errDump.dump(buff, errDump.TYPE_SYS, false) --Read the exception log recorded by the system
local result = errDump.dump(nil, errDump.TYPE_SYS, true) --Clear the exception log recorded by the system*/
static int l_errdump_dump(lua_State *L) {
	int is_delete = 0;
	if (LUA_TBOOLEAN == lua_type(L, 3))
	{
		is_delete = lua_toboolean(L, 3);
	}
	luat_zbuff_t *buff = NULL;
	if (lua_touserdata(L, 1))
	{
		buff = tozbuff(L);
	}
	int result = 0;
	const char *path = NULL;
	int type = luaL_optinteger(L, 2, LUAT_ERRDUMP_RECORD_TYPE_USR);
	if (type >= LUAT_ERRDUMP_RECORD_TYPE_NONE)
	{
		lua_pushboolean(L, 1);
		return 1;
	}

	switch(type)
	{
	case LUAT_ERRDUMP_RECORD_TYPE_SYS:
		result = (econf.sys_error_r_cnt != econf.sys_error_w_cnt);
		path = sys_error_log_file_path;
		break;
	case LUAT_ERRDUMP_RECORD_TYPE_USR:
		result = (econf.user_error_r_cnt != econf.user_error_w_cnt);
		path = user_error_log_file_path;
		break;
	}
	if (buff)
	{
		Buffer_Struct buffer;
		buffer.Data = buff->addr;
		buffer.MaxLen = buff->len;
		buffer.Pos = 0;
		switch(type)
		{
		case LUAT_ERRDUMP_RECORD_TYPE_SYS:
			econf.sys_error_r_cnt = econf.sys_error_w_cnt;
			break;
		case LUAT_ERRDUMP_RECORD_TYPE_USR:
			econf.user_error_r_cnt = econf.user_error_w_cnt;
			break;
		}
		luat_errdump_load(path, &buffer);
		buff->addr = buffer.Data;
		buff->len = buffer.MaxLen;
		buff->used = buffer.Pos;
	}
	lua_pushboolean(L, result);
	if (is_delete)
	{
		luat_errdump_clear(path);
	}
	return 1;
}

/*Write the user's exception log. Note that the maximum is only 4KB. If the excess part is new, the old one will be overwritten. After automatic upload is turned on, it will be uploaded to the Hezhou IOT platform.
@api errDump.record(string)
@string log
@return nil no return value
@usage
errDump.record("socket long time no connect") --record "socket long time no connect"*/
static int l_errdump_record(lua_State *L) {
	if (LUA_TSTRING == lua_type(L, 1))
	{
		size_t len = 0;
		const char *str = luaL_tolstring(L, 1, &len);
		if (len)
		{
			econf.user_error_w_cnt++;
			luat_errdump_save(user_error_log_file_path, (const uint8_t *)str, len);
			econf.user_error_w_cnt++;
		}
	}
	return 0;
}

/*Configure key logs to be uploaded to the IOT platform. The logs here include logs that cause luavm to exit abnormally and logs written by users through record, similar to air's errDump.
@api errDump.config(enable, period, user_flag, custom_id, host, port)
@boolean Whether to enable the logging function, if false, no logs will be recorded
@int Timing upload cycle, unit second, default 600 seconds, this is the retry time after automatic upload. It will try to upload to Hezhou IOT platform once soon after booting or recording operation. If it is 0, then It will not be uploaded. The user will upload it to his own platform after dumping.
@string The user’s special identifier, which can be empty
@string device identification number, the default for 4G devices is imei, and the default for other devices is mcu.unique_id
@string server domain name, default dev_msg1.openluat.com
@int server port, default
@return nil no return value
@usage
errDump.config(true, 3600, "12345678") --Try the last time every hour. 12345678 will be appended to imei when uploading.
errDump.config(false) --Turn off the recording function and no longer upload
errDump.config(true, 0) --record, but will not actively upload, and the upload function is implemented by the user

-- Added custom_id parameter on 2023.09.22
errDump.config(true, 3600, nil, "ABC") --Try the last time every hour and use the customized device identification number ABC when uploading

-- 2023.12.8 Added host and port parameters
errDump.config(true, 3600, nil, nil, "dev_msg1.openluat.com", 12425)*/
static int l_errdump_upload_config(lua_State *L) {
	if (LUA_TBOOLEAN == lua_type(L, 1))
	{
		luat_errdump_record_init(lua_toboolean(L, 1), luaL_optinteger(L, 2, 600));
	}
	if (econf.error_dump_enable)
	{
		size_t len = 0;
		const char *str;
		if (LUA_TSTRING == lua_type(L, 3))
		{
			str = luaL_checklstring(L, 3, &len);
			if (econf.user_string)
			{
				luat_heap_free(econf.user_string);
				econf.user_string = NULL;
			}
			econf.user_string = luat_heap_malloc(len + 1);
			memcpy(econf.user_string, str, len + 1);
		}
		if (LUA_TSTRING == lua_type(L, 4)) {
			str = luaL_checklstring(L, 4, &len);
			if (len < 48) 
				memcpy(econf.custom_id, str, len + 1);
		}
		if (LUA_TSTRING == lua_type(L, 5)) {
			str = luaL_checklstring(L, 5, &len);
			if (len < 48) 
				memcpy(econf.custom_domain_host, str, len + 1); 
			if (lua_isinteger(L, 6)) {
				econf.custom_domain_port = lua_tointeger(L, 6);
			}
			if (econf.custom_domain_port < 1) {
				econf.custom_domain_port = LUAT_ERRDUMP_PORT;
			}
			LLOGD("Custom server %s %d", econf.custom_domain_host, econf.custom_domain_port);
		}
	}
	return 0;
}

#include "rotable2.h"
static const rotable_Reg_t reg_errdump[] =
{
	{ "dump",		    ROREG_FUNC(l_errdump_dump)},
	{ "record", 	    ROREG_FUNC(l_errdump_record)},
	{ "config", 	ROREG_FUNC(l_errdump_upload_config)},
	{ "TYPE_SYS",       ROREG_INT(LUAT_ERRDUMP_RECORD_TYPE_SYS)},
	{ "TYPE_USR",       ROREG_INT(LUAT_ERRDUMP_RECORD_TYPE_USR)},
	{ NULL,             ROREG_INT(0) }
};

LUAMOD_API int luaopen_errdump( lua_State *L ) {
    luat_newlib2(L, reg_errdump);
    return 1;
}

#endif
