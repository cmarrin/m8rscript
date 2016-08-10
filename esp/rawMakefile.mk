#====================================================================================
# rawMakefile.mk
#
# A makefile for ESP8286 Arduino projects.
# Edit the contents of this file to suit your project
# or just include it and override the applicable macros.
#
# License: GPL 2.1
# General and full license information is available at:
#    https://github.com/plerup/makeEspArduino
#
# Copyright (c) 2016 Peter Lerup. All rights reserved.
#
#====================================================================================

#====================================================================================
# User editable area
#====================================================================================

DEBUG_FLAGS = -Os
#DEBUG_FLAGS = -Og -g

#=== Project specific definitions
MAIN_SRC ?= app/m8rscript.cpp
SRC ?=  ../m8rscript/Array.cpp \
		../m8rscript/Atom.cpp \
		../m8rscript/Error.cpp \
		../m8rscript/ExecutionUnit.cpp \
		../m8rscript/Function.cpp \
		../m8rscript/Global.cpp \
		../m8rscript/Object.cpp \
		../m8rscript/ParseEngine.cpp \
		../m8rscript/Parser.cpp \
		../m8rscript/Program.cpp \
		../m8rscript/Scanner.cpp \
		../m8rscript/Value.cpp \
		../m8rscript/base64.cpp \
		app/PlatformGlobal.cpp \
        ../m8rscript/main.cpp

# Esp8266 location
ESP_ROOT ?= $(HOME)/esp8266/tools
FLASH_LAYOUT ?= eagle.flash.4m.ld
ESP_TOOL = PATH=$(TOOLS_BIN):$(PATH) && ./esptool.py

#ESP_ROOT ?= $(HOME)/esp-open-sdk
#FLASH_LAYOUT ?= eagle.app.v6.ld 
#ESP_TOOL ?= PATH=$(TOOLS_BIN):$(PATH) && $(TOOLS_BIN)/esptool.py

# Output directory
BUILD_BASE	= build
FW_BASE		= firmware

# Board definitions
FLASH_SIZE ?= 4M
FLASH_MODE ?= dio
FLASH_SPEED ?= 40

# Upload parameters
UPLOAD_SPEED ?= 115200
UPLOAD_PORT ?= /dev/tty.usbserial-AD02CUJ5
UPLOAD_VERB ?= -v
UPLOAD_RESET ?= ck

# OTA parameters
ESP_ADDR ?= ESP_DA6ABC
ESP_PORT ?= 8266
ESP_PWD ?= 123
#====================================================================================
# The area below should normally not need to be edited
#====================================================================================

MKESPARD_VERSION = 1.0.0

START_TIME := $(shell perl -e "print time();")
# Main output definitions
MAIN_NAME = $(basename $(notdir $(MAIN_SRC)))
MAIN_EXE = $(BUILD_BASE)/$(MAIN_NAME).bin
MAIN_ELF = $(OBJ_DIR)/$(MAIN_NAME).elf
SRC_GIT_VERSION = $(call git_description,$(dir $(MAIN_SRC)))

# esp8266 arduino directories
ESP_GIT_VERSION = $(call git_description,$(ESP_ROOT))

TOOLS_BIN = $(ESP_ROOT)/xtensa-lx106-elf/bin
SDK_ROOT = $(ESP_ROOT)/sdk

# Directory for intermedite build files
OBJ_DIR = $(BUILD_BASE)/obj
OBJ_EXT = .o
DEP_EXT = .d

# Compiler definitions
CC = $(TOOLS_BIN)/xtensa-lx106-elf-gcc
CPP = $(TOOLS_BIN)/xtensa-lx106-elf-g++
LD =  $(CC)
AR = $(TOOLS_BIN)/xtensa-lx106-elf-ar

FW_FILE_1_ADDR	= 0x00000
FW_FILE_1	:= $(addprefix $(FW_BASE)/,$(FW_FILE_1_ADDR).bin)
FW_FILE_2_ADDR	= 0x40000
FW_FILE_2	:= $(addprefix $(FW_BASE)/,$(FW_FILE_2_ADDR).bin)

