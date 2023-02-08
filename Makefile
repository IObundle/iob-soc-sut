include submodules/LIB/setup.mk

sim-test:
	make clean && make setup SETUP_ARGS="INIT_MEM=1 RUN_EXTMEM=0" && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup SETUP_ARGS="INIT_MEM=0 RUN_EXTMEM=0" && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup SETUP_ARGS="INIT_MEM=1 RUN_EXTMEM=1" && make -C ../iob_soc_sut_V*/ sim-test
	make clean && make setup SETUP_ARGS="INIT_MEM=0 RUN_EXTMEM=1" && make -C ../iob_soc_sut_V*/ sim-test

tester-sim-test:
	make clean && make setup SETUP_ARGS="INIT_MEM=1 RUN_EXTMEM=0 TESTER=1" && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	#make clean && make setup SETUP_ARGS="INIT_MEM=0 RUN_EXTMEM=0 TESTER=1" && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	make clean && make setup SETUP_ARGS="INIT_MEM=1 RUN_EXTMEM=1 TESTER=1" && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null
	#make clean && make setup SETUP_ARGS="INIT_MEM=0 RUN_EXTMEM=1 TESTER=1" && make -C ../iob_soc_sut_V*/ sim-run | tee /dev/tty | grep "Verification successful!" > /dev/null

fpga-test:
	make clean && make setup SETUP_ARGS="INIT_MEM=1 RUN_EXTMEM=0" && make -C ../iob_soc_sut_V*/ fpga-test
	make clean && make setup SETUP_ARGS="INIT_MEM=0 RUN_EXTMEM=0" && make -C ../iob_soc_sut_V*/ fpga-test
	make clean && make setup SETUP_ARGS="INIT_MEM=1 RUN_EXTMEM=1" && make -C ../iob_soc_sut_V*/ fpga-test
	make clean && make setup SETUP_ARGS="INIT_MEM=0 RUN_EXTMEM=1" && make -C ../iob_soc_sut_V*/ fpga-test

test-all:
	make clean && make setup && make -C ../iob_soc_sut_V*/ pc-emul-test
	#make sim-test SIMULATOR=icarus
	make sim-test SIMULATOR=verilator
	make fpga-test BOARD=CYCLONEV-GT-DK
	make fpga-test BOARD=AES-KU040-DB-G
	make clean && make setup && make -C ../iob_soc_sut_V*/ doc-test
