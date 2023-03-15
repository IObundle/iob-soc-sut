/*********************************************************
 *                    Tester Firmware                    *
 *********************************************************/
#include "stdlib.h"
#include <stdio.h>
#include "bsp.h"
#include "iob_soc_tester_system.h"
#include "iob_soc_tester_conf.h"
#include "iob_soc_tester_periphs.h"
#include "iob-uart.h"
#include "printf.h"
#include "iob_regfileif_swreg.h"

//Enable debug messages.
#define DEBUG 0

#define SUT_FIRMWARE_SIZE 24000

int main()
{
	char c, msgBuffer[5096], *sutStr;
	int i = 0;

	//Init uart0
	uart_init(UART0_BASE,FREQ/BAUD);   
	//Init REGFILEIF of SUT (connected through IOBNATIVEBRIDGEIF)
	IOB_REGFILEIF_INIT_BASEADDR(IOBNATIVEBRIDGEIF0_BASE);

	uart_puts("\n\n[Tester]: Hello from tester!\n\n\n");

	uart_puts("[Tester]: Initializing SUT via UART...\n");
	//Init and switch to uart1 (connected to the SUT)
	uart_init(UART1_BASE,FREQ/BAUD);   

	//Wait for ENQ signal from SUT
	while(uart_getc()!=ENQ);
	//Send ack to sut
	uart_putc(ACK);

	IOB_UART_INIT_BASEADDR(UART0_BASE);
	uart_puts("[Tester]: Received SUT UART enquiry and sent acknowledge.\n");
	IOB_UART_INIT_BASEADDR(UART1_BASE);

#ifndef INIT_MEM
	IOB_UART_INIT_BASEADDR(UART0_BASE);
	uart_puts("[Tester]: SUT memory is not initalized. Waiting for firmware transfer request from SUT...\n");
	IOB_UART_INIT_BASEADDR(UART1_BASE);

  char *sut_firmware = malloc(SUT_FIRMWARE_SIZE);
  //receive firmware request from SUT 
  //Wait for FRX signal from SUT
  while(uart_getc()!=FRX);
  //Receive filename
  for(i=0;(msgBuffer[i]=uart_getc())!='\0';i++);
	//Switch back to UART0
	IOB_UART_INIT_BASEADDR(UART0_BASE);

	uart_puts("[Tester]: Received firmware transfer request with filename: ");
	uart_puts(msgBuffer);
	uart_putc('\n');
	uart_puts("[Tester]: Sending transfer request to console...\n");

  //Make request to host
  uint32_t file_size = 0;
  file_size = uart_recvfile(msgBuffer, sut_firmware);
  
	uart_puts("[Tester]: SUT firmware obtained. Transfering it to SUT via UART...\n");

	//Switch back to UART1
	IOB_UART_INIT_BASEADDR(UART1_BASE);

  // send file size
  uart_putc((char)(file_size & 0x0ff));
  uart_putc((char)((file_size & 0x0ff00) >> 8));
  uart_putc((char)((file_size & 0x0ff0000) >> 16));
  uart_putc((char)((file_size & 0x0ff000000) >> 24));
  
  //Wait for ACK signal from SUT
  while(uart_getc()!=ACK);
  if(DEBUG){
    IOB_UART_INIT_BASEADDR(UART0_BASE);
    uart_puts("[Tester] Got ack! Sending firmware to SUT...\n");
    IOB_UART_INIT_BASEADDR(UART1_BASE);
  }
  
  // send file contents
  for (i = 0; i < file_size; i++)
    uart_putc(sut_firmware[i]);

  //Ignore firmware sent back
  IOB_UART_INIT_BASEADDR(UART0_BASE);
  uart_puts("[Tester]: SUT firmware transfered. Ignoring firmware readback sent by SUT...\n");
  IOB_UART_INIT_BASEADDR(UART1_BASE);
  
  //Wait for FTX signal from SUT
  while(uart_getc()!=FTX);
  //Receive filename
  for(i=0;(msgBuffer[i]=uart_getc())!='\0';i++);
  
  //receive file size
  file_size = uart_getc();
  file_size |= ((uint32_t) uart_getc()) << 8;
  file_size |= ((uint32_t) uart_getc()) << 16;
  file_size |= ((uint32_t) uart_getc()) << 24;

  //ignore file contents received
  for (i = 0; i < file_size; i++) {
    uart_getc();
  }

  IOB_UART_INIT_BASEADDR(UART0_BASE);
  uart_puts("[Tester]: Finished receiving firmware readback.\n");
  IOB_UART_INIT_BASEADDR(UART1_BASE);

#endif

  IOB_UART_INIT_BASEADDR(UART0_BASE);
  uart_puts("[Tester]: Reading SUT messages...\n");
  IOB_UART_INIT_BASEADDR(UART1_BASE);

	i=0;
	//Read and store messages sent from SUT
	while ((c=uart_getc())!=EOT){
		msgBuffer[i]=c;
		if(DEBUG){
			IOB_UART_INIT_BASEADDR(UART0_BASE);
			uart_putc(c);
			IOB_UART_INIT_BASEADDR(UART1_BASE);
		}
		i++;
	}
	msgBuffer[i]=EOT;

	//End UART1 connection with SUT
	uart_finish();

	//Switch back to UART0
	IOB_UART_INIT_BASEADDR(UART0_BASE);

	//Send messages previously stored from SUT
	uart_puts("[Tester]: #### Messages received from SUT: ####\n\n");
	for(i=0; msgBuffer[i]!=EOT; i++){
		uart_putc(msgBuffer[i]);
	}
	uart_puts("\n[Tester]: #### End of messages received from SUT ####\n\n");

	//Read data from REGFILEIF (was written by the SUT)
	uart_puts("[Tester]: Reading REGFILEIF contents:\n");
	printf("[Tester] Register 3: %d \n", IOB_REGFILEIF_GET_REG3());
	printf("[Tester] Register 4: %d \n", IOB_REGFILEIF_GET_REG4());

#ifdef USE_EXTMEM
	uart_puts("\n[Tester] Using shared external memory. Obtain SUT memory string pointer via REGFILEIF register 5...\n");
	//Get address of first char in string stored in SUT's memory with first bit inverted
	sutStr=(char*)(IOB_REGFILEIF_GET_REG5() ^ (0b1 << (MEM_ADDR_W-1))); //Note, MEM_ADDR_W may not be the same as DDR_ADDR_W when running in fpga
	uart_puts("[Tester]: String pointer is: ");
	printf("0x%x",sutStr);
	uart_putc('\n');

	//Print the string by accessing that address
	uart_puts("[Tester]: String read from SUT's memory via shared memory:\n");
	for(i=0; sutStr[i]!='\0'; i++){
		uart_putc(sutStr[i]);
	}
	uart_putc('\n');
#endif

	uart_puts("\n[Tester]: Verification successful!\n\n");

	//End UART0 connection
	uart_finish();
}
