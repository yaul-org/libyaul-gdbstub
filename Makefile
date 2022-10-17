THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

# Override the build process to prevent an ISO/CUE from being built
.PHONY: override-build-process
override-build-process: .build-binary

.PHONY: .clean-remove-binary
.clean-remove-binary:
	$(ECHO)rm -f $(SH_PROGRAM).bin

clean: .clean-remove-binary

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= gdbstub

SH_SRCS:= \
	gdbstub.c \
	sh2.c \
	helpers.c \
	exceptions.sx

SH_SPECS:= $(THIS_ROOT)/gdb.specs

SH_LIBRARIES:=
SH_CFLAGS+= -g -Os -I.

include $(YAUL_INSTALL_ROOT)/share/post.common.mk

.PHONY: .build-binary
.build-binary: $(SH_BUILD_PATH)/$(SH_PROGRAM).bin
	$(ECHO)cp $(SH_BUILD_PATH)/$(SH_PROGRAM).bin $(THIS_ROOT)/$(SH_PROGRAM).bin

# Prevent _master_stack and _slave_stack from being generated
SH_DEFSYMS:=
