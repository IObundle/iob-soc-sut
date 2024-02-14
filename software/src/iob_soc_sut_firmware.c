#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bsp.h"
#include "iob_soc_sut_system.h"
#include "iob_soc_sut_periphs.h"
#include "iob_soc_sut_conf.h"
#include "iob-uart16550.h"
#include "iob-gpio.h"
#include "iob-eth.h"
#include "printf.h"
#include "iob_regfileif_inverted_swreg.h"
#include "iob-axistream-in.h"
#include "iob-axistream-out.h"
#if __has_include("iob_soc_tester_conf.h")
#define USE_TESTER
#endif

void axistream_loopback();

void clear_cache(){
  // Delay to ensure all data is written to memory
  for ( unsigned int i = 0; i < 10; i++)asm volatile("nop");
  // Flush VexRiscv CPU internal cache
  asm volatile(".word 0x500F" ::: "memory");
}

// Send signal by uart to receive file by ethernet
uint32_t uart16550_recvfile_ethernet(char *file_name) {

  uart16550_puts(UART_PROGNAME);
  uart16550_puts (": requesting to receive file by ethernet\n");

  //send file receive by ethernet request
  uart16550_putc (0x13);

  //send file name (including end of string)
  uart16550_puts(file_name); uart16550_putc(0);

  // receive file size
  uint32_t file_size = uart16550_getc();
  file_size |= ((uint32_t)uart16550_getc()) << 8;
  file_size |= ((uint32_t)uart16550_getc()) << 16;
  file_size |= ((uint32_t)uart16550_getc()) << 24;

  // send ACK before receiving file
  uart16550_putc(ACK);

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
  uart16550_init(UART0_BASE, FREQ/(16*BAUD));
  printf_init(&uart16550_putc);
  //init regfileif
  IOB_REGFILEIF_INVERTED_INIT_BASEADDR(REGFILEIF0_BASE);
  //init gpio
  gpio_init(GPIO0_BASE);   
  //init axistream
  axistream_in_init(AXISTREAMIN0_BASE);   
  axistream_out_init(AXISTREAMOUT0_BASE, 4);
  axistream_in_enable();
  axistream_out_enable();
  // init eth
  eth_init(ETH0_BASE, &clear_cache);

  // Wait for PHY reset to finish
  eth_wait_phy_rst();

#ifdef USE_TESTER
  // Receive a special string message from tester to tell if its running linux
  char tester_run_type[] = "TESTER_RUN_";
  for ( i = 0; i < 11; ) {
    if (uart16550_getc() == tester_run_type[i])
      i++;
    else
      i = 0;
  }
  char tester_run_type2[] = "LINUX";
  int tester_run_linux = 1;
  for ( i = 0; i < 5; ) {
    if (uart16550_getc() == tester_run_type2[i]) {
      i++;
    } else {
      tester_run_linux = 0;
      break;
    }
  }

  if (!tester_run_linux) { //Ethernet does not work on Linux yet
    uart16550_puts("[SUT]: Tester running on bare metal\n");
    // Receive data from Tester via Ethernet
    ethernet_connected = 1;
    eth_rcv_file(buffer, 64);

    //Delay to allow time for tester to print debug messages
    for ( i = 0; i < (FREQ/BAUD)*128; i++)asm("nop");
  } else {
    uart16550_puts("[SUT]: Tester running on Linux\n");
  }
#else //USE_TESTER
#ifndef SIMULATION
  // Receive data from console via Ethernet
  uint32_t file_size;
  file_size = uart16550_recvfile_ethernet("../src/eth_example.txt");
  eth_rcv_file(file_buffer,file_size);
  uart16550_puts("\n[SUT]: File received from console via ethernet:\n");
  for(i=0; i<file_size; i++)
    uart16550_putc(file_buffer[i]);
#endif //SIMULATION
#endif //USE_TESTER

  uart16550_puts("\n\n\n[SUT]: Hello world!\n\n\n");

  //Write to UART0 connected to the Tester.
  uart16550_puts("[SUT]: This message was sent from SUT!\n\n");

  if(ethernet_connected){
    uart16550_puts("[SUT]: Data received via ethernet:\n");
    for(i=0; i<64; i++)
      printf("%d ", buffer[i]);
    uart16550_putc('\n'); uart16550_putc('\n');
  }
  
  //Print contents of REGFILEIF registers 1 and 2
  uart16550_puts("[SUT]: Reading REGFILEIF contents:\n");
  printf("[SUT]: Register 1: %d \n", IOB_REGFILEIF_INVERTED_GET_REG1());
  printf("[SUT]: Register 2: %d \n\n", IOB_REGFILEIF_INVERTED_GET_REG2());

  //Write data to the registers of REGFILEIF to be read by the Tester.
  IOB_REGFILEIF_INVERTED_SET_REG3(128);
  IOB_REGFILEIF_INVERTED_SET_REG4(2048);
  uart16550_puts("[SUT]: Stored values 128 and 2048 in REGFILEIF registers 3 and 4.\n\n");
  
  //Print contents of GPIO inputs 
  printf("[SUT]: Pattern read from GPIO inputs: 0x%x\n\n",gpio_get());

  //Write the same pattern to GPIO outputs
  gpio_set(0xabcd1234);
  uart16550_puts("[SUT]: Placed test pattern 0xabcd1234 in GPIO outputs.\n\n");

  // Read AXI stream input and relay data to AXI stream output
  axistream_loopback();

  char sutMemoryMessage[]="This message is stored in SUT's memory\n";

  uart16550_puts("\n[SUT]: Using external memory. Stored a string in memory at location: ");
  printf("0x%x\n", (int)sutMemoryMessage);
  
  //Give address of stored message to Tester using regfileif register 4
  IOB_REGFILEIF_INVERTED_SET_REG5((int)sutMemoryMessage);
  uart16550_puts("[SUT]: Stored string memory location in REGFILEIF register 5.\n");

  uart16550_sendfile("test.log", strlen(pass_string), pass_string);

  uart16550_finish();
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
    uart16550_puts("[SUT]: Received AXI stream bytes: ");
    for(i=0;i<received_words*4;i++)
      printf("0x%02x ", ((uint8_t *)byte_stream)[i]);

    // Send bytes to AXI stream output
    for(i=0;i<received_words-1;i++)
      axistream_out_push(byte_stream[i],1,0);
    axistream_out_push(byte_stream[i],1,1); // Send the last word with the TLAST signal
    uart16550_puts("\n[SUT]: Sent AXI stream bytes back via output interface.\n\n");
  } else {
    // Input AXI stream queue is empty
    uart16550_puts("[SUT]: AXI stream input is empty. Skipping AXI stream tranfer.\n\n");
  }

}
