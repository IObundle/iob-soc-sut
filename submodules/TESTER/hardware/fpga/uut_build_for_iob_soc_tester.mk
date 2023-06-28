# Add iob-soc-sut software as a build dependency
RUN_DEPS+=iob_soc_sut_boot.hex iob_soc_sut_firmware.hex
# Don't add firmware to BUILD_DEPS if we are not initializing memory since we don't want to rebuild the bitstream when we modify it.
BUILD_DEPS+=iob_soc_sut_boot.hex $(if $(filter $(INIT_MEM),1),iob_soc_sut_firmware.hex)

TEST_LIST:=test-ila-vcd
test-ila-vcd:
	make -C ../../ fw-clean BOARD=$(BOARD)
	make -C ../../ fpga-clean BOARD=$(BOARD)
	make run BOARD=$(BOARD) && echo "Test passed!" > test.log
ifneq ($(BOARD_SERVER),)
	scp $(BOARD_USER)@$(BOARD_SERVER):$(REMOTE_FPGA_DIR)/ila_data.bin .
endif
	../../scripts/ilaDataToVCD.py ILA0 ila_data.bin ila_data.vcd
	gtkwave ila_data.vcd

.PHONY: test-ila-vcd
