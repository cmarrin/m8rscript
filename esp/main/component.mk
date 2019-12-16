#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

SRC_DIR := ../../src

COMPONENT_ADD_INCLUDEDIRS := $(SRC_DIR)
COMPONENT_SRCDIRS := . $(SRC_DIR)
COMPONENT_OBJS := m8rscript.o \
    RtosSystemInterface.o \
    $(SRC_DIR)/Application.o \
    $(SRC_DIR)/Atom.o \
    $(SRC_DIR)/Base64.o \
    $(SRC_DIR)/Closure.o \
    $(SRC_DIR)/Error.o \
    $(SRC_DIR)/ExecutionUnit.o \
    $(SRC_DIR)/Function.o \
    $(SRC_DIR)/GC.o \
    $(SRC_DIR)/Global.o \
    $(SRC_DIR)/GPIO.o \
    $(SRC_DIR)/IPAddr.o \
    $(SRC_DIR)/Iterator.o \
    $(SRC_DIR)/JSON.o \
    $(SRC_DIR)/Mallocator.o \
    $(SRC_DIR)/MFS.o \
    $(SRC_DIR)/MString.o \
    $(SRC_DIR)/Object.o \
    $(SRC_DIR)/Parser.o \
    $(SRC_DIR)/ParseEngine.o \
    $(SRC_DIR)/Program.o \
    $(SRC_DIR)/Scanner.o \
    $(SRC_DIR)/SharedAtoms.o \
    $(SRC_DIR)/SpiffsFS.o \
    $(SRC_DIR)/SystemInterface.o \
    $(SRC_DIR)/SystemTime.o \
    $(SRC_DIR)/Task.o \
    $(SRC_DIR)/TaskManager.o \
    $(SRC_DIR)/TCP.o \
    $(SRC_DIR)/Telnet.o \
    $(SRC_DIR)/UDP.o \
    $(SRC_DIR)/Value.o \
    $(SRC_DIR)/slre.o \
