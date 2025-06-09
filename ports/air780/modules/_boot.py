import sys
import os
import gc
sys.path.append('.frozen')

try:
    import air
    os.mount(air.flash, "/")
except:
    print("Mount flash failed")

gc.collect()

# import cellular