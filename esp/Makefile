#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

IDF_PATH=$(HOME)/esp/ESP8266_RTOS_SDK

PROJECT_NAME := m8r

include $(IDF_PATH)/make/project.mk

spiffs.bin:
	rm -rf build/spiffsdata
	mkdir build/spiffsdata
	cp ../scripts/mrsh build/spiffsdata
	cp ../scripts/mem build/spiffsdata
	cp ../scripts/simple/hello.m8r build/spiffsdata
	python spiffsgen.py 3145728 build/spiffsdata build/spiffs.bin
	echo '*** generated spiffs.bin'

upload_spiffs: spiffs.bin
	$(ESPTOOLPY_WRITE_FLASH) 0x100000 build/spiffs.bin

upload_fs:
	$(ESPTOOLPY_WRITE_FLASH) 0x100000 ../m8rFSFile
