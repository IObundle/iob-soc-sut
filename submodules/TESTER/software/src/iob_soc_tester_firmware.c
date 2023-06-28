/*********************************************************
 *                    Tester Firmware                    *
 *********************************************************/
#include "bsp.h"
#include "iob-axistream-in.h"
#include "iob-axistream-out.h"
#include "iob-gpio.h"
#include "iob-uart.h"
#include "iob-ila.h"
#include "ILA0.h" // ILA0 instance specific defines
#include "iob_soc_sut_swreg.h"
#include "iob_soc_tester_conf.h"
#include "iob_soc_tester_periphs.h"
#include "iob_soc_tester_system.h"
#include "printf.h"
#include "stdlib.h"
#include <stdio.h>

// Enable debug messages.
#define DEBUG 0

#define SUT_FIRMWARE_SIZE 29000

void print_ila_samples();
void send_axistream();
void receive_axistream();

int main() {
  uint32_t file_size = 0;
  char c, msgBuffer[5096], *sutStr;
  int i = 0;
#ifndef INIT_MEM
  char sut_firmware[SUT_FIRMWARE_SIZE];
#endif

  // Init uart0
  uart_init(UART0_BASE, FREQ / BAUD);
  // Init SUT (connected through REGFILEIF)
  IOB_SOC_SUT_INIT_BASEADDR(SUT0_BASE);
  // Init gpio
  gpio_init(GPIO0_BASE);
  // init axistream
  axistream_in_init(AXISTREAMIN0_BASE);
  axistream_out_init_tdata_w(AXISTREAMOUT0_BASE, 4);
  // init integrated logic analyzer
  ila_init(ILA0_BASE);

  uart_puts("\n\n[Tester]: Hello from tester!\n\n\n");

  // Write data to the registers of the SUT to be read by it.
  IOB_SOC_SUT_SET_REG1(64);
  IOB_SOC_SUT_SET_REG2(1024);
  uart_puts("[Tester]: Stored values 64 and 1024 in registers 1 and 2 of the "
            "SUT.\n\n");

  // Write a test pattern to the GPIO outputs to be read by the SUT.
  gpio_set(0x1234abcd);
  uart_puts("[Tester]: Placed test pattern 0x1234abcd in GPIO outputs.\n\n");

  // Enable all ILA triggers
  ila_enable_all_triggers();

  // Send byte stream via AXI stream
  send_axistream();
  
  // Disable all ILA triggers
  ila_disable_all_triggers();
  
  // Print sampled ILA values
  print_ila_samples();

  uart_puts("[Tester]: Initializing SUT via UART...\n");
  // Init and switch to uart1 (connected to the SUT)
  uart_init(UART1_BASE, FREQ / BAUD);

  // Wait for ENQ signal from SUT
  while (uart_getc() != ENQ)
    ;
  // Send ack to sut
  uart_putc(ACK);

  IOB_UART_INIT_BASEADDR(UART0_BASE);
  uart_puts("[Tester]: Received SUT UART enquiry and sent acknowledge.\n");
  IOB_UART_INIT_BASEADDR(UART1_BASE);

#ifndef INIT_MEM
  IOB_UART_INIT_BASEADDR(UART0_BASE);
  uart_puts("[Tester]: SUT memory is not initalized. Waiting for firmware "
            "transfer request from SUT...\n");
  IOB_UART_INIT_BASEADDR(UART1_BASE);

  // receive firmware request from SUT
  // Wait for FRX signal from SUT
  while (uart_getc() != FRX)
    ;
  // Receive filename
  for (i = 0; (msgBuffer[i] = uart_getc()) != '\0'; i++)
    ;
  // Switch back to UART0
  IOB_UART_INIT_BASEADDR(UART0_BASE);

  uart_puts("[Tester]: Received firmware transfer request with filename: ");
  uart_puts(msgBuffer);
  uart_putc('\n');
  uart_puts("[Tester]: Sending transfer request to console...\n");

  // Make request to host
  file_size = uart_recvfile(msgBuffer, sut_firmware);

  uart_puts(
      "[Tester]: SUT firmware obtained. Transfering it to SUT via UART...\n");

  // Switch back to UART1
  IOB_UART_INIT_BASEADDR(UART1_BASE);

  // send file size
  uart_putc((char)(file_size & 0x0ff));
  uart_putc((char)((file_size & 0x0ff00) >> 8));
  uart_putc((char)((file_size & 0x0ff0000) >> 16));
  uart_putc((char)((file_size & 0x0ff000000) >> 24));

  // Wait for ACK signal from SUT
  while (uart_getc() != ACK)
    ;
  if (DEBUG) {
    IOB_UART_INIT_BASEADDR(UART0_BASE);
    uart_puts("[Tester] Got ack! Sending firmware to SUT...\n");
    IOB_UART_INIT_BASEADDR(UART1_BASE);
  }

  // send file contents
  for (i = 0; i < file_size; i++)
    uart_putc(sut_firmware[i]);

  // Ignore firmware sent back
  IOB_UART_INIT_BASEADDR(UART0_BASE);
  uart_puts("[Tester]: SUT firmware transfered. Ignoring firmware readback "
            "sent by SUT...\n");
  IOB_UART_INIT_BASEADDR(UART1_BASE);

  // Wait for FTX signal from SUT
  while (uart_getc() != FTX)
    ;
  // Receive filename
  for (i = 0; (msgBuffer[i] = uart_getc()) != '\0'; i++)
    ;

  // receive file size
  file_size = uart_getc();
  file_size |= ((uint32_t)uart_getc()) << 8;
  file_size |= ((uint32_t)uart_getc()) << 16;
  file_size |= ((uint32_t)uart_getc()) << 24;

  // ignore file contents received
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

  i = 0;
  // Read and store messages sent from SUT
  // Up until it sends the test.log file
  while ((c = uart_getc()) != FTX) {
    msgBuffer[i] = c;
    if (DEBUG) {
      IOB_UART_INIT_BASEADDR(UART0_BASE);
      uart_putc(c);
      IOB_UART_INIT_BASEADDR(UART1_BASE);
    }
    i++;
  }
  msgBuffer[i] = EOT;

  // Receive filename (test.log)
  for (i = 0; uart_getc() != '\0'; i++)
    ;

  // receive file size (test.log)
  file_size = uart_getc();
  file_size |= ((uint32_t)uart_getc()) << 8;
  file_size |= ((uint32_t)uart_getc()) << 16;
  file_size |= ((uint32_t)uart_getc()) << 24;

  // ignore file contents received (test.log)
  for (i = 0; i < file_size; i++)
    uart_getc();

  // End UART1 connection with SUT
  uart_finish();

  // Switch back to UART0
  IOB_UART_INIT_BASEADDR(UART0_BASE);

  // Send messages previously stored from SUT
  uart_puts("[Tester]: #### Messages received from SUT: ####\n\n");
  for (i = 0; msgBuffer[i] != EOT; i++) {
    uart_putc(msgBuffer[i]);
  }
  uart_puts("\n[Tester]: #### End of messages received from SUT ####\n\n");

  // Read data from the SUT's registers
  uart_puts("[Tester]: Reading SUT's register contents:\n");
  printf("[Tester]: Register 3: %d \n", IOB_SOC_SUT_GET_REG3());
  printf("[Tester]: Register 4: %d \n", IOB_SOC_SUT_GET_REG4());

  // Read pattern from GPIO inputs (was set by the SUT)
  printf("\n[Tester]: Pattern read from GPIO inputs: 0x%x\n", gpio_get());

  // Read byte stream via AXI stream
  receive_axistream();

#ifdef USE_EXTMEM
  uart_puts("\n[Tester] Using shared external memory. Obtain SUT memory string "
            "pointer via SUT's register 5...\n");
  uart_puts("[Tester]: String pointer is: ");
  printf("0x%x", IOB_SOC_SUT_GET_REG5());
  uart_putc('\n');
  // Get address of string stored in SUT's memory
  // and invert the highest bit of MEM_ADDR_W to access the SUT's memory zone
  sutStr = (char *)(IOB_SOC_SUT_GET_REG5() ^ (1 << (MEM_ADDR_W - 1)));

  // Print the string by accessing that address
  uart_puts("[Tester]: String read from SUT's memory via shared memory:\n");
  for (i = 0; sutStr[i] != '\0'; i++) {
    uart_putc(sutStr[i]);
  }
  uart_putc('\n');
#endif

  // Allocate memory for ILA output data
  uint32_t ila_n_samples = ila_number_samples();
  uint32_t ila_data_size = ila_output_data_size(ila_n_samples, ILA0_DWORD_SIZE);
  char* ila_data = (char *)malloc(ila_data_size);
  // Write data to allocated memory
  ila_output_data(ila_data, 0, ila_n_samples, ILA0_DWORD_SIZE);
  // Send ila data to file via UART
  uart_sendfile("ila_data.bin", ila_data_size-1, ila_data); //Don't send last byte (\0)
  free(ila_data);

  uart_puts("\n[Tester]: Verification successful!\n\n");

  // End UART0 connection
  uart_finish();
}

