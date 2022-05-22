#
ifeq ($(INCLUDING_PATHS),)
# MAKEFILE VARIABLES: PLACE BELOW VARIABLES USED BY TESTER
#

# Name of this unit under test
UUT_NAME=IOBSOC

# Tester peripherals to add (besides the default ones in IOb-SoC-Tester)
PERIPHERALS+=IOBNATIVEBRIDGEIF

# Submodule paths for Tester peripherals (listed above)
IOBNATIVEBRIDGEIF_DIR=$($(UUT_NAME)_DIR)/submodules/IOBNATIVEBRIDGEIF

#Root directory on remote machines
REMOTE_UUT_DIR ?=sandbox/iob-soc-sut

#Extra tester dependencies
SIM_DEPS+=
FPGA_DEPS+=
CLEAN_DEPS+=clean-top-module
#TODO: This can be useful to copy files from remote machines at the end of a run sequence
#FPGA_RUN_FINISH_DEPS+=

#
else
# MAKEFILE TARGETS: PLACE BELOW EXTRA TARGETS USED BY TESTER
#

create-top-module:
	(cd $($(UUT_NAME)_DIR)/hardware/src; make -C .. -f hardware.mk system.v ROOT_DIR=..)
	mv $($(UUT_NAME)_DIR)/hardware/src/system.v $($(UUT_NAME)_DIR)/hardware/src/iob_system_top.v

clean-top-module:
	rm -f $($(UUT_NAME)_DIR)/hardware/src/iob_system_top.v

endif