LIBS = c gcc hal pp phy net80211 lwip wpa main hal
LIBS := $(addprefix -l,$(LIBS))

USE_PARSE_ENGINE ?= 1
FIXED_POINT_FLOAT ?= 1
INCLUDE_DIRS += $(SDK_ROOT)/include $(OBJ_DIR) $(CORE_DIR) 
C_DEFINES = -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ -DF_CPU=80000000L -DARDUINO=10605 -DARDUINO_ESP8266_ESP01 -DARDUINO_ARCH_ESP8266 -DESP8266 -DUSE_PARSE_ENGINE=$(USE_PARSE_ENGINE) -DFIXED_POINT_FLOAT=$(FIXED_POINT_FLOAT)
C_INCLUDES = $(foreach dir,$(INCLUDE_DIRS) $(USER_DIRS),-I$(dir))
C_FLAGS ?= -c $(DEBUG_FLAGS) -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=gnu99 -ffunction-sections -fdata-sections
CPP_FLAGS ?= -c $(DEBUG_FLAGS) -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -std=c++11 -MMD -ffunction-sections -fdata-sections
S_FLAGS ?= -c -x assembler-with-cpp -MMD
LD_FLAGS ?= -flto -w $(DEBUG_FLAGS) -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -L$(SDK_ROOT)/lib -L$(SDK_ROOT)/ld -T$(FLASH_LAYOUT) -Wl,--gc-sections
# LD_STD_LIBS = $(LIBS) -lm -lsmartconfig -lwps -lcrypto -laxtls
# LD_STD_LIBS = $(LIBS)

#LD_STD_LIBS = -lgcc -lhal -lphy -lnet80211 -llwip -lwpa -lmain -lpp -lsmartconfig -lwps -lcrypto -laxtls
LD_STD_LIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lcrypto -Wl,--end-group -lgcc

# Core source files
#CORE_DIR =
CORE_DIR = ./core
CORE_SRC = $(shell find $(CORE_DIR) -name "*.S" -o -name "*.c" -o -name "*.cpp")
CORE_OBJ = $(patsubst %,$(OBJ_DIR)/%$(OBJ_EXT),$(notdir $(CORE_SRC)))
CORE_LIB = $(OBJ_DIR)/core.ar

# User defined compilation units
USER_SRC = $(MAIN_SRC) $(SRC)

# Object file suffix seems to be significant for the linker...
USER_OBJ = $(subst .ino,.cpp,$(patsubst %,$(OBJ_DIR)/%$(OBJ_EXT),$(notdir $(USER_SRC))))
USER_DIRS = $(sort $(dir $(USER_SRC)))

VPATH += $(CORE_DIR) $(USER_DIRS)

# Automatically generated build information data
# Makes the build date and git descriptions at the actual build
# event available as string constants in the program
BUILD_INFO_H = $(OBJ_DIR)/buildinfo.h
BUILD_INFO_CPP = $(OBJ_DIR)/buildinfo.cpp
BUILD_INFO_OBJ = $(BUILD_INFO_CPP)$(OBJ_EXT)

$(BUILD_INFO_H): | $(OBJ_DIR)
	echo "typedef struct { const char *date, *time, *src_version, *env_version;} _tBuildInfo; extern _tBuildInfo _BuildInfo;" >$@

# Utility functions
git_description = $(shell git -C  $(1) describe --tags --always --dirty 2>/dev/null)
time_string = $(shell perl -e 'use POSIX qw(strftime); print strftime($(1), localtime());')
MEM_USAGE = \
  'while (<>) { \
      $$r += $$1 if /^\.(?:data|rodata|bss)\s+(\d+)/;\
		  $$f += $$1 if /^\.(?:irom0\.text|text|data|rodata)\s+(\d+)/;\
	 }\
	 print "\nMemory usage\n";\
	 print sprintf("  %-6s %6d bytes\n" x 2 ."\n", "Ram:", $$r, "Flash:", $$f);'

