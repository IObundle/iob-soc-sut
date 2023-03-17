#include <stdio.h>
#include "bsp.h"
#include "iob_soc_sut_system.h"
#include "iob_soc_sut_periphs.h"
#include "iob_soc_sut_conf.h"
#include "iob-uart.h"
#include "printf.h"
#include "iob_regfileif_inverted_swreg.h"

int main()
{
  //init uart
  uart_init(UART0_BASE,FREQ/BAUD);   
  uart_puts("\n\n\n[SUT]: Hello world!\n\n\n");

  IOB_REGFILEIF_INVERTED_INIT_BASEADDR(REGFILEIF0_BASE);

  //Write to UART0 connected to the Tester.
  uart_puts("[SUT]: This message was sent from SUT!\n\n");
  
  //Print contents of REGFILEIF registers 1 and 2
  uart_puts("[SUT]: Reading REGFILEIF contents:\n");
  printf("[SUT] Register 1: %d \n", IOB_REGFILEIF_INVERTED_GET_REG1());
  printf("[SUT] Register 2: %d \n\n", IOB_REGFILEIF_INVERTED_GET_REG2());

  //Write data to the registers of REGFILEIF to be read by the Tester.
  IOB_REGFILEIF_INVERTED_SET_REG3(128);
  IOB_REGFILEIF_INVERTED_SET_REG4(2048);
  uart_puts("[SUT]: Stored values 128 and 2048 in REGFILEIF registers 3 and 4.\n");

#ifdef USE_EXTMEM
  char sutMemoryMessage[]="This message is stored in SUT's memory\n";

  uart_puts("\n[SUT]: Using external memory. Stored a string in memory at location: ");
  printf("0x%x\n", (int)sutMemoryMessage);
  
  //Give address of stored message to Tester using regfileif register 4
  IOB_REGFILEIF_INVERTED_SET_REG5((int)sutMemoryMessage);
  uart_puts("[SUT]: Stored string memory location in REGFILEIF register 5.\n");
#endif

  uart_finish();
}
