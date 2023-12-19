# Add iob-soc-sut software as a build dependency
# It must be added at the start of the list, because the list already contains the tester target `init_ddr_contents.hex`, and that one must come after these.
HEX:=iob_soc_sut_boot.hex iob_soc_sut_firmware.hex $(HEX)

TEST_LIST:=test-ila-vcd
test-ila-vcd:
	make -C ../../ sim-run SIMULATOR=$(SIMULATOR) && echo "Test passed!" > test.log
ifneq ($(VSIM_SERVER)$(IVSIM_SERVER),)
	scp $(SIM_SCP_FLAGS) $(SIM_USER)@$(SIM_SERVER):$(REMOTE_SIM_DIR)/ila_data.bin .
endif
	../../scripts/ilaDataToVCD.py ILA0 ila_data.bin ila_data.vcd
	gtkwave ila_data.vcd

.PHONY: test-ila-vcd
