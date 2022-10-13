create_clock -period 20.00 [get_ports {clk}]
#create_clock -period 20.00 [get_ports {clk50}]
#create_clock -period 10.00 [get_ports {clk100}]

# Ethernet RX_CLK is 25MHz for 100Mbps operation
create_clock -period 40 [get_ports {ENET_RX_CLK}]

derive_pll_clocks

derive_clock_uncertainty

#set_max_delay -from [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_half_clk}] -to [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_clk}] 100
#set_max_delay -from [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_clk}] -to [get_clocks {ddr3_ctrl|mem_if_ddr3_emif_0|pll0|pll_afi_half_clk}] 100
