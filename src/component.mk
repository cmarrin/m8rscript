#
# "libm8r" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

CXXFLAGS += -std=c++14

COMPONENT_NAME := libm8r

COMPONENT_OBJS := \
    Closure.o \
    CodePrinter.o \
    ExecutionUnit.o \
    FSProto.o \
    Function.o \
    GC.o \
    GeneratedValues.o \
    Global.o \
    GPIO.o \
    IPAddrProto.o \
    Iterator.o \
    JSONProto.o \
    Object.o \
    ParseEngine.o \
    Parser.o \
    Program.o \
    StreamProto.o \
    TaskProto.o \
    TCPProto.o \
    TimerProto.o \
    Value.o \
