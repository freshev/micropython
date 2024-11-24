#include "common_api.h"
void fdb_init(void)
{
    am_kv_init();
    char value[2];
    int ret = am_kv_get("flag", &value, 2);
    //Read whether the kv database user has been initialized. If not, write a flag and the value that needs to be initialized, indicating that the user has been initialized; if the user has been initialized, no operation is performed.
    DBG("get value result %d", ret);
    if (ret > 0)
    {
        DBG("get value %s", value);
        if(memcmp("1", value, strlen("1")))
        {
            DBG("need init");
            ret = am_kv_set("flag", "1", 2);
            DBG("init result1 %d", ret);
            int volume = 4;
            ret = am_kv_set("volume", &volume, 1);
        }
        else
        {
            DBG("no need init");
        }
    }
    else
    {
        ret = am_kv_set("flag", "1", 2);
        int volume = 4;
        ret = am_kv_set("volume", &volume, 1);
        DBG("init result2 %d", ret);
    }
}
