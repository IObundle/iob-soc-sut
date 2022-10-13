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
#BOARD:=CYCLONEV-GT-DK

#Add Unit Under Test to Tester peripherals list
#this works even if UUT is not a "peripheral"
PERIPHERALS+=$(UUT_NAME)[\`ADDR_W,\`DATA_W,AXI_ID_W]
# Tester peripherals to add (besides the default ones in IOb-SoC-Tester)
PERIPHERALS+=UART IOBNATIVEBRIDGEIF
# Instance 0 of ETHERNET has default MAC address. Instance 1 has the same MAC address as the console (this way, the UUT always connects to the console's MAC address).
PERIPHERALS+=ETHERNET
PERIPHERALS+=ETHERNET[32,\`iob_eth_swreg_ADDR_W,48'h$(RMAC_ADDR)]

# Submodule paths for Tester peripherals (listed above)
IOBNATIVEBRIDGEIF_DIR=$($(UUT_NAME)_DIR)/submodules/IOBNATIVEBRIDGEIF
ETHERNET_DIR=$($(UUT_NAME)_DIR)/submodules/ETHERNET

#Root directory on remote machines
REMOTE_UUT_DIR ?=sandbox/iob-soc-sut

#Set SIM macro for firmware.
#Can be used for UART to transfer files from/to console, as these transfers are not compatible with ETHERNET during simulation.
ifneq ($(ISSIMULATION),)
DEFINE+=$(defmacro)SIM=1
endif

#Mac address of pc interface connected to ethernet peripheral
ifeq ($(BOARD),AES-KU040-DB-G) # Arroz eth if mac
RMAC_ADDR:=4437e6a6893b
else # Pudim eth if mac
RMAC_ADDR:=309c231e624a
endif
#Auto-set ethernet interface name based on MAC address
ETH_IF:=$(shell ip -br link | sed 's/://g' | grep $(RMAC_ADDR) | cut -d " " -f1)

#Configure Tester to use ethernet
USE_ETHERNET:=1
DEFINE+=$(defmacro)USE_ETHERNET=1

# Override Tester top system to include ethernet clock generation
SIMULATION_TOP_SYSTEM=$($(UUT_NAME)_DIR)/tester_top_system/SIMULATION_top_system.v
ifeq ($(BOARD),AES-KU040-DB-G)
BOARD_TOP_SYSTEM=$($(UUT_NAME)_DIR)/tester_top_system/KU040_top_system.v
else
BOARD_TOP_SYSTEM=$($(UUT_NAME)_DIR)/tester_top_system/CYCLONEV_top_system.v
endif

#Extra tester target dependencies
#Run before building system
BUILD_DEPS+=$($(UUT_NAME)_DIR)/hardware/src/system.v
#Run before building system for simulation
SIM_DEPS+=set-simulation-variable
#Run before building system for fpga
FPGA_DEPS+=
#Run when cleaning tester
CLEAN_DEPS+=clean-top-module clean-sut-fw
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
	make -C $($(UUT_NAME)_DIR)/software/firmware build-all BAUD=$(BAUD)
	make -C $($(UUT_NAME)_DIR)/software/firmware -f ../../hardware/hardware.mk boot.hex firmware.hex ROOT_DIR=../..

clean-sut-fw:
	make -C $($(UUT_NAME)_DIR) fw-clean

#Set ISSIMULATION variable
set-simulation-variable:
	$(eval export ISSIMULATION=1)

#Test targets
check-sim:
	rm -f $(SIM_DIR)/test.log
	make clean sim-build sim-run INIT_MEM=1 USE_DDR=0 RUN_EXTMEM=0 TEST_LOG=">> test.log"
	#make clean sim-build sim-run INIT_MEM=0 USE_DDR=0 RUN_EXTMEM=0 TEST_LOG=">> test.log"
	make clean sim-build sim-run INIT_MEM=1 USE_DDR=1 RUN_EXTMEM=0 TEST_LOG=">> test.log"
	make clean sim-build sim-run INIT_MEM=1 USE_DDR=1 RUN_EXTMEM=1 TEST_LOG=">> test.log"
	#make clean sim-build sim-run INIT_MEM=0 USE_DDR=1 RUN_EXTMEM=1 TEST_LOG=">> test.log"
	diff $(SIM_DIR)/test.log $($(UUT_NAME)_DIR)/tester_expected/test-sim.expected

check-fpga:
	rm -f $(BOARD_DIR)/test.log
	make clean fpga-build fpga-run INIT_MEM=1 USE_DDR=0 RUN_EXTMEM=0 TEST_LOG=">> test.log"
	#make clean fpga-build fpga-run INIT_MEM=0 USE_DDR=0 RUN_EXTMEM=0 TEST_LOG=">> test.log"
	make clean fpga-build fpga-run INIT_MEM=1 USE_DDR=1 RUN_EXTMEM=0 TEST_LOG=">> test.log"
	#make clean fpga-build fpga-run INIT_MEM=1 USE_DDR=1 RUN_EXTMEM=1 TEST_LOG=">> test.log"
	diff $(BOARD_DIR)/test.log $($(UUT_NAME)_DIR)/tester_expected/test-fpga.expected

.PHONY: clean-top-module clean-sut-fw set-simulation-variable check-sim check-fpga
endif
