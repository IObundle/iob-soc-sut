#!/usr/bin/env python3
import os

from iob_soc_opencryptolinux import iob_soc_opencryptolinux
from iob_soc_sut import iob_soc_sut
from iob_gpio import iob_gpio
from iob_uart16550 import iob_uart16550
from iob_axistream_in import iob_axistream_in
from iob_axistream_out import iob_axistream_out
from iob_ila import iob_ila
from iob_pfsm import iob_pfsm
from iob_dma import iob_dma
from iob_eth import iob_eth
from iob_ram_2p_be import iob_ram_2p_be
from mk_configuration import append_str_config_build_mk
from verilog_tools import insert_verilog_in_module, remove_verilog_line_from_source
from iob_pfsm_program import iob_pfsm_program, iob_fsm_record


class iob_soc_tester(iob_soc_opencryptolinux):
    name = "iob_soc_tester"
    version = "V0.70"
    flows = "pc-emul emb sim fpga"
    setup_dir = os.path.dirname(__file__)
    build_dir = f"../{iob_soc_sut.name}_{iob_soc_sut.version}"

    @classmethod
    def _create_submodules_list(cls):
        """Create submodules list with dependencies of this module"""
        super()._create_submodules_list(
            [
                iob_uart16550,
                iob_soc_sut,
                iob_gpio,
                iob_axistream_in,
                iob_axistream_out,
                iob_ila,
                iob_pfsm,
                iob_dma,
                # iob_eth,
                # Modules required for AXISTREAM
                (iob_ram_2p_be, {"purpose": "simulation"}),
                (iob_ram_2p_be, {"purpose": "fpga"}),
            ]
        )

    # Method that runs the setup process of this class
    @classmethod
    def _specific_setup(cls):
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
                    # SUT memory offset is equal to half the address width (MEM_ADDR_W-1)
                    # The first half of the memory is used for the Tester.
                    # The second half of the memory is used for the SUT.
                    "MEM_ADDR_OFFSET": eval(
                        "2**("
                        + next(i["val"] for i in cls.confs if i["name"] == "MEM_ADDR_W")
                        + "-1)"
                    ),
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
        cls.ila0_instance = iob_ila(
            "ILA0",
            "Tester Integrated Logic Analyzer for SUT signals",
            parameters={
                "BUFFER_W": "4",
                "SIGNAL_W": "38",
                "TRIGGER_W": "1",
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
                    "N_INPUTS": "1",
                    "N_OUTPUTS": "1",
                },
            )
        )
        # cls.peripherals.append(iob_eth("ETH0", "Tester ethernet interface for console"))
        # cls.peripherals.append(iob_eth("ETH1", "Tester ethernet interface for SUT"))

        # Set name of sut firmware (used to join sut firmware with tester firmware)
        cls.sut_fw_name = "iob_soc_sut_firmware.c"

        # Run IOb-SoC setup
        super()._specific_setup()

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

        # Create a Monitor PFSM program that always has the trigger output
        # enabled (to keep recording samples) until a certain sequence is detected
        # in the input.
        # In this example, When the input is 1 for one pulse, it disables the trigger output
        # 4 clocks after it. This allows us to record the two pulses emitted by the Tester's example PFSM.
        monitor_prog.set_records(
            [
                # Format: iob_fsm_record("label", "input_cond", "next_state", "output_expr")
                iob_fsm_record(
                    # Keep jumping to this state while input is not high.
                    "state_0",
                    "0",
                    "state_0",
                    "i[0]",
                ),
                iob_fsm_record(
                    # If the input is still high (more than 1 clock pulse), then go back to state 0.
                    "",
                    "1",
                    "state_0",
                    "1",
                ),
                iob_fsm_record("", "", "", "1"),  # Wait one clock
                iob_fsm_record("", "-", "state_0", "1"),  # Jump to state 0
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

        # Modify iob_soc_tester.v to include ILA probe wires
        iob_ila.generate_system_wires(
            cls.ila0_instance,
            "hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
            sampling_clk="clk_i",  # Name of the internal system signal to use as the sampling clock
            trigger_list=[
                "SUT0.AXISTREAMIN0.axis_tvalid_i"
            ],  # List of signals to use as triggers
            probe_list=[  # List of signals to probe
                ("SUT0.AXISTREAMIN0.axis_tdata_i", 32),
                ("SUT0.AXISTREAMIN0.data_fifo.w_level_o", 5),
                ("PFSM0.output_ports", 1),
            ],
        )

        # Create a probe for input of (independent) PFSM
        # This PFSM will be used as an example, reacting to values of tvalid_i.
        # The output of this PFSM will be captured by the ILA.
        insert_verilog_in_module(
            "   assign PFSM0_input_ports = {SUT0.AXISTREAMIN0.axis_tvalid_i};",
            cls.build_dir
            + "/hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
        )

        # Remove UART0 interrupt connection to PLIC
        remove_verilog_line_from_source(
            "assign PLIC0_src",
            cls.build_dir + "/hardware/src/iob_soc_tester.v",
        )
        # Connect UART0 and UART1 interrupt signals
        insert_verilog_in_module(
            "   assign PLIC0_src     = {{30{1'b0}}, UART1_interrupt_o, uart_interrupt_o};\n",
            cls.build_dir
            + "/hardware/src/iob_soc_tester.v",  # Name of the system file to generate the probe wires
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

        cls._generate_monitor_bitstream()
        cls._generate_pfsm_bitstream()

        # Use Verilator and AES-KU040-DB-G by default.
        if cls.is_top_module:
            append_str_config_build_mk("SIMULATOR:=verilator\n", cls.build_dir)
            append_str_config_build_mk("BOARD:=AES-KU040-DB-G\n", cls.build_dir)

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
            # RS232
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
                    "if_name": "dma",
                    "port": "tvalid_o",
                    "bits": [],
                },
                {
                    "corename": "DMA0",
                    "if_name": "dma_input",
                    "port": "tvalid_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "dma",
                    "port": "tready_i",
                    "bits": [],
                },
                {
                    "corename": "DMA0",
                    "if_name": "dma_input",
                    "port": "tready_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "dma",
                    "port": "tdata_o",
                    "bits": [],
                },
                {
                    "corename": "DMA0",
                    "if_name": "dma_input",
                    "port": "tdata_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "interrupt",
                    "port": "fifo_threshold_o",
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
                    "if_name": "dma",
                    "port": "tvalid_i",
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
                    "if_name": "dma",
                    "port": "tready_o",
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
                    "if_name": "dma",
                    "port": "tdata_i",
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
                    "if_name": "interrupt",
                    "port": "fifo_threshold_o",
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
                    "corename": "internal",
                    "if_name": "ILA0",
                    "port": "",
                    "bits": [],
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
                    "corename": "internal",
                    "if_name": "ILA0",
                    "port": "",
                    "bits": [],
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
                    "corename": "internal",
                    "if_name": "ILA0",
                    "port": "",
                    "bits": [],
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
                    "val": "17",
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
