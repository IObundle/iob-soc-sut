#!/usr/bin/env python3
import os
import copy

import copy_srcs

from iob_soc_opencryptolinux import iob_soc_opencryptolinux
from iob_regfileif import iob_regfileif
from iob_gpio import iob_gpio
from iob_axistream_in import iob_axistream_in
from iob_axistream_out import iob_axistream_out
from iob_ram_2p_be import iob_ram_2p_be
from verilog_gen import insert_verilog_in_module
from config_gen import append_str_config_build_mk

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
                "log2n_items": 0,
                "autoreg": True,
                "descr": "Write register: 8 bit",
            },
            {
                "name": "REG2",
                "type": "W",
                "n_bits": 16,
                "rst_val": 0,
                "log2n_items": 0,
                "autoreg": True,
                "descr": "Write register: 16 bit",
            },
            {
                "name": "REG3",
                "type": "R",
                "n_bits": 8,
                "rst_val": 0,
                "log2n_items": 0,
                "autoreg": True,
                "descr": "Read register: 8 bit",
            },
            {
                "name": "REG4",
                "type": "R",
                "n_bits": 16,
                "rst_val": 0,
                "log2n_items": 0,
                "autoreg": True,
                "descr": "Read register 16 bit",
            },
            {
                "name": "REG5",
                "type": "R",
                "n_bits": 32,
                "rst_val": 0,
                "log2n_items": 0,
                "autoreg": True,
                "descr": "Read register 32 bit. In this example, we use this to pass the sutMemoryMessage address.",
            },
        ],
    }
]


