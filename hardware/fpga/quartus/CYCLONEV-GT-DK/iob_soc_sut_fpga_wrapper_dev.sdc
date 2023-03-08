set clk_period 20.00
set clk_port clk

#create_clock -period 20.00 [get_ports {clk50}]
#create_clock -period 10.00 [get_ports {clk100}]

<<<<<<<< HEAD:hardware/fpga/quartus/CYCLONEV-GT-DK/iob_soc_sut_fpga_wrapper.sdc
# Ethernet RX_CLK is 25MHz for 100Mbps operation
create_clock -period 40 [get_ports {ENET_RX_CLK}]

derive_pll_clocks

derive_clock_uncertainty

========
>>>>>>>> f277ccd02840f87e22e31056592026588548bbc9:hardware/fpga/quartus/CYCLONEV-GT-DK/iob_soc_fpga_wrapper_dev.sdc
#set_max_delay -from [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_half_clk}] -to [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_clk}] 100
#set_max_delay -from [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_clk}] -to [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_half_clk}] 100
