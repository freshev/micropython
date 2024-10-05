call ..\..\lib\CSDTK42-windows\CSDTKvars.bat 
@rem set SOFT_WORKDIR=./
@rem set PATH=D:\projects\project\soft\env\utils;D:\projects\project\soft\env\win32;%PATH%
@rem make -r -j4 CT_TARGET=target ......
@rem make.exe -r -j4 %1
@rem CT_TARGET=target %1
make.exe %1
