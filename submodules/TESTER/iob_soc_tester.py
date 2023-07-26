#!/usr/bin/env python3
import os

from iob_soc import iob_soc
from iob_soc_sut import iob_soc_sut
from iob_gpio import iob_gpio
from iob_uart import iob_uart
from iob_axistream_in import iob_axistream_in
from iob_axistream_out import iob_axistream_out
from iob_ila import iob_ila
from iob_pfsm import iob_pfsm
from iob_eth import iob_eth
from iob_ram_2p_be import iob_ram_2p_be
from mk_configuration import append_str_config_build_mk
from verilog_tools import insert_verilog_in_module
from iob_pfsm_program import iob_pfsm_program, iob_fsm_record


class iob_soc_tester(iob_soc):
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
                iob_uart,
                iob_soc_sut,
                iob_gpio,
                iob_axistream_in,
                iob_axistream_out,
                iob_ila,
                iob_pfsm,
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
            iob_uart("UART1", "UART interface for communication with SUT")
        )
        cls.peripherals.append(
            iob_soc_sut(
                "SUT0",
                "System Under Test (SUT) peripheral",
                parameters={
                    "AXI_ID_W": "AXI_ID_W",
                    "AXI_LEN_W": "AXI_LEN_W",
                    "AXI_ADDR_W": "AXI_ADDR_W",
                },
            )
        )

        cls.peripherals.append(iob_gpio("GPIO0", "GPIO interface"))
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
                "BUFFER_W": "3",
                "SIGNAL_W": "38",
                "TRIGGER_W": "1",
                "CLK_COUNTER": "1",
                "MONITOR": "1",
                "MONITOR_STATE_W": "3",
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
                    "state_0", "0", "state_0", "1"
                ),  # Keep jumping to this state while input is not high.
                iob_fsm_record(
                    "", "1", "state_0", "1"
                ),  # If the input is still high (more than 1 clock pulse), then go back to state 0.
                iob_fsm_record("", "", "", "1"),  # Wait one clock
                iob_fsm_record("", "", "", "1"),  # Wait one clock
                iob_fsm_record(
                    "state_finish", "-", "state_finish", "0"
                ),  # Disable the trigger output. Keep jumping to this state to keep the trigger output disabled.
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
                "SUT0.AXISTREAMIN0.tvalid_i"
            ],  # List of signals to use as triggers
            probe_list=[  # List of signals to probe
                ("SUT0.AXISTREAMIN0.tdata_i", 32),
                ("SUT0.AXISTREAMIN0.fifo.level_o", 5),
                ("PFSM0.output_ports", 1),
            ],
        )

        # Create a probe for input of (independent) PFSM
        # This PFSM will be used as an example, reacting to values of tvalid_i.
        # The output of this PFSM will be captured by the ILA.
        insert_verilog_in_module(
            "   assign PFSM0_input_ports = {SUT0.AXISTREAMIN0.tvalid_i};",
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
            # ({'corename':'SUT0', 'if_name':'UART_rs232', 'port':'', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'', 'bits':[]}), #Map UART0 of SUT to UART1 of Tester
            # Python scripts do not yet support 'UART0_rs232'. Need to connect each signal independently
            (
                {"corename": "SUT0", "if_name": "UART", "port": "rxd", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "txd", "bits": []},
            ),
            (
                {"corename": "SUT0", "if_name": "UART", "port": "txd", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "rxd", "bits": []},
            ),
            (
                {"corename": "SUT0", "if_name": "UART", "port": "cts", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "rts", "bits": []},
            ),
            (
                {"corename": "SUT0", "if_name": "UART", "port": "rts", "bits": []},
                {"corename": "UART1", "if_name": "rs232", "port": "cts", "bits": []},
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
                {
                    "corename": "SUT0",
                    "if_name": "GPIO0",
                    "port": "input_ports",
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
                    "port": "output_enable",
                    "bits": [],
                },
                {"corename": "external", "if_name": "SUT_GPIO", "port": "", "bits": []},
            ),
            # SUT AXISTREAM IN
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "tvalid_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "tvalid_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "tready_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "tready_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "tdata_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "tdata_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMIN0",
                    "port": "tlast_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMOUT0",
                    "if_name": "axistream",
                    "port": "tlast_o",
                    "bits": [],
                },
            ),
            # SUT AXISTREAM OUT
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "tvalid_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "tvalid_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "tready_i",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "tready_o",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "tdata_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "tdata_i",
                    "bits": [],
                },
            ),
            (
                {
                    "corename": "SUT0",
                    "if_name": "AXISTREAMOUT0",
                    "port": "tlast_o",
                    "bits": [],
                },
                {
                    "corename": "AXISTREAMIN0",
                    "if_name": "axistream",
                    "port": "tlast_i",
                    "bits": [],
                },
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
