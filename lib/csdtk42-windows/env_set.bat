@echo off

>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' EQU '0' (
    goto STARTSET
) else (
    echo ========================
    echo **********FAIL**********
    echo Need Admin To Run Script
    echo ========================
    pause
    exit
)

:STARTSET

set flag=1
set evnameOld=GPRS_CSDTK41_PATH
set evname=GPRS_CSDTK42_PATH
set regpath=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment

:OPERATION


if %GPRS_CSDTK41_PATH%a==a goto DONT_DELETE
echo Delete %evnameOld% before from environment 
reg delete "%regpath%" /v "%evnameOld%" /f
:DONT_DELETE
if %GPRS_CSDTK42_PATH%a==a goto DONT_DELETE2
echo Delete %evname% before from environment 
reg delete "%regpath%" /v "%evname%" /f
:DONT_DELETE2

set pathValue=%~dp0
set pathValue=%pathValue:~0,-1%
echo Add %evname%=%pathValue% to environment
reg add "%regpath%" /v %evname% /t reg_sz /d %pathValue% /f
if %flag%==2 goto END
set regpath=HKCU\Environment
set flag=2
goto OPERATION

:END

@echo restart explorer
taskkill /im explorer.exe /f
start explorer.exe

echo =========================
echo *********SUCCESS*********
echo =========================
pause