MAIN_SRC = app/m8rscript.cpp

ESP_ROOT ?= /Volumes/esp-open-sdk/esp-open-sdk
TOOLS_BIN = $(ESP_ROOT)/xtensa-lx106-elf/bin
CC = $(TOOLS_BIN)/xtensa-lx106-elf-gcc
ESPTOOL = PATH=$(TOOLS_BIN):$(PATH) && $(TOOLS_BIN)/esptool.py

MAIN_NAME = $(basename $(notdir $(MAIN_SRC)))

CFLAGS = -I. -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -Wl,--end-group -lgcc
LDFLAGS = -Teagle.app.v6.ld

$(MAIN_NAME)-0x00000.bin: $(MAIN_NAME)
	$(ESPTOOL) elf2image $^

$(MAIN_NAME): $(MAIN_NAME).o

$(MAIN_NAME).o: $(MAIN_SRC)

flash: $(MAIN_NAME)-0x00000.bin
	$(ESPTOOL) write_flash 0 $(MAIN_NAME)-0x00000.bin 0x40000 $(MAIN_NAME)-0x40000.bin

clean:
	rm -f $(MAIN_NAME) $(MAIN_NAME).o $(MAIN_NAME)-0x00000.bin $(MAIN_NAME)-0x40000.bin
