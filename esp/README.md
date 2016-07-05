# MaterScript For ESP8266
### Howto
MaterScript for the ESP currently uses the [Sming framework](https://github.com/SmingHub/Sming). To build on Mac:

1. Setup Sming using the instructions [Here](https://github.com/SmingHub/Sming/wiki/MacOS-Quickstart). 
2. cd esp
3. make clean
4. make

From there you can run 'make flash' to upload to an ESP8266 device. But you may have to change the COM_PORT variable in Makefile-user.mk. Alternatively you can 'export COM_PORT=/dev/tty.\<your com port\>'
