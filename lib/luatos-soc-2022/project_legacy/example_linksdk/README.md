### example_sysdep_api_test.c

Linksdk's own adaptation interface test code is used to test whether there are any problems with interface adaptation.



### example_mqtt_basic.c

Linksdk comes with a demo for basic function testing of connecting to Alibaba Cloud server mqtt





### Use the following two lines of code in xmake.lua to switch the demo compilation

```lua
add_files("/src/example_sysdep_api_test.c",{public = true})
-- add_files("/src/example_mqtt_basic.c",{public = true})
```



