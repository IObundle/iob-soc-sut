
#Lines below were auto generated by iob_soc_utils.py
UTARGETS+=iob_eth_rmac.h
EMUL_HDR+=iob_eth_rmac.h
iob_eth_rmac.h:
	echo "#define ETH_RMAC_ADDR 0x$(RMAC_ADDR)" > $@

CONSOLE_CMD ?=rm -f soc2cnsl cnsl2soc; $(IOB_CONSOLE_PYTHON_ENV) $(PYTHON_DIR)/console_ethernet.py -L -c $(PYTHON_DIR)/console.py -m "$(RMAC_ADDR)"


#########################################
#            Embedded targets           #
#########################################
ROOT_DIR ?=..
# Local embedded makefile settings for custom bootloader and firmware targets.

# Bootloader flow options:
# 1. CONSOLE_TO_EXTMEM: default: load firmware from console to external memory
# 2. CONSOLE_TO_FLASH: program flash with firmware
# 3. FLASH_TO_EXTMEM: load firmware from flash to external memory 
BOOT_FLOW ?= CONSOLE_TO_EXTMEM
UTARGETS += boot_flow

boot_flow:
	echo -n "$(BOOT_FLOW)" > boot.flow
	# -n to avoid newline

#Function to obtain parameter named $(1) in verilog header file located in $(2)
#Usage: $(call GET_MACRO,<param_name>,<vh_path>)
GET_MACRO = $(shell grep "define $(1)" $(2) | rev | cut -d" " -f1 | rev)

#Function to obtain parameter named $(1) from iob_soc_tester_conf.vh
GET_IOB_SOC_TESTER_CONF_MACRO = $(call GET_MACRO,IOB_SOC_TESTER_$(1),$(ROOT_DIR)/hardware/src/iob_soc_tester_conf.vh)

ifneq ($(shell grep -s "#define SIMULATION" src/bsp.h),)
SIMULATION=1
endif

iob_soc_tester_boot.hex: ../../software/iob_soc_tester_boot.bin
	../../scripts/makehex.py $< $(call GET_IOB_SOC_TESTER_CONF_MACRO,BOOTROM_ADDR_W) > $@
	../../scripts/hex_split.py iob_soc_tester_boot .

#OS
ifeq ($(RUN_LINUX),1)
OS_DIR = ../../software/src
OPENSBI_DIR = fw_jump.bin
DTB_DIR = iob_soc.dtb
DTB_ADDR:=00F80000
LINUX_DIR = Image
LINUX_ADDR:=00400000
ROOTFS_DIR = rootfs.cpio.gz
ROOTFS_ADDR:=01000000
FIRM_ARGS = $(OPENSBI_DIR)
FIRM_ARGS += $(DTB_DIR) $(DTB_ADDR)
FIRM_ARGS += $(LINUX_DIR) $(LINUX_ADDR)
FIRM_ARGS += $(ROOTFS_DIR) $(ROOTFS_ADDR)
FIRM_ADDR_W = $(call GET_IOB_SOC_TESTER_CONF_MACRO,OS_ADDR_W)
FIRMWARE := fw_jump.bin iob_soc.dtb Image rootfs.cpio.gz
else
FIRM_ARGS = $<
FIRM_ADDR_W = $(call GET_IOB_SOC_TESTER_CONF_MACRO,MEM_ADDR_W)
FIRMWARE := iob_soc_tester_firmware.bin
endif
iob_soc_tester_firmware.hex: $(FIRMWARE)
	../../scripts/makehex.py $(FIRM_ARGS) $(FIRM_ADDR_W) > $@
	../../scripts/hex_split.py iob_soc_tester_firmware .

iob_soc_tester_firmware.bin: ../../software/iob_soc_tester_firmware.bin
	cp $< $@

Image rootfs.cpio.gz:
	cp $(OS_DIR)/$@ .

fw_jump.bin iob_soc.dtb:
	if [ "$(FPGA_TOOL)" != "" ]; then\
		cp $(FPGA_TOOL)/$(BOARD)/$@ .;\
	fi
# Set targets as PHONY to ensure that they are copied even if $(BOARD) is changed
.PHONY: fw_jump.bin iob_soc.dtb boot_flow

