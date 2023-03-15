#!/usr/bin/env python3

import os, sys

sys.path.insert(0, os.getcwd()+'/submodules/LIB/scripts')
import setup
from mk_configuration import update_define
from submodule_utils import import_setup

sys.path.insert(0, os.path.dirname(__file__)+'/submodules/IOBSOC/scripts')
import iob_soc

sys.path.insert(0, os.path.dirname(__file__)+'/submodules/TESTER/scripts')
import tester

name='iob_soc_sut'
version='V0.70'
flows='pc-emul emb sim doc fpga'
if setup.is_top_module(sys.modules[__name__]):
    setup_dir=os.path.dirname(__file__)
    build_dir=f"../{name}_{version}"

# ######### Register file peripheral configuration ##############
regfileif_options = {
    'regs':
    [
        {'name': 'regfileif', 'descr':'REGFILEIF software accessible registers.', 'regs': [
            {'name':'REG1','type':'W', 'n_bits':8,  'rst_val':0, 'addr':-1, 'log2n_items':0, 'autologic':True, 'descr':'Write register: 8 bit'},
            {'name':'REG2','type':'W', 'n_bits':16, 'rst_val':0, 'addr':-1, 'log2n_items':0, 'autologic':True, 'descr':'Write register: 16 bit'},
            {'name':'REG3','type':'R', 'n_bits':8,  'rst_val':0, 'addr':-1, 'log2n_items':0, 'autologic':True, 'descr':'Read register: 8 bit'},
            {'name':'REG4','type':'R', 'n_bits':16, 'rst_val':0, 'addr':-1, 'log2n_items':0, 'autologic':True, 'descr':'Read register 16 bit'},
            {'name':'REG5','type':'R', 'n_bits':32, 'rst_val':0, 'addr':-1, 'log2n_items':0, 'autologic':True, 'descr':'Read register 32 bit. In this example, we use this to pass the sutMemoryMessage address.'},
        ]}
    ]
}
# ############### End of REGFILEIF configuration ################

