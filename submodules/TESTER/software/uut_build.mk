#Function to obtain parameter named $(1) from iob_soc_sut_conf.vh
GET_IOB_SOC_SUT_CONF_MACRO = $(call GET_MACRO,IOB_SOC_SUT_$(1),../src/iob_soc_sut_conf.vh)

iob_soc_sut_boot.hex: ../../software/iob_soc_sut_boot.bin
	../../scripts/makehex.py $< $(call GET_IOB_SOC_SUT_CONF_MACRO,BOOTROM_ADDR_W) > $@

iob_soc_sut_firmware.hex: iob_soc_sut_firmware.bin
ifeq ($(USE_EXTMEM),1)
	../../scripts/makehex.py $< $(call GET_IOB_SOC_SUT_CONF_MACRO,MEM_ADDR_W) > $@
else
	../../scripts/makehex.py $< $(call GET_IOB_SOC_SUT_CONF_MACRO,SRAM_ADDR_W) > $@
endif
	../../scripts/hex_split.py iob_soc_sut_firmware .

iob_soc_sut_firmware.bin: ../../software/iob_soc_sut_firmware.bin
	cp $< $@

UTARGETS+=build_iob_soc_sut_software

IOB_SOC_SUT_INCLUDES=-I. -Isrc 

IOB_SOC_SUT_BOOT_LFLAGS=-Wl,-Bstatic,-T,src/iob_soc_sut_boot.lds,--strip-debug
IOB_SOC_SUT_FW_LFLAGS=-Wl,-Bstatic,-T,src/iob_soc_sut_firmware.lds,--strip-debug

# FIRMWARE SOURCES
IOB_SOC_SUT_FW_SRC=src/iob_soc_sut_firmware.S
IOB_SOC_SUT_FW_SRC+=src/iob_soc_sut_firmware.c
IOB_SOC_SUT_FW_SRC+=src/printf.c
# PERIPHERAL SOURCES
IOB_SOC_SUT_FW_SRC+=$(wildcard src/iob-*.c)
IOB_SOC_SUT_FW_SRC+=$(filter-out %_emul.c, $(wildcard src/*swreg*.c))

# BOOTLOADER SOURCES
IOB_SOC_SUT_BOOT_SRC+=src/iob_soc_sut_boot.S
IOB_SOC_SUT_BOOT_SRC+=src/iob_soc_sut_boot.c
IOB_SOC_SUT_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*uart*.c))
IOB_SOC_SUT_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*cache*.c))

build_iob_soc_sut_software:
	make iob_soc_sut_firmware.elf INCLUDES="$(IOB_SOC_SUT_INCLUDES)" LFLAGS="$(IOB_SOC_SUT_FW_LFLAGS) -Wl,-Map,iob_soc_sut_firmware.map" SRC="$(IOB_SOC_SUT_FW_SRC)" TEMPLATE_LDS="src/iob_soc_sut_firmware.lds"
	make iob_soc_sut_boot.elf INCLUDES="$(IOB_SOC_SUT_INCLUDES)" LFLAGS="$(IOB_SOC_SUT_BOOT_LFLAGS) -Wl,-Map,iob_soc_sut_boot.map" SRC="$(IOB_SOC_SUT_BOOT_SRC)" TEMPLATE_LDS="src/iob_soc_sut_boot.lds"


.PHONE: build_iob_soc_sut_software
