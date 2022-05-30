
ifeq ($(filter IOBNATIVEBRIDGEIF, $(HW_MODULES)),)

include $(IOBNATIVEBRIDGEIF_DIR)/config.mk

#add itself to HW_MODULES list
HW_MODULES+=IOBNATIVEBRIDGEIF

IOBNATIVEBRIDGEIF_INC_DIR:=$(IOBNATIVEBRIDGEIF_HW_DIR)/include
IOBNATIVEBRIDGEIF_SRC_DIR:=$(IOBNATIVEBRIDGEIF_HW_DIR)/src

#include files
VHDR+=$(wildcard $(IOBNATIVEBRIDGEIF_INC_DIR)/*.vh)
VHDR+=iob_nativebridgeif_swreg_gen.vh iob_nativebridgeif_swreg_def.vh
VHDR+=$(LIB_DIR)/hardware/include/iob_lib.vh $(LIB_DIR)/hardware/include/iob_s_if.vh $(LIB_DIR)/hardware/include/iob_gen_if.vh



#hardware include dirs
INCLUDE+=$(incdir). $(incdir)$(IOBNATIVEBRIDGEIF_INC_DIR) $(incdir)$(LIB_DIR)/hardware/include

#sources
VSRC+=$(IOBNATIVEBRIDGEIF_SRC_DIR)/iob_nativebridgeif.v

endif
    