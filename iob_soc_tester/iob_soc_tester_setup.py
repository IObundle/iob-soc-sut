#!/usr/bin/env python3

import os, sys

sys.path.insert(0, os.getcwd() + "/submodules/LIB/scripts")

import setup

name = "iob_soc_sut"
version = "V0.70"
flows = "pc-emul emb sim doc fpga"
if setup.is_top_module(sys.modules[__name__]):
    setup_dir = os.path.dirname(__file__)
    build_dir = f"../../{name}_{version}"

# ######### Register file peripheral configuration ##############
regfileif_options = {
    "regs": [
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
}
# ############### End of REGFILEIF configuration ################

# ################## Tester configuration to generate system via IOb-SoC #######################
iob_soc_options = {
    # NOTE: Ethernet peripheral is disabled for the time being as it has not yet been updated for compatibility with the python-setup branch.
    "peripherals": [
        # {'name':'UART0', 'type':'UART', 'descr':'Default UART interface', 'params':{}}, # It is possible to override default tester peripherals with new parameters
        {
            "name": "UART1",
            "type": "UART",
            "descr": "UART interface for communication with SUT",
            "params": {},
        },
        ##{'name':'ETHERNET0', 'type':'ETHERNET', 'descr':'Ethernet interface for communication with the console', 'params':{}},
        ##{'name':'ETHERNET0', 'type':'ETHERNET', 'descr':'Ethernet interface for communication with the SUT', 'params':{}},
        {
            "name": "SUT0",
            "type": "SUT",
            "descr": "System Under Test (SUT) peripheral",
            "params": {
                "AXI_ID_W": "AXI_ID_W",
                "AXI_LEN_W": "AXI_LEN_W",
                "AXI_ADDR_W": "AXI_ADDR_W",
            },
        },
        {
            "name": "IOBNATIVEBRIDGEIF0",
            "type": "IOBNATIVEBRIDGEIF",
            "descr": "IOb native interface for communication with SUT. Essentially a REGFILEIF without any registers.",
            "params": {},
        },
        {"name": "GPIO0", "type": "GPIO", "descr": "GPIO interface", "params": {}},
    ],
    "peripherals_dirs": {
        "SUT": f"{setup_dir}/submodules/SUT",
        #"ETHERNET": f"{setup_dir}/submodules/ETHERNET",
        "REGFILEIF": f"{setup_dir}/submodules/REGFILEIF",
        "IOBNATIVEBRIDGEIF": f"{setup_dir}/submodules/REGFILEIF/nativebridgeif_wrappper",
        "GPIO": f"{setup_dir}/submodules/GPIO",
    },
    "peripheral_portmap": [
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
        # SUT REGFILEIF0
        # ({'corename':'SUT0', 'if_name':'REGFILEIF0_external_iob_s_port', 'port':'', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'', 'bits':[]}), #Map REGFILEIF of SUT to IOBNATIVEBRIDGEIF of Tester
        # Python scripts do not yet support 'REGFILEIF0_external_iob_s_port'. Need to connect each signal independently
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_avalid_i",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_avalid_o",
                "bits": [],
            },
        ),
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_addr_i",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_addr_o",
                "bits": [],
            },
        ),
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_wdata_i",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_wdata_o",
                "bits": [],
            },
        ),
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_wstrb_i",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_wstrb_o",
                "bits": [],
            },
        ),
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_rvalid_o",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_rvalid_i",
                "bits": [],
            },
        ),
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_rdata_o",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_rdata_i",
                "bits": [],
            },
        ),
        (
            {
                "corename": "SUT0",
                "if_name": "REGFILEIF0",
                "port": "external_iob_ready_o",
                "bits": [],
            },
            {
                "corename": "IOBNATIVEBRIDGEIF0",
                "if_name": "iob_m_port",
                "port": "iob_ready_i",
                "bits": [],
            },
        ),
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
    ],
    "confs": [
        # Override default values of Tester params
        # {'name':'BOOTROM_ADDR_W','type':'P', 'val':'13', 'min':'1', 'max':'32', 'descr':"Boot ROM address width"},
        {
            "name": "SRAM_ADDR_W",
            "type": "P",
            "val": "17",
            "min": "1",
            "max": "32",
            "descr": "SRAM address width",
        },
    ],
    "sut_fw_name": "iob_soc_sut_firmware",
    # Note: Even though REGFILEIF is a module of the SUT, some os its files are needed for the IOBNATIVEBRIDGEIF, and they wont be generated by the SUT if we use the TESTER_ONLY argument, therefore we add the REGFILEIF configuration here aswell for it to be always generated with Tester
    "submodules": {
        "hw_setup": {
            "headers": [],
            "modules": [("REGFILEIF", regfileif_options), "IOBNATIVEBRIDGEIF"],
        },
    },
}

# ############### End of Tester configuration ###################

submodules = {
    "hw_setup": {
        "headers": [],
        "modules": [("IOBSOC", iob_soc_options)],
    },
}

blocks = []

confs = []

regs = []

ios = []

# Function to rename the sim_build.mk and fpga_build.mk files of thee SUT to files named `uut_build_for_iob_soc_tester.mk`
# The tester has its own sim_build.mk and fpga_build.mk files, so we need to rename the SUT ones to prevent overwriting
def create_sut_uut_build():
    os.rename(os.path.join(build_dir, "hardware/simulation/sim_build.mk"), os.path.join(build_dir, "hardware/simulation/uut_build_for_iob_soc_tester.mk"))
    os.rename(os.path.join(build_dir, "hardware/fpga/fpga_build.mk"), os.path.join(build_dir, "hardware/fpga/uut_build_for_iob_soc_tester.mk"))


def custom_setup():
    # Add the following arguments:
    # "TESTER_ONLY": setup tester without the SUT (as SUT will be a manually added netlist)
    if "TESTER_ONLY" not in sys.argv[1:]:
        # Add SUT module as dependency of the Tester's iob-soc
        iob_soc_options["submodules"]["hw_setup"]["modules"].insert(0, "SUT")
        iob_soc_options["submodules"]["hw_setup"]["modules"].insert(1, create_sut_uut_build)


# Main function to setup this system and its components
def main():
    custom_setup()
    # Setup this system
    setup.setup(sys.modules[__name__])


if __name__ == "__main__":
    main()
