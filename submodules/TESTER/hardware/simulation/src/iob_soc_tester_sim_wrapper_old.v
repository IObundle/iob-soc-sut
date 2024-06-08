/*
`timescale 1ns / 1ps

`include "bsp.vh"
`include "iob_soc_tester_conf.vh"
`include "iob_uart_conf.vh"
`include "iob_uart_swreg_def.vh"

//Peripherals _swreg_def.vh file includes.
`include "iob_uart16550_swreg_def.vh"
`include "iob_soc_sut_swreg_def.vh"
`include "iob_gpio_swreg_def.vh"
`include "iob_axistream_in_swreg_def.vh"
`include "iob_axistream_out_swreg_def.vh"
`include "iob_ila_swreg_def.vh"
`include "iob_pfsm_swreg_def.vh"
`include "iob_dma_swreg_def.vh"
`include "iob_eth_swreg_def.vh"
`include "iob_timer_swreg_def.vh"
`include "iob_spi_master_swreg_def.vh"
`include "iob_versat_swreg_def.vh"

`ifndef IOB_ETH_SWREG_ADDR_W
`define IOB_ETH_SWREG_ADDR_W 12
`endif

module iob_soc_tester_sim_wrapper (
input [1-1:0] clk_i,
input [1-1:0] arst_i,
   output                             trap_o,

`ifdef IOB_SOC_TESTER_USE_ETHERNET
   // Ethernet for testbench
   input                                        ethernet_valid_i,
   input [`IOB_ETH_SWREG_ADDR_W-1:0]            ethernet_addr_i,
   input [`IOB_SOC_TESTER_DATA_W-1:0]  ethernet_wdata_i,
   input [3:0]                                  ethernet_wstrb_i,
   output [`IOB_SOC_TESTER_DATA_W-1:0] ethernet_rdata_o,
   output                                       ethernet_ready_o,
   output                                       ethernet_rvalid_o,
`endif

   // UART for testbench
   input                                        uart_valid_i,
   input [`IOB_UART_SWREG_ADDR_W-1:0]           uart_addr_i,
   input [`IOB_SOC_TESTER_DATA_W-1:0]  uart_wdata_i,
   input [3:0]                                  uart_wstrb_i,
   output [`IOB_SOC_TESTER_DATA_W-1:0] uart_rdata_o,
   output                                       uart_ready_o,
   output                                       uart_rvalid_o
);

   localparam AXI_ID_W = 4;
   localparam AXI_LEN_W = 8;
   localparam AXI_ADDR_W = `DDR_ADDR_W;
   localparam AXI_DATA_W = `DDR_DATA_W;

   wire uart_txd_o;
   wire uart_rxd_i;
   wire uart_cts_i;
   wire uart_rts_o;
   wire ETH0_MTxClk;
   wire [4-1:0] ETH0_MTxD;
   wire ETH0_MTxEn;
   wire ETH0_MTxErr;
   wire ETH0_MRxClk;
   wire ETH0_MRxDv;
   wire [4-1:0] ETH0_MRxD;
   wire ETH0_MRxErr;
   wire ETH0_MColl;
   wire ETH0_MCrS;
   wire ETH0_MDC;
   wire ETH0_MDIO;
   wire ETH0_phy_rstn_o;
   wire spi_SS;
   wire spi_SCLK;
   wire spi_MISO;
   wire spi_MOSI;
   wire spi_WP_N;
   wire spi_HOLD_N;
   wire [`IOB_SOC_TESTER_GPIO0_GPIO_W-1:0] GPIO_output_enable;
   wire [`IOB_SOC_TESTER_SUT0_GPIO0_GPIO_W-1:0] SUT_GPIO0_output_enable;

   //DDR AXI interface signals
`ifdef IOB_SOC_TESTER_USE_EXTMEM
   // Wires for the system and its peripherals
wire [8*AXI_ID_W-1:0] axi_awid; //Address write channel ID.
wire [8*AXI_ADDR_W-1:0] axi_awaddr; //Address write channel address.
wire [8*AXI_LEN_W-1:0] axi_awlen; //Address write channel burst length.
wire [8*3-1:0] axi_awsize; //Address write channel burst size. This signal indicates the size of each transfer in the burst.
wire [8*2-1:0] axi_awburst; //Address write channel burst type.
wire [8*2-1:0] axi_awlock; //Address write channel lock type.
wire [8*4-1:0] axi_awcache; //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
wire [8*3-1:0] axi_awprot; //Address write channel protection type. Set to 000 if master output; ignored if slave input.
wire [8*4-1:0] axi_awqos; //Address write channel quality of service.
wire [8*1-1:0] axi_awvalid; //Address write channel valid.
wire [8*1-1:0] axi_awready; //Address write channel ready.
wire [8*AXI_DATA_W-1:0] axi_wdata; //Write channel data.
wire [8*(AXI_DATA_W/8)-1:0] axi_wstrb; //Write channel write strobe.
wire [8*1-1:0] axi_wlast; //Write channel last word flag.
wire [8*1-1:0] axi_wvalid; //Write channel valid.
wire [8*1-1:0] axi_wready; //Write channel ready.
wire [8*AXI_ID_W-1:0] axi_bid; //Write response channel ID.
wire [8*2-1:0] axi_bresp; //Write response channel response.
wire [8*1-1:0] axi_bvalid; //Write response channel valid.
wire [8*1-1:0] axi_bready; //Write response channel ready.
wire [8*AXI_ID_W-1:0] axi_arid; //Address read channel ID.
wire [8*AXI_ADDR_W-1:0] axi_araddr; //Address read channel address.
wire [8*AXI_LEN_W-1:0] axi_arlen; //Address read channel burst length.
wire [8*3-1:0] axi_arsize; //Address read channel burst size. This signal indicates the size of each transfer in the burst.
wire [8*2-1:0] axi_arburst; //Address read channel burst type.
wire [8*2-1:0] axi_arlock; //Address read channel lock type.
wire [8*4-1:0] axi_arcache; //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
wire [8*3-1:0] axi_arprot; //Address read channel protection type. Set to 000 if master output; ignored if slave input.
wire [8*4-1:0] axi_arqos; //Address read channel quality of service.
wire [8*1-1:0] axi_arvalid; //Address read channel valid.
wire [8*1-1:0] axi_arready; //Address read channel ready.
wire [8*AXI_ID_W-1:0] axi_rid; //Read channel ID.
wire [8*AXI_DATA_W-1:0] axi_rdata; //Read channel data.
wire [8*2-1:0] axi_rresp; //Read channel response.
wire [8*1-1:0] axi_rlast; //Read channel last word.
wire [8*1-1:0] axi_rvalid; //Read channel valid.
wire [8*1-1:0] axi_rready; //Read channel ready.
   // Wires to connect the interconnect with the memory
wire [AXI_ID_W-1:0] memory_axi_awid; //Address write channel ID.
wire [AXI_ADDR_W-1:0] memory_axi_awaddr; //Address write channel address.
wire [AXI_LEN_W-1:0] memory_axi_awlen; //Address write channel burst length.
wire [3-1:0] memory_axi_awsize; //Address write channel burst size. This signal indicates the size of each transfer in the burst.
wire [2-1:0] memory_axi_awburst; //Address write channel burst type.
wire [2-1:0] memory_axi_awlock; //Address write channel lock type.
wire [4-1:0] memory_axi_awcache; //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
wire [3-1:0] memory_axi_awprot; //Address write channel protection type. Set to 000 if master output; ignored if slave input.
wire [4-1:0] memory_axi_awqos; //Address write channel quality of service.
wire [1-1:0] memory_axi_awvalid; //Address write channel valid.
wire [1-1:0] memory_axi_awready; //Address write channel ready.
wire [AXI_DATA_W-1:0] memory_axi_wdata; //Write channel data.
wire [(AXI_DATA_W/8)-1:0] memory_axi_wstrb; //Write channel write strobe.
wire [1-1:0] memory_axi_wlast; //Write channel last word flag.
wire [1-1:0] memory_axi_wvalid; //Write channel valid.
wire [1-1:0] memory_axi_wready; //Write channel ready.
wire [AXI_ID_W-1:0] memory_axi_bid; //Write response channel ID.
wire [2-1:0] memory_axi_bresp; //Write response channel response.
wire [1-1:0] memory_axi_bvalid; //Write response channel valid.
wire [1-1:0] memory_axi_bready; //Write response channel ready.
wire [AXI_ID_W-1:0] memory_axi_arid; //Address read channel ID.
wire [AXI_ADDR_W-1:0] memory_axi_araddr; //Address read channel address.
wire [AXI_LEN_W-1:0] memory_axi_arlen; //Address read channel burst length.
wire [3-1:0] memory_axi_arsize; //Address read channel burst size. This signal indicates the size of each transfer in the burst.
wire [2-1:0] memory_axi_arburst; //Address read channel burst type.
wire [2-1:0] memory_axi_arlock; //Address read channel lock type.
wire [4-1:0] memory_axi_arcache; //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
wire [3-1:0] memory_axi_arprot; //Address read channel protection type. Set to 000 if master output; ignored if slave input.
wire [4-1:0] memory_axi_arqos; //Address read channel quality of service.
wire [1-1:0] memory_axi_arvalid; //Address read channel valid.
wire [1-1:0] memory_axi_arready; //Address read channel ready.
wire [AXI_ID_W-1:0] memory_axi_rid; //Read channel ID.
wire [AXI_DATA_W-1:0] memory_axi_rdata; //Read channel data.
wire [2-1:0] memory_axi_rresp; //Read channel response.
wire [1-1:0] memory_axi_rlast; //Read channel last word.
wire [1-1:0] memory_axi_rvalid; //Read channel valid.
wire [1-1:0] memory_axi_rready; //Read channel ready.
`endif

`include "iob_regfileif_inverted_swreg_def.vh"
                

   /////////////////////////////////////////////
   // TEST PROCEDURE
   //
   initial begin
`ifdef VCD
      $dumpfile("uut.fst");
      $dumpvars();
`endif
   end

   //
   // INSTANTIATE COMPONENTS
   //

   //
   // IOb-SoC-OpenCryptoLinux (may also include Unit Under Test)
   //
   iob_soc_tester #(
      .AXI_ID_W  (AXI_ID_W),
      .AXI_LEN_W (AXI_LEN_W),
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W)
   ) soc0 (
               .uart_txd_o(uart_txd_o),
               .uart_rxd_i(uart_rxd_i),
               .uart_cts_i(uart_cts_i),
               .uart_rts_o(uart_rts_o),
               .ETH0_MTxClk(ETH0_MTxClk),
               .ETH0_MTxD(ETH0_MTxD),
               .ETH0_MTxEn(ETH0_MTxEn),
               .ETH0_MTxErr(ETH0_MTxErr),
               .ETH0_MRxClk(ETH0_MRxClk),
               .ETH0_MRxDv(ETH0_MRxDv),
               .ETH0_MRxD(ETH0_MRxD),
               .ETH0_MRxErr(ETH0_MRxErr),
               .ETH0_MColl(ETH0_MColl),
               .ETH0_MCrS(ETH0_MCrS),
               .ETH0_MDC(ETH0_MDC),
               .ETH0_MDIO(ETH0_MDIO),
               .ETH0_phy_rstn_o(ETH0_phy_rstn_o),
               .spi_SS(spi_SS),
               .spi_SCLK(spi_SCLK),
               .spi_MISO(spi_MISO),
               .spi_MOSI(spi_MOSI),
               .spi_WP_N(spi_WP_N),
               .spi_HOLD_N(spi_HOLD_N),
               .GPIO_output_enable(GPIO_output_enable),
               .SUT_GPIO0_output_enable(SUT_GPIO0_output_enable),

`ifdef IOB_SOC_TESTER_USE_EXTMEM
.axi_awid_o(axi_awid[0+:8*AXI_ID_W]), //Address write channel ID.
.axi_awaddr_o(axi_awaddr[0+:8*AXI_ADDR_W]), //Address write channel address.
.axi_awlen_o(axi_awlen[0+:8*AXI_LEN_W]), //Address write channel burst length.
.axi_awsize_o(axi_awsize[0+:8*3]), //Address write channel burst size. This signal indicates the size of each transfer in the burst.
.axi_awburst_o(axi_awburst[0+:8*2]), //Address write channel burst type.
.axi_awlock_o(axi_awlock[0+:8*2]), //Address write channel lock type.
.axi_awcache_o(axi_awcache[0+:8*4]), //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
.axi_awprot_o(axi_awprot[0+:8*3]), //Address write channel protection type. Set to 000 if master output; ignored if slave input.
.axi_awqos_o(axi_awqos[0+:8*4]), //Address write channel quality of service.
.axi_awvalid_o(axi_awvalid[0+:8*1]), //Address write channel valid.
.axi_awready_i(axi_awready[0+:8*1]), //Address write channel ready.
.axi_wdata_o(axi_wdata[0+:8*AXI_DATA_W]), //Write channel data.
.axi_wstrb_o(axi_wstrb[0+:8*(AXI_DATA_W/8)]), //Write channel write strobe.
.axi_wlast_o(axi_wlast[0+:8*1]), //Write channel last word flag.
.axi_wvalid_o(axi_wvalid[0+:8*1]), //Write channel valid.
.axi_wready_i(axi_wready[0+:8*1]), //Write channel ready.
.axi_bid_i(axi_bid[0+:8*AXI_ID_W]), //Write response channel ID.
.axi_bresp_i(axi_bresp[0+:8*2]), //Write response channel response.
.axi_bvalid_i(axi_bvalid[0+:8*1]), //Write response channel valid.
.axi_bready_o(axi_bready[0+:8*1]), //Write response channel ready.
.axi_arid_o(axi_arid[0+:8*AXI_ID_W]), //Address read channel ID.
.axi_araddr_o(axi_araddr[0+:8*AXI_ADDR_W]), //Address read channel address.
.axi_arlen_o(axi_arlen[0+:8*AXI_LEN_W]), //Address read channel burst length.
.axi_arsize_o(axi_arsize[0+:8*3]), //Address read channel burst size. This signal indicates the size of each transfer in the burst.
.axi_arburst_o(axi_arburst[0+:8*2]), //Address read channel burst type.
.axi_arlock_o(axi_arlock[0+:8*2]), //Address read channel lock type.
.axi_arcache_o(axi_arcache[0+:8*4]), //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
.axi_arprot_o(axi_arprot[0+:8*3]), //Address read channel protection type. Set to 000 if master output; ignored if slave input.
.axi_arqos_o(axi_arqos[0+:8*4]), //Address read channel quality of service.
.axi_arvalid_o(axi_arvalid[0+:8*1]), //Address read channel valid.
.axi_arready_i(axi_arready[0+:8*1]), //Address read channel ready.
.axi_rid_i(axi_rid[0+:8*AXI_ID_W]), //Read channel ID.
.axi_rdata_i(axi_rdata[0+:8*AXI_DATA_W]), //Read channel data.
.axi_rresp_i(axi_rresp[0+:8*2]), //Read channel response.
.axi_rlast_i(axi_rlast[0+:8*1]), //Read channel last word.
.axi_rvalid_i(axi_rvalid[0+:8*1]), //Read channel valid.
.axi_rready_o(axi_rready[0+:8*1]), //Read channel ready.
`endif
      .clk_i (clk_i),
      .cke_i (1'b1),
      .arst_i(arst_i),
      .trap_o(trap_o)
   );


    // interconnect clk and arst
    wire clk_interconnect;
    wire arst_interconnect;
    assign clk_interconnect = clk_i;
    assign arst_interconnect = arst_i;


`ifdef IOB_SOC_TESTER_USE_EXTMEM
   //instantiate axi interconnect
   axi_interconnect #(
      .ID_WIDTH    (AXI_ID_W),
      .DATA_WIDTH  (AXI_DATA_W),
      .ADDR_WIDTH  (AXI_ADDR_W),
      .M_ADDR_WIDTH(AXI_ADDR_W),
      .S_COUNT     (8),
      .M_COUNT     (1)
   ) system_axi_interconnect (
      .clk(clk_interconnect),
      .rst(arst_interconnect),

      // Need to use manually defined connections because awlock and arlock of interconnect is only on bit for each slave
      .s_axi_awid    (axi_awid),    //Address write channel ID.
      .s_axi_awaddr  (axi_awaddr),  //Address write channel address.
      .s_axi_awlen   (axi_awlen),   //Address write channel burst length.
      .s_axi_awsize  (axi_awsize),  //Address write channel burst size. This signal indicates the size of each transfer in the burst.
      .s_axi_awburst (axi_awburst), //Address write channel burst type.
      .s_axi_awlock  ({ axi_awlock[14], axi_awlock[12], axi_awlock[10], axi_awlock[8], axi_awlock[6], axi_awlock[4], axi_awlock[2], axi_awlock[0] }),//Address write channel lock type.
      .s_axi_awcache (axi_awcache), //Address write channel memory type. Transactions set with Normal, Non-cacheable, Modifiable, and Bufferable (0011).
      .s_axi_awprot  (axi_awprot),  //Address write channel protection type. Transactions set with Normal, Secure, and Data attributes (000).
      .s_axi_awqos   (axi_awqos),   //Address write channel quality of service.
      .s_axi_awvalid (axi_awvalid), //Address write channel valid.
      .s_axi_awready (axi_awready), //Address write channel ready.
      .s_axi_wdata   (axi_wdata),   //Write channel data.
      .s_axi_wstrb   (axi_wstrb),   //Write channel write strobe.
      .s_axi_wlast   (axi_wlast),   //Write channel last word flag.
      .s_axi_wvalid  (axi_wvalid),  //Write channel valid.
      .s_axi_wready  (axi_wready),  //Write channel ready.
      .s_axi_bid     (axi_bid),     //Write response channel ID.
      .s_axi_bresp   (axi_bresp),   //Write response channel response.
      .s_axi_bvalid  (axi_bvalid),  //Write response channel valid.
      .s_axi_bready  (axi_bready),  //Write response channel ready.
      .s_axi_arid    (axi_arid),    //Address read channel ID.
      .s_axi_araddr  (axi_araddr),  //Address read channel address.
      .s_axi_arlen   (axi_arlen),   //Address read channel burst length.
      .s_axi_arsize  (axi_arsize),  //Address read channel burst size. This signal indicates the size of each transfer in the burst.
      .s_axi_arburst (axi_arburst), //Address read channel burst type.
      .s_axi_arlock  ({ axi_arlock[14], axi_arlock[12], axi_arlock[10], axi_arlock[8], axi_arlock[6], axi_arlock[4], axi_arlock[2], axi_arlock[0] }),//Address read channel lock type.
      .s_axi_arcache (axi_arcache), //Address read channel memory type. Transactions set with Normal, Non-cacheable, Modifiable, and Bufferable (0011).
      .s_axi_arprot  (axi_arprot),  //Address read channel protection type. Transactions set with Normal, Secure, and Data attributes (000).
      .s_axi_arqos   (axi_arqos),   //Address read channel quality of service.
      .s_axi_arvalid (axi_arvalid), //Address read channel valid.
      .s_axi_arready (axi_arready), //Address read channel ready.
      .s_axi_rid     (axi_rid),     //Read channel ID.
      .s_axi_rdata   (axi_rdata),   //Read channel data.
      .s_axi_rresp   (axi_rresp),   //Read channel response.
      .s_axi_rlast   (axi_rlast),   //Read channel last word.
      .s_axi_rvalid  (axi_rvalid),  //Read channel valid.
      .s_axi_rready  (axi_rready),  //Read channel ready.

      // Used manually defined connections because awlock and arlock of interconnect is only on bit.
      .m_axi_awid    (memory_axi_awid[0+:AXI_ID_W]),         //Address write channel ID.
      .m_axi_awaddr  (memory_axi_awaddr[0+:AXI_ADDR_W]),     //Address write channel address.
      .m_axi_awlen   (memory_axi_awlen[0+:AXI_LEN_W]),       //Address write channel burst length.
      .m_axi_awsize  (memory_axi_awsize[0+:3]),              //Address write channel burst size. This signal indicates the size of each transfer in the burst.
      .m_axi_awburst (memory_axi_awburst[0+:2]),             //Address write channel burst type.
      .m_axi_awlock  (memory_axi_awlock[0+:1]),              //Address write channel lock type.
      .m_axi_awcache (memory_axi_awcache[0+:4]),             //Address write channel memory type. Transactions set with Normal, Non-cacheable, Modifiable, and Bufferable (0011).
      .m_axi_awprot  (memory_axi_awprot[0+:3]),              //Address write channel protection type. Transactions set with Normal, Secure, and Data attributes (000).
      .m_axi_awqos   (memory_axi_awqos[0+:4]),               //Address write channel quality of service.
      .m_axi_awvalid (memory_axi_awvalid[0+:1]),             //Address write channel valid.
      .m_axi_awready (memory_axi_awready[0+:1]),             //Address write channel ready.
      .m_axi_wdata   (memory_axi_wdata[0+:AXI_DATA_W]),      //Write channel data.
      .m_axi_wstrb   (memory_axi_wstrb[0+:(AXI_DATA_W/8)]),  //Write channel write strobe.
      .m_axi_wlast   (memory_axi_wlast[0+:1]),               //Write channel last word flag.
      .m_axi_wvalid  (memory_axi_wvalid[0+:1]),              //Write channel valid.
      .m_axi_wready  (memory_axi_wready[0+:1]),              //Write channel ready.
      .m_axi_bid     (memory_axi_bid[0+:AXI_ID_W]),          //Write response channel ID.
      .m_axi_bresp   (memory_axi_bresp[0+:2]),               //Write response channel response.
      .m_axi_bvalid  (memory_axi_bvalid[0+:1]),              //Write response channel valid.
      .m_axi_bready  (memory_axi_bready[0+:1]),              //Write response channel ready.
      .m_axi_arid    (memory_axi_arid[0+:AXI_ID_W]),         //Address read channel ID.
      .m_axi_araddr  (memory_axi_araddr[0+:AXI_ADDR_W]),     //Address read channel address.
      .m_axi_arlen   (memory_axi_arlen[0+:AXI_LEN_W]),       //Address read channel burst length.
      .m_axi_arsize  (memory_axi_arsize[0+:3]),              //Address read channel burst size. This signal indicates the size of each transfer in the burst.
      .m_axi_arburst (memory_axi_arburst[0+:2]),             //Address read channel burst type.
      .m_axi_arlock  (memory_axi_arlock[0+:1]),              //Address read channel lock type.
      .m_axi_arcache (memory_axi_arcache[0+:4]),             //Address read channel memory type. Transactions set with Normal, Non-cacheable, Modifiable, and Bufferable (0011).
      .m_axi_arprot  (memory_axi_arprot[0+:3]),              //Address read channel protection type. Transactions set with Normal, Secure, and Data attributes (000).
      .m_axi_arqos   (memory_axi_arqos[0+:4]),               //Address read channel quality of service.
      .m_axi_arvalid (memory_axi_arvalid[0+:1]),             //Address read channel valid.
      .m_axi_arready (memory_axi_arready[0+:1]),             //Address read channel ready.
      .m_axi_rid     (memory_axi_rid[0+:AXI_ID_W]),          //Read channel ID.
      .m_axi_rdata   (memory_axi_rdata[0+:AXI_DATA_W]),      //Read channel data.
      .m_axi_rresp   (memory_axi_rresp[0+:2]),               //Read channel response.
      .m_axi_rlast   (memory_axi_rlast[0+:1]),               //Read channel last word.
      .m_axi_rvalid  (memory_axi_rvalid[0+:1]),              //Read channel valid.
      .m_axi_rready  (memory_axi_rready[0+:1]),              //Read channel ready.

      //optional signals
      .s_axi_awuser(8'b0),
      .s_axi_wuser (8'b0),
      .s_axi_aruser(8'b0),
      .s_axi_buser (),
      .s_axi_ruser (),
      .m_axi_awuser(),
      .m_axi_wuser (),
      .m_axi_aruser(),
      .m_axi_buser (1'b0),
      .m_axi_ruser (1'b0),
      .m_axi_awregion (),
      .m_axi_arregion ()
   );
`endif


   //instantiate the axi memory
   //IOb-SoC-OpenCryptoLinux and SUT access the same memory.
   axi_ram #(
`ifdef IOB_SOC_TESTER_INIT_MEM
      .FILE      ("init_ddr_contents"),  //This file contains firmware for both systems
`endif
      .ID_WIDTH  (AXI_ID_W),
      .DATA_WIDTH(AXI_DATA_W),
      .ADDR_WIDTH(AXI_ADDR_W)
   ) ddr_model_mem (
.axi_awid_i(memory_axi_awid), //Address write channel ID.
.axi_awaddr_i(memory_axi_awaddr), //Address write channel address.
.axi_awlen_i(memory_axi_awlen), //Address write channel burst length.
.axi_awsize_i(memory_axi_awsize), //Address write channel burst size. This signal indicates the size of each transfer in the burst.
.axi_awburst_i(memory_axi_awburst), //Address write channel burst type.
.axi_awlock_i(memory_axi_awlock), //Address write channel lock type.
.axi_awcache_i(memory_axi_awcache), //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
.axi_awprot_i(memory_axi_awprot), //Address write channel protection type. Set to 000 if master output; ignored if slave input.
.axi_awqos_i(memory_axi_awqos), //Address write channel quality of service.
.axi_awvalid_i(memory_axi_awvalid), //Address write channel valid.
.axi_awready_o(memory_axi_awready), //Address write channel ready.
.axi_wdata_i(memory_axi_wdata), //Write channel data.
.axi_wstrb_i(memory_axi_wstrb), //Write channel write strobe.
.axi_wlast_i(memory_axi_wlast), //Write channel last word flag.
.axi_wvalid_i(memory_axi_wvalid), //Write channel valid.
.axi_wready_o(memory_axi_wready), //Write channel ready.
.axi_bid_o(memory_axi_bid), //Write response channel ID.
.axi_bresp_o(memory_axi_bresp), //Write response channel response.
.axi_bvalid_o(memory_axi_bvalid), //Write response channel valid.
.axi_bready_i(memory_axi_bready), //Write response channel ready.
.axi_arid_i(memory_axi_arid), //Address read channel ID.
.axi_araddr_i(memory_axi_araddr), //Address read channel address.
.axi_arlen_i(memory_axi_arlen), //Address read channel burst length.
.axi_arsize_i(memory_axi_arsize), //Address read channel burst size. This signal indicates the size of each transfer in the burst.
.axi_arburst_i(memory_axi_arburst), //Address read channel burst type.
.axi_arlock_i(memory_axi_arlock), //Address read channel lock type.
.axi_arcache_i(memory_axi_arcache), //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
.axi_arprot_i(memory_axi_arprot), //Address read channel protection type. Set to 000 if master output; ignored if slave input.
.axi_arqos_i(memory_axi_arqos), //Address read channel quality of service.
.axi_arvalid_i(memory_axi_arvalid), //Address read channel valid.
.axi_arready_o(memory_axi_arready), //Address read channel ready.
.axi_rid_o(memory_axi_rid), //Read channel ID.
.axi_rdata_o(memory_axi_rdata), //Read channel data.
.axi_rresp_o(memory_axi_rresp), //Read channel response.
.axi_rlast_o(memory_axi_rlast), //Read channel last word.
.axi_rvalid_o(memory_axi_rvalid), //Read channel valid.
.axi_rready_i(memory_axi_rready), //Read channel ready.

      .clk_i(clk_i),
      .rst_i(arst_i)
   );

   //Manually added testbench uart core. RS232 pins attached to the same pins
   //of the iob_soc UART0 instance to communicate with it
   // The interface of iob_soc UART0 is assumed to be the first portmapped interface (UART_*)
   iob_uart uart_tb (
      .clk_i (clk_i),
      .cke_i (1'b1),
      .arst_i(arst_i),

      .iob_valid_i(uart_valid_i),
      .iob_addr_i  (uart_addr_i),
      .iob_wdata_i (uart_wdata_i),
      .iob_wstrb_i (uart_wstrb_i),
      .iob_rdata_o (uart_rdata_o),
      .iob_rvalid_o(uart_rvalid_o),
      .iob_ready_o (uart_ready_o),

      .txd_o(uart_rxd_i),
      .rxd_i(uart_txd_o),
      .rts_o(uart_cts_i),
      .cts_i(uart_rts_o)
   );

   //Ethernet
`ifdef IOB_SOC_TESTER_USE_ETHERNET
   //ethernet clock: 4x slower than system clock
   reg [1:0] eth_cnt = 2'b0;
   reg       eth_clk;

   always @(posedge clk_i) begin
      eth_cnt <= eth_cnt + 1'b1;
      eth_clk <= eth_cnt[1];
   end

   // Ethernet Interface signals
   assign ETH0_MRxClk     = eth_clk;
   assign ETH0_MTxClk     = eth_clk;

   //Manually added testbench ethernet core. MII pins attached to the same pins
   //of the iob_soc ETH0 instance to communicate with it
   // The interface of iob_soc ETH0 is assumed to be the first portmapped interface (ETH_*)
   iob_eth
     #(
      .AXI_ID_W(AXI_ID_W),
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W),
      .AXI_LEN_W(AXI_LEN_W)
   ) eth_tb (
      .inta_o(),
      .MTxClk(eth_clk),
      .MTxD(ETH0_MRxD),
      .MTxEn(ETH0_MRxDv),
      .MTxErr(ETH0_MRxErr),
      .MRxClk(eth_clk),
      .MRxDv(ETH0_MTxEn),
      .MRxD(ETH0_MTxD),
      .MRxErr(ETH0_MTxErr),
      .MColl(1'b0),
      .MCrS(1'b0),
      .MDC(),
      .MDIO(),
      .iob_valid_i(ethernet_valid_i),
      .iob_addr_i  (ethernet_addr_i),
      .iob_wdata_i (ethernet_wdata_i),
      .iob_wstrb_i (ethernet_wstrb_i),
      .iob_rvalid_o(ethernet_rvalid_o),
      .iob_rdata_o (ethernet_rdata_o),
      .iob_ready_o (ethernet_ready_o),
      .axi_awid_o        (),
      .axi_awaddr_o      (),
      .axi_awlen_o       (),
      .axi_awsize_o      (),
      .axi_awburst_o     (),
      .axi_awlock_o      (),
      .axi_awcache_o     (),
      .axi_awprot_o      (),
      .axi_awqos_o       (),
      .axi_awvalid_o     (),
      .axi_awready_i     (1'b0),
      .axi_wdata_o       (),
      .axi_wstrb_o       (),
      .axi_wlast_o       (),
      .axi_wvalid_o      (),
      .axi_wready_i      (1'b0),
      .axi_bid_i         ({AXI_ID_W{1'b0}}),
      .axi_bresp_i       (2'b0),
      .axi_bvalid_i      (1'b0),
      .axi_bready_o      (),
      .axi_arid_o        (),
      .axi_araddr_o      (),
      .axi_arlen_o       (),
      .axi_arsize_o      (),
      .axi_arburst_o     (),
      .axi_arlock_o      (),
      .axi_arcache_o     (),
      .axi_arprot_o      (),
      .axi_arqos_o       (),
      .axi_arvalid_o     (),
      .axi_arready_i     (1'b0),
      .axi_rid_i         ({AXI_ID_W{1'b0}}),
      .axi_rdata_i       ({AXI_DATA_W{1'b0}}),
      .axi_rresp_i       (2'b0),
      .axi_rlast_i       (1'b0),
      .axi_rvalid_i      (1'b0),
      .axi_rready_o      (),
      .clk_i(clk_i),
      .arst_i(arst_i),
      .cke_i(1'b1)
      );
`endif

`ifndef VERILATOR
   wire [31:0] Vcc = 'd1800;
   N25Qxxx flash_memory (
      .S(spi_SS),
      .C_(spi_SCLK),
      .HOLD_DQ3(spi_HOLD_N),
      .DQ0(spi_MOSI),
      .DQ1(spi_MISO),
      .Vcc(Vcc),
      .Vpp_W_DQ2(spi_WP_N)
   );
`endif // `ifndef VERILATOR

endmodule
*/