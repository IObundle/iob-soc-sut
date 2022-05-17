#
# MAKEFILE VARIABLES: VARIABLES USED BY TESTER
ifeq ($(INCLUDING_PATHS),)
#

# Name of this unit under test
UUT_NAME=IOBSOC

# Tester peripherals to add (besides the default ones in IOb-SoC)
TESTER_PERIPHERALS=IOBNATIVEBRIDGEIF

# Submodule paths for Tester peripherals (listed above)
IOBNATIVEBRIDGEIF_DIR=$($(CORE_UT)_DIR)/submodules/IOBNATIVEBRIDGEIF

#Root directory on remote machines
REMOTE_CUT_DIR ?=sandbox/iob-soc-sut

#Extra tester dependencies
SIM_DEPS+=
FPGA_DEPS+=
CLEAN_DEPS+=

#
# MAKEFILE TARGETS: EXTRA TARGETS USED BY TESTER
else
#

endif