# ################## Tester configuration #######################
tester_options = {
    #NOTE: Ethernet peripheral is disabled for the time being as it has not yet been updated for compatibility with the python-setup branch.
    'extra_peripherals': 
    [
        #{'name':'UART0', 'type':'UART', 'descr':'Default UART interface', 'params':{}}, # It is possible to override default tester peripherals with new parameters
        {'name':'UART1', 'type':'UART', 'descr':'UART interface for communication with SUT', 'params':{}},
        ##{'name':'ETHERNET0', 'type':'ETHERNET', 'descr':'Ethernet interface for communication with the console', 'params':{}},
        ##{'name':'ETHERNET0', 'type':'ETHERNET', 'descr':'Ethernet interface for communication with the SUT', 'params':{}},
        {'name':'SUT0', 'type':'SUT', 'descr':'System Under Test (SUT) peripheral', 'params':{'AXI_ID_W':'AXI_ID_W','AXI_LEN_W':'AXI_LEN_W'}},
        {'name':'IOBNATIVEBRIDGEIF0', 'type':'IOBNATIVEBRIDGEIF', 'descr':'IOb native interface for communication with SUT. Essentially a REGFILEIF without any registers.', 'params':{}},
    ],

    'extra_peripherals_dirs':
    {
        'SUT':setup_dir,
        'ETHERNET':f"{setup_dir}/submodules/ETHERNET",
        'REGFILEIF':f"{setup_dir}/submodules/REGFILEIF",
        'IOBNATIVEBRIDGEIF':f"{setup_dir}/submodules/REGFILEIF/nativebridgeif_wrappper",
    },

    'peripheral_portmap':
    [
        ({'corename':'UART0', 'if_name':'rs232', 'port':'', 'bits':[]},                         {'corename':'', 'if_name':'', 'port':'', 'bits':[]}), #Map UART0 of Tester to external interface
        # ================================================================== SUT IO mappings ==================================================================
        # SUT UART0
        #({'corename':'SUT0', 'if_name':'UART0_rs232', 'port':'', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'', 'bits':[]}), #Map UART0 of SUT to UART1 of Tester
        # Python scripts do not yet support 'UART0_rs232'. Need to connect each signal independently
        ({'corename':'SUT0', 'if_name':'UART0', 'port':'rxd', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'txd', 'bits':[]}), 
        ({'corename':'SUT0', 'if_name':'UART0', 'port':'txd', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'rxd', 'bits':[]}), 
        ({'corename':'SUT0', 'if_name':'UART0', 'port':'cts', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'rts', 'bits':[]}), 
        ({'corename':'SUT0', 'if_name':'UART0', 'port':'rts', 'bits':[]},                    {'corename':'UART1', 'if_name':'rs232', 'port':'cts', 'bits':[]}), 
        # SUT ETHERNET0
        ####({'corename':'SUT0', 'if_name':'ETHERNET0_ethernet', 'port':'', 'bits':[]},         {'corename':'', 'if_name':'', 'port':'', 'bits':[]}), #Map ETHERNET0 of Tester to external interface
        ####({'corename':'SUT0', 'if_name':'ETHERNET1_ethernet', 'port':'', 'bits':[]},         {'corename':'ETHERNET0', 'if_name':'ethernet', 'port':'', 'bits':[]}), #Map ETHERNET0 of SUT to ETHERNET0 of Tester
        # SUT REGFILEIF0
        #({'corename':'SUT0', 'if_name':'REGFILEIF0_external_iob_s_port', 'port':'', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'external_iob_s_port', 'port':'', 'bits':[]}), #Map REGFILEIF of SUT to IOBNATIVEBRIDGEIF of Tester
        # Python scripts do not yet support 'REGFILEIF0_external_iob_s_port'. Need to connect each signal independently
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_avalid_i', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_avalid_o', 'bits':[]}),
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_addr_i', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_addr_o', 'bits':[]}),
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_wdata_i', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_wdata_o', 'bits':[]}),
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_wstrb_i', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_wstrb_o', 'bits':[]}),
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_rvalid_o', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_rvalid_i', 'bits':[]}),
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_rdata_o', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_rdata_i', 'bits':[]}),
        ({'corename':'SUT0', 'if_name':'REGFILEIF0', 'port':'external_iob_ready_o', 'bits':[]}, {'corename':'IOBNATIVEBRIDGEIF0', 'if_name':'iob_m_port', 'port':'iob_ready_i', 'bits':[]}),
    ],

    'confs':
    [
        # Override default values of Tester params
        #{'name':'BOOTROM_ADDR_W','type':'P', 'val':'13', 'min':'1', 'max':'32', 'descr':"Boot ROM address width"},
        {'name':'SRAM_ADDR_W',   'type':'P', 'val':'17', 'min':'1', 'max':'32', 'descr':"SRAM address width"},
    ],

    'sut_fw_name':name+'_firmware',

    # Note: Even though REGFILEIF is a module of the SUT, some os its files are needed for the IOBNATIVEBRIDGEIF, and they wont be generated by the SUT if we use the TESTER_ONLY argument, therefore we add the REGFILEIF configuration here aswell for it to be always generated with Tester
    'extra_submodules':{
        'hw_setup': {
            'headers':[],
#            'modules':[ 'ETHERNET', ('REGFILEIF',regfileif_options), 'IOBNATIVEBRIDGEIF' ],
            'modules':[ ('REGFILEIF',regfileif_options), 'IOBNATIVEBRIDGEIF' ],
        },
        'sw_setup': {
            'headers':[],
#            'modules':[ 'ETHERNET', ('REGFILEIF',regfileif_options) ],
            'modules':[ ('REGFILEIF',regfileif_options) ],
        },

    }
}

# ############### End of Tester configuration ###################

submodules = {
    'hw_setup': {
        'headers' : [ 'iob_wire', 'axi_wire', 'axi_m_m_portmap', 'axi_m_port', 'axi_m_m_portmap', 'axi_m_portmap' ],
        'modules': [ 'PICORV32', 'CACHE', 'UART', ('REGFILEIF',regfileif_options), 'iob_merge', 'iob_split', 'iob_rom_sp.v', 'iob_ram_dp_be.v', 'iob_ram_dp_be_xil.v', 'iob_pulse_gen.v', 'iob_counter.v', 'iob_ram_2p_asym.v', 'iob_reg.v', 'iob_reg_re.v', 'iob_ram_sp_be.v', 'iob_ram_dp.v', 'iob_reset_sync']
    },
    'sim_setup': {
        'headers' : [ 'axi_s_portmap' ],
        'modules': [ 'axi_ram.v', 'iob_tasks.vh' ]
    },
    'sw_setup': {
        'headers': [  ],
        'modules': [ 'CACHE', 'UART', ('REGFILEIF',regfileif_options) ]
    },
}

blocks = \
[
    {'name':'cpu', 'descr':'CPU module', 'blocks': [
        {'name':'cpu', 'descr':'PicoRV32 CPU'},
    ]},
    {'name':'bus_split', 'descr':'Split modules for buses', 'blocks': [
        {'name':'ibus_split', 'descr':'Split CPU instruction bus into internal and external memory buses'},
        {'name':'dbus_split', 'descr':'Split CPU data bus into internal and external memory buses'},
        {'name':'int_dbus_split', 'descr':'Split internal data bus into internal memory and peripheral buses'},
        {'name':'pbus_split', 'descr':'Split peripheral bus into a bus for each peripheral'},
    ]},
    {'name':'memories', 'descr':'Memory modules', 'blocks': [
        {'name':'int_mem0', 'descr':'Internal SRAM memory'},
        {'name':'ext_mem0', 'descr':'External DDR memory'},
    ]},
    {'name':'peripherals', 'descr':'peripheral modules', 'blocks': [
        {'name':'UART0', 'type':'UART', 'descr':'Default UART interface', 'params':{}},
        ##{'name':'ETHERNET0', 'type':'ETHERNET', 'descr':'Ethernet interface', 'params':{}},
        {'name':'REGFILEIF0', 'type':'REGFILEIF', 'descr':'Register file interface', 'params':{}},
    ]},
]

