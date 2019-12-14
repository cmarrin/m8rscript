#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

SRC_DIR := ../../src

COMPONENT_ADD_INCLUDEDIRS := $(SRC_DIR)
COMPONENT_SRCDIRS := . $(SRC_DIR)
COMPONENT_OBJS := m8rscript.o \
    $(SRC_DIR)/Mallocator.o \
