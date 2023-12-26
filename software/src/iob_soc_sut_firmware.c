#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bsp.h"
#include "iob_soc_sut_system.h"
#include "iob_soc_sut_periphs.h"
#include "iob_soc_sut_conf.h"
#include "iob-uart.h"
#include "iob-gpio.h"
#include "iob_cache_swreg.h"
#include "iob-eth.h"
#include "printf.h"
#include "iob_regfileif_inverted_swreg.h"
#include "iob-axistream-in.h"
#include "iob-axistream-out.h"
#if __has_include("iob_soc_tester_conf.h")
#define USE_TESTER
#endif

void axistream_loopback();
void clear_cache();

// Send signal by uart to receive file by ethernet
uint32_t uart_recvfile_ethernet(char *file_name) {

  uart_puts(UART_PROGNAME);
  uart_puts (": requesting to receive by ethernet file\n");

  //send file receive by ethernet request
  uart_putc (0x13);

  //send file name (including end of string)
  uart_puts(file_name); uart_putc(0);

  // receive file size
  uint32_t file_size = uart_getc();
  file_size |= ((uint32_t)uart_getc()) << 8;
  file_size |= ((uint32_t)uart_getc()) << 16;
  file_size |= ((uint32_t)uart_getc()) << 24;

  // send ACK before receiving file
  uart_putc(ACK);

  return file_size;
}

int main()
{
  char pass_string[] = "Test passed!";
  char fail_string[] = "Test failed!";
  int i;
  char buffer[64];
  char file_buffer[256];
  int ethernet_connected = 0;

  //init uart
  uart_init(UART0_BASE,FREQ/BAUD);   
  printf_init(&uart_putc);
  //init regfileif
  IOB_REGFILEIF_INVERTED_INIT_BASEADDR(REGFILEIF0_BASE);
  //init gpio
  gpio_init(GPIO0_BASE);   
  //init axistream
  axistream_in_init(AXISTREAMIN0_BASE);   
  axistream_out_init(AXISTREAMOUT0_BASE, 4);
  axistream_in_enable();
  axistream_out_enable();
  // init cache
  IOB_CACHE_INIT_BASEADDR((1 << IOB_SOC_SUT_E) + (1 << IOB_SOC_SUT_MEM_ADDR_W));
  // init eth
  eth_init(ETH0_BASE, &clear_cache);

  // Wait for PHY reset to finish
  eth_wait_phy_rst();

#ifdef USE_TESTER
  // Receive data from Tester via Ethernet
  ethernet_connected = 1;
  eth_rcv_file(buffer, 64);

  //Delay to allow time for tester to print debug messages
  for ( i = 0; i < (FREQ/BAUD)*128; i++)asm("nop");
#else
#ifndef SIMULATION
  // Receive data from console via Ethernet
  uint32_t file_size;
  file_size = uart_recvfile_ethernet("../src/eth_example.txt");
  eth_rcv_file(file_buffer,file_size);
  uart_puts("\n[SUT]: File received from console via ethernet:\n");
  for(i=0; i<file_size; i++)
    uart_putc(file_buffer[i]);
#endif
#endif

  uart_puts("\n\n\n[SUT]: Hello world!\n\n\n");

  //Write to UART0 connected to the Tester.
  uart_puts("[SUT]: This message was sent from SUT!\n\n");

  if(ethernet_connected){
    uart_puts("[SUT]: Data received via ethernet:\n");
    for(i=0; i<64; i++)
      printf("%d ", buffer[i]);
    uart_putc('\n'); uart_putc('\n');
  }
  
  //Print contents of REGFILEIF registers 1 and 2
  uart_puts("[SUT]: Reading REGFILEIF contents:\n");
  printf("[SUT]: Register 1: %d \n", IOB_REGFILEIF_INVERTED_GET_REG1());
  printf("[SUT]: Register 2: %d \n\n", IOB_REGFILEIF_INVERTED_GET_REG2());

  //Write data to the registers of REGFILEIF to be read by the Tester.
  IOB_REGFILEIF_INVERTED_SET_REG3(128);
  IOB_REGFILEIF_INVERTED_SET_REG4(2048);
  uart_puts("[SUT]: Stored values 128 and 2048 in REGFILEIF registers 3 and 4.\n\n");
  
  //Print contents of GPIO inputs 
  printf("[SUT]: Pattern read from GPIO inputs: 0x%x\n\n",gpio_get());

  //Write the same pattern to GPIO outputs
  gpio_set(0xabcd1234);
  uart_puts("[SUT]: Placed test pattern 0xabcd1234 in GPIO outputs.\n\n");

  // Read AXI stream input and relay data to AXI stream output
  axistream_loopback();

#ifdef IOB_SOC_SUT_USE_EXTMEM
  char sutMemoryMessage[]="This message is stored in SUT's memory\n";

  uart_puts("\n[SUT]: Using external memory. Stored a string in memory at location: ");
  printf("0x%x\n", (int)sutMemoryMessage);
  
  //Give address of stored message to Tester using regfileif register 4
  IOB_REGFILEIF_INVERTED_SET_REG5((int)sutMemoryMessage);
  uart_puts("[SUT]: Stored string memory location in REGFILEIF register 5.\n");
#endif

//#ifdef IOB_SOC_SUT_USE_EXTMEM
//  if(memory_access_failed)
//      uart_sendfile("test.log", strlen(fail_string), fail_string);
//      uart_finish();
//#endif
  uart_sendfile("test.log", strlen(pass_string), pass_string);

  uart_finish();
}


// Read AXI stream input, print, and relay data to AXI stream output
void axistream_loopback(){
  uint32_t byte_stream[16];
  uint8_t i, rstrb, received_words;
  
  //Check if we are receiving an AXI stream
  if(!axistream_in_empty()){
    // Receive bytes while stream does not end (by TLAST signal), or up to 16 32-bit words
    for(received_words=0, i=0; i<1 && received_words<16; received_words++)
      byte_stream[received_words] = axistream_in_pop(&rstrb, &i);
    
    // Print received bytes
    uart_puts("[SUT]: Received AXI stream bytes: ");
    for(i=0;i<received_words*4;i++)
      printf("0x%02x ", ((uint8_t *)byte_stream)[i]);

    // Send bytes to AXI stream output
    for(i=0;i<received_words-1;i++)
      axistream_out_push(byte_stream[i],1,0);
    axistream_out_push(byte_stream[i],1,1); // Send the last word with the TLAST signal
    uart_puts("\n[SUT]: Sent AXI stream bytes back via output interface.\n\n");
  } else {
    // Input AXI stream queue is empty
    uart_puts("[SUT]: AXI stream input is empty. Skipping AXI stream tranfer.\n\n");
  }

}

void clear_cache(){
  // Delay to ensure all data is written to memory
  for ( unsigned int i = 0; i < 10; i++)asm volatile("nop");
  // Flush system cache
  IOB_CACHE_SET_INVALIDATE(1);
}
