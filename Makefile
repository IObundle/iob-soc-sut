CORE := iob_soc_sut
BOARD ?= AES-KU040-DB-G

# Disable Linter while rules are not finished
DISABLE_LINT:=1

ifeq ($(TESTER),1)
TOP_MODULE_NAME :=iob_soc_tester
ifneq ($(USE_EXTMEM),1)
$(warning WARNING: USE_EXTMEM must be set to support tester with iob-soc-opencryptolinux. Auto-adding USE_EXTMEM=1...)
SETUP_ARGS += USE_EXTMEM
endif
endif

include submodules/LIB/setup.mk

INIT_MEM ?= 1
RUN_LINUX ?= 0

ifeq ($(INIT_MEM),1)
SETUP_ARGS += INIT_MEM
endif

ifeq ($(USE_EXTMEM),1)
SETUP_ARGS += USE_EXTMEM
endif

ifeq ($(TESTER_ONLY),1)
SETUP_ARGS += TESTER_ONLY
endif

ifeq ($(RUN_LINUX),1)
SETUP_ARGS += RUN_LINUX
endif

sim-test:
	make clean && make setup && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup INIT_MEM=0 && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup USE_EXTMEM=1 && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup INIT_MEM=0 USE_EXTMEM=1 && make -C ../iob_soc_sut_V*/ sim-test

tester-sim-test: build_dir_name
	# IOb-SoC-Opencryptolinux only supports USE_EXTMEM=1. Ethernet also only supports USE_EXTMEM=1.
	#make clean && make setup INIT_MEM=1 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) sim-run | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) sim-run | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	make clean && make setup INIT_MEM=1 USE_EXTMEM=1 TESTER=1 && make -C $(BUILD_DIR) sim-run | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	# Disabled test with INIT_MEM=0 because it takes too long for Tester based on Opencryptolinux
	# make clean && make setup INIT_MEM=0 USE_EXTMEM=1 TESTER=1 && make -C $(BUILD_DIR) sim-run | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null

tester-sim-test-icarus: build_dir_name
	# IOb-SoC-Opencryptolinux only supports USE_EXTMEM=1. Ethernet also only supports USE_EXTMEM=1.
	#make clean && make setup INIT_MEM=1 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) sim-run SIMULATOR=icarus | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	make clean && make setup INIT_MEM=1 USE_EXTMEM=1 TESTER=1 && make -C $(BUILD_DIR) sim-run SIMULATOR=icarus | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null

fpga-test:
	make clean && make setup && make -C ../iob_soc_sut_V*/ fpga-test BOARD=$(BOARD)
	make clean && make setup INIT_MEM=0 && make -C ../iob_soc_sut_V*/ fpga-test BOARD=$(BOARD)
	make clean && make setup INIT_MEM=0 USE_EXTMEM=1 && make -C ../iob_soc_sut_V*/ fpga-test BOARD=$(BOARD)

tester-fpga-test: build_dir_name
	# IOb-SoC-Opencryptolinux only supports USE_EXTMEM=1
	#make clean && make setup INIT_MEM=1 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) fpga-run BOARD=$(BOARD) | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) fpga-run BOARD=$(BOARD) | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	make clean && make setup INIT_MEM=0 USE_EXTMEM=1 TESTER=1 && make -C $(BUILD_DIR) fpga-run BOARD=$(BOARD) | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null

tester-fpga-test-cyclone: build_dir_name
	# IOb-SoC-Opencryptolinux only supports USE_EXTMEM=1
	#make clean && make setup INIT_MEM=1 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) fpga-run BOARD=CYCLONEV-GT-DK | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	# Disable test for now, as they have timing problems
	# Also disable because ILA is not supported by Quartus
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=0 TESTER=1 && make -C $(BUILD_DIR) fpga-run BOARD=CYCLONEV-GT-DK | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=1 TESTER=1 && make -C $(BUILD_DIR) fpga-run BOARD=CYCLONEV-GT-DK | tee $(BUILD_DIR)/test.log && grep "Verification successful!" $(BUILD_DIR)/test.log > /dev/null

test-all:
	make clean && make setup && make -C ../iob_soc_sut_V*/ pc-emul-test
	#make sim-test SIMULATOR=icarus
	make sim-test SIMULATOR=verilator
	make fpga-test BOARD=CYCLONEV-GT-DK
	make fpga-test BOARD=AES-KU040-DB-G
	make clean && make setup && make -C ../iob_soc_sut_V*/ doc-test

