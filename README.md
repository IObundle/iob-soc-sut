# IOb-SoC-SUT

IOb-SoC-SUT is a generic RISC-V SoC, based on [IOb-SoC](https://github.com/IObundle/iob-soc), used to verify the [IOb-SoC-Tester](https://github.com/IObundle/iob-soc-tester). 
This repository is a System Under Test (SUT) example, demonstrating the Tester's abilities for verification purposes.

This system runs on bare metal and has UART and IOb-native interfaces.

## Build and run the SUT

This system's build and run steps are similar to the ones used in [IOb-SoC](https://github.com/IObundle/iob-soc).

The SUT's main configuration, stored in `config.mk`, sets the UART and REGFILEIF peripherals. In total, the SUT has one UART and one IOb-native (provided by the REGFILEIF peripheral) interface.

The SUT's firmware, stored in `software/firmware/firmware.c` has three modes of operation:
- Without DDR memory (USE\_DDR=0, RUN\_EXTMEM=0)
- With DDR and running from internal memory (USE\_DDR=1, RUN\_EXTMEM=0)
- Running from DDR memory (USE\_DDR=1, RUN\_EXTMEM=1)

When running without DDR, the SUT only prints a few `Hello Word!` messages via UART and inserts values into the registers of its IOb-native interface.

When running from the internal memory but with DDR available, the system does the same as without DDR but also writes a string to the DDR memory starting at address zero.

When running from the DDR memory, the system does the same as without DDR. It also allocates and stores a string in memory and writes its pointer to a register in the IOb-native interface.

### Emulate the SUT on the PC 

To emulate the SUT's embedded software on a PC, type:
```
make pc-emul [<control parameters>]
```

`<control parameters>` are system configuration parameters passed in the
command line, overriding those in the `config.mk` file. Example control
parameters are `USE_DDR=1 RUN_EXTMEM=0`. For example,

### Simulate the SUT

To build and run the SUT in simulation, type:
```
make sim-run [SIMULATOR=<simulator directory name>] [<control parameters>]
```

`<simulator directory name>` is the name of the simulator's run directory,

### Build and run the SUT on the FPGA board

To build the SUT for FPGA, type:
``` 
make fpga-build [BOARD=<board directory name>] [<control parameters>]
``` 

`<board directory name>` is the name of the board's run directory.

To run the SUT in FPGA, type:
``` 
make fpga-run [BOARD=<board directory name>] [<control parameters>]
``` 

## Build and run the Tester (along with SUT)

The Tester's main configuration, stored in `tester.mk`, adds the IOBNATIVEBRIDGEIF and another UART instance to the default Tester peripherals. In total, the Tester has two UART interfaces, one IOb-native (provided by IOBNATIVEBRIDGEIF).

The SUT and Tester's peripheral IO connections, stored in `peripheral_portmap.conf`, have the following configuration:
- Instance 0 of Tester's UART is connected to the PC's console.
- Instance 1 of Tester's UART is connected to the SUT's UART. 
- Tester's IOBNATIVEBRIDGEIF is connected to SUT's REGFILEIF. These are the IOb-native interfaces of both systems.

The Tester's firmware, stored in `software/tester_firmware.c`, also has three modes of operation:
- Without DDR memory (USE\_DDR=0, RUN\_EXTMEM=0)
- With DDR and running from internal memory (USE\_DDR=1, RUN\_EXTMEM=0)
- Running from DDR memory (USE\_DDR=1, RUN\_EXTMEM=1)

When running without DDR, the Tester only relays messages printed from the SUT to the console and reads values from the IOb-native interface connected to the SUT.

When running from the internal memory but with DDR available, the Tester does the same as without DDR. Also, it reads a string to the DDR memory starting at address zero.

When running from the DDR memory, the Tester does the same as without DDR. It also reads a string pointer from the IOb-native interface. It inverts the most significant bit of that pointer to access the SUT's address space and then reads the string stored at that location.

More details on configuring, building and running the Tester are available in the [IOb-SoC-Tester](https://github.com/IObundle/iob-soc-tester) repository.

### Simulate the Tester (along with SUT)

To build and run in simulation, type:
```
make tester-sim-run [SIMULATOR=<simulator directory name>] [<control parameters>]
```

`<control parameters>` are system configuration parameters passed in the
command line, overriding those in the `config.mk` file. Example control
parameters are `USE_DDR=1 RUN_EXTMEM=0`. For example,

`<simulator directory name>` is the name of the simulator's run directory,

### Build and run the Tester (along with SUT) on the FPGA board

To build for FPGA, type:
``` 
make tester-fpga-build [BOARD=<board directory name>] [<control parameters>]
``` 

`<board directory name>` is the name of the board's run directory.

To run in FPGA, type:
``` 
make tester-fpga-run [BOARD=<board directory name>] [<control parameters>]
``` 

## Clean

The following command will clean the selected simulation, board, document, and Tester directories, locally and in the remote servers:
```
make clean
```
