# Tutorial: Add a new Device to be tested
This tutorial uses [iob-spi](https://github.com/iobundle/iob-spi) as an example
device to be added for testing.

1. Add `SPI` as a submodule:
```bash
git submodule add git@github.com:IObundle/iob-spi.git submodules/SPI
git submodule update --init --recursive
```

2. Add `SPI` to `iob_soc_tester.py`:
2.1. Import module:
```python
# Add to import list
from iob_spi_master import iob_spi_master
```
2.2. Add to submodule list:
```python
    def _create_submodules_list(cls):
        """Create submodules list with dependencies of this module"""
        submodules = [
            # other submodule here
            iob_spi_master,
            # other submodules here
        ]
```
2.3. Add to peripheral list:
```python
    def _create_instances(cls):
        # Instantiate TESTER peripherals
        # other peripherals here
        cls.peripherals.append(
            iob_spi_master("SPI1", "SPI interface for communication with Flash memory")
        )
```
2.4. Add to connections (optional)?
3. Update FPGA files (AES-KU040-DB-G only):
3.1. FPGA wrapper: `submodules/TESTER/hardware/fpga/vivado/AES-KU040-DB-G/iob_soc_tester_fpga_wrapper.v`
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
3.2. Add SPI1 FPGA constraints in
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

4. Update `submodules/TESTER/software/src/iob_soc_tester_firmware.c` to use SPI:
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

5. [Required for Linux] Update Linux Device Tree and OpenSBI:
```bash
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-dts MACROS_FILE=../../../TESTER/hardware/simulation/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/simulation/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-opensbi MACROS_FILE=../../../TESTER/hardware/simulation/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/simulation/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-dts MACROS_FILE=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-opensbi MACROS_FILE=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/vivado/AES-KU040-DB-G/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-dts MACROS_FILE=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/'
nix-shell submodules/OPENCRYPTOLINUX/submodules/OS/default.nix --run 'make -C submodules/OPENCRYPTOLINUX/submodules/OS build-opensbi MACROS_FILE=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/linux_build_macros.txt OS_BUILD_DIR=../../../TESTER/hardware/fpga/quartus/CYCLONEV-GT-DK/'
```
