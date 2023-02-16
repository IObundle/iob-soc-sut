include submodules/LIB/setup.mk

INIT_MEM ?= 1

ifeq ($(INIT_MEM),1)
SETUP_ARGS += INIT_MEM
endif

ifeq ($(USE_EXTMEM),1)
SETUP_ARGS += USE_EXTMEM
endif

ifeq ($(TESTER),1)
SETUP_ARGS += TESTER
endif

ifeq ($(TESTER_ONLY),1)
SETUP_ARGS += TESTER_ONLY
endif

sim-test:
	make clean && make setup && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup INIT_MEM=0 && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup USE_EXTMEM=1 && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup INIT_MEM=0 USE_EXTMEM=1 && make -C ../iob_soc_sut_V*/ sim-test

tester-sim-test:
	make clean && make setup INIT_MEM=1 USE_EXTMEM=0 TESTER=1 && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=0 TESTER=1 && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	make clean && make setup INIT_MEM=1 USE_EXTMEM=1 TESTER=1 && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=1 TESTER=1 && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null

fpga-test:
	make clean && make setup && make -C ../iob_soc_sut_V*/ fpga-test
	make clean && make setup INIT_MEM=0 && make -C ../iob_soc_sut_V*/ fpga-test
	make clean && make setup  INIT_MEM=0 USE_EXTMEM=1 && make -C ../iob_soc_sut_V*/ fpga-test

tester-fpga-test:
	make clean && make setup INIT_MEM=1 USE_EXTMEM=0 TESTER=1 && make -C ../iob_soc_sut_V*/ fpga-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=0 TESTER=1 && make -C ../iob_soc_sut_V*/ fpga-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	#make clean && make setup INIT_MEM=0 USE_EXTMEM=1 TESTER=1 && make -C ../iob_soc_sut_V*/ fpga-run | tee /dev/tty | grep "Verification successful!" > /dev/null

test-all:
	make clean && make setup && make -C ../iob_soc_sut_V*/ pc-emul-test
	#make sim-test SIMULATOR=icarus
	make sim-test SIMULATOR=verilator
	make fpga-test BOARD=CYCLONEV-GT-DK
	make fpga-test BOARD=AES-KU040-DB-G
	make clean && make setup && make -C ../iob_soc_sut_V*/ doc-test

.PHONY: sim-test fpga-test test-all
.PHONY: tester-sim-test tester-fpga-test

build-sut-netlist:
	make clean && make setup 
	# Rename constraint files
	FPGA_DIR=`ls -d ../iob_soc_sut_V*/hardware/fpga/quartus/CYCLONEV-GT-DK` &&\
	mv $$FPGA_DIR/iob_soc_sut_fpga_wrapper.sdc $$FPGA_DIR/iob_soc_sut.sdc
	FPGA_DIR=`ls -d ../iob_soc_sut_V*/hardware/fpga/vivado/AES-KU040-DB-G` &&\
	mv $$FPGA_DIR/iob_soc_sut_fpga_wrapper.xdc $$FPGA_DIR/iob_soc_sut.xdc
	# Build netlist 
	make -C ../iob_soc_sut_V*/ fpga-build IS_FPGA=0 NETLIST_NAME="iob_soc_sut"

tester-sut-netlist: build-sut-netlist
	#Build tester without sut sources, but with netlist instead
	NETLIST_EXTENSION=`ls ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut.* | rev | cut -d. -f1 | rev` &&\
	TESTER_VER=`cat submodules/TESTER/iob_soc_tester_setup.py | grep version= | cut -d"'" -f2` &&\
	rm -fr ../iob_soc_tester_V* && make setup TESTER_ONLY=1 BUILD_DIR="../iob_soc_tester_$$TESTER_VER" &&\
	cp ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut.* ../iob_soc_tester_$$TESTER_VER/hardware/fpga/ &&\
	cp ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_firmware.* ../iob_soc_tester_$$TESTER_VER/hardware/fpga/ &&\
	if [ -f ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_stub.v ]; then cp ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_stub.v ../iob_soc_tester_$$TESTER_VER/hardware/src/; fi &&\
	echo -e "\nIP+=iob_soc_sut.$$NETLIST_EXTENSION" >> ../iob_soc_tester_$$TESTER_VER/hardware/fpga/fpga_build.mk &&\
	cp software/firmware/iob_soc_tester_firmware.c ../iob_soc_tester_$$TESTER_VER/software/firmware
	# Copy and modify iob_soc_sut_params.vh (needed for stub) and modify *_stub.v to insert the SUT parameters 
	TESTER_VER=`cat submodules/TESTER/iob_soc_tester_setup.py | grep version= | cut -d"'" -f2` &&\
	if [ -f ../iob_soc_sut_V*/hardware/fpga/iob_soc_sut_stub.v ]; then\
		cp ../iob_soc_sut_V0.70/hardware/src/iob_soc_sut_params.vh ../iob_soc_tester_$$TESTER_VER/hardware/src/;\
		sed -i -E 's/=[^,]*(,?)$$/=0\1/g' ../iob_soc_tester_$$TESTER_VER/hardware/src/iob_soc_sut_params.vh;\
		sed -i 's/_sut(/_sut#(\n`include "iob_soc_sut_params.vh"\n)(/g' ../iob_soc_tester_$$TESTER_VER/hardware/src/iob_soc_sut_stub.v;\
	fi
	# Run Tester on fpga
	make -C ../iob_soc_tester_V*/ fpga-run | tee /dev/tty | grep "Verification successful!" > /dev/null

.PHONY: build-sut-netlist test-sut-netlist
