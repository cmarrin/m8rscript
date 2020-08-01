#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

CFLAGS += 
CXXFLAGS += -std=c++14

COMPONENT_OBJS := \
    m8rscript.o \
    RtosGPIOInterface.o \
    RtosSystemInterface.o \
    RtosTCP.o \
    RtosWifi.o \
