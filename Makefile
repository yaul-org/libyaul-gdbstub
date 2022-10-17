THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

SH_PROGRAM:= gdbstub

SH_SRCS:= \
	gdbstub.c \
	sh2.c \
	helpers.c \
	exceptions.sx

SH_SPECS:= $(THIS_ROOT)/gdb.specs

SH_LIBRARIES:=
SH_CFLAGS+= -g -Os -I.

# Build the executable only
include $(YAUL_INSTALL_ROOT)/share/build.post.bin.mk