../../software/%.bin:
	make -C ../../ fw-build


UTARGETS+=build_iob_soc_tester_software

TEMPLATE_LDS=src/$@.lds

# define simulator in uppercase
ifneq ($(SIMULATOR),)
SIM_DEFINE=-D$(shell echo $(SIMULATOR) | tr  '[:lower:]' '[:upper:]')
endif

IOB_SOC_TESTER_CFLAGS ?=-Os -nostdlib -march=rv32imac -mabi=ilp32 --specs=nano.specs -Wcast-align=strict $(SIM_DEFINE)

IOB_SOC_TESTER_INCLUDES=-I. -Isrc -Isrc/crypto/McEliece -Isrc/crypto/McEliece/common

IOB_SOC_TESTER_LFLAGS=-Wl,-Bstatic,-T,$(TEMPLATE_LDS),--strip-debug

# FIRMWARE SOURCES
IOB_SOC_TESTER_FW_SRC=src/iob_soc_tester_firmware.S
IOB_SOC_TESTER_FW_SRC+=src/iob_soc_tester_firmware.c
IOB_SOC_TESTER_FW_SRC+=src/printf.c

IOB_SOC_TESTER_FW_SRC+=src/crypto/McEliece/arena.c

# PERIPHERAL SOURCES
IOB_SOC_TESTER_FW_SRC+=$(wildcard src/iob-*.c)
IOB_SOC_TESTER_FW_SRC+=$(filter-out %_emul.c, $(wildcard src/*swreg*.c))

# BOOTLOADER SOURCES
IOB_SOC_TESTER_BOOT_SRC+=src/iob_soc_tester_boot.S
IOB_SOC_TESTER_BOOT_SRC+=src/iob_soc_tester_boot.c
IOB_SOC_TESTER_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*uart*.c))
IOB_SOC_TESTER_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*cache*.c))
IOB_SOC_TESTER_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*eth*.c))
IOB_SOC_TESTER_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*spi*.c))
IOB_SOC_TESTER_BOOT_SRC+=src/printf.c

build_iob_soc_tester_software: iob_soc_tester_firmware iob_soc_tester_boot

iob_soc_tester_firmware: check_if_run_linux
	make $@.elf INCLUDES="$(IOB_SOC_TESTER_INCLUDES)" LFLAGS="$(IOB_SOC_TESTER_LFLAGS) -Wl,-Map,$@.map" SRC="$(IOB_SOC_TESTER_FW_SRC)" TEMPLATE_LDS="$(TEMPLATE_LDS)" CFLAGS="$(IOB_SOC_TESTER_CFLAGS)"

check_if_run_linux:
	python3 $(ROOT_DIR)/scripts/check_if_run_linux.py $(ROOT_DIR) iob_soc_tester $(RUN_LINUX)

iob_soc_tester_boot:
	make $@.elf INCLUDES="$(IOB_SOC_TESTER_INCLUDES)" LFLAGS="$(IOB_SOC_TESTER_LFLAGS) -Wl,-Map,$@.map" SRC="$(IOB_SOC_TESTER_BOOT_SRC)" TEMPLATE_LDS="$(TEMPLATE_LDS)" CFLAGS="$(IOB_SOC_TESTER_CFLAGS)"


.PHONY: build_iob_soc_tester_software iob_soc_tester_firmware check_if_run_linux iob_soc_tester_boot

#########################################
#         PC emulation targets          #
#########################################
# Local pc-emul makefile settings for custom pc emulation targets.

# SOURCES
EMUL_SRC+=src/iob_soc_tester_firmware.c
EMUL_SRC+=src/printf.c

# PERIPHERAL SOURCES
EMUL_SRC+=$(wildcard src/iob-*.c)


#Auto-generated target to create init_ddr_contents.hex
HEX+=init_ddr_contents.hex
# init file for external mem with firmware of both systems
init_ddr_contents.hex: iob_soc_tester_firmware.hex
	../../scripts/hex_join.py $^ iob_soc_sut_firmware.hex 26 > $@
#Function to obtain parameter named $(1) from iob_soc_sut_conf.vh
GET_IOB_SOC_SUT_CONF_MACRO = $(call GET_MACRO,IOB_SOC_SUT_$(1),../src/iob_soc_sut_conf.vh)

iob_soc_sut_boot.hex: ../../software/iob_soc_sut_boot.bin
	../../scripts/makehex.py $< $(call GET_IOB_SOC_SUT_CONF_MACRO,BOOTROM_ADDR_W) > $@
	../../scripts/hex_split.py iob_soc_sut_boot .

iob_soc_sut_firmware.hex: iob_soc_sut_firmware.bin
	../../scripts/makehex.py $< $(call GET_IOB_SOC_SUT_CONF_MACRO,MEM_ADDR_W) > $@
	../../scripts/hex_split.py iob_soc_sut_firmware .

iob_soc_sut_firmware.bin: ../../software/iob_soc_sut_firmware.bin
	cp $< $@

UTARGETS+=build_iob_soc_sut_software

IOB_SOC_SUT_INCLUDES=-I. -Isrc -Isrc/crypto/McEliece -Isrc/crypto/McEliece/common

IOB_SOC_SUT_BOOT_LFLAGS=-Wl,-Bstatic,-T,src/iob_soc_sut_boot.lds,--strip-debug
IOB_SOC_SUT_FW_LFLAGS=-Wl,-Bstatic,-T,src/iob_soc_sut_firmware.lds,--strip-debug

# FIRMWARE SOURCES
IOB_SOC_SUT_FW_SRC=src/iob_soc_sut_firmware.S
IOB_SOC_SUT_FW_SRC+=src/iob_soc_sut_firmware.c
IOB_SOC_SUT_FW_SRC+=src/printf.c

IOB_SOC_SUT_FW_SRC+=src/versat_crypto.c
IOB_SOC_SUT_FW_SRC+=src/crypto/aes.c
IOB_SOC_SUT_FW_SRC+=src/versat_crypto_common_tests.c
IOB_SOC_SUT_FW_SRC+=src/versat_crypto_tests.c
IOB_SOC_SUT_FW_SRC+=src/versat_simple_crypto_tests.c
IOB_SOC_SUT_FW_SRC+=src/versat_mceliece.c
IOB_SOC_SUT_FW_SRC+=$(wildcard src/crypto/McEliece/*.c)
IOB_SOC_SUT_FW_SRC+=$(wildcard src/crypto/McEliece/common/*.c)

# PERIPHERAL SOURCES
IOB_SOC_SUT_FW_SRC+=$(wildcard src/iob-*.c)
IOB_SOC_SUT_FW_SRC+=$(filter-out %_emul.c, $(wildcard src/*swreg*.c))

# BOOTLOADER SOURCES
IOB_SOC_SUT_BOOT_SRC+=src/iob_soc_sut_boot.S
IOB_SOC_SUT_BOOT_SRC+=src/iob_soc_sut_boot.c
IOB_SOC_SUT_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*uart*.c))
IOB_SOC_SUT_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*cache*.c))
IOB_SOC_SUT_BOOT_SRC+=$(filter-out %_emul.c, $(wildcard src/iob*eth*.c))
IOB_SOC_SUT_BOOT_SRC+=src/printf.c

build_iob_soc_sut_software: check_if_run_linux_sut
	make iob_soc_sut_firmware.elf INCLUDES="$(IOB_SOC_SUT_INCLUDES)" LFLAGS="$(IOB_SOC_SUT_FW_LFLAGS) -Wl,-Map,iob_soc_sut_firmware.map" SRC="$(IOB_SOC_SUT_FW_SRC)" TEMPLATE_LDS="src/iob_soc_sut_firmware.lds"
	make iob_soc_sut_boot.elf INCLUDES="$(IOB_SOC_SUT_INCLUDES)" LFLAGS="$(IOB_SOC_SUT_BOOT_LFLAGS) -Wl,-Map,iob_soc_sut_boot.map" SRC="$(IOB_SOC_SUT_BOOT_SRC)" TEMPLATE_LDS="src/iob_soc_sut_boot.lds"

# Never run linux on SUT
check_if_run_linux_sut:
	python3 $(ROOT_DIR)/scripts/check_if_run_linux.py $(ROOT_DIR) iob_soc_sut 0

.PHONE: build_iob_soc_sut_software check_if_run_linux_sut

