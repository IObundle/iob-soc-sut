# Add iob-soc-sut software as a build dependency
# It must be added at the start of the list, because the list already contains the tester target `init_ddr_contents.hex`, and that one must come after these.
HEX:=iob_soc_sut_boot.hex iob_soc_sut_firmware.hex $(HEX)

