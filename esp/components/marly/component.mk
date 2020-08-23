#
# "m8rScript" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

MARLY_DIR := ../../../lib/marly/src

CXXFLAGS += -std=c++14

COMPONENT_ADD_INCLUDEDIRS := $(MARLY_DIR)
COMPONENT_EXTRA_INCLUDES := 
COMPONENT_SRCDIRS := . $(MARLY_DIR)
COMPONENT_OBJS := \
    $(MARLY_DIR)/Marly.o \
    $(MARLY_DIR)/MarlyValue.o \
    $(MARLY_DIR)/GeneratedValues.o \
