# Tutorial: Add a new Device to be tested
This tutorial uses [iob-spi](https://github.com/iobundle/iob-spi) as an example
device to be added for testing.

1. Add peripheral as a submodule:
```bash
git submodule add git@github.com:IObundle/iob-spi.git submodules/SPI
git submodule update --init --recursive
```


2. Add peripheral to `iob_soc_tester.py`:
2.1. Import module:
```python
from iob_spi_master import iob_spi_master
```

2.2. Add to submodule list:
```python
    def _create_submodules_list(cls):
        submodules = [
            iob_spi_master,
            # other submodules here
        ]
```

2.3. Add to peripheral list:
```python
    
    def _create_instances(cls):
        cls.peripherals.append(
            iob_spi_master("SPI1", "SPI interface for communication with Flash memory")
        )
        # other peripherals here
```

2.4. Add to connections (optional):
The connections will vary according to the type of peripheral added.
Some peripherals may not have configurable connections.
In this example, the SPI peripheral connects to the external flash memory interface.
```python
    def _setup_portmap(cls):
        super()._setup_portmap()
        cls.peripheral_portmap += [
            (
                {
                    "corename": "SPI1",
                    "if_name": "flash_if",
                    "port": "SS",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "flash_if",
                    "port": "SCLK",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "flash_if",
                    "port": "MISO",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "flash_if",
                    "port": "MOSI",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "flash_if",
                    "port": "WP_N",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "flash_if",
                    "port": "HOLD_N",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "avalid_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "address_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "wdata_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "wstrb_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "rdata_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "rvalid_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SPI1",
                    "if_name": "iob_s_cache",
                    "port": "ready_cache",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "SPI1",
                    "port": "",
                    "bits": [],
                },
            ),
            # other peripheral ports here
        ]
```


3. Update FPGA files (optional):
System changes for each FPGA/board may be required if the peripheral uses FPGA/board specific components.
In this example, the SPI peripheral connects to the 'AES-KU040-DB-G' FPGA board's flash memory.

3.1. Update FPGA wrapper:
`submodules/TESTER/hardware/fpga/vivado/AES-KU040-DB-G/iob_soc_tester_fpga_wrapper.v`
```verilog
// add SPI1 to top level ports

module iob_soc_tester_fpga_wrapper (

   // other ports
   // ...

   //spi1
   output SPI1_SS_o,
   output SPI1_SCLK_o,
   inout SPI1_MISO_io,
   inout SPI1_MOSI_io,
   inout SPI1_WP_N_io,
   inout SPI1_HOLD_N_io,

   // ... other ports

)

   // ...

   iob_soc_tester #(
      .AXI_ID_W  (AXI_ID_W),
      .AXI_LEN_W (AXI_LEN_W),
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W)
   ) iob_soc_tester0 (
               // ... instance ports
               // add SPI1 portmap
               .SPI1_SS(SPI1_SS_o),
               .SPI1_SCLK(SPI1_SCLK_o),
               .SPI1_MISO(SPI1_MISO_io),
               .SPI1_MOSI(SPI1_MOSI_io),
               .SPI1_WP_N(SPI1_WP_N_io),
               .SPI1_HOLD_N(SPI1_HOLD_N_io),
               .SPI1_avalid_cache(1'b0),
               .SPI1_address_cache(1'b0),
               .SPI1_wdata_cache(1'b0),
               .SPI1_wstrb_cache(1'b0),
               .SPI1_rdata_cache(),
               .SPI1_rvalid_cache(),
               .SPI1_ready_cache(),
               // ... other iob_soc_tester0 instance ports


```

3.2. Update FPGA constraints:
`submodules/TESTER/hardware/fpga/vivado/AES-KU040-DB-G/iob_soc_tester_fpga_wrapper_dev.sdc`
```
#
# SPI1 constraints
#

# CS
set_property PACKAGE_PIN D19 [get_ports SPI1_SS_o]
set_property IOSTANDARD LVCMOS18 [get_ports SPI1_SS_o]
# CLK
set_property PACKAGE_PIN F10 [get_ports SPI1_SCLK_o]
set_property IOSTANDARD LVCMOS18 [get_ports SPI1_SCLK_o]
# DQ0
set_property PACKAGE_PIN G11 [get_ports SPI1_MOSI_io]
set_property IOSTANDARD LVCMOS18 [get_ports SPI1_MOSI_io]
# DQ1
set_property PACKAGE_PIN H11 [get_ports SPI1_MISO_io]
set_property IOSTANDARD LVCMOS18 [get_ports SPI1_MISO_io]
# DQ2
set_property PACKAGE_PIN J11 [get_ports SPI1_WP_N_io]
set_property IOSTANDARD LVCMOS18 [get_ports SPI1_WP_N_io]
# DQ3
set_property PACKAGE_PIN H12 [get_ports SPI1_HOLD_N_io]
set_property IOSTANDARD LVCMOS18 [get_ports SPI1_HOLD_N_io]

set_property IOB TRUE [get_ports SPI1_SS_o]
set_property IOB TRUE [get_ports SPI1_SCLK_o]
set_property IOB TRUE [get_cells iob_soc_tester0/SPI1/fl_spi0/dq_out_r_reg[0]]
set_property IOB TRUE [get_cells iob_soc_tester0/SPI1/fl_spi0/dq_out_r_reg[1]]
set_property IOB TRUE [get_cells iob_soc_tester0/SPI1/fl_spi0/dq_out_r_reg[2]]
set_property IOB TRUE [get_cells iob_soc_tester0/SPI1/fl_spi0/dq_out_r_reg[3]]
```


4. Update baremetal firmware:
`submodules/TESTER/software/src/iob_soc_tester_firmware.c`
```C
// Add includes
#include "iob-spi.h"
#include "iob-spiplatform.h"
// ...

/* Test SPI:
 * - arguments:
 *   - spi_base: base address of SPI peripheral
 *   - flash_addr: base address of flash memory
 *   - test_size: number of bytes to write and read
 * - returns:
 *   - 0: test passed
 *   - 1: otherwise
 */
int test_spi(int spi_base, unsigned int flash_addr, int test_size){
    int test_failed = 0;
    int i = 0, j = 0;
    char data_buf[4] = {0};
    unsigned int read_data = 0;

    spiflash_init(spi_base);

    // erase flash region
    spiflash_erase_address_range(flash_addr, test_size);

    // write data to flash
    for (i = 0; i < test_size; i = i+4){
        // set data buffer
        for (j = 0; j < 4; j++){
            data_buf[j] = (char) (i+j & 0xFF);
        }

        if (test_size > i+4){
            spiflash_memProgram(data_buf, 4, flash_addr + i);
        } else{
            // if test_size is not multiple of 4, write remainder bytes
            spiflash_memProgram(data_buf, test_size-i, flash_addr + i);
        }
    }

    // check data
    for (i = 0; i < test_size; i = i+4 ){
        read_data = spiflash_readmem(flash_addr + i);
        for (j = 0; j < 4; j++){
            if ((char)(i+j & 0xFF) != (read_data >> (j*8) & 0xFF)){
            test_failed = 1;
            break;
            }
        }
    }
    return test_failed;
}

// ...
int main(){
    // ...
#ifdef XILINX
  // SPI test
  // 0x01FF F000: last subsector of flash memory
  if (test_spi(SPI1_BASE, 0x01FFF000, 64) == 0){
    uart16550_puts("[Tester]: SPI test passed!\n\n");
  } else {
    uart16550_puts("[Tester]: SPI test failed!\n\n");
  }
#endif // ifdef XILINX
}
```


5. Update Linux files:
See [SPI device driver tutorial](https://github.com/IObundle/iob-soc-opencryptolinux/blob/master/document/device_driver_tutorial.md) for more details.

5.1 Update Linux Device Tree:
`submodules/TESTER/software/iob_soc.dts`
```dts
/dts-v1/;
/ {
    // other properties here
    soc {
        SPI1: spi@/*SPI1_ADDR_MACRO*/ {
            compatible = "iobundle,spi0";
            reg = <0x/*SPI1_ADDR_MACRO*/ 0x100>;
        };
        // other peripherals here
    };
};
```

5.2. Build Linux Device Tree and OpenSBI:
```bash
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-dts MACROS_FILE=../../../TESTER/hardware/simulation/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/simulation/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-opensbi MACROS_FILE=../../../TESTER/hardware/simulation/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/simulation/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-dts MACROS_FILE=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-opensbi MACROS_FILE=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-dts MACROS_FILE=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-opensbi MACROS_FILE=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/'
```

5.3. Update Linux software:
Add new test software inside the following folder, and compile it if needed:
`submodules/TESTER/software/buildroot/board/IObundle/iob-soc/rootfs-overlay/root/tester_verification/`
See the [SPI python example](https://github.com/IObundle/iob-soc-sut/blob/main/submodules/TESTER/software/buildroot/board/IObundle/iob-soc/rootfs-overlay/root/tester_verification/micropython_spi_test.py).


5.4. Load Linux kernel module and launch test during boot (optional):
`submodules/TESTER/software/buildroot/board/IObundle/iob-soc/rootfs-overlay/etc/init.d/S99IObundleVerification`
```bash
insmod /drivers/iob_spi_master.ko
# other modules here

/root/tester_verification/micropython_spi_test.py
# other software tests here
```

5.5. Build Buildroot:
Add peripheral to the `MODULE_NAMES` list of the [Makefile](https://github.com/IObundle/iob-soc-sut/blob/48c0f15f5f956102b538097e51f26cebc11219de/Makefile#L141).
```Make
MODULE_NAMES += iob_spi_master
```
Call the [build-linux-buildroot](https://github.com/IObundle/iob-soc-sut/blob/48c0f15f5f956102b538097e51f26cebc11219de/Makefile#L175) makefile target to include previously created files in the Linux root filesystem.
```bash
make build-linux-buildroot
```