class iob_soc_sut(iob_soc_opencryptolinux):
    name = "iob_soc_sut"
    version = "V0.70"
    flows = "pc-emul emb sim doc fpga"
    setup_dir = os.path.dirname(__file__)

    @classmethod
    def _create_submodules_list(cls):
        """Create submodules list with dependencies of this module"""
        super()._create_submodules_list(
            [
                iob_regfileif_custom,
                iob_gpio,
                iob_axistream_in,
                iob_axistream_out,
                # Modules required for AXISTREAM
                (iob_ram_2p_be, {"purpose": "simulation"}),
                (iob_ram_2p_be, {"purpose": "fpga"}),
            ]
        )

    @classmethod
    def _create_instances(cls):
        """Method that runs the setup process of this class"""
        # Instantiate SUT peripherals
        cls.peripherals.append(
            iob_regfileif_custom(
                "REGFILEIF0",
                "Register file interface",
                parameters={
                    "SYSTEM_VERSION": "16'h"
                    + copy_srcs.version_str_to_digits(cls.version)
                },
            )
        )
        cls.peripherals.append(iob_gpio("GPIO0", "GPIO interface"))
        cls.peripherals.append(
            iob_axistream_in(
                "AXISTREAMIN0",
                "SUT AXI input stream interface",
                parameters={"TDATA_W": "32"},
            )
        )
        cls.peripherals.append(
            iob_axistream_out(
                "AXISTREAMOUT0",
                "SUT AXI output stream interface",
                parameters={"TDATA_W": "32"},
            )
        )
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
            # AXISTREAM IN
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_clk_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_cke_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_arst_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tvalid_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tready_o",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tdata_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tlast_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            # AXISTREAM IN DMA
            # Connect these signals to internal floating wires.
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "sys_axis",
                    "port": "sys_tvalid_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "sys_axis",
                    "port": "sys_tready_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "sys_axis",
                    "port": "sys_tdata_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "general",
                    "port": "interrupt_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            # AXISTREAM OUT
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_clk_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_cke_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_arst_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tvalid_o",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tready_i",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tdata_o",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tlast_o",
                    "bits": [],
                },
                {
                    "corename": "external",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            # AXISTREAM OUT DMA
            # Connect these signals to internal floating wires.
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "sys_axis",
                    "port": "sys_tvalid_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTRREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "sys_axis",
                    "port": "sys_tready_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTRREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "sys_axis",
                    "port": "sys_tdata_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTRREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "general",
                    "port": "interrupt_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTRREAMOUT0",
                    "port": "input_ports",
                    "bits": [1],
                },
            ),
        ]

        super()._create_instances()

    @classmethod
    def _generate_files(cls):
        super()._generate_files()
        # Remove iob_soc_sut_swreg_gen.v as it is not used
        os.remove(os.path.join(cls.build_dir, "hardware/src/iob_soc_sut_swreg_gen.v"))
        # Connect unused peripheral inputs
        insert_verilog_in_module(
            """
    assign AXISTREAMIN0_tready_i = 1'b0;
    assign AXISTRREAMOUT0_tvalid_i = 1'b0;
    assign AXISTRREAMOUT0_tdata_i = 1'b0;
             """,
            cls.build_dir + "/hardware/src/iob_soc_sut.v",
        )
        # Update sim_wrapper connections
        if cls.is_top_module:
            insert_verilog_in_module(
                """
`include "iob_regfileif_inverted_swreg_def.vh"

   assign GPIO0_input_ports = `IOB_SOC_SUT_GPIO0_GPIO_W'h0;
   assign AXISTREAMIN0_axis_clk_i = clk_i;
   assign AXISTREAMIN0_axis_cke_i = 1'b1;
   assign AXISTREAMIN0_axis_arst_i = arst_i;
   assign AXISTREAMIN0_axis_tvalid_i = 1'b0;
   assign AXISTREAMIN0_axis_tdata_i = {`IOB_SOC_SUT_AXISTREAMIN0_TDATA_W{1'b0}};
   assign AXISTREAMIN0_axis_tlast_i = 1'b0;

   assign AXISTREAMOUT0_axis_clk_i = clk_i;
   assign AXISTREAMOUT0_axis_cke_i = 1'b1;
   assign AXISTREAMOUT0_axis_arst_i = arst_i;
   assign AXISTREAMOUT0_axis_tready_i = 1'b0;


   wire [1-1:0] iob_valid_i = 1'b0;
   wire [`IOB_SOC_SUT_REGFILEIF0_ADDR_W-1:0] iob_addr_i = `IOB_SOC_SUT_REGFILEIF0_ADDR_W'h0;
   wire [`IOB_SOC_SUT_REGFILEIF0_DATA_W-1:0] iob_wdata_i = `IOB_SOC_SUT_REGFILEIF0_DATA_W'h0;
   wire [(`IOB_SOC_SUT_REGFILEIF0_DATA_W/8)-1:0] iob_wstrb_i = `IOB_SOC_SUT_REGFILEIF0_DATA_W / 8'h0;
                """,
                cls.build_dir
                + "/hardware/simulation/src/iob_soc_sut_sim_wrapper.v",  # Name of the system file to generate the probe wires
                after_line="iob_soc_sut_wrapper_pwires.vs",
            )
            insert_verilog_in_module(
                """
      .iob_valid_i(iob_valid_i),
      .iob_addr_i  (iob_addr_i),
      .iob_wdata_i (iob_wdata_i),
      .iob_wstrb_i (iob_wstrb_i),
                """,
                cls.build_dir
                + "/hardware/simulation/src/iob_soc_sut_sim_wrapper.v",  # Name of the system file to generate the probe wires
                after_line="soc0",
            )
        # Set ethernet MAC address
        if cls.is_top_module:
            append_str_config_build_mk(
                """
#Mac address of pc interface connected to ethernet peripheral (based on board name)
$(if $(findstring sim,$(MAKECMDGOALS))$(SIMULATOR),$(eval BOARD=))
ifeq ($(BOARD),AES-KU040-DB-G)
RMAC_ADDR ?=989096c0632c
endif
ifeq ($(BOARD),CYCLONEV-GT-DK)
RMAC_ADDR ?=309c231e624b
endif
RMAC_ADDR ?=000000000000
export RMAC_ADDR
#Set correct environment if running on IObundle machines
ifneq ($(filter pudim-flan sericaia,$(shell hostname)),)
IOB_CONSOLE_PYTHON_ENV ?= /opt/pyeth3/bin/python
endif
                """,
                cls.build_dir,
            )

    @classmethod
    def _init_attributes(cls):
        super()._init_attributes()

    @classmethod
    def _setup_regs(cls):
        cls.regs += sut_regs

    @classmethod
    def _setup_confs(cls):
        # Append confs or override them if they exist
        super()._setup_confs(
            [
                {
                    "name": "SRAM_ADDR_W",
                    "type": "P",
                    "val": "18",
                    "min": "1",
                    "max": "32",
                    "descr": "SRAM address width",
                },
            ]
        )


# Custom iob_regfileif subclass for use in SUT system
class iob_regfileif_custom(iob_regfileif):
    @classmethod
    def _init_attributes(cls):
        super()._init_attributes()
        cls.regs = copy.deepcopy(sut_regs)
