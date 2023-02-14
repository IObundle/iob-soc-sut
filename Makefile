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
