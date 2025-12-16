@echo o
rem pip install pyserial
python -c "import serial.tools.list_ports, serial, sys; target = next((p.device for p in serial.tools.list_ports.comports() if 'Device 0 Modem' in (p.description or '')), None); print(f'Found port: {target}' if target else 'Port not found'); serial.Serial(target, 115200, timeout=1).write(b'AT*DOWNLOAD=1\r\n') if target else sys.exit(1)"
rem All next burns (-SkipFileID can be BOOTLOADER,AP,PS,SFFS,FLASH,NV)
rem If you have more than one COM add -port XX to CmdDloader.exe arguments (XX should be two digits COM port number with device name "SPRD U2S Diag")
echo %*
..\..\lib\Luat_CSDK_Air724U\tools\win32\CmdDloader\CmdDloader.exe -pac hex/micropython_Air724/micropython_Air724.pac -c -SkipFileID BOOTLOADER,AP,PS,SFFS,FLASH,NV %*
