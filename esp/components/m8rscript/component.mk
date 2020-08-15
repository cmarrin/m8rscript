#
# "m8rScript" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

M8R_DIR := ../../../lib/m8rscript/src
LUA_DIR := ../../../lib/lua-5.3.5/src

CFLAGS += -DLUA_32BITS
CXXFLAGS += -std=c++14 -DLUA_32BITS

COMPONENT_ADD_INCLUDEDIRS := $(M8R_DIR) $(M8R_DIR)/littlefs $(LUA_DIR)
COMPONENT_EXTRA_INCLUDES := 
COMPONENT_SRCDIRS := . $(M8R_DIR) $(M8R_DIR)/littlefs
COMPONENT_OBJS := \
    $(M8R_DIR)/Application.o \
    $(M8R_DIR)/Atom.o \
    $(M8R_DIR)/Base64.o \
    $(M8R_DIR)/Closure.o \
    $(M8R_DIR)/CodePrinter.o \
    $(M8R_DIR)/Error.o \
    $(M8R_DIR)/Executable.o \
    $(M8R_DIR)/ExecutionUnit.o \
    $(M8R_DIR)/Function.o \
    $(M8R_DIR)/GC.o \
    $(M8R_DIR)/Global.o \
    $(M8R_DIR)/GPIO.o \
    $(M8R_DIR)/HTTPServer.o \
    $(M8R_DIR)/IPAddr.o \
    $(M8R_DIR)/Iterator.o \
    $(M8R_DIR)/JSON.o \
    $(M8R_DIR)/LuaEngine.o \
    $(M8R_DIR)/Mallocator.o \
    $(M8R_DIR)/Marly.o \
    $(M8R_DIR)/MFS.o \
    $(M8R_DIR)/MString.o \
    $(M8R_DIR)/MUDP.o \
    $(M8R_DIR)/Object.o \
    $(M8R_DIR)/Parser.o \
    $(M8R_DIR)/ParseEngine.o \
    $(M8R_DIR)/Program.o \
    $(M8R_DIR)/Scanner.o \
    $(M8R_DIR)/GeneratedValues.o \
    $(M8R_DIR)/Shell.o \
    $(M8R_DIR)/SystemInterface.o \
    $(M8R_DIR)/SystemTime.o \
    $(M8R_DIR)/Task.o \
    $(M8R_DIR)/TaskManager.o \
    $(M8R_DIR)/TCP.o \
    $(M8R_DIR)/TCPServer.o \
    $(M8R_DIR)/Telnet.o \
    $(M8R_DIR)/Terminal.o \
    $(M8R_DIR)/Timer.o \
    $(M8R_DIR)/Value.o \
    $(M8R_DIR)/littlefs/MLittleFS.o \
    $(M8R_DIR)/littlefs/lfs.o \
    $(M8R_DIR)/littlefs/lfs_util.o \
