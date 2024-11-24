
# LVGL binding to Lua API

The project is in progress and has not yet been completed. It may not even be able to run.

## Directory description

```
src - the source code of LVGL 7.11, almost unchanged (only the parameters of one method have been modified, and it is equivalent to the original method)
gen - Files generated in batches through scripts. Do not modify them. If there are problems, you must modify the scripts.
binding - binding code, this is handwritten code that can be modified/added functions
```

## Other functions planned to be implemented

* File system
* Event callback, such as key event
* Picture decoder
* The docking of output devices is currently empty.
* Input device docking
