`timescale 1ns / 1ps
`include "bsp.vh"
`include "iob_soc_tester_conf.vh"

module iob_soc_tester_fpga_wrapper (

   //differential clock input and reset
   input c0_sys_clk_clk_p,
   input c0_sys_clk_clk_n,
   input reset,

   //uart
   output txd_o,
   input  rxd_i,

   //spi0
   output spi_SS_o,
   output spi_SCLK_o,
   inout spi_MISO_io,
   inout spi_MOSI_io,
   inout spi_WP_N_io,
   inout spi_HOLD_N_io,

   //spi1
   output SPI1_SS_o,
   output SPI1_SCLK_o,
   inout SPI1_MISO_io,
   inout SPI1_MOSI_io,
   inout SPI1_WP_N_io,
   inout SPI1_HOLD_N_io,


   // SUT spi
   output sut_spi_SS_o,
   output sut_spi_SCLK_o,
   inout sut_spi_MISO_io,
   inout sut_spi_MOSI_io,
   inout sut_spi_WP_N_io,
   inout sut_spi_HOLD_N_io,


`ifdef IOB_SOC_TESTER_USE_EXTMEM
   output        c0_ddr4_act_n,
   output [16:0] c0_ddr4_adr,
   output [ 1:0] c0_ddr4_ba,
   output [ 0:0] c0_ddr4_bg,
   output [ 0:0] c0_ddr4_cke,
   output [ 0:0] c0_ddr4_odt,
   output [ 0:0] c0_ddr4_cs_n,
   output [ 0:0] c0_ddr4_ck_t,
   output [ 0:0] c0_ddr4_ck_c,
   output        c0_ddr4_reset_n,
   inout  [ 3:0] c0_ddr4_dm_dbi_n,
   inout  [31:0] c0_ddr4_dq,
   inout  [ 3:0] c0_ddr4_dqs_c,
   inout  [ 3:0] c0_ddr4_dqs_t,
`endif

`ifdef IOB_SOC_TESTER_USE_ETHERNET
   output ENET_RESETN,
   input  ENET_RX_CLK,
   output ENET_GTX_CLK,
   input  ENET_RX_D0,
   input  ENET_RX_D1,
   input  ENET_RX_D2,
   input  ENET_RX_D3,
   input  ENET_RX_DV,
   //input  ENET_RX_ERR,
   output ENET_TX_D0,
   output ENET_TX_D1,
   output ENET_TX_D2,
   output ENET_TX_D3,
   output ENET_TX_EN,
   //output ENET_TX_ERR,