# Build rules
$(OBJ_DIR)/%.cpp$(OBJ_EXT): %.cpp $(BUILD_INFO_H)
	echo  $(<F)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(OBJ_DIR)/%.cpp$(OBJ_EXT): %.ino $(BUILD_INFO_H)
	echo  $(<F)
	$(CPP) -x c++ -include $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $< -o $@

$(OBJ_DIR)/%.c$(OBJ_EXT): %.c
	echo  $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) $< -o $@

$(OBJ_DIR)/%.S$(OBJ_EXT): %.S
	echo  $(<F)
	$(CC) $(C_DEFINES) $(C_INCLUDES) $(S_FLAGS) $< -o $@

$(CORE_LIB): $(CORE_OBJ)
	echo  Creating core archive
	rm -f $@
	$(AR) cru $@  $^

BUILD_DATE = $(call time_string,"%Y-%m-%d")
BUILD_TIME = $(call time_string,"%H:%M:%S")

$(FW_BASE)/%.bin: $(MAIN_ELF) | $(FW_BASE)
	$(ESP_TOOL) elf2image --version=2 -o $(MAIN_EXE) $(MAIN_ELF)

$(MAIN_EXE): $(USER_OBJ) $(CORE_LIB)
	echo Linking $(MAIN_EXE)
	echo "  Versions: $(SRC_GIT_VERSION), $(ESP_GIT_VERSION)"
	echo 	'#include <buildinfo.h>' >$(BUILD_INFO_CPP)
	echo '_tBuildInfo _BuildInfo = {"$(BUILD_DATE)","$(BUILD_TIME)","$(SRC_GIT_VERSION)","$(ESP_GIT_VERSION)"};' >>$(BUILD_INFO_CPP)
	$(CPP) $(C_DEFINES) $(C_INCLUDES) $(CPP_FLAGS) $(BUILD_INFO_CPP) -o $(BUILD_INFO_OBJ)
	$(LD) $(LD_FLAGS) -Wl,--start-group $^ $(BUILD_INFO_OBJ) $(LD_STD_LIBS) -Wl,--end-group -L$(OBJ_DIR) -o $(MAIN_ELF)
	$(TOOLS_BIN)/xtensa-lx106-elf-size -A $(MAIN_ELF) | perl -e $(MEM_USAGE)
	perl -e 'print "Build complete. Elapsed time: ", time()-$(START_TIME),  " seconds\n\n"'

flash: all $(FW_FILE_1) $(FW_FILE_2)
	$(ESP_TOOL) --port $(UPLOAD_PORT) write_flash 0 $(MAIN_EXE)
	python -m serial.tools.miniterm $(UPLOAD_PORT) $(UPLOAD_SPEED)

upload: all
	$(ESP_TOOL) $(UPLOAD_VERB) -cd $(UPLOAD_RESET) -cb $(UPLOAD_SPEED) -cp $(UPLOAD_PORT) -ca 0x00000 -cf $(MAIN_EXE)
	python -m serial.tools.miniterm $(UPLOAD_PORT) $(UPLOAD_SPEED)

ota: all
	$(OTA_TOOL) -i $(ESP_ADDR) -p $(ESP_PORT) -a $(ESP_PWD) -f $(MAIN_EXE)

clean:
	echo Removing all intermediate build files...
	rm  -f $(OBJ_DIR)/*

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

.PHONY: all
all: $(OBJ_DIR) $(BUILD_INFO_H) $(MAIN_EXE) $(FW_FILE_1) $(FW_FILE_2)


# Include all available dependencies
-include $(wildcard $(OBJ_DIR)/*$(DEP_EXT))

.DEFAULT_GOAL = all

ifndef VERBOSE
# Set silent mode as default
MAKEFLAGS += --silent
endif
