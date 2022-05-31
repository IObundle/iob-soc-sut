# Tester configuration file
# Use this file to set/override tester parameters and makefile targets
#
ifeq ($(INCLUDING_PATHS),)
# MAKEFILE VARIABLES: PLACE BELOW VARIABLES USED BY TESTER
#

# Name of this unit under test
UUT_NAME=IOBSOC

#FIRMWARE SIZE (LOG2)
FIRM_ADDR_W:=15

#SRAM SIZE (LOG2)
SRAM_ADDR_W:=15

#DDR
USE_DDR:=0
RUN_EXTMEM:=0

#DATA CACHE ADDRESS WIDTH (tag + index + offset)
DCACHE_ADDR_W:=24

#ROM SIZE (LOG2)
BOOTROM_ADDR_W:=12

#PRE-INIT MEMORY WITH PROGRAM AND DATA
INIT_MEM:=1

#SIMULATION
#default simulator running locally or remotely
#check the respective Makefile in TESTER/hardware/simulation/$(SIMULATOR) for specific settings
SIMULATOR:=icarus

#BOARD
#default board running locally or remotely
#check the respective Makefile in TESTER/hardware/fpga/$(BOARD) for specific settings
BOARD:=CYCLONEV-GT-DK

# Tester peripherals to add (besides the default ones in IOb-SoC-Tester)
PERIPHERALS+=UART IOBNATIVEBRIDGEIF

# Submodule paths for Tester peripherals (listed above)
IOBNATIVEBRIDGEIF_DIR=$($(UUT_NAME)_DIR)/submodules/IOBNATIVEBRIDGEIF

#Root directory on remote machines
REMOTE_UUT_DIR ?=sandbox/iob-soc-sut

#Extra tester target dependencies
#Run before building system
BUILD_DEPS+=$($(UUT_NAME)_DIR)/hardware/src/system.v
#Run before building system for simulation
SIM_DEPS+=
#Run before building system for fpga
FPGA_DEPS+=
#Run when cleaning tester
CLEAN_DEPS+=clean-top-module
#Run after finishing fpga run (useful to copy files from remote machines at the end of a run sequence)
FPGA_POST_RUN_DEPS+=

#
else
# MAKEFILE TARGETS: PLACE BELOW EXTRA TARGETS USED BY TESTER
#

#Target to build UUT topsystem
$($(UUT_NAME)_DIR)/hardware/src/system.v:
	make -C $($(UUT_NAME)_DIR)/hardware/src -f ../hardware.mk system.v ROOT_DIR=../..

clean-top-module:
	rm -f $($(UUT_NAME)_DIR)/hardware/src/system.v

#Target to build UUT bootloader and firmware
$($(UUT_NAME)_DIR)/software/firmware/boot.hex $($(UUT_NAME)_DIR)/software/firmware/firmware.hex:
	make -C $($(UUT_NAME)_DIR)/software/firmware build-all
	make -C $($(UUT_NAME)_DIR)/software/firmware -f ../../hardware/hardware.mk boot.hex firmware.hex ROOT_DIR=../..

.PHONY: clean-top-module
endif
