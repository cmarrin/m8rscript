#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

CFLAGS += 
CXXFLAGS += -std=c++14

COMPONENT_NAME := test

COMPONENT_PRIV_INCLUDEDIRS :=  ../../libm8r/components/libm8r ../components/m8rscript

COMPONENT_OBJS := \
    m8rscript.o \
