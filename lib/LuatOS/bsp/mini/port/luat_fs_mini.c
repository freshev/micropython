#include "luat_base.h"
#include "luat_fs.h"
#include "luat_mem.h"

extern const struct luat_vfs_filesystem vfs_fs_posix;

int luat_fs_init(void) {
	#ifdef LUAT_USE_FS_VFS
	//vfs performs necessary initialization
	luat_vfs_init(NULL);
	//Register vfs for posix implementation
	luat_vfs_reg(&vfs_fs_posix);

	luat_fs_conf_t conf = {
		.busname = "",
		.type = "posix",
		.filesystem = "posix",
		.mount_point = "", // In the window environment, it is necessary to support reading from any path, and it cannot be forced to be /
	};
	luat_fs_mount(&conf);
	#endif
	return 0;
}
