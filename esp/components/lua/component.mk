#
# "Lua" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

LUA_DIR := ../../../lib/lua-5.3.5/src

CFLAGS += -DLUA_32BITS
CXXFLAGS += -std=c++14 -DLUA_32BITS

COMPONENT_SRCDIRS := $(LUA_DIR)
COMPONENT_OBJS := \
    $(LUA_DIR)/lapi.o \
    $(LUA_DIR)/lauxlib.o \
    $(LUA_DIR)/lbaselib.o \
    $(LUA_DIR)/lbitlib.o \
    $(LUA_DIR)/lcode.o \
    $(LUA_DIR)/lcorolib.o \
    $(LUA_DIR)/lctype.o \
    $(LUA_DIR)/ldblib.o \
    $(LUA_DIR)/ldebug.o \
    $(LUA_DIR)/ldo.o \
    $(LUA_DIR)/ldump.o \
    $(LUA_DIR)/lfunc.o \
    $(LUA_DIR)/lgc.o \
    $(LUA_DIR)/linit.o \
    $(LUA_DIR)/liolib.o \
    $(LUA_DIR)/llex.o \
    $(LUA_DIR)/lmathlib.o \
    $(LUA_DIR)/lmem.o \
    $(LUA_DIR)/loadlib.o \
    $(LUA_DIR)/lobject.o \
    $(LUA_DIR)/lopcodes.o \
    $(LUA_DIR)/loslib.o \
    $(LUA_DIR)/lparser.o \
    $(LUA_DIR)/lstate.o \
    $(LUA_DIR)/lstring.o \
    $(LUA_DIR)/lstrlib.o \
    $(LUA_DIR)/ltable.o \
    $(LUA_DIR)/ltablib.o \
    $(LUA_DIR)/ltm.o \
    $(LUA_DIR)/lundump.o \
    $(LUA_DIR)/lutf8lib.o \
    $(LUA_DIR)/lvm.o \
    $(LUA_DIR)/lzio.o \
