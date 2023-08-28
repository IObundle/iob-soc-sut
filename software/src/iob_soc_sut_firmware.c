#include <stdio.h>
#include "bsp.h"
#include "iob_soc_sut_system.h"
#include "iob_soc_sut_periphs.h"
#include "iob_soc_sut_conf.h"
#include "iob-uart.h"
#include "iob-gpio.h"
#include "printf.h"
#include "iob_regfileif_inverted_swreg.h"
#include "iob_str.h"
#include "iob-axistream-in.h"
#include "iob-axistream-out.h"

void axistream_loopback();

int main()
{
  char pass_string[] = "Test passed!";
  char fail_string[] = "Test failed!";

  //init uart
  uart_init(UART0_BASE,FREQ/BAUD);   
  printf_init(&uart_putc);
  uart_puts("\n\n\n[SUT]: Hello world!\n\n\n");

  //init regfileif
  IOB_REGFILEIF_INVERTED_INIT_BASEADDR(REGFILEIF0_BASE);
  //init gpio
  gpio_init(GPIO0_BASE);   
  //init axistream
  axistream_in_init(AXISTREAMIN0_BASE);   
  axistream_out_init(AXISTREAMOUT0_BASE, 4);

  //Write to UART0 connected to the Tester.
  uart_puts("[SUT]: This message was sent from SUT!\n\n");
  
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

#ifdef USE_EXTMEM
  char sutMemoryMessage[]="This message is stored in SUT's memory\n";

  uart_puts("\n[SUT]: Using external memory. Stored a string in memory at location: ");
  printf("0x%x\n", (int)sutMemoryMessage);
  
  //Give address of stored message to Tester using regfileif register 4
  IOB_REGFILEIF_INVERTED_SET_REG5((int)sutMemoryMessage);
  uart_puts("[SUT]: Stored string memory location in REGFILEIF register 5.\n");
#endif

//#ifdef USE_EXTMEM
//  if(memory_access_failed)
//      uart_sendfile("test.log", iob_strlen(fail_string), fail_string);
//      uart_finish();
//#endif
  uart_sendfile("test.log", iob_strlen(pass_string), pass_string);

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
