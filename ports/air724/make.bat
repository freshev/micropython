@call ..\..\lib\Luat_CSDK_Air724U\CSDKvars.bat
@cd build
@rem cmake clean ..\ 
@cmake ..\ -G Ninja & ninja
@cd ..

