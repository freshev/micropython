/*@Modules  ftp
@summary ftp 客户端
@version 1.0
@date    2022.09.05
@demo    ftp
@tag LUAT_USE_FTP*/

#include "luat_base.h"

#include "luat_network_adapter.h"
#include "luat_msgbus.h"
#include "luat_mem.h"
#include "luat_ftp.h"

#define LUAT_LOG_TAG "ftp"
#include "luat_log.h"

static uint64_t ftp_idp = 0;

static int32_t l_ftp_callback(lua_State *L, void* ptr){
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
	luat_ftp_ctrl_t *luat_ftp_ctrl = (luat_ftp_ctrl_t *)msg->ptr;
	// LLOGD("l_ftp_callback arg1:%d arg2:%d idp:%lld",msg->arg1,msg->arg2,ftp_idp);
	if (ftp_idp){
		if (msg->arg1 == FTP_ERROR){
			lua_pushboolean(L, 0);
		}else if (msg->arg1 == FTP_SUCCESS_DATE){
			lua_pushlstring(L,(const char *)(luat_ftp_ctrl->result_buffer.Data),luat_ftp_ctrl->result_buffer.Pos);
		}else{
			lua_pushboolean(L, 1);
		}
		luat_cbcwait(L, ftp_idp, 1);
		ftp_idp = 0;
	}
	OS_DeInitBuffer(&luat_ftp_ctrl->result_buffer);
	return 0;
}


static void luat_ftp_cb(luat_ftp_ctrl_t *luat_ftp_ctrl, FTP_SUCCESS_STATE_e event){
	rtos_msg_t msg = {0};
	msg.handler = l_ftp_callback;
	msg.ptr = luat_ftp_ctrl;
	msg.arg1 = event;
	luat_msgbus_put(&msg, 0);
}

/*FTP client
@api ftp.login(adapter,ip_addr,port,username,password)
@int Adapter serial number, which can only be socket.ETH0, socket.STA, socket.AP. If not filled in, the platform's own method will be selected, and then the last registered adapter will be selected.
@string ip_addr address
@string port port, default 21
@string username username
@string password password
@bool/table Whether to use SSL encrypted connection, the default is not encryption, true is the simplest encryption without certificate, table is encryption with certificate<br>server_cert server ca certificate data<br>client_cert client ca certificate data<br>client_key Client private key encrypted data<br>client_password Client private key password data
@return bool/string Returns true on success and string on failure
@usage
ftp_login = ftp.login(nil,"xxx")*/
static int l_ftp_login(lua_State *L) {
	int result = 0;
	size_t len = 0;
	luat_ftp_tls_t* luat_ftp_tls = NULL;
	uint8_t adapter = luaL_optinteger(L, 1, network_get_last_register_adapter());
	const char *ip_addr = luaL_checklstring(L, 2, &len);
	uint16_t port = luaL_optinteger(L, 3, 21);
	const char *username = luaL_optlstring(L, 4, "",&len);
	const char *password = luaL_optlstring(L, 5, "",&len);

	// Encryption related
	if (lua_isboolean(L, 6)){
		if (lua_toboolean(L, 6)){
			luat_ftp_tls = (luat_ftp_tls_t *)luat_heap_malloc(sizeof(luat_ftp_tls_t));
			memset(luat_ftp_tls, 0, sizeof(luat_ftp_tls_t));
		}
	}else if (lua_istable(L, 6)){
		luat_ftp_tls = (luat_ftp_tls_t *)luat_heap_malloc(sizeof(luat_ftp_tls_t));
		memset(luat_ftp_tls, 0, sizeof(luat_ftp_tls_t));
		lua_pushstring(L, "server_cert");
		if (LUA_TSTRING == lua_gettable(L, 6)) {
			luat_ftp_tls->server_cert = luaL_checklstring(L, -1, &len);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "client_cert");
		if (LUA_TSTRING == lua_gettable(L, 6)) {
			luat_ftp_tls->client_cert = luaL_checklstring(L, -1, &len);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "client_key");
		if (LUA_TSTRING == lua_gettable(L, 6)) {
			luat_ftp_tls->client_key = luaL_checklstring(L, -1, &len);
		}
		lua_pop(L, 1);

		lua_pushstring(L, "client_password");
		if (LUA_TSTRING == lua_gettable(L, 6)) {
			luat_ftp_tls->client_password = luaL_checklstring(L, -1, &len);
		}
		lua_pop(L, 1);
	}

	if (luat_ftp_tls!=NULL){
		if (lua_isstring(L, 6)){
			luat_ftp_tls->server_cert = luaL_checklstring(L, 6, &len);
		}
		if (lua_isstring(L, 7)){
			luat_ftp_tls->client_cert = luaL_checklstring(L, 7, &len);
		}
		if (lua_isstring(L, 8)){
			luat_ftp_tls->client_key = luaL_checklstring(L, 8, &len);
		}
		if (lua_isstring(L, 9)){
			luat_ftp_tls->client_password = luaL_checklstring(L, 9, &len);
		}
	}
	
	if (0!=(result = luat_ftp_login(adapter,ip_addr,port,username,password,luat_ftp_tls,luat_ftp_cb))){
		LLOGE("ftp login fail");
		luat_ftp_release();
		lua_pushinteger(L,result);
		luat_pushcwait_error(L,1);
	}else{
		ftp_idp = luat_pushcwait(L);
	}
	if (luat_ftp_tls){
		luat_heap_free(luat_ftp_tls);
	}
	return 1;
}

