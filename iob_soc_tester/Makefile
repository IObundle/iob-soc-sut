CORE := iob_soc_sut
BOARD ?= AES-KU040-DB-G
include submodules/LIB/setup.mk

INIT_MEM ?= 1

ifeq ($(INIT_MEM),1)
SETUP_ARGS += INIT_MEM
endif

ifeq ($(USE_EXTMEM),1)
SETUP_ARGS += USE_EXTMEM
endif

ifeq ($(TESTER_ONLY),1)
SETUP_ARGS += TESTER_ONLY
endif

