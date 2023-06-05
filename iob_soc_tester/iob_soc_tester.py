#!/usr/bin/env python3
import os

from iob_soc import iob_soc
from iob_gpio import iob_gpio
from iob_uart import iob_uart
from iob_soc_sut import iob_soc_sut


class iob_soc_tester(iob_soc):
    name = "iob_soc_tester"
    version = "V0.70"
    flows = "pc-emul emb sim doc fpga"
    setup_dir = os.path.dirname(__file__)

    # Method that runs the setup process of this class
    @classmethod
    def _run_setup(cls):
        # Setup submodules
        iob_uart.setup()
        iob_soc_sut.setup()
        iob_gpio.setup()

        # Instantiate SUT peripherals
        cls.peripherals.append(iob_uart.instance("UART1", "UART interface for communication with SUT"))
        cls.peripherals.append(
                iob_soc_sut.instance("SUT0", "System Under Test (SUT) peripheral",
                                    parameters={
                                        "AXI_ID_W": "AXI_ID_W",
                                        "AXI_LEN_W": "AXI_LEN_W",
                                        "AXI_ADDR_W": "AXI_ADDR_W",
                                    }))

        cls.peripherals.append(iob_gpio.instance("GPIO0", "GPIO interface"))

        cls._setup_portmap()

        # Set name of sut firmware (used to join sut firmware with tester firmware)
        cls.sut_fw_name = "iob_soc_sut_firmware.c"

        # Run IOb-SoC setup
        super()._run_setup()

    @classmethod
    def _setup_portmap(cls):
        cls.peripheral_portmap += [
            (
                {"corename": "UART0", "if_name": "rs232", "port": "", "bits": []},
                {"corename": "external", "if_name": "UART", "port": "", "bits": []},
            ),  # Map UART0 of Tester to external interface
            # ================================================================== SUT IO mappings ==================================================================
            # SUT UART0
            # ({'corename':'SUT0', 'if_name':'UART0_rs232', 'port':'', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'', 'bits':[]}), #Map UART0 of SUT to UART1 of Tester
            # Python scripts do not yet support 'UART0_rs232'. Need to connect each signal independently
            (
                {"corename": "SUT0", "if_name": "UART0", "port": "rxd", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "txd", "bits": []},
            ),
            (
                {"corename": "SUT0", "if_name": "UART0", "port": "txd", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "rxd", "bits": []},
            ),
            (
                {"corename": "SUT0", "if_name": "UART0", "port": "cts", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "rts", "bits": []},
            ),
            (
                {"corename": "SUT0", "if_name": "UART0", "port": "rts", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "cts", "bits": []},
            ),
            # SUT ETHERNET0
            ####({'corename':'SUT0', 'if_name':'ETHERNET0_ethernet', 'port':'', 'bits':[]},         {'corename':'', 'if_name':'', 'port':'', 'bits':[]}), #Map ETHERNET0 of Tester to external interface
            ####({'corename':'SUT0', 'if_name':'ETHERNET1_ethernet', 'port':'', 'bits':[]},         {'corename':'ETHERNET0', 'if_name':'ethernet', 'port':'', 'bits':[]}), #Map ETHERNET0 of SUT to ETHERNET0 of Tester
            # SUT GPIO0
            (
                {"corename": "GPIO0", "if_name": "gpio", "port": "input_ports", "bits": []},
                {
                    "corename": "SUT0",
                    "if_name": "GPIO0",
                    "port": "output_ports",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "GPIO0",
                    "if_name": "gpio",
                    "port": "output_ports",
                    "bits": [],
                },
                {"corename": "SUT0", "if_name": "GPIO0", "port": "input_ports", "bits": []},
            ),
            (
                {
                    "corename": "GPIO0",
                    "if_name": "gpio",
                    "port": "output_enable",
                    "bits": [],
                },
                {"corename": "external", "if_name": "GPIO", "port": "", "bits": []},
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "GPIO0",
                    "port": "output_enable",
                    "bits": [],
                },
                {"corename": "external", "if_name": "SUT_GPIO", "port": "", "bits": []},
            ),
        ]

    @classmethod
    def _setup_confs(cls):
        # Append confs or override them if they exist
        super()._setup_confs(
            [
                # {'name':'BOOTROM_ADDR_W','type':'P', 'val':'13', 'min':'1', 'max':'32', 'descr':"Boot ROM address width"},
                {
                    "name": "SRAM_ADDR_W",
                    "type": "P",
                    "val": "17",
                    "min": "1",
                    "max": "32",
                    "descr": "SRAM address width",
                },
            ]
        )




## Function to rename the sim_build.mk and fpga_build.mk files of thee SUT to files named `uut_build_for_iob_soc_tester.mk`
## The tester has its own sim_build.mk and fpga_build.mk files, so we need to rename the SUT ones to prevent overwriting
#def create_sut_uut_build():
#    os.rename(
#        os.path.join(build_dir, "hardware/simulation/sim_build.mk"),
#        os.path.join(build_dir, "hardware/simulation/uut_build_for_iob_soc_tester.mk"),
#    )
#    os.rename(
#        os.path.join(build_dir, "hardware/fpga/fpga_build.mk"),
#        os.path.join(build_dir, "hardware/fpga/uut_build_for_iob_soc_tester.mk"),
#    )
#
#
#def custom_setup():
#    # Add the following arguments:
#    # "TESTER_ONLY": setup tester without the SUT (as SUT will be a manually added netlist)
#    if "TESTER_ONLY" not in sys.argv[1:]:
#        # Add SUT module as dependency of the Tester's iob-soc
#        iob_soc_options["submodules"]["hw_setup"]["modules"].insert(0, "SUT")
#        iob_soc_options["submodules"]["hw_setup"]["modules"].insert(
#            1, create_sut_uut_build
#        )
