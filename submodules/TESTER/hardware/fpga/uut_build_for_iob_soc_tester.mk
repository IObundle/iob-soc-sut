# Add iob-soc-sut software as a build dependency
RUN_DEPS+=iob_soc_sut_boot.hex iob_soc_sut_firmware.hex
# Don't add firmware to BUILD_DEPS if we are not initializing memory since we don't want to rebuild the bitstream when we modify it.
BUILD_DEPS+=iob_soc_sut_boot.hex $(if $(filter $(INIT_MEM),1),iob_soc_sut_firmware.hex)
