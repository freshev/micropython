# Demonstration of TCP/UART large data delivery

This demo demonstrates reading 8M bytes of data from the TCP side and writing it to the UART in a blocking manner.

## File description

1. Main.lua is the code downloaded to the Modules side and used with the latest firmware.
2. main.go server-side code, in golang language, the client will immediately start sending 8M bytes of data after connecting
3. README.md This documentation

