#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

SRC_DIR := ../../src

COMPONENT_ADD_INCLUDEDIRS := $(SRC_DIR)
COMPONENT_SRCDIRS := . $(SRC_DIR)
COMPONENT_OBJS := m8rscript.o \
    RtosSystemInterface.o \
    $(SRC_DIR)/Mallocator.o \
    $(SRC_DIR)/MString.o \
    $(SRC_DIR)/Scanner.o \
    $(SRC_DIR)/SystemInterface.o \
$(SRC_DIR)/slre.o \
