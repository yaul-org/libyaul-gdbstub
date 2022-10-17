THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

TRAPA_VECTOR_NUMBER:= 32

# Override the build process
.PHONY: override-build-process
override-build-process: .build-binary

.PHONY: .clean-remove-binary
.clean-remove-binary:
	$(ECHO)rm -f $(SH_PROGRAM).bin

.PHONY: .clean-remove-processed-headers
.clean-remove-processed-headers:
	$(ECHO)rm -f gdbstub.h

clean: .clean-remove-binary
clean: .clean-remove-processed-headers

gdbstub.h: gdbstub.h.in
	@sed -E 's#\$$TRAPA_VECTOR_NUMBER\$$#$(TRAPA_VECTOR_NUMBER)#g' gdbstub.h.in > gdbstub.h

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

SH_PROGRAM:= gdbstub

SH_SRCS:= \
	gdbstub.c \
	exceptions.sx

SH_SPECS:= $(THIS_ROOT)/gdb.specs

SH_LIBRARIES:=
SH_CFLAGS+= -g -Os -I. -DTRAPA_VECTOR_NUMBER=$(TRAPA_VECTOR_NUMBER)

include $(YAUL_INSTALL_ROOT)/share/post.common.mk

.PHONY: .process-header
.process-header: gdbstub.h

.PHONY: .build-binary
.build-binary: .process-header $(SH_BUILD_PATH)/$(SH_PROGRAM).bin
	$(ECHO)cp $(SH_BUILD_PATH)/$(SH_PROGRAM).bin $(THIS_ROOT)/$(SH_PROGRAM).bin

# Prevent _master_stack and _slave_stack from being generated
SH_DEFSYMS:=
