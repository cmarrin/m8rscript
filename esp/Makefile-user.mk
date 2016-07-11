## Local build configuration
## Parameters configured here will override default and ENV values.
## Uncomment and change examples:

## Add your source directories here separated by space
MODULES = app ../m8rscript
EXTRA_INCDIR = include ../m8rscript

ESP_HOME = /opt/esp-open-sdk
SMING_HOME = /opt/Sming/Sming/
ESPTOOL2 = /opt/esp-open-sdk/utils/esptool2
ESPTOOL = /opt/esp-open-sdk/utils/esptool.py

COM_PORT ?= /dev/tty.usbserial-AD02CUJ5
COM_SPEED	= 115200

## Configure flash parameters (for ESP12-E and other new boards):
# SPI_MODE = dio

## SPIFFS options
DISABLE_SPIFFS = 1
# SPIFF_FILES = files

ENABLE_GDB=0
