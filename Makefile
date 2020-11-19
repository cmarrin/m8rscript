#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

IDF_PATH=$(HOME)/esp/ESP8266_RTOS_SDK

PROJECT_NAME := m8resp

EXTRA_COMPONENT_DIRS := ../src

include $(IDF_PATH)/make/project.mk

upload_fs:
	$(ESPTOOLPY_WRITE_FLASH) 0x100000 ../m8rFSFile
