## Local build configuration
## Parameters configured here will override default and ENV values.
## Uncomment and change examples:

## Add your source directories here separated by space
MODULES = core-sming app ../m8rscript
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
#DISABLE_SPIFFS = 1
SPIFF_FILES = files

ENABLE_GDB=0

MEM_USAGE = \
  'while (<>) { \
      $$r += $$1 if /^\.(?:data|rodata|bss)\s+(\d+)/;\
		  $$f += $$1 if /^\.(?:irom0\.text|text|data|rodata)\s+(\d+)/;\
	 }\
	 print "\nMemory usage\n";\
	 print sprintf("  %-6s %6d bytes\n" x 2 ."\n", "Ram:", $$r, "Flash:", $$f);'

# Include main Sming Makefile
ifeq ($(RBOOT_ENABLED), 1)
include $(SMING_HOME)/Makefile-rboot.mk
else
include $(SMING_HOME)/Makefile-project.mk
endif

size: $(MAIN_ELF)
	$(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-size -A $(TARGET_OUT) | perl -e $(MEM_USAGE)
