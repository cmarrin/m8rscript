#
# "libm8r" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

LIBM8R_DIR := ../../../lib/libm8r/src

CXXFLAGS += -std=c++14

COMPONENT_ADD_INCLUDEDIRS := $(LIBM8R_DIR) $(LIBM8R_DIR)/littlefs
COMPONENT_EXTRA_INCLUDES := 
COMPONENT_SRCDIRS := . $(LIBM8R_DIR) $(LIBM8R_DIR)/littlefs
COMPONENT_OBJS := \
    $(LIBM8R_DIR)/Application.o \
    $(LIBM8R_DIR)/Atom.o \
    $(LIBM8R_DIR)/Base64.o \
    $(LIBM8R_DIR)/Containers.o \
    $(LIBM8R_DIR)/Error.o \
    $(LIBM8R_DIR)/Executable.o \
    $(LIBM8R_DIR)/HTTPServer.o \
    $(LIBM8R_DIR)/IPAddr.o \
    $(LIBM8R_DIR)/Mallocator.o \
    $(LIBM8R_DIR)/MFS.o \
    $(LIBM8R_DIR)/MString.o \
    $(LIBM8R_DIR)/MUDP.o \
    $(LIBM8R_DIR)/Scanner.o \
    $(LIBM8R_DIR)/Shell.o \
    $(LIBM8R_DIR)/SystemInterface.o \
    $(LIBM8R_DIR)/SystemTime.o \
    $(LIBM8R_DIR)/Task.o \
    $(LIBM8R_DIR)/TaskManager.o \
    $(LIBM8R_DIR)/TCP.o \
    $(LIBM8R_DIR)/TCPServer.o \
    $(LIBM8R_DIR)/Telnet.o \
    $(LIBM8R_DIR)/Terminal.o \
    $(LIBM8R_DIR)/Timer.o \
    $(LIBM8R_DIR)/littlefs/MLittleFS.o \
    $(LIBM8R_DIR)/littlefs/lfs.o \
    $(LIBM8R_DIR)/littlefs/lfs_util.o \
