#
# "m8rScript" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

M8RSCRIPT_DIR := ../../../lib/m8rscript/src
M8R_DIR := ../../../lib/libm8r/src

CXXFLAGS += -std=c++14

COMPONENT_ADD_INCLUDEDIRS := $(M8RSCRIPT_DIR) $(M8R_DIR)
COMPONENT_EXTRA_INCLUDES := 
COMPONENT_SRCDIRS := . $(M8RSCRIPT_DIR)
COMPONENT_OBJS := \
    $(M8RSCRIPT_DIR)/Closure.o \
    $(M8RSCRIPT_DIR)/CodePrinter.o \
    $(M8RSCRIPT_DIR)/ExecutionUnit.o \
    $(M8RSCRIPT_DIR)/FSProto.o \
    $(M8RSCRIPT_DIR)/Function.o \
    $(M8RSCRIPT_DIR)/GC.o \
    $(M8RSCRIPT_DIR)/GeneratedValues.o \
    $(M8RSCRIPT_DIR)/Global.o \
    $(M8RSCRIPT_DIR)/GPIO.o \
    $(M8RSCRIPT_DIR)/IPAddrProto.o \
    $(M8RSCRIPT_DIR)/Iterator.o \
    $(M8RSCRIPT_DIR)/JSONProto.o \
    $(M8RSCRIPT_DIR)/Object.o \
    $(M8RSCRIPT_DIR)/Parser.o \
    $(M8RSCRIPT_DIR)/ParseEngine.o \
    $(M8RSCRIPT_DIR)/Program.o \
    $(M8RSCRIPT_DIR)/StreamProto.o \
    $(M8RSCRIPT_DIR)/TaskProto.o \
    $(M8RSCRIPT_DIR)/TCPProto.o \
    $(M8RSCRIPT_DIR)/TimerProto.o \
    $(M8RSCRIPT_DIR)/Value.o \