`endif

   output trap
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
   wire SUT0_SPI0_SS_spi_SS;
   wire SUT0_SPI0_SCLK_spi_SCLK;
   wire SUT0_SPI0_MISO_spi_MISO;
   wire SUT0_SPI0_MOSI_spi_MOSI;
   wire SUT0_SPI0_WP_N_spi_WP_N;
   wire SUT0_SPI0_HOLD_N_spi_HOLD_N;

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

   wire clk;
   wire arst;


   // 
   // Logic to contatenate data pins and ethernet clock
   //
`ifdef IOB_SOC_TESTER_USE_ETHERNET
   //buffered eth clock
   wire       ETH_Clk;

   //eth clock
   BUFGCE_1 rxclk_buf (
      .I(ENET_RX_CLK),
      .CE(1'b1),
      .O(ETH_Clk)
   );
   ODDRE1 ODDRE1_inst (
      .Q (ENET_GTX_CLK),
      .C (ETH_Clk),
      .D1(1'b1),
      .D2(1'b0),
      .SR(~ENET_RESETN)
   );

   //MII
   assign ETH0_MRxClk = ETH_Clk;
   assign ETH0_MRxD = {ENET_RX_D3, ENET_RX_D2, ENET_RX_D1, ENET_RX_D0};
   assign ETH0_MRxDv = ENET_RX_DV;
   //assign ETH0_MRxErr = ENET_RX_ERR;
   assign ETH0_MRxErr = 1'b0;

   assign ETH0_MTxClk = ETH_Clk;
   assign {ENET_TX_D3, ENET_TX_D2, ENET_TX_D1, ENET_TX_D0} = ETH0_MTxD;
   assign ENET_TX_EN = ETH0_MTxEn;
   //assign ENET_TX_ERR = ETH0_MTxErr;

   assign ENET_RESETN = ETH0_phy_rstn_o;

   assign ETH0_MColl = 1'b0;
   assign ETH0_MCrS = 1'b0;
`endif


   //
   // IOb-SoC
   //

   iob_soc_tester #(
      .AXI_ID_W  (AXI_ID_W),
      .AXI_LEN_W (AXI_LEN_W),
      .AXI_ADDR_W(AXI_ADDR_W),
      .AXI_DATA_W(AXI_DATA_W)
   ) iob_soc_tester0 (
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
               .SUT0_SPI0_SS_spi_SS(SUT0_SPI0_SS_spi_SS),
               .SUT0_SPI0_SCLK_spi_SCLK(SUT0_SPI0_SCLK_spi_SCLK),
               .SUT0_SPI0_MISO_spi_MISO(SUT0_SPI0_MISO_spi_MISO),
               .SUT0_SPI0_MOSI_spi_MOSI(SUT0_SPI0_MOSI_spi_MOSI),
               .SUT0_SPI0_WP_N_spi_WP_N(SUT0_SPI0_WP_N_spi_WP_N),
               .SUT0_SPI0_HOLD_N_spi_HOLD_N(SUT0_SPI0_HOLD_N_spi_HOLD_N),
               .SPI1_SS(SPI1_SS_o),
               .SPI1_SCLK(SPI1_SCLK_o),
               .SPI1_MISO(SPI1_MISO_io),
               .SPI1_MOSI(SPI1_MOSI_io),
               .SPI1_WP_N(SPI1_WP_N_io),
               .SPI1_HOLD_N(SPI1_HOLD_N_io),
               .SPI1_avalid_cache(1'b0),
               .SPI1_address_cache(1'b0),
               .SPI1_wdata_cache(1'b0),
               .SPI1_wstrb_cache(1'b0),
               .SPI1_rdata_cache(),
               .SPI1_rvalid_cache(),
               .SPI1_ready_cache(),

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
      .clk_i (clk),
      .cke_i (1'b1),
      .arst_i(arst),
      .trap_o(trap)
   );
   
   // UART
   assign txd_o = uart_txd_o;
   assign uart_rxd_i = rxd_i;
   assign uart_cts_i = 1'b1;
   // uart_rts_o unconnected
   
   // SPI
   assign spi_SS_o = spi_SS;
   assign spi_SCLK_o = spi_SCLK;
   assign spi_MISO_io = spi_MISO;
   assign spi_MOSI_io = spi_MOSI;
   assign spi_WP_N_io = spi_WP_N;
   assign spi_HOLD_N_io = spi_HOLD_N;

   // SUT SPI
   assign sut_spi_SS_o = SUT0_SPI0_SS_spi_SS;
   assign sut_spi_SCLK_o = SUT0_SPI0_SCLK_spi_SCLK;
   assign sut_spi_MISO_io = SUT0_SPI0_MISO_spi_MISO;
   assign sut_spi_MOSI_io = SUT0_SPI0_MOSI_spi_MOSI;
   assign sut_spi_WP_N_io = SUT0_SPI0_WP_N_spi_WP_N;
   assign sut_spi_HOLD_N_io = SUT0_SPI0_HOLD_N_spi_HOLD_N;


   //
   // DDR4 CONTROLLER
   //

`ifdef IOB_SOC_TESTER_USE_EXTMEM
   localparam DDR4_AXI_ID_W = AXI_ID_W;
   localparam DDR4_AXI_LEN_W = AXI_LEN_W;
   localparam DDR4_AXI_ADDR_W = AXI_ADDR_W;
   localparam DDR4_AXI_DATA_W = AXI_DATA_W;


      wire [8-1:0] rstn;      assign arst = ~rstn[0] || ~rstn[1] || ~rstn[2] || ~rstn[3] || ~rstn[4] || ~rstn[5] || ~rstn[6] || ~rstn[7];
   //axi wires between ddr4 contrl and axi interconnect
wire [DDR4_AXI_ID_W-1:0] ddr4_axi_awid; //Address write channel ID.
wire [DDR4_AXI_ADDR_W-1:0] ddr4_axi_awaddr; //Address write channel address.
wire [DDR4_AXI_LEN_W-1:0] ddr4_axi_awlen; //Address write channel burst length.
wire [3-1:0] ddr4_axi_awsize; //Address write channel burst size. This signal indicates the size of each transfer in the burst.
wire [2-1:0] ddr4_axi_awburst; //Address write channel burst type.
wire [2-1:0] ddr4_axi_awlock; //Address write channel lock type.
wire [4-1:0] ddr4_axi_awcache; //Address write channel memory type. Set to 0000 if master output; ignored if slave input.
wire [3-1:0] ddr4_axi_awprot; //Address write channel protection type. Set to 000 if master output; ignored if slave input.
wire [4-1:0] ddr4_axi_awqos; //Address write channel quality of service.
wire [1-1:0] ddr4_axi_awvalid; //Address write channel valid.
wire [1-1:0] ddr4_axi_awready; //Address write channel ready.
wire [DDR4_AXI_DATA_W-1:0] ddr4_axi_wdata; //Write channel data.
wire [(DDR4_AXI_DATA_W/8)-1:0] ddr4_axi_wstrb; //Write channel write strobe.
wire [1-1:0] ddr4_axi_wlast; //Write channel last word flag.
wire [1-1:0] ddr4_axi_wvalid; //Write channel valid.
wire [1-1:0] ddr4_axi_wready; //Write channel ready.
wire [DDR4_AXI_ID_W-1:0] ddr4_axi_bid; //Write response channel ID.
wire [2-1:0] ddr4_axi_bresp; //Write response channel response.
wire [1-1:0] ddr4_axi_bvalid; //Write response channel valid.
wire [1-1:0] ddr4_axi_bready; //Write response channel ready.
wire [DDR4_AXI_ID_W-1:0] ddr4_axi_arid; //Address read channel ID.
wire [DDR4_AXI_ADDR_W-1:0] ddr4_axi_araddr; //Address read channel address.
wire [DDR4_AXI_LEN_W-1:0] ddr4_axi_arlen; //Address read channel burst length.
wire [3-1:0] ddr4_axi_arsize; //Address read channel burst size. This signal indicates the size of each transfer in the burst.
wire [2-1:0] ddr4_axi_arburst; //Address read channel burst type.
wire [2-1:0] ddr4_axi_arlock; //Address read channel lock type.
wire [4-1:0] ddr4_axi_arcache; //Address read channel memory type. Set to 0000 if master output; ignored if slave input.
wire [3-1:0] ddr4_axi_arprot; //Address read channel protection type. Set to 000 if master output; ignored if slave input.
wire [4-1:0] ddr4_axi_arqos; //Address read channel quality of service.
wire [1-1:0] ddr4_axi_arvalid; //Address read channel valid.
wire [1-1:0] ddr4_axi_arready; //Address read channel ready.
wire [DDR4_AXI_ID_W-1:0] ddr4_axi_rid; //Read channel ID.
wire [DDR4_AXI_DATA_W-1:0] ddr4_axi_rdata; //Read channel data.
wire [2-1:0] ddr4_axi_rresp; //Read channel response.
wire [1-1:0] ddr4_axi_rlast; //Read channel last word.
wire [1-1:0] ddr4_axi_rvalid; //Read channel valid.
wire [1-1:0] ddr4_axi_rready; //Read channel ready.

   //DDR4 controller axi side clocks and resets
   wire c0_ddr4_ui_clk;  //controller output clock 200MHz
   wire ddr4_axi_arstn;  //controller input

   wire c0_ddr4_ui_clk_sync_rst;

   wire calib_done;


   //
   // ASYNC AXI BRIDGE (between user logic (clk) and DDR controller (c0_ddr4_ui_clk)
   //
   axi_interconnect_0 axi_async_bridge (
      .INTERCONNECT_ACLK   (c0_ddr4_ui_clk),           //from ddr4 controller 
      .INTERCONNECT_ARESETN(~c0_ddr4_ui_clk_sync_rst), //from ddr4 controller


      //
      // External memory connection 0
      //
      .S00_AXI_ARESET_OUT_N(rstn[0]),  //to system reset
      .S00_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S00_AXI_AWID   (axi_awid[0*AXI_ID_W+:1]),
      .S00_AXI_AWADDR (axi_awaddr[0*AXI_ADDR_W+:AXI_ADDR_W]),
      .S00_AXI_AWLEN  (axi_awlen[0*AXI_LEN_W+:AXI_LEN_W]),
      .S00_AXI_AWSIZE (axi_awsize[0*3+:3]),
      .S00_AXI_AWBURST(axi_awburst[0*2+:2]),
      .S00_AXI_AWLOCK (axi_awlock[0*2+:1]),
      .S00_AXI_AWCACHE(axi_awcache[0*4+:4]),
      .S00_AXI_AWPROT (axi_awprot[0*3+:3]),
      .S00_AXI_AWQOS  (axi_awqos[0*4+:4]),
      .S00_AXI_AWVALID(axi_awvalid[0*1+:1]),
      .S00_AXI_AWREADY(axi_awready[0*1+:1]),

      //Write data
      .S00_AXI_WDATA (axi_wdata[0*AXI_DATA_W+:AXI_DATA_W]),
      .S00_AXI_WSTRB (axi_wstrb[0*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S00_AXI_WLAST (axi_wlast[0*1+:1]),
      .S00_AXI_WVALID(axi_wvalid[0*1+:1]),
      .S00_AXI_WREADY(axi_wready[0*1+:1]),

      //Write response
      .S00_AXI_BID   (axi_bid[0*AXI_ID_W+:1]),
      .S00_AXI_BRESP (axi_bresp[0*2+:2]),
      .S00_AXI_BVALID(axi_bvalid[0*1+:1]),
      .S00_AXI_BREADY(axi_bready[0*1+:1]),

      //Read address
      .S00_AXI_ARID   (axi_arid[0*AXI_ID_W+:1]),
      .S00_AXI_ARADDR (axi_araddr[0*AXI_ADDR_W+:AXI_ADDR_W]),
      .S00_AXI_ARLEN  (axi_arlen[0*AXI_LEN_W+:AXI_LEN_W]),
      .S00_AXI_ARSIZE (axi_arsize[0*3+:3]),
      .S00_AXI_ARBURST(axi_arburst[0*2+:2]),
      .S00_AXI_ARLOCK (axi_arlock[0*2+:1]),
      .S00_AXI_ARCACHE(axi_arcache[0*4+:4]),
      .S00_AXI_ARPROT (axi_arprot[0*3+:3]),
      .S00_AXI_ARQOS  (axi_arqos[0*4+:4]),
      .S00_AXI_ARVALID(axi_arvalid[0*1+:1]),
      .S00_AXI_ARREADY(axi_arready[0*1+:1]),

      //Read data
      .S00_AXI_RID   (axi_rid[0*AXI_ID_W+:1]),
      .S00_AXI_RDATA (axi_rdata[0*AXI_DATA_W+:AXI_DATA_W]),
      .S00_AXI_RRESP (axi_rresp[0*2+:2]),
      .S00_AXI_RLAST (axi_rlast[0*1+:1]),
      .S00_AXI_RVALID(axi_rvalid[0*1+:1]),
      .S00_AXI_RREADY(axi_rready[0*1+:1]),


      //
      // External memory connection 1
      //
      .S01_AXI_ARESET_OUT_N(rstn[1]),  //to system reset
      .S01_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S01_AXI_AWID   (axi_awid[1*AXI_ID_W+:1]),
      .S01_AXI_AWADDR (axi_awaddr[1*AXI_ADDR_W+:AXI_ADDR_W]),
      .S01_AXI_AWLEN  (axi_awlen[1*AXI_LEN_W+:AXI_LEN_W]),
      .S01_AXI_AWSIZE (axi_awsize[1*3+:3]),
      .S01_AXI_AWBURST(axi_awburst[1*2+:2]),
      .S01_AXI_AWLOCK (axi_awlock[1*2+:1]),
      .S01_AXI_AWCACHE(axi_awcache[1*4+:4]),
      .S01_AXI_AWPROT (axi_awprot[1*3+:3]),
      .S01_AXI_AWQOS  (axi_awqos[1*4+:4]),
      .S01_AXI_AWVALID(axi_awvalid[1*1+:1]),
      .S01_AXI_AWREADY(axi_awready[1*1+:1]),

      //Write data
      .S01_AXI_WDATA (axi_wdata[1*AXI_DATA_W+:AXI_DATA_W]),
      .S01_AXI_WSTRB (axi_wstrb[1*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S01_AXI_WLAST (axi_wlast[1*1+:1]),
      .S01_AXI_WVALID(axi_wvalid[1*1+:1]),
      .S01_AXI_WREADY(axi_wready[1*1+:1]),

      //Write response
      .S01_AXI_BID   (axi_bid[1*AXI_ID_W+:1]),
      .S01_AXI_BRESP (axi_bresp[1*2+:2]),
      .S01_AXI_BVALID(axi_bvalid[1*1+:1]),
      .S01_AXI_BREADY(axi_bready[1*1+:1]),

      //Read address
      .S01_AXI_ARID   (axi_arid[1*AXI_ID_W+:1]),
      .S01_AXI_ARADDR (axi_araddr[1*AXI_ADDR_W+:AXI_ADDR_W]),
      .S01_AXI_ARLEN  (axi_arlen[1*AXI_LEN_W+:AXI_LEN_W]),
      .S01_AXI_ARSIZE (axi_arsize[1*3+:3]),
      .S01_AXI_ARBURST(axi_arburst[1*2+:2]),
      .S01_AXI_ARLOCK (axi_arlock[1*2+:1]),
      .S01_AXI_ARCACHE(axi_arcache[1*4+:4]),
      .S01_AXI_ARPROT (axi_arprot[1*3+:3]),
      .S01_AXI_ARQOS  (axi_arqos[1*4+:4]),
      .S01_AXI_ARVALID(axi_arvalid[1*1+:1]),
      .S01_AXI_ARREADY(axi_arready[1*1+:1]),

      //Read data
      .S01_AXI_RID   (axi_rid[1*AXI_ID_W+:1]),
      .S01_AXI_RDATA (axi_rdata[1*AXI_DATA_W+:AXI_DATA_W]),
      .S01_AXI_RRESP (axi_rresp[1*2+:2]),
      .S01_AXI_RLAST (axi_rlast[1*1+:1]),
      .S01_AXI_RVALID(axi_rvalid[1*1+:1]),
      .S01_AXI_RREADY(axi_rready[1*1+:1]),


      //
      // External memory connection 2
      //
      .S02_AXI_ARESET_OUT_N(rstn[2]),  //to system reset
      .S02_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S02_AXI_AWID   (axi_awid[2*AXI_ID_W+:1]),
      .S02_AXI_AWADDR (axi_awaddr[2*AXI_ADDR_W+:AXI_ADDR_W]),
      .S02_AXI_AWLEN  (axi_awlen[2*AXI_LEN_W+:AXI_LEN_W]),
      .S02_AXI_AWSIZE (axi_awsize[2*3+:3]),
      .S02_AXI_AWBURST(axi_awburst[2*2+:2]),
      .S02_AXI_AWLOCK (axi_awlock[2*2+:1]),
      .S02_AXI_AWCACHE(axi_awcache[2*4+:4]),
      .S02_AXI_AWPROT (axi_awprot[2*3+:3]),
      .S02_AXI_AWQOS  (axi_awqos[2*4+:4]),
      .S02_AXI_AWVALID(axi_awvalid[2*1+:1]),
      .S02_AXI_AWREADY(axi_awready[2*1+:1]),

      //Write data
      .S02_AXI_WDATA (axi_wdata[2*AXI_DATA_W+:AXI_DATA_W]),
      .S02_AXI_WSTRB (axi_wstrb[2*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S02_AXI_WLAST (axi_wlast[2*1+:1]),
      .S02_AXI_WVALID(axi_wvalid[2*1+:1]),
      .S02_AXI_WREADY(axi_wready[2*1+:1]),

      //Write response
      .S02_AXI_BID   (axi_bid[2*AXI_ID_W+:1]),
      .S02_AXI_BRESP (axi_bresp[2*2+:2]),
      .S02_AXI_BVALID(axi_bvalid[2*1+:1]),
      .S02_AXI_BREADY(axi_bready[2*1+:1]),

      //Read address
      .S02_AXI_ARID   (axi_arid[2*AXI_ID_W+:1]),
      .S02_AXI_ARADDR (axi_araddr[2*AXI_ADDR_W+:AXI_ADDR_W]),
      .S02_AXI_ARLEN  (axi_arlen[2*AXI_LEN_W+:AXI_LEN_W]),
      .S02_AXI_ARSIZE (axi_arsize[2*3+:3]),
      .S02_AXI_ARBURST(axi_arburst[2*2+:2]),
      .S02_AXI_ARLOCK (axi_arlock[2*2+:1]),
      .S02_AXI_ARCACHE(axi_arcache[2*4+:4]),
      .S02_AXI_ARPROT (axi_arprot[2*3+:3]),
      .S02_AXI_ARQOS  (axi_arqos[2*4+:4]),
      .S02_AXI_ARVALID(axi_arvalid[2*1+:1]),
      .S02_AXI_ARREADY(axi_arready[2*1+:1]),

      //Read data
      .S02_AXI_RID   (axi_rid[2*AXI_ID_W+:1]),
      .S02_AXI_RDATA (axi_rdata[2*AXI_DATA_W+:AXI_DATA_W]),
      .S02_AXI_RRESP (axi_rresp[2*2+:2]),
      .S02_AXI_RLAST (axi_rlast[2*1+:1]),
      .S02_AXI_RVALID(axi_rvalid[2*1+:1]),
      .S02_AXI_RREADY(axi_rready[2*1+:1]),


      //
      // External memory connection 3
      //
      .S03_AXI_ARESET_OUT_N(rstn[3]),  //to system reset
      .S03_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S03_AXI_AWID   (axi_awid[3*AXI_ID_W+:1]),
      .S03_AXI_AWADDR (axi_awaddr[3*AXI_ADDR_W+:AXI_ADDR_W]),
      .S03_AXI_AWLEN  (axi_awlen[3*AXI_LEN_W+:AXI_LEN_W]),
      .S03_AXI_AWSIZE (axi_awsize[3*3+:3]),
      .S03_AXI_AWBURST(axi_awburst[3*2+:2]),
      .S03_AXI_AWLOCK (axi_awlock[3*2+:1]),
      .S03_AXI_AWCACHE(axi_awcache[3*4+:4]),
      .S03_AXI_AWPROT (axi_awprot[3*3+:3]),
      .S03_AXI_AWQOS  (axi_awqos[3*4+:4]),
      .S03_AXI_AWVALID(axi_awvalid[3*1+:1]),
      .S03_AXI_AWREADY(axi_awready[3*1+:1]),

      //Write data
      .S03_AXI_WDATA (axi_wdata[3*AXI_DATA_W+:AXI_DATA_W]),
      .S03_AXI_WSTRB (axi_wstrb[3*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S03_AXI_WLAST (axi_wlast[3*1+:1]),
      .S03_AXI_WVALID(axi_wvalid[3*1+:1]),
      .S03_AXI_WREADY(axi_wready[3*1+:1]),

      //Write response
      .S03_AXI_BID   (axi_bid[3*AXI_ID_W+:1]),
      .S03_AXI_BRESP (axi_bresp[3*2+:2]),
      .S03_AXI_BVALID(axi_bvalid[3*1+:1]),
      .S03_AXI_BREADY(axi_bready[3*1+:1]),

      //Read address
      .S03_AXI_ARID   (axi_arid[3*AXI_ID_W+:1]),
      .S03_AXI_ARADDR (axi_araddr[3*AXI_ADDR_W+:AXI_ADDR_W]),
      .S03_AXI_ARLEN  (axi_arlen[3*AXI_LEN_W+:AXI_LEN_W]),
      .S03_AXI_ARSIZE (axi_arsize[3*3+:3]),
      .S03_AXI_ARBURST(axi_arburst[3*2+:2]),
      .S03_AXI_ARLOCK (axi_arlock[3*2+:1]),
      .S03_AXI_ARCACHE(axi_arcache[3*4+:4]),
      .S03_AXI_ARPROT (axi_arprot[3*3+:3]),
      .S03_AXI_ARQOS  (axi_arqos[3*4+:4]),
      .S03_AXI_ARVALID(axi_arvalid[3*1+:1]),
      .S03_AXI_ARREADY(axi_arready[3*1+:1]),

      //Read data
      .S03_AXI_RID   (axi_rid[3*AXI_ID_W+:1]),
      .S03_AXI_RDATA (axi_rdata[3*AXI_DATA_W+:AXI_DATA_W]),
      .S03_AXI_RRESP (axi_rresp[3*2+:2]),
      .S03_AXI_RLAST (axi_rlast[3*1+:1]),
      .S03_AXI_RVALID(axi_rvalid[3*1+:1]),
      .S03_AXI_RREADY(axi_rready[3*1+:1]),


      //
      // External memory connection 4
      //
      .S04_AXI_ARESET_OUT_N(rstn[4]),  //to system reset
      .S04_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S04_AXI_AWID   (axi_awid[4*AXI_ID_W+:1]),
      .S04_AXI_AWADDR (axi_awaddr[4*AXI_ADDR_W+:AXI_ADDR_W]),
      .S04_AXI_AWLEN  (axi_awlen[4*AXI_LEN_W+:AXI_LEN_W]),
      .S04_AXI_AWSIZE (axi_awsize[4*3+:3]),
      .S04_AXI_AWBURST(axi_awburst[4*2+:2]),
      .S04_AXI_AWLOCK (axi_awlock[4*2+:1]),
      .S04_AXI_AWCACHE(axi_awcache[4*4+:4]),
      .S04_AXI_AWPROT (axi_awprot[4*3+:3]),
      .S04_AXI_AWQOS  (axi_awqos[4*4+:4]),
      .S04_AXI_AWVALID(axi_awvalid[4*1+:1]),
      .S04_AXI_AWREADY(axi_awready[4*1+:1]),

      //Write data
      .S04_AXI_WDATA (axi_wdata[4*AXI_DATA_W+:AXI_DATA_W]),
      .S04_AXI_WSTRB (axi_wstrb[4*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S04_AXI_WLAST (axi_wlast[4*1+:1]),
      .S04_AXI_WVALID(axi_wvalid[4*1+:1]),
      .S04_AXI_WREADY(axi_wready[4*1+:1]),

      //Write response
      .S04_AXI_BID   (axi_bid[4*AXI_ID_W+:1]),
      .S04_AXI_BRESP (axi_bresp[4*2+:2]),
      .S04_AXI_BVALID(axi_bvalid[4*1+:1]),
      .S04_AXI_BREADY(axi_bready[4*1+:1]),

      //Read address
      .S04_AXI_ARID   (axi_arid[4*AXI_ID_W+:1]),
      .S04_AXI_ARADDR (axi_araddr[4*AXI_ADDR_W+:AXI_ADDR_W]),
      .S04_AXI_ARLEN  (axi_arlen[4*AXI_LEN_W+:AXI_LEN_W]),
      .S04_AXI_ARSIZE (axi_arsize[4*3+:3]),
      .S04_AXI_ARBURST(axi_arburst[4*2+:2]),
      .S04_AXI_ARLOCK (axi_arlock[4*2+:1]),
      .S04_AXI_ARCACHE(axi_arcache[4*4+:4]),
      .S04_AXI_ARPROT (axi_arprot[4*3+:3]),
      .S04_AXI_ARQOS  (axi_arqos[4*4+:4]),
      .S04_AXI_ARVALID(axi_arvalid[4*1+:1]),
      .S04_AXI_ARREADY(axi_arready[4*1+:1]),

      //Read data
      .S04_AXI_RID   (axi_rid[4*AXI_ID_W+:1]),
      .S04_AXI_RDATA (axi_rdata[4*AXI_DATA_W+:AXI_DATA_W]),
      .S04_AXI_RRESP (axi_rresp[4*2+:2]),
      .S04_AXI_RLAST (axi_rlast[4*1+:1]),
      .S04_AXI_RVALID(axi_rvalid[4*1+:1]),
      .S04_AXI_RREADY(axi_rready[4*1+:1]),


      //
      // External memory connection 5
      //
      .S05_AXI_ARESET_OUT_N(rstn[5]),  //to system reset
      .S05_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S05_AXI_AWID   (axi_awid[5*AXI_ID_W+:1]),
      .S05_AXI_AWADDR (axi_awaddr[5*AXI_ADDR_W+:AXI_ADDR_W]),
      .S05_AXI_AWLEN  (axi_awlen[5*AXI_LEN_W+:AXI_LEN_W]),
      .S05_AXI_AWSIZE (axi_awsize[5*3+:3]),
      .S05_AXI_AWBURST(axi_awburst[5*2+:2]),
      .S05_AXI_AWLOCK (axi_awlock[5*2+:1]),
      .S05_AXI_AWCACHE(axi_awcache[5*4+:4]),
      .S05_AXI_AWPROT (axi_awprot[5*3+:3]),
      .S05_AXI_AWQOS  (axi_awqos[5*4+:4]),
      .S05_AXI_AWVALID(axi_awvalid[5*1+:1]),
      .S05_AXI_AWREADY(axi_awready[5*1+:1]),

      //Write data
      .S05_AXI_WDATA (axi_wdata[5*AXI_DATA_W+:AXI_DATA_W]),
      .S05_AXI_WSTRB (axi_wstrb[5*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S05_AXI_WLAST (axi_wlast[5*1+:1]),
      .S05_AXI_WVALID(axi_wvalid[5*1+:1]),
      .S05_AXI_WREADY(axi_wready[5*1+:1]),

      //Write response
      .S05_AXI_BID   (axi_bid[5*AXI_ID_W+:1]),
      .S05_AXI_BRESP (axi_bresp[5*2+:2]),
      .S05_AXI_BVALID(axi_bvalid[5*1+:1]),
      .S05_AXI_BREADY(axi_bready[5*1+:1]),

      //Read address
      .S05_AXI_ARID   (axi_arid[5*AXI_ID_W+:1]),
      .S05_AXI_ARADDR (axi_araddr[5*AXI_ADDR_W+:AXI_ADDR_W]),
      .S05_AXI_ARLEN  (axi_arlen[5*AXI_LEN_W+:AXI_LEN_W]),
      .S05_AXI_ARSIZE (axi_arsize[5*3+:3]),
      .S05_AXI_ARBURST(axi_arburst[5*2+:2]),
      .S05_AXI_ARLOCK (axi_arlock[5*2+:1]),
      .S05_AXI_ARCACHE(axi_arcache[5*4+:4]),
      .S05_AXI_ARPROT (axi_arprot[5*3+:3]),
      .S05_AXI_ARQOS  (axi_arqos[5*4+:4]),
      .S05_AXI_ARVALID(axi_arvalid[5*1+:1]),
      .S05_AXI_ARREADY(axi_arready[5*1+:1]),

      //Read data
      .S05_AXI_RID   (axi_rid[5*AXI_ID_W+:1]),
      .S05_AXI_RDATA (axi_rdata[5*AXI_DATA_W+:AXI_DATA_W]),
      .S05_AXI_RRESP (axi_rresp[5*2+:2]),
      .S05_AXI_RLAST (axi_rlast[5*1+:1]),
      .S05_AXI_RVALID(axi_rvalid[5*1+:1]),
      .S05_AXI_RREADY(axi_rready[5*1+:1]),


      //
      // External memory connection 6
      //
      .S06_AXI_ARESET_OUT_N(rstn[6]),  //to system reset
      .S06_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S06_AXI_AWID   (axi_awid[6*AXI_ID_W+:1]),
      .S06_AXI_AWADDR (axi_awaddr[6*AXI_ADDR_W+:AXI_ADDR_W]),
      .S06_AXI_AWLEN  (axi_awlen[6*AXI_LEN_W+:AXI_LEN_W]),
      .S06_AXI_AWSIZE (axi_awsize[6*3+:3]),
      .S06_AXI_AWBURST(axi_awburst[6*2+:2]),
      .S06_AXI_AWLOCK (axi_awlock[6*2+:1]),
      .S06_AXI_AWCACHE(axi_awcache[6*4+:4]),
      .S06_AXI_AWPROT (axi_awprot[6*3+:3]),
      .S06_AXI_AWQOS  (axi_awqos[6*4+:4]),
      .S06_AXI_AWVALID(axi_awvalid[6*1+:1]),
      .S06_AXI_AWREADY(axi_awready[6*1+:1]),

      //Write data
      .S06_AXI_WDATA (axi_wdata[6*AXI_DATA_W+:AXI_DATA_W]),
      .S06_AXI_WSTRB (axi_wstrb[6*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S06_AXI_WLAST (axi_wlast[6*1+:1]),
      .S06_AXI_WVALID(axi_wvalid[6*1+:1]),
      .S06_AXI_WREADY(axi_wready[6*1+:1]),

      //Write response
      .S06_AXI_BID   (axi_bid[6*AXI_ID_W+:1]),
      .S06_AXI_BRESP (axi_bresp[6*2+:2]),
      .S06_AXI_BVALID(axi_bvalid[6*1+:1]),
      .S06_AXI_BREADY(axi_bready[6*1+:1]),

      //Read address
      .S06_AXI_ARID   (axi_arid[6*AXI_ID_W+:1]),
      .S06_AXI_ARADDR (axi_araddr[6*AXI_ADDR_W+:AXI_ADDR_W]),
      .S06_AXI_ARLEN  (axi_arlen[6*AXI_LEN_W+:AXI_LEN_W]),
      .S06_AXI_ARSIZE (axi_arsize[6*3+:3]),
      .S06_AXI_ARBURST(axi_arburst[6*2+:2]),
      .S06_AXI_ARLOCK (axi_arlock[6*2+:1]),
      .S06_AXI_ARCACHE(axi_arcache[6*4+:4]),
      .S06_AXI_ARPROT (axi_arprot[6*3+:3]),
      .S06_AXI_ARQOS  (axi_arqos[6*4+:4]),
      .S06_AXI_ARVALID(axi_arvalid[6*1+:1]),
      .S06_AXI_ARREADY(axi_arready[6*1+:1]),

      //Read data
      .S06_AXI_RID   (axi_rid[6*AXI_ID_W+:1]),
      .S06_AXI_RDATA (axi_rdata[6*AXI_DATA_W+:AXI_DATA_W]),
      .S06_AXI_RRESP (axi_rresp[6*2+:2]),
      .S06_AXI_RLAST (axi_rlast[6*1+:1]),
      .S06_AXI_RVALID(axi_rvalid[6*1+:1]),
      .S06_AXI_RREADY(axi_rready[6*1+:1]),


      //
      // External memory connection 7
      //
      .S07_AXI_ARESET_OUT_N(rstn[7]),  //to system reset
      .S07_AXI_ACLK        (clk),      //from ddr4 controller PLL to be used by system

      //Write address
      .S07_AXI_AWID   (axi_awid[7*AXI_ID_W+:1]),
      .S07_AXI_AWADDR (axi_awaddr[7*AXI_ADDR_W+:AXI_ADDR_W]),
      .S07_AXI_AWLEN  (axi_awlen[7*AXI_LEN_W+:AXI_LEN_W]),
      .S07_AXI_AWSIZE (axi_awsize[7*3+:3]),
      .S07_AXI_AWBURST(axi_awburst[7*2+:2]),
      .S07_AXI_AWLOCK (axi_awlock[7*2+:1]),
      .S07_AXI_AWCACHE(axi_awcache[7*4+:4]),
      .S07_AXI_AWPROT (axi_awprot[7*3+:3]),
      .S07_AXI_AWQOS  (axi_awqos[7*4+:4]),
      .S07_AXI_AWVALID(axi_awvalid[7*1+:1]),
      .S07_AXI_AWREADY(axi_awready[7*1+:1]),

      //Write data
      .S07_AXI_WDATA (axi_wdata[7*AXI_DATA_W+:AXI_DATA_W]),
      .S07_AXI_WSTRB (axi_wstrb[7*(AXI_DATA_W/8)+:(AXI_DATA_W/8)]),
      .S07_AXI_WLAST (axi_wlast[7*1+:1]),
      .S07_AXI_WVALID(axi_wvalid[7*1+:1]),
      .S07_AXI_WREADY(axi_wready[7*1+:1]),

      //Write response
      .S07_AXI_BID   (axi_bid[7*AXI_ID_W+:1]),
      .S07_AXI_BRESP (axi_bresp[7*2+:2]),
      .S07_AXI_BVALID(axi_bvalid[7*1+:1]),
      .S07_AXI_BREADY(axi_bready[7*1+:1]),

      //Read address
      .S07_AXI_ARID   (axi_arid[7*AXI_ID_W+:1]),
      .S07_AXI_ARADDR (axi_araddr[7*AXI_ADDR_W+:AXI_ADDR_W]),
      .S07_AXI_ARLEN  (axi_arlen[7*AXI_LEN_W+:AXI_LEN_W]),
      .S07_AXI_ARSIZE (axi_arsize[7*3+:3]),
      .S07_AXI_ARBURST(axi_arburst[7*2+:2]),
      .S07_AXI_ARLOCK (axi_arlock[7*2+:1]),
      .S07_AXI_ARCACHE(axi_arcache[7*4+:4]),
      .S07_AXI_ARPROT (axi_arprot[7*3+:3]),
      .S07_AXI_ARQOS  (axi_arqos[7*4+:4]),
      .S07_AXI_ARVALID(axi_arvalid[7*1+:1]),
      .S07_AXI_ARREADY(axi_arready[7*1+:1]),

      //Read data
      .S07_AXI_RID   (axi_rid[7*AXI_ID_W+:1]),
      .S07_AXI_RDATA (axi_rdata[7*AXI_DATA_W+:AXI_DATA_W]),
      .S07_AXI_RRESP (axi_rresp[7*2+:2]),
      .S07_AXI_RLAST (axi_rlast[7*1+:1]),
      .S07_AXI_RVALID(axi_rvalid[7*1+:1]),
      .S07_AXI_RREADY(axi_rready[7*1+:1]),


      //
      // DDR CONTROLLER SIDE (master)
      //

      .M00_AXI_ARESET_OUT_N(ddr4_axi_arstn),  //to ddr controller axi slave port
      .M00_AXI_ACLK        (c0_ddr4_ui_clk),  //from ddr4 controller 200MHz clock

      //Write address
      .M00_AXI_AWID   (ddr4_axi_awid),
      .M00_AXI_AWADDR (ddr4_axi_awaddr),
      .M00_AXI_AWLEN  (ddr4_axi_awlen),
      .M00_AXI_AWSIZE (ddr4_axi_awsize),
      .M00_AXI_AWBURST(ddr4_axi_awburst),
      .M00_AXI_AWLOCK (ddr4_axi_awlock[0]),
      .M00_AXI_AWCACHE(ddr4_axi_awcache),
      .M00_AXI_AWPROT (ddr4_axi_awprot),
      .M00_AXI_AWQOS  (ddr4_axi_awqos),
      .M00_AXI_AWVALID(ddr4_axi_awvalid),
      .M00_AXI_AWREADY(ddr4_axi_awready),

      //Write data
      .M00_AXI_WDATA (ddr4_axi_wdata),
      .M00_AXI_WSTRB (ddr4_axi_wstrb),
      .M00_AXI_WLAST (ddr4_axi_wlast),
      .M00_AXI_WVALID(ddr4_axi_wvalid),
      .M00_AXI_WREADY(ddr4_axi_wready),

      //Write response
      .M00_AXI_BID   (ddr4_axi_bid),
      .M00_AXI_BRESP (ddr4_axi_bresp),
      .M00_AXI_BVALID(ddr4_axi_bvalid),
      .M00_AXI_BREADY(ddr4_axi_bready),

      //Read address
      .M00_AXI_ARID   (ddr4_axi_arid),
      .M00_AXI_ARADDR (ddr4_axi_araddr),
      .M00_AXI_ARLEN  (ddr4_axi_arlen),
      .M00_AXI_ARSIZE (ddr4_axi_arsize),
      .M00_AXI_ARBURST(ddr4_axi_arburst),
      .M00_AXI_ARLOCK (ddr4_axi_arlock[0]),
      .M00_AXI_ARCACHE(ddr4_axi_arcache),
      .M00_AXI_ARPROT (ddr4_axi_arprot),
      .M00_AXI_ARQOS  (ddr4_axi_arqos),
      .M00_AXI_ARVALID(ddr4_axi_arvalid),
      .M00_AXI_ARREADY(ddr4_axi_arready),

      //Read data
      .M00_AXI_RID   (ddr4_axi_rid),
      .M00_AXI_RDATA (ddr4_axi_rdata),
      .M00_AXI_RRESP (ddr4_axi_rresp),
      .M00_AXI_RLAST (ddr4_axi_rlast),
      .M00_AXI_RVALID(ddr4_axi_rvalid),
      .M00_AXI_RREADY(ddr4_axi_rready)
   );

   ddr4_0 ddr4_ctrl (
      .sys_rst     (reset),
      .c0_sys_clk_p(c0_sys_clk_clk_p),
      .c0_sys_clk_n(c0_sys_clk_clk_n),

      .dbg_clk(),
      .dbg_bus(),

      //USER LOGIC CLOCK AND RESET      
      .c0_ddr4_ui_clk_sync_rst(c0_ddr4_ui_clk_sync_rst),  //to axi intercon
      .addn_ui_clkout1        (clk),                      //to user logic 

      //AXI INTERFACE (slave)
      .c0_ddr4_ui_clk (c0_ddr4_ui_clk),  //to axi intercon general and master clocks
      .c0_ddr4_aresetn(ddr4_axi_arstn),  //from interconnect axi master

      //address write 
      .c0_ddr4_s_axi_awid   (ddr4_axi_awid),
      .c0_ddr4_s_axi_awaddr (ddr4_axi_awaddr),
      .c0_ddr4_s_axi_awlen  (ddr4_axi_awlen),
      .c0_ddr4_s_axi_awsize (ddr4_axi_awsize),
      .c0_ddr4_s_axi_awburst(ddr4_axi_awburst),
      .c0_ddr4_s_axi_awlock (ddr4_axi_awlock[0]),
      .c0_ddr4_s_axi_awprot (ddr4_axi_awprot),
      .c0_ddr4_s_axi_awcache(ddr4_axi_awcache),
      .c0_ddr4_s_axi_awqos  (ddr4_axi_awqos),
      .c0_ddr4_s_axi_awvalid(ddr4_axi_awvalid),
      .c0_ddr4_s_axi_awready(ddr4_axi_awready),

      //write  
      .c0_ddr4_s_axi_wvalid(ddr4_axi_wvalid),
      .c0_ddr4_s_axi_wready(ddr4_axi_wready),
      .c0_ddr4_s_axi_wdata (ddr4_axi_wdata),
      .c0_ddr4_s_axi_wstrb (ddr4_axi_wstrb),
      .c0_ddr4_s_axi_wlast (ddr4_axi_wlast),

      //write response
      .c0_ddr4_s_axi_bready(ddr4_axi_bready),
      .c0_ddr4_s_axi_bid   (ddr4_axi_bid),
      .c0_ddr4_s_axi_bresp (ddr4_axi_bresp),
      .c0_ddr4_s_axi_bvalid(ddr4_axi_bvalid),

      //address read
      .c0_ddr4_s_axi_arid   (ddr4_axi_arid),
      .c0_ddr4_s_axi_araddr (ddr4_axi_araddr),
      .c0_ddr4_s_axi_arlen  (ddr4_axi_arlen),
      .c0_ddr4_s_axi_arsize (ddr4_axi_arsize),
      .c0_ddr4_s_axi_arburst(ddr4_axi_arburst),
      .c0_ddr4_s_axi_arlock (ddr4_axi_arlock[0]),
      .c0_ddr4_s_axi_arcache(ddr4_axi_arcache),
      .c0_ddr4_s_axi_arprot (ddr4_axi_arprot),
      .c0_ddr4_s_axi_arqos  (ddr4_axi_arqos),
      .c0_ddr4_s_axi_arvalid(ddr4_axi_arvalid),
      .c0_ddr4_s_axi_arready(ddr4_axi_arready),

      //read   
      .c0_ddr4_s_axi_rready(ddr4_axi_rready),
      .c0_ddr4_s_axi_rid   (ddr4_axi_rid),
      .c0_ddr4_s_axi_rdata (ddr4_axi_rdata),
      .c0_ddr4_s_axi_rresp (ddr4_axi_rresp),
      .c0_ddr4_s_axi_rlast (ddr4_axi_rlast),
      .c0_ddr4_s_axi_rvalid(ddr4_axi_rvalid),

      //DDR4 INTERFACE (master of external DDR4 module)
      .c0_ddr4_act_n         (c0_ddr4_act_n),
      .c0_ddr4_adr           (c0_ddr4_adr),
      .c0_ddr4_ba            (c0_ddr4_ba),
      .c0_ddr4_bg            (c0_ddr4_bg),
      .c0_ddr4_cke           (c0_ddr4_cke),
      .c0_ddr4_odt           (c0_ddr4_odt),
      .c0_ddr4_cs_n          (c0_ddr4_cs_n),
      .c0_ddr4_ck_t          (c0_ddr4_ck_t),
      .c0_ddr4_ck_c          (c0_ddr4_ck_c),
      .c0_ddr4_reset_n       (c0_ddr4_reset_n),
      .c0_ddr4_dm_dbi_n      (c0_ddr4_dm_dbi_n),
      .c0_ddr4_dq            (c0_ddr4_dq),
      .c0_ddr4_dqs_c         (c0_ddr4_dqs_c),
      .c0_ddr4_dqs_t         (c0_ddr4_dqs_t),
      .c0_init_calib_complete(calib_done)
   );


`else
   //if DDR not used use PLL to generate system clock
   clock_wizard #(
      .OUTPUT_PER(10),
      .INPUT_PER (4)
   ) clk_250_to_100_MHz (
      .clk_in1_p(c0_sys_clk_clk_p),
      .clk_in1_n(c0_sys_clk_clk_n),
      .clk_out1 (clk)
   );

   wire start;
   iob_reset_sync start_sync (
      .clk_i (clk),
      .arst_i(reset),
      .arst_o(start)
   );

   //create reset pulse as reset is never activated manually
   //also, during bitstream loading, the reset pin is not pulled high
   iob_pulse_gen #(
      .START   (5),
      .DURATION(10)
   ) reset_pulse (
      .clk_i  (clk),
      .arst_i (reset),
      .cke_i  (1'b1),
      .start_i(start),
      .pulse_o(arst)
   );
`endif

endmodule
