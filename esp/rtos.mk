#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

IDF_PATH=$(HOME)/esp/ESP8266_RTOS_SDK

PROJECT_NAME := m8rscript

include $(IDF_PATH)/make/project.mk

ESPTOOL_ALL_FLASH_ARGS += 0x100000 build/spiffs.bin

spiffs.bin:
	python spiffsgen.py 3145728 spiffsdata build/spiffs.bin
	echo '*** generated spiffs.bin'