confs = \
[
    # macros
    {'name':'USE_MUL_DIV',   'type':'M', 'val':'1', 'min':'0', 'max':'1', 'descr':"Enable MUL and DIV CPU instructions"},
    {'name':'USE_COMPRESSED','type':'M', 'val':'1', 'min':'0', 'max':'1', 'descr':"Use compressed CPU instructions"},
    {'name':'E',             'type':'M', 'val':'31', 'min':'1', 'max':'32', 'descr':"Address selection bit for external memory"},
    {'name':'P',             'type':'M', 'val':'30', 'min':'1', 'max':'32', 'descr':"Address selection bit for peripherals"},
    {'name':'B',             'type':'M', 'val':'29', 'min':'1', 'max':'32', 'descr':"Address selection bit for boot ROM"},

    # parameters
    {'name':'BOOTROM_ADDR_W','type':'P', 'val':'12', 'min':'1', 'max':'32', 'descr':"Boot ROM address width"},
    {'name':'SRAM_ADDR_W',   'type':'P', 'val':'15', 'min':'1', 'max':'32', 'descr':"SRAM address width"},

    #mandatory parameters (do not change them!)
    {'name':'ADDR_W',        'type':'P', 'val':'32', 'min':'1', 'max':'32', 'descr':"Address bus width"},
    {'name':'DATA_W',        'type':'P', 'val':'32', 'min':'1', 'max':'32', 'descr':"Data bus width"},
    {'name':'AXI_ID_W',      'type':'P', 'val':'0', 'min':'1', 'max':'32', 'descr':"AXI ID bus width"},
    {'name':'AXI_ADDR_W',    'type':'P', 'val':'`MEM_ADDR_W', 'min':'1', 'max':'32', 'descr':"AXI address bus width"},
    {'name':'AXI_DATA_W',    'type':'P', 'val':'DATA_W', 'min':'1', 'max':'32', 'descr':"AXI data bus width"},
    {'name':'AXI_LEN_W',     'type':'P', 'val':'4', 'min':'1', 'max':'4', 'descr':"AXI burst length width"},
]

regs = [] 

ios = \
[
    {'name': 'general', 'descr':'General interface signals', 'ports': [
        {'name':"clk_i", 'type':"I", 'n_bits':'1', 'descr':"System clock input"},
        {'name':"arst_i", 'type':"I", 'n_bits':'1', 'descr':"System reset, synchronous and active high"},
        {'name':"trap_o", 'type':"O", 'n_bits':'1', 'descr':"CPU trap signal"}
    ]},
    {'name': 'axi_m_port', 'descr':'General interface signals', 'ports': [], 'if_defined':'USE_EXTMEM'},
]

# Add IOb-SoC modules. These will copy and generate common files from the IOb-SoC repository.
iob_soc.add_iob_soc_modules(sys.modules[__name__])

def custom_setup():
    # By default, don't setup with tester
    setup_with_tester = False

    # Add the following arguments:
    # "INIT_MEM": if should setup with init_mem or not
    # "USE_EXTMEM": if should setup with extmem or not
    # "TESTER": if should setup with tester or not
    for arg in sys.argv[1:]:
        if arg == "INIT_MEM":
            update_define(confs, "INIT_MEM",True)
        if arg == "USE_EXTMEM":
            update_define(confs, "USE_EXTMEM",True)
        if arg == "TESTER":
            setup_with_tester=True
        if arg == "TESTER_ONLY":
            only_setup_tester()

    # Add Tester as a hardware module
    if setup_with_tester: tester.add_tester_modules(sys.modules[__name__],tester_options)
    
    for conf in confs:
        if (conf['name'] == 'USE_EXTMEM') and conf['val']:
            submodules['hw_setup']['headers'].append({ 'file_prefix':'ddr4_', 'interface':'axi_wire', 'wire_prefix':'ddr4_', 'port_prefix':'ddr4_' })

# Setup the Tester without the SUT sources
def only_setup_tester():
    tester_module = import_setup(setup_dir+"/submodules/TESTER", module_parameters=tester_options)
    tester_module.main()
    exit(0)

# Main function to setup this system and its components
def main():
    custom_setup()
    # Setup this system
    setup.setup(sys.modules[__name__])

if __name__ == "__main__":
    main()
