# Copyright (C) 2008-2015 Marvell International Ltd.
# All Rights Reserved.
#

exec-y += firmata
firmata-objs-y := src/main.c
firmata-cflags-y := -I$(d)/src -DAPPCONFIG_DEBUG_ENABLE=1

ifneq ($(wildcard $(d)/www),)
firmata.app: firmata.ftfs
firmata-ftfs-y     := $(d)/www
firmata-ftfs-api-y := 100
endif

# Applications could also define custom linker files if required using following:
#firmata-ld-y := /path/to/ldscript
# Applications could also define custom board files if required using following:
#firmata-board-y := /path/to/boardfile
