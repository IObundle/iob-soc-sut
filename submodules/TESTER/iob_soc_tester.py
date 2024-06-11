#!/usr/bin/env python3
import os
import sys
import glob
import shutil

from iob_soc_opencryptolinux import iob_soc_opencryptolinux
from iob_soc_sut import iob_soc_sut
from iob_gpio import iob_gpio
from iob_uart16550 import iob_uart16550
from iob_axistream_in import iob_axistream_in
from iob_axistream_out import iob_axistream_out
from iob_spi_master import iob_spi_master
from iob_ila import iob_ila
from iob_pfsm import iob_pfsm
from iob_dma import iob_dma
from iob_eth import iob_eth
from iob_ram_2p_be import iob_ram_2p_be
from config_gen import append_str_config_build_mk
from verilog_gen import insert_verilog_in_module, inplace_change
from iob_pfsm_program import iob_pfsm_program, iob_fsm_record
from iob_timer import iob_timer

# Select if should include ILA and PFSM peripherals.
# Disable this to reduce the amount of FPGA resources used.
USE_ILA_PFSM = False if "NO_ILA" in sys.argv else True


class iob_soc_tester(iob_soc_opencryptolinux):
    name = "iob_soc_tester"
    version = "V0.70"
    flows = "pc-emul emb sim fpga"
    setup_dir = os.path.dirname(__file__)
    build_dir = f"../{iob_soc_sut.name}_{iob_soc_sut.version}"

    @classmethod
    def _create_submodules_list(cls):
        """Create submodules list with dependencies of this module"""
        submodules = [
            iob_uart16550,
            iob_soc_sut,
            iob_gpio,
            iob_axistream_in,
            iob_axistream_out,
            iob_dma,
            iob_eth,
            # Modules required for AXISTREAM
            (iob_ram_2p_be, {"purpose": "simulation"}),
            (iob_ram_2p_be, {"purpose": "fpga"}),
        ]
        if USE_ILA_PFSM:
            submodules += [
                iob_ila,
                iob_pfsm,
            ]
        super()._create_submodules_list(submodules)

    @classmethod
    def _create_instances(cls):
        # Instantiate TESTER peripherals
        cls.peripherals.append(
            iob_uart16550("UART1", "UART interface for communication with SUT")
        )
        cls.peripherals.append(
            iob_soc_sut(
                "SUT0",
                "System Under Test (SUT) peripheral",
                parameters={
                    "AXI_ID_W": "AXI_ID_W",
                    "AXI_LEN_W": "AXI_LEN_W",
                    "AXI_ADDR_W": "AXI_ADDR_W",
                    "AXI_DATA_W": "`IOB_SOC_TESTER_SUT0_DATA_W",
                    # SUT memory offset is equal to half the address width (MEM_ADDR_W-1)
                    # The first half of the memory is used for the Tester.
                    # The second half of the memory is used for the SUT.
                    "MEM_ADDR_OFFSET": str(
                        eval(
                            "2**("
                            + next(
                                i["val"] for i in cls.confs if i["name"] == "MEM_ADDR_W"
                            )
                            + "-1)"
                        )
                    ),
                    "ETH0_MEM_ADDR_OFFSET": "`IOB_SOC_TESTER_SUT0_MEM_ADDR_OFFSET",
                    "VERSAT0_MEM_ADDR_OFFSET": "`IOB_SOC_TESTER_SUT0_MEM_ADDR_OFFSET",
                },
            )
        )

        cls.peripherals.append(iob_gpio("GPIO0", "GPIO interface"))
        cls.peripherals.append(
            iob_gpio(
                "GPIO1",
                "GPIO interface to receive FIFO interrupts.",
                parameters={"GPIO_W": "2"},
            )
        )
        cls.peripherals.append(
            iob_axistream_in(
                "AXISTREAMIN0",
                "Tester AXI input stream interface",
                parameters={"TDATA_W": "32"},
            )
        )
        cls.peripherals.append(
            iob_axistream_out(
                "AXISTREAMOUT0",
                "Tester AXI output stream interface",
                parameters={"TDATA_W": "32"},
            )
        )
        if USE_ILA_PFSM:
            cls.ila0_instance = iob_ila(
                "ILA0",
                "Tester Integrated Logic Analyzer for SUT signals",
                parameters={
                    "BUFFER_W": "3",
                    "SIGNAL_W": "38",
                    "TRIGGER_W": "2",
                    "CLK_COUNTER": "1",
                    "MONITOR": "1",
                    "MONITOR_STATE_W": "2",
                },
            )
            cls.peripherals.append(cls.ila0_instance)
            cls.peripherals.append(
                iob_pfsm(
                    "PFSM0",
                    "PFSM interface",
                    parameters={"STATE_W": "2", "INPUT_W": "1", "OUTPUT_W": "1"},
                )
            )

        cls.peripherals.append(
            iob_dma(
                "DMA0",
                "DMA interface",
                parameters={
                    "AXI_ID_W": "AXI_ID_W",
                    "AXI_LEN_W": "AXI_LEN_W",
                    "AXI_ADDR_W": "AXI_ADDR_W",
                    "N_INPUTS": "2" if USE_ILA_PFSM else "1",
                    "N_OUTPUTS": "1",
                },
            )
        )
        cls.peripherals.append(
            iob_eth(
                "ETH1",
                "Tester ethernet interface for SUT",
                parameters={
                    "AXI_ID_W": "AXI_ID_W",
                    "AXI_LEN_W": "AXI_LEN_W",
                    "AXI_ADDR_W": "AXI_ADDR_W",
                    "AXI_DATA_W": "AXI_DATA_W",
                },
            )
        )

        # Set name of sut firmware (used to join sut firmware with tester firmware)
        cls.sut_fw_name = "iob_soc_sut_firmware.c"

        super()._create_instances()

        versatInst = None
        for x in iob_soc_tester.peripherals:
            if x.name == "VERSAT0":
                versatInst = x
        iob_soc_tester.peripherals.remove(versatInst)

        """
        cls.peripherals.append(
            iob_eth(
                "ETH0",
                "Ethernet interface",
                parameters={
                    "AXI_ID_W": "AXI_ID_W",
                    "AXI_LEN_W": "AXI_LEN_W",
                    "AXI_ADDR_W": "AXI_ADDR_W",
                    "AXI_DATA_W": "AXI_DATA_W",
                    "MEM_ADDR_OFFSET": "MEM_ADDR_OFFSET",
                },
            )
        )

        cls.peripherals.append(iob_uart16550("UART0", "Default UART interface"))
        cls.peripherals.append(
            iob_spi_master("SPI0", "SPI master peripheral")
        )  # Not being used but setup complains if not inserted
        cls.peripherals.append(iob_timer("TIMER0"))

        # This extra copy is used to prevent the automatic insertion of: TIMER0,UART0,SPI0,VERSAT0,ETH0 - from the opencrypto repo
        # Some of these are needed, but these must be instantiated before hand, do not rely on opencrypto instantiate them
        saved = cls.peripherals.copy()
        super()._create_instances()
        cls.peripherals = saved

        print([x.name for x in cls.peripherals], file=sys.stderr)
        """

    @classmethod
    def _generate_monitor_bitstream(cls):
        """Generate bitstream for the Monitor PFSM of the ILA."""
        ila_parameters = cls.ila0_instance.parameters
        monitor_prog = iob_pfsm_program(
            ila_parameters[
                "MONITOR_STATE_W"
            ],  # Monitor STATE_W defined in ILA parameter
            ila_parameters[
                "TRIGGER_W"
            ],  # Monitor INPUT_W = ILA TRIGGER_W because ILA trigger signals connected as inputs of Monitor.
            1,  # Monitor OUTPUT_W always 1. It is the internal trigger signal for the ILA.
        )

        # Create a Monitor PFSM program that triggers when an AXI Stream word is being transfered (input bit 0 is high),
        # and also trigger during 3 clock cycles after the last word (after input bit 1 is high).
        # These last 3 samples allow capturing the output data of the PFSM peripheral.
        monitor_prog.set_records(
            [
                # Format: iob_fsm_record("label", "input_cond", "next_state", "output_expr")
                iob_fsm_record(
                    # Keep jumping to this state while input bit 1 is not high.
                    # Output is the same as bit 0 of input
                    "state_0",
                    "0-",
                    "state_0",
                    "i[0]",
                ),
                iob_fsm_record(
                    # If the input bit 1 is still high (more than 1 clock pulse), then go back to state 0.
                    "",
                    "1-",
                    "state_0",
                    "1",
                ),
                iob_fsm_record("", "", "", "1"),  # Wait one clock
                iob_fsm_record("", "--", "state_0", "1"),  # Jump to state 0
            ]
        )

        # monitor_prog.print_truth_table(0)  # DEBUG: Print truth table for record with index given

        # Generate bitstream in simulation directory
        monitor_prog.generate_bitstream(
            os.path.join(cls.build_dir, "hardware/simulation/monitor_pfsm.bit")
        )
        # Create symlink for this bitstream in the fpga directory
        os.symlink(
            os.path.join("../simulation/monitor_pfsm.bit"),
            os.path.join(cls.build_dir, "hardware/fpga/monitor_pfsm.bit"),
        )

    @classmethod
    def _generate_pfsm_bitstream(cls):
        """Generate bitstream for the independent PFSM peripheral."""
        pfsm_prog = iob_pfsm_program(
            2,  # This PFSM has 2^2 states
            1,  # This PFSM has 1 input
            1,  # This PFSM has 1 output
        )

        # Create a PFSM program that pulses two times, when its input is set.
        pfsm_prog.set_records(
            [
                # Format: iob_fsm_record("label", "input_cond", "next_state", "output_expr")
                iob_fsm_record(
                    "state_0", "0", "state_0", "0"
                ),  # Keep jumping to state_0 while input is not high.
                iob_fsm_record("", "", "", "1"),  # Set output to 1
                iob_fsm_record("", "", "", "0"),  # Set output to 0
                iob_fsm_record(
                    "", "-", "state_0", "1"
                ),  # Set output to 1. Always jump to state_0.
            ]
        )

        # pfsm_prog.print_truth_table(0)  # DEBUG: Print truth table for record with index given

        # Generate bitstream in simulation directory
        pfsm_prog.generate_bitstream(
            os.path.join(cls.build_dir, "hardware/simulation/pfsm.bit")
        )
        # Create symlink for this bitstream in the fpga directory
        os.symlink(
            os.path.join("../simulation/pfsm.bit"),
            os.path.join(cls.build_dir, "hardware/fpga/pfsm.bit"),
        )

    @classmethod
    def _generate_files(cls):
        super()._generate_files()

        # Don't use hierarchical references for Quartus boards
        if os.getenv("BOARD") != "CYCLONEV-GT-DK" and USE_ILA_PFSM:
            # Modify iob_soc_tester.v to include ILA probe wires
            iob_ila.generate_system_wires(
                cls.ila0_instance,
                "hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
                sampling_clk="clk_i",  # Name of the internal system signal to use as the sampling clock
                trigger_list=[
                    "SUT0.AXISTREAMIN0.axis_tvalid_i & SUT0.AXISTREAMIN0.axis_tready_o",
                    # Bit 1 is enabled when the last word is being transfered
                    "SUT0.AXISTREAMIN0.axis_tlast_i & SUT0.AXISTREAMIN0.axis_tvalid_i & SUT0.AXISTREAMIN0.axis_tready_o",
                ],  # List of signals to use as triggers
                probe_list=[  # List of signals to probe
                    ("SUT0.AXISTREAMIN0.axis_tdata_i", 32),
                    ("SUT0.AXISTREAMIN0.data_fifo.w_level_o", 5),
                    ("PFSM0.output_ports", 1),
                ],
            )

            # Create a probe for input of (independent) PFSM
            # This PFSM will be used as an example, reacting to values of tlast_i.
            # The output of this PFSM will be captured by the ILA.
            insert_verilog_in_module(
                "   assign PFSM0_input_ports = {SUT0.AXISTREAMIN0.axis_tlast_i & SUT0.AXISTREAMIN0.axis_tvalid_i & SUT0.AXISTREAMIN0.axis_tready_o};",
                cls.build_dir
                + "/hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
            )

        # Connect UART0 and UART1 interrupt signals
        inplace_change(
            cls.build_dir
            + "/hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
            ".plicInterrupts({{30{1'b0}}, uart_interrupt_o, 1'b0}),",
            ".plicInterrupts({{29{1'b0}}, UART1_interrupt_o, uart_interrupt_o, 1'b0}),",
        )

        # Connect General signals from iob-axis cores
        insert_verilog_in_module(
            """
    assign SUT_AXISTREAMIN_AXISTREAMIN0_axis_clk_i = clk_i;
    assign SUT_AXISTREAMIN_AXISTREAMIN0_axis_cke_i = cke_i;
    assign SUT_AXISTREAMIN_AXISTREAMIN0_axis_arst_i = arst_i;
    assign AXISTREAMIN0_axis_clk_i = clk_i;
    assign AXISTREAMIN0_axis_cke_i = cke_i;
    assign AXISTREAMIN0_axis_arst_i = arst_i;
    assign SUT_AXISTREAMOUT_AXISTREAMOUT0_axis_clk_i = clk_i;
    assign SUT_AXISTREAMOUT_AXISTREAMOUT0_axis_cke_i = cke_i;
    assign SUT_AXISTREAMOUT_AXISTREAMOUT0_axis_arst_i = arst_i;
    assign AXISTREAMOUT0_axis_clk_i = clk_i;
    assign AXISTREAMOUT0_axis_cke_i = cke_i;
    assign AXISTREAMOUT0_axis_arst_i = arst_i;
             """,
            cls.build_dir
            + "/hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
        )

        insert_verilog_in_module(
            """
    // Connect ethernet clocks
    assign ETH1_MTxClk = ETH0_MTxClk;
    assign SUT0_ETH0_ETH0_MTxClk = ETH0_MTxClk;
    assign ETH1_MRxClk = ETH0_MRxClk;
    assign SUT0_ETH0_ETH0_MRxClk = ETH0_MRxClk;
    // Connect unused inputs to ground
    assign ETH1_MColl = 1'b0;
    assign ETH1_MCrS = 1'b0;
    assign SUT0_ETH0_ETH0_MColl = 1'b0;
    assign SUT0_ETH0_ETH0_MCrS = 1'b0;
             """,
            cls.build_dir
            + "/hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
        )

        if USE_ILA_PFSM:
            cls._generate_monitor_bitstream()
            cls._generate_pfsm_bitstream()

        # Temporary fix for regfileif (will not be needed with python-gen)
        if cls.is_top_module:
            insert_verilog_in_module(
                """
`include "iob_regfileif_inverted_swreg_def.vh"
                """,
                cls.build_dir
                + "/hardware/simulation/src/iob_soc_tester_sim_wrapper.v",  # Name of the system file to generate the probe wires
                after_line="iob_soc_tester_wrapper_pwires.vs",
            )

        if cls.is_top_module:
            # Use Verilator and AES-KU040-DB-G by default.
            append_str_config_build_mk("SIMULATOR ?=verilator\n", cls.build_dir)
            append_str_config_build_mk("BOARD ?=AES-KU040-DB-G\n", cls.build_dir)
            # Set ethernet MAC address
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
            # Targets to copy ila_data.bin from remote machines
            append_str_config_build_mk(
                """

# Targets to copy ila_data.bin from remote machines
copy_remote_fpga_ila_data:
	scp $(BOARD_USER)@$(BOARD_SERVER):$(REMOTE_FPGA_DIR)/ila_data.bin . 2> /dev/null | true
copy_remote_simulation_ila_data:
	scp $(SIM_SCP_FLAGS) $(SIM_USER)@$(SIM_SERVER):$(REMOTE_SIM_DIR)/ila_data.bin . 2> /dev/null | true
.PHONY: copy_remote_fpga_ila_data copy_remote_simulation_ila_data

                """,
                cls.build_dir,
            )

        # Append uut_build.mk files to Tester's build files
        for filepath in [
            "software/sw_build.mk",
            "hardware/simulation/sim_build.mk",
            "hardware/fpga/fpga_build.mk",
        ]:
            with open(
                os.path.join(cls.setup_dir, filepath.rsplit("/", 1)[0], "uut_build.mk")
            ) as fp:
                data2append = fp.read()
            with open(os.path.join(cls.build_dir, filepath), "a") as fp:
                fp.write(data2append)

        shutil.copyfile(
            f"{__class__.setup_dir}/software/true_sw_build.mk",
            f"{cls.build_dir}/software/sw_build.mk",
        )

        # Replace `MEM_ADDR_W` macro in *_firmware.S files by `SRAM_ADDR_W`
        # This prevents the Tester+SUT from using entire shared memory, therefore preventing conflicts
        firmware_list = glob.glob(cls.build_dir + "/software/src/*firmware.S")
        for firmware in firmware_list:
            inplace_change(firmware, "MEM_ADDR_W", "SRAM_ADDR_W")

    @classmethod
    def _setup_portmap(cls):
        super()._setup_portmap()
        cls.peripheral_portmap += [
            # ================================================================== SUT IO mappings ==================================================================
            # SUT UART0
            # Map interrupt port to internal wire
            (
                {
                    "corename": "UART1",
                    "if_name": "interrupt",
                    "port": "interrupt_o",
                    "bits": [],
                },
                {"corename": "internal", "if_name": "UART1", "port": "", "bits": []},
            ),
            # Connect RX and TX of UART1 and SUT
            (
                {
                    "corename": "SUT0",
                    "if_name": "uart",
                    "port": "uart_rxd_i",
                    "bits": [],
                },
                {"corename": "UART1", "if_name": "rs232", "port": "txd_o", "bits": []},
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "uart",
                    "port": "uart_txd_o",
                    "bits": [],
                },
                {"corename": "UART1", "if_name": "rs232", "port": "rxd_i", "bits": []},
            ),
            # Connect CTS and RTS of UART1 and SUT
            (
                {
                    "corename": "SUT0",
                    "if_name": "uart",
                    "port": "uart_cts_i",
                    "bits": [],
                },
                {"corename": "UART1", "if_name": "rs232", "port": "rts_o", "bits": []},
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "uart",
                    "port": "uart_rts_o",
                    "bits": [],
                },
                {"corename": "UART1", "if_name": "rs232", "port": "cts_i", "bits": []},
            ),
            # SUT ETHERNET0
            ####({'corename':'SUT0', 'if_name':'ETHERNET0_ethernet', 'port':'', 'bits':[]},         {'corename':'', 'if_name':'', 'port':'', 'bits':[]}), #Map ETHERNET0 of Tester to external interface
            ####({'corename':'SUT0', 'if_name':'ETHERNET1_ethernet', 'port':'', 'bits':[]},         {'corename':'ETHERNET0', 'if_name':'ethernet', 'port':'', 'bits':[]}), #Map ETHERNET0 of SUT to ETHERNET0 of Tester
            # SUT GPIO0
            (
                {
                    "corename": "GPIO0",
                    "if_name": "gpio",
                    "port": "input_ports",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "GPIO0",
                    "port": "GPIO0_output_ports",
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
                {
                    "corename": "SUT0",
                    "if_name": "GPIO0",
                    "port": "GPIO0_input_ports",
                    "bits": [],
                },
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
                    "port": "GPIO0_output_enable",
                    "bits": [],
                },
                {"corename": "external", "if_name": "SUT", "port": "", "bits": []},
            ),
            # SUT AXISTREAM IN - General signals
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_clk_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT_AXISTREAMIN",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_cke_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT_AXISTREAMIN",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_arst_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT_AXISTREAMIN",
                    "port": "",
                    "bits": [],
                },
            ),
            # Tester AXISTREAM IN - General signals
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_clk_i",
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
                    "if_name": "axistream",
                    "port": "axis_cke_i",
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
                    "if_name": "axistream",
                    "port": "axis_arst_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "AXISTREAMIN0",
                    "port": "",
                    "bits": [],
                },
            ),
            # SUT AXISTREAM IN
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_tvalid_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tvalid_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_tready_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tready_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_tdata_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tdata_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "AXISTREAMIN0_axis_tlast_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_tlast_o",
                    "bits": [],
                },
            ),
            # SUT AXISTREAM OUT - General signals
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_clk_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT_AXISTREAMOUT",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_cke_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT_AXISTREAMOUT",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_arst_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT_AXISTREAMOUT",
                    "port": "",
                    "bits": [],
                },
            ),
            # Tester AXISTREAM OUT - General signals
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "axis_clk_i",
                    "bits": [],
                },
                {
                    "corename": "internal",
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
                    "corename": "internal",
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
                    "corename": "internal",
                    "if_name": "AXISTREAMOUT0",
                    "port": "",
                    "bits": [],
                },
            ),
            # SUT AXISTREAM OUT
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_tvalid_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tvalid_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_tready_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tready_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_tdata_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tdata_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "AXISTREAMOUT0_axis_tlast_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "axis_tlast_i",
                    "bits": [],
                },
            ),
            # Tester AXISTREAM IN DMA
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "sys_axis",
                    "port": "sys_tvalid_o",
                    "bits": [],
                },
                {
                    "corename": "DMA0",
                    "if_name": "dma_input",
                    "port": "tvalid_i",
                    "bits": [0],
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
                    "corename": "DMA0",
                    "if_name": "dma_input",
                    "port": "tready_o",
                    "bits": [0],
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
                    "corename": "DMA0",
                    "if_name": "dma_input",
                    "port": "tdata_i",
                    "bits": list(range(32)),
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
                    "corename": "GPIO1",
                    "if_name": "gpio",
                    "port": "input_ports",
                    "bits": [0],
                },
            ),
            # TESTER AXISTREAM OUT DMA
            (
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "sys_axis",
                    "port": "sys_tvalid_i",
                    "bits": [],
                },
                {
                    "corename": "DMA0",
                    "if_name": "dma_output",
                    "port": "tvalid_o",
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
                    "corename": "DMA0",
                    "if_name": "dma_output",
                    "port": "tready_i",
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
                    "corename": "DMA0",
                    "if_name": "dma_output",
                    "port": "tdata_o",
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
                    "corename": "GPIO1",
                    "if_name": "gpio",
                    "port": "input_ports",
                    "bits": [1],
                },
            ),
            # GPIO1 unused ports
            # Connect them to internal floating wires
            (
                {
                    "corename": "GPIO1",
                    "if_name": "gpio",
                    "port": "output_ports",
                    "bits": [],
                },
                {"corename": "internal", "if_name": "GPIO1", "port": "", "bits": []},
            ),
            (
                {
                    "corename": "GPIO1",
                    "if_name": "gpio",
                    "port": "output_enable",
                    "bits": [],
                },
                {"corename": "internal", "if_name": "GPIO1", "port": "", "bits": []},
            ),
            # ETHERNET 1
            (
                {
                    "corename": "ETH1",
                    "if_name": "general",
                    "port": "inta_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            # phy - connect to SUT ETH0 interface
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MTxClk",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MTxD",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MRxD",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MTxEn",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MRxDv",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MTxErr",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MRxErr",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MRxClk",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MRxDv",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MTxEn",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MRxD",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MTxD",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MRxErr",
                    "bits": [],
                },
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MTxErr",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MColl",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MCrS",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MDC",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "MDIO",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "ETH1",
                    "if_name": "phy",
                    "port": "phy_rstn_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "ETH1",
                    "port": "",
                    "bits": [],
                },
            ),
            # Remaining SUT0 ETH0 signals
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MTxClk",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MRxClk",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MColl",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MCrS",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MDC",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_phy_rstn_o",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "ETH0",
                    "port": "ETH0_MDIO",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_ETH0",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "spi",
                    "port": "spi_SS",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_SPI0_SS",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "spi",
                    "port": "spi_SCLK",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_SPI0_SCLK",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "spi",
                    "port": "spi_MISO",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_SPI0_MISO",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "spi",
                    "port": "spi_MOSI",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_SPI0_MOSI",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "spi",
                    "port": "spi_WP_N",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_SPI0_WP_N",
                    "port": "",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "spi",
                    "port": "spi_HOLD_N",
                    "bits": [],
                },
                {
                    "corename": "internal",
                    "if_name": "SUT0_SPI0_HOLD_N",
                    "port": "",
                    "bits": [],
                },
            ),
        ]

        if USE_ILA_PFSM:
            cls.peripheral_portmap += [
                # ILA IO --- Connect IOs of Integrated Logic Analyzer to internal system signals
                (
                    {
                        "corename": "ILA0",
                        "if_name": "ila",
                        "port": "signal",
                        "bits": [],
                    },
                    {
                        "corename": "internal",
                        "if_name": "ILA0",
                        "port": "",
                        "bits": [],
                    },
                ),
                (
                    {
                        "corename": "ILA0",
                        "if_name": "ila",
                        "port": "trigger",
                        "bits": [],
                    },
                    {
                        "corename": "internal",
                        "if_name": "ILA0",
                        "port": "",
                        "bits": [],
                    },
                ),
                (
                    {
                        "corename": "ILA0",
                        "if_name": "ila",
                        "port": "sampling_clk",
                        "bits": [],
                    },
                    {
                        "corename": "internal",
                        "if_name": "ILA0",
                        "port": "",
                        "bits": [],
                    },
                ),
                # ILA DMA
                # Connect these signals to internal floating wires.
                (
                    {
                        "corename": "ILA0",
                        "if_name": "dma",
                        "port": "tvalid_o",
                        "bits": [],
                    },
                    {
                        "corename": "DMA0",
                        "if_name": "dma_input",
                        "port": "tvalid_i",
                        "bits": [1],
                    },
                ),
                (
                    {
                        "corename": "ILA0",
                        "if_name": "dma",
                        "port": "tready_i",
                        "bits": [],
                    },
                    {
                        "corename": "DMA0",
                        "if_name": "dma_input",
                        "port": "tready_o",
                        "bits": [1],
                    },
                ),
                (
                    {
                        "corename": "ILA0",
                        "if_name": "dma",
                        "port": "tdata_o",
                        "bits": [],
                    },
                    {
                        "corename": "DMA0",
                        "if_name": "dma_input",
                        "port": "tdata_i",
                        "bits": list(range(32, 64)),
                    },
                ),
                # PFSM IO --- Connect IOs of Programmable Finite State Machine to internal system signals
                (
                    {
                        "corename": "PFSM0",
                        "if_name": "pfsm",
                        "port": "input_ports",
                        "bits": [],
                    },
                    {
                        "corename": "internal",
                        "if_name": "PFSM0",
                        "port": "",
                        "bits": [],
                    },
                ),
                (
                    {
                        "corename": "PFSM0",
                        "if_name": "pfsm",
                        "port": "output_ports",
                        "bits": [],
                    },
                    {
                        "corename": "internal",
                        "if_name": "PFSM0",
                        "port": "",
                        "bits": [],
                    },
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
                    "val": "23",
                    "min": "1",
                    "max": "32",
                    "descr": "SRAM address width",
                },
            ]
        )


## Function to rename the sim_build.mk and fpga_build.mk files of thee SUT to files named `uut_build_for_iob_soc_tester.mk`
## The tester has its own sim_build.mk and fpga_build.mk files, so we need to rename the SUT ones to prevent overwriting
# def create_sut_uut_build():
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
# def custom_setup():
#    # Add the following arguments:
#    # "TESTER_ONLY": setup tester without the SUT (as SUT will be a manually added netlist)
#    if "TESTER_ONLY" not in sys.argv[1:]:
#        # Add SUT module as dependency of the Tester's iob-soc
#        iob_soc_options["submodules"]["hw_setup"]["modules"].insert(0, "SUT")
#        iob_soc_options["submodules"]["hw_setup"]["modules"].insert(
#            1, create_sut_uut_build
#        )