.PHONY: sim-test fpga-test test-all
.PHONY: tester-sim-test tester-sim-test-icarus tester-fpga-test tester-fpga-test-cyclone

build-sut-netlist:
	make clean && make setup 
	# Rename constraint files
	#FPGA_DIR=`ls -d ../iob_soc_sut_V*/hardware/fpga/quartus/CYCLONEV-GT-DK` &&\
	#mv $$FPGA_DIR/iob_soc_sut_fpga_wrapper_dev.sdc $$FPGA_DIR/iob_soc_sut_dev.sdc
	#FPGA_DIR=`ls -d ../iob_soc_sut_V*/hardware/fpga/vivado/AES-KU040-DB-G` &&\
	#mv $$FPGA_DIR/iob_soc_sut_fpga_wrapper_dev.sdc $$FPGA_DIR/iob_soc_sut_dev.sdc
	# Build netlist 
	make -C ../iob_soc_sut_V*/ fpga-build BOARD=$(BOARD) IS_FPGA=0

tester-sut-netlist: build-sut-netlist
	#Build tester without sut sources, but with netlist instead
	TESTER_VER=`cat submodules/TESTER/iob_soc_tester_setup.py | grep version= | cut -d"'" -f2` &&\
	rm -fr ../iob_soc_tester_V* && make setup TESTER_ONLY=1 BUILD_DIR="../iob_soc_tester_$$TESTER_VER" &&\
	cp ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_fpga_wrapper_netlist.v ../iob_soc_tester_$$TESTER_VER/hardware/fpga/iob_soc_sut.v &&\
	cp ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_firmware.* ../iob_soc_tester_$$TESTER_VER/hardware/fpga/ &&\
	if [ -f ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_stub.v ]; then cp ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_stub.v ../iob_soc_tester_$$TESTER_VER/hardware/src/; fi &&\
	echo -e "\nIP+=iob_soc_sut.v" >> ../iob_soc_tester_$$TESTER_VER/hardware/fpga/fpga_build.mk &&\
	cp software/firmware/iob_soc_tester_firmware.c ../iob_soc_tester_$$TESTER_VER/software/firmware
	# Copy and modify iob_soc_sut_params.vh (needed for stub) and modify *_stub.v to insert the SUT parameters 
	TESTER_VER=`cat submodules/TESTER/iob_soc_tester_setup.py | grep version= | cut -d"'" -f2` &&\
	if [ -f ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_stub.v ]; then\
		cp ../iob_soc_sut_V0.70/hardware/src/iob_soc_sut_params.vh ../iob_soc_tester_$$TESTER_VER/hardware/src/;\
		sed -i -E 's/=[^,]*(,?)$$/=0\1/g' ../iob_soc_tester_$$TESTER_VER/hardware/src/iob_soc_sut_params.vh;\
		sed -i 's/_sut(/_sut#(\n`include "iob_soc_sut_params.vh"\n)(/g' ../iob_soc_tester_$$TESTER_VER/hardware/src/iob_soc_sut_stub.v;\
	fi
	# Run Tester on fpga
	TESTER_VER=`cat submodules/TESTER/iob_soc_tester_setup.py | grep version= | cut -d"'" -f2` &&\
	make -C ../iob_soc_tester_V*/ fpga-run BOARD=$(BOARD) | tee ../iob_soc_tester_$$TESTER_VER/test.log && grep "Verification successful!" ../iob_soc_tester_$$TESTER_VER/test.log > /dev/null

.PHONY: build-sut-netlist test-sut-netlist

# Target to create vcd file based on ila_data.bin generated by the ILA Tester peripheral
ila-vcd:
	# Create VCD file from simulation ila data
	if [ -f ../iob_soc_sut_V0.70/hardware/simulation/ila_data.bin ]; then \
		./../iob_soc_sut_V0.70/./scripts/ilaDataToVCD.py ILA0 ../iob_soc_sut_V0.70/hardware/simulation/ila_data.bin ila_sim.vcd; fi
	# Create VCD file from fpga ila data
	if [ -f ../iob_soc_sut_V0.70/hardware/fpga/ila_data.bin ]; then \
		./../iob_soc_sut_V0.70/./scripts/ilaDataToVCD.py ILA0 ../iob_soc_sut_V0.70/hardware/fpga/ila_data.bin ila_fpga.vcd; fi
	#gtkwave ./ila_sim.vcd
.PHONY: ila-vcd