void print_ila_samples() {
  uart_puts("[Tester]: ILA values sampled from the AXI input FIFO of SUT: \n");
  uart_puts("[Tester]: | Timestamp | FIFO level | AXI input value |\n");
  // From the ILA0 configuration: bits 0-15 are the timestamp; bits 16-47 are fifo_value; bits 48-52 are the fifo_level
  uint32_t i, fifo_value;
  uint16_t initial_time = (uint16_t)ila_get_large_value(0,0);
  for(i=0; i< ila_number_samples(); i++){
    fifo_value = ila_get_large_value(i,1)<<16 | ila_get_large_value(i,0)>>16;
    printf("[Tester]: | %06d    | 0x%02x       | 0x%08x      |\n",(uint16_t)(ila_get_large_value(i,0)-initial_time), ila_get_large_value(i,1)>>16, fifo_value);
  }
  uart_putc('\n');
}

void send_axistream() {
  uint8_t byte_stream[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  uint8_t i;
  // Print byte stream to send
  uart_puts("[Tester]: Sending AXI stream bytes: ");
  for (i = 0; i < sizeof(byte_stream); i++)
    printf("0x%x ", byte_stream[i]);
  uart_puts("\n\n");
  // Send bytes to AXI stream output
  for (i = 0; i < sizeof(byte_stream) - 1; i++)
    axistream_out_push(byte_stream + i, 1, 0);
  axistream_out_push(byte_stream + i, 1,
                     1); // Send the last byte with the TLAST signal
}

void receive_axistream() {
  uint8_t byte_stream[64];
  uint8_t i, total_received_bytes;

  // Check if we are receiving an AXI stream
  if (!axistream_in_empty()) {
    // Receive bytes while stream does not end (by TLAST signal), or up to 64
    // bytes
    for (total_received_bytes = 0;
         !axistream_in_pop(byte_stream + total_received_bytes, &i) &&
         total_received_bytes < 64;
         total_received_bytes += i)
      ;
    if (total_received_bytes < 64)
      total_received_bytes += i;
    // Print received bytes
    uart_puts("[Tester]: Received AXI stream bytes: ");
    for (i = 0; i < total_received_bytes; i++)
      printf("0x%x ", byte_stream[i]);
    uart_puts("\n\n");
  } else {
    // Input AXI stream queue is empty
    uart_puts("[Tester]: Error: AXI stream input is empty.\n\n");
  }
}