/*FTP command
@api ftp.command(cmd)
@string cmd command currently supports: NOOP SYST TYPE PWD MKD CWD CDUP RMD DELE LIST
@return string Returns true on success and string on failure
@usage
// No operation to prevent the connection from being disconnected
    print(ftp.command("NOOP").wait())
// Report the operating system type of the remote system
    print(ftp.command("SYST").wait())
//Specify file type
    print(ftp.command("TYPE I").wait())
//Display the current working directory name
    print(ftp.command("PWD").wait())
//Create directory
    print(ftp.command("MKD QWER").wait())
//Change the current working directory
    print(ftp.command("CWD /QWER").wait())
// Return to the previous directory
    print(ftp.command("CDUP").wait())
//delete directory
    print(ftp.command("RMD QWER").wait())
// Get the list of file names in the current working directory
print(ftp.command("LIST").wait())
// delete file
print(ftp.command("DELE /1/12222.txt").wait())*/
static int l_ftp_command(lua_State *L) {
	size_t len;
	const char * command = luaL_optlstring(L, 1, "",&len);
	if (luat_ftp_command(command)){
		LLOGE("ftp command fail");
		lua_pushinteger(L,FTP_ERROR_FILE);
		luat_pushcwait_error(L,1);
	}else{
		ftp_idp = luat_pushcwait(L);
	}
	return 1;
}

/*FTP file download
@api ftp.pull(local_name,remote_name)
@string local_name local file
@string remote_name server file
@return bool/string Returns true on success and string on failure
@usage
ftp.pull("/1222.txt","/1222.txt").wait()*/
static int l_ftp_pull(lua_State *L) {
	size_t len;
	const char * local_name = luaL_optlstring(L, 1, "",&len);
	const char * remote_name = luaL_optlstring(L, 2, "",&len);
	if (luat_ftp_pull(local_name,remote_name)){
		LLOGE("ftp pull fail");
		lua_pushinteger(L,FTP_ERROR_FILE);
		luat_pushcwait_error(L,1);
	}else{
		ftp_idp = luat_pushcwait(L);
	}
	return 1;
}


/*FTP file upload
@api ftp.push(local_name,remote_name)
@string local_name local file
@string remote_name server file
@return bool/string Returns true on success and string on failure
@usage
ftp.push("/1222.txt","/1222.txt").wait()*/
static int l_ftp_push(lua_State *L) {
	size_t len;
	const char * local_name = luaL_optlstring(L, 1, "",&len);
	const char * remote_name = luaL_optlstring(L, 2, "",&len);
	if (luat_ftp_push(local_name,remote_name)){
		LLOGE("ftp push fail");
		lua_pushinteger(L,FTP_ERROR_CONNECT);
		luat_pushcwait_error(L,1);
	}else{
		ftp_idp = luat_pushcwait(L);
	}
	return 1;
}

/*FTP client closed
@api ftp.close()
@return bool/string Returns true on success and string on failure
@usage
ftp.close().wait()*/
static int l_ftp_close(lua_State *L) {
	if (luat_ftp_close()){
		lua_pushinteger(L,FTP_ERROR_CONNECT);
		luat_pushcwait_error(L,1);
	}else{
		ftp_idp = luat_pushcwait(L);
	}
	return 1;
}

/*Configure whether to turn on debug information
@api ftp.debug(onoff)
@boolean whether to turn on the debug switch
@return nil no return value
@usage ftp.debug(true)*/
static int l_ftp_set_debug(lua_State *L){

	if (lua_isboolean(L, 1)){
		luat_ftp_debug(lua_toboolean(L, 1));
	}
	return 0;
}
#include "rotable2.h"
#ifdef LUAT_USE_NETWORK
static const rotable_Reg_t reg_ftp[] =
{
	{"login",			ROREG_FUNC(l_ftp_login)},
	{"command",			ROREG_FUNC(l_ftp_command)},
	{"pull",			ROREG_FUNC(l_ftp_pull)},
	{"push",			ROREG_FUNC(l_ftp_push)},
	{"close",			ROREG_FUNC(l_ftp_close)},
	{"debug",			ROREG_FUNC(l_ftp_set_debug)},
	{ NULL,             ROREG_INT(0)}
};
#else
static const rotable_Reg_t reg_ftp_emtry[] =
{
	{ NULL,             ROREG_INT(0)}
};
#endif

LUAMOD_API int luaopen_ftp( lua_State *L ) {
#ifdef LUAT_USE_NETWORK
    luat_newlib2(L, reg_ftp);
#else
    luat_newlib2(L, reg_ftp_emtry);
	LLOGE("ftp require network enable!!");
#endif
    return 1;
}
