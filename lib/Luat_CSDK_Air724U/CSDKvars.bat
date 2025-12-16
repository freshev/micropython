@set CSDKINSTALLDIR=%~dp0
@if not exist .\build mkdir .\build
@set PATH=%CSDKINSTALLDIR%prebuilts\win32\cmake\bin;%CSDKINSTALLDIR%prebuilts\win32\python3;%CSDKINSTALLDIR%prebuilts\win32\gcc-arm-none-eabi\bin;%CSDKINSTALLDIR%tools;%CSDKINSTALLDIR%tools\win32;%CSDKINSTALLDIR%prebuilts\win32\bin;%CSDKINSTALLDIR%\..\csdtk42-windows\mingw32\bin;%CSDKINSTALLDIR%\..\csdtk42-windows\make64;%PATH%
