#!/usr/bin/env python3
import os
import copy

from iob_soc import iob_soc
from iob_regfileif import iob_regfileif
from iob_gpio import iob_gpio
from iob_axistream_in import iob_axistream_in
from iob_axistream_out import iob_axistream_out
from iob_eth import iob_eth

sut_regs = [
    {
        "name": "regfileif",
        "descr": "REGFILEIF software accessible registers.",
        "regs": [
            {
                "name": "REG1",
                "type": "W",
                "n_bits": 8,
                "rst_val": 0,
                "addr": -1,
                "log2n_items": 0,
                "autologic": True,
                "descr": "Write register: 8 bit",
            },
            {
                "name": "REG2",
                "type": "W",
                "n_bits": 16,
                "rst_val": 0,
                "addr": -1,
                "log2n_items": 0,
                "autologic": True,
                "descr": "Write register: 16 bit",
            },
            {
                "name": "REG3",
                "type": "R",
                "n_bits": 8,
                "rst_val": 0,
                "addr": -1,
                "log2n_items": 0,
                "autologic": True,
                "descr": "Read register: 8 bit",
            },
            {
                "name": "REG4",
                "type": "R",
                "n_bits": 16,
                "rst_val": 0,
                "addr": -1,
                "log2n_items": 0,
                "autologic": True,
                "descr": "Read register 16 bit",
            },
            {
                "name": "REG5",
                "type": "R",
                "n_bits": 32,
                "rst_val": 0,
                "addr": -1,
                "log2n_items": 0,
                "autologic": True,
                "descr": "Read register 32 bit. In this example, we use this to pass the sutMemoryMessage address.",
            },
        ],
    }
]


class iob_soc_sut(iob_soc):
    name = "iob_soc_sut"
    version = "V0.70"
    flows = "pc-emul emb sim doc fpga"
    setup_dir = os.path.dirname(__file__)

    # Method that runs the setup process of this class
    @classmethod
    def _run_setup(cls):
        # Setup submodules
        iob_regfileif_custom.setup()
        iob_gpio.setup()
        iob_axistream_in.setup()
        iob_axistream_out.setup()
        # iob_eth.setup()

        # Instantiate SUT peripherals
        cls.peripherals.append(
            iob_regfileif_custom.instance("REGFILEIF0", "Register file interface")
        )
        cls.peripherals.append(iob_gpio.instance("GPIO0", "GPIO interface"))
        cls.peripherals.append(
            iob_axistream_in.instance(
                "AXISTREAMIN0",
                "SUT AXI input stream interface",
                parameters={"TDATA_W": "32"},
            )
        )
        cls.peripherals.append(
            iob_axistream_out.instance(
                "AXISTREAMOUT0",
                "SUT AXI output stream interface",
                parameters={"TDATA_W": "32"},
            )
        )
        # cls.peripherals.append(iob_eth.instance("ETH0", "Ethernet interface"))

        cls.peripheral_portmap += [
            (  # Map REGFILEIF0 to external interface
                {
                    "corename": "REGFILEIF0",
                    "if_name": "external_iob_s_port",
                    "port": "",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "REGFILEIF0",
                    "port": "",
                    "bits": [],
                    "ios_table_prefix": False,  # Don't add interface table prefix (REGFILEIF0) to the signal names
                    "remove_string_from_port_names": "external_",  # Remove this string from the port names of the external IO
                },
            ),
        ]

        # Run IOb-SoC setup
        super()._run_setup()

        # Remove iob_soc_sut_swreg_gen.v as it is not used
        os.remove(os.path.join(cls.build_dir, "hardware/src/iob_soc_sut_swreg_gen.v"))

    # Public method to set dynamic attributes
    # This method is automatically called by the `setup` method
    @classmethod
    def set_dynamic_attributes(cls):
        super().set_dynamic_attributes()
        cls.regs = sut_regs

    @classmethod
    def _setup_confs(cls):
        # Append confs or override them if they exist
        super()._setup_confs(
            [
                # {'name':'BOOTROM_ADDR_W','type':'P', 'val':'13', 'min':'1', 'max':'32', 'descr':"Boot ROM address width"},
                {
                    "name": "SRAM_ADDR_W",
                    "type": "P",
                    "val": "16",
                    "min": "1",
                    "max": "32",
                    "descr": "SRAM address width",
                },
            ]
        )


# Custom iob_regfileif subclass for use in SUT system
class iob_regfileif_custom(iob_regfileif):
    # Public method to set dynamic attributes
    # This method is automatically called by the `setup` method
    @classmethod
    def set_dynamic_attributes(cls):
        super().set_dynamic_attributes()
        cls.regs = copy.deepcopy(sut_regs)
