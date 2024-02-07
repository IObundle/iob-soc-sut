/*********************************************************
 *             Tester Verification Program               *
 *********************************************************/
 // Place the compiled source in /etc/init.d/S99IObundleVerification

//#include "bsp.h"
//#include "iob-axistream-in.h"
//#include "iob-axistream-out.h"
//#include "iob-gpio.h"
#include "iob-uart16550.h"
//
//// System may not use ILA/PFSM for Quartus boards
//#if __has_include("ILA0.h")
//#include "ILA0.h" // ILA0 instance specific defines
//#include "iob-ila.h"
//#include "iob-pfsm.h"
//#define USE_ILA_PFSM
//#endif
//
//#include "iob-dma.h"
//#include "iob-eth.h"
//#include "iob_soc_sut_swreg.h"
//#include "iob_soc_tester_conf.h"
//#include "iob_soc_tester_periphs.h"
//#include "iob_soc_tester_system.h"
//#include "printf.h"
#include <stdlib.h>
#include <stdio.h>
//#include <stdint.h>
//#include <string.h>
//#include "iob_eth_rmac.h"
//#define ETH_MAC_ADDR 0x01606e11020f
//
// Enable debug messages.
#define DEBUG 0

#define SUT_FIRMWARE_SIZE 29000

// We should ideally receive the INIT_MEM information from the PC
// However, since most of the time we run linux on the FPGA,
// the INIT_MEM will be 0, therefore we hardcode it for now.
#undef IOB_SOC_TESTER_INIT_MEM

// Serial port connected to SUT
#define UART1_BASE "/dev/ttyS0"

//void print_ila_samples();
//void send_axistream();
//void receive_axistream();
//void pfsm_program(char *);
//void ila_monitor_program(char *);
//void clear_cache();

/*
 * Receive a file transfer request from SUT;
 * relay that request to the console;
 * receive file contents from console; 
 * and send them to the SUT.
 *
 * small_buffer: buffer to store strings
 * file_content_buffer: buffer to store file content
*/
void relay_file_transfer_to_sut(char *small_buffer, char *file_content_buffer){
  int i;
  uint32_t file_size = 0;
  FILE *fptr;

  // receive file transfer request from SUT
  // Wait for FRX signal from SUT
  while (uart16550_getc() != FRX)
    ;
  // Receive filename
  for (i = 0; (small_buffer[i] = uart16550_getc()) != '\0'; i++);

  puts("[Tester]: Received file transfer request with filename: ");
  puts(small_buffer);
  putchar('\n');
  puts("[Tester]: Sending transfer request to console...\n");

  // Make request to host
  //file_size = uart16550_recvfile(small_buffer, file_content_buffer);
  sprintf(small_buffer+1000, "rz -e %s", small_buffer);
  i = system(small_buffer+1000);
  if (i != 0) puts("[Tester]: File transfer via rz failed!\n");
  fptr = fopen(small_buffer, "r");
  file_size = fread(file_content_buffer, 1, SUT_FIRMWARE_SIZE, fptr);
  fclose(fptr);

  puts(
      "[Tester]: SUT file obtained. Transfering it to SUT via UART...\n");

  // send file size
  uart16550_putc((char)(file_size & 0x0ff));
  uart16550_putc((char)((file_size & 0x0ff00) >> 8));
  uart16550_putc((char)((file_size & 0x0ff0000) >> 16));
  uart16550_putc((char)((file_size & 0x0ff000000) >> 24));
  // Wait for ACK signal from SUT
  while (uart16550_getc() != ACK);

  if (DEBUG) {
    puts("[Tester] Got ack! Sending firmware to SUT...\n");
  }

  // send file contents to SUT
  for (i = 0; i < file_size; i++)
    uart16550_putc(file_content_buffer[i]);
}

int main() {
  char pass_string[] = "Test passed!";
  //char fail_string[] = "Test failed!";
  uint32_t file_size = 0;
  char c, buffer[5096];//, *sutStr;
  int i;
#ifndef IOB_SOC_TESTER_INIT_MEM
  char sut_firmware[SUT_FIRMWARE_SIZE];
#endif

//    // Init uart0
//    uart16550_init(UART0_BASE, FREQ/(16*BAUD));
//    printf_init(&uart16550_putc);
//    // Init SUT (connected through REGFILEIF)
//    IOB_SOC_SUT_INIT_BASEADDR(SUT0_BASE);
//    // Init gpio
//    gpio_init(GPIO0_BASE);
//    // init axistream
//    axistream_in_init(AXISTREAMIN0_BASE);
//    axistream_out_init(AXISTREAMOUT0_BASE, 4);
//    axistream_in_enable();
//    axistream_out_enable();
//  
//  #ifdef USE_ILA_PFSM
//      // init integrated logic analyzer
//      ila_init(ILA0_BASE);
//      // Enable ILA circular buffer
//      // This allows for continuous sampling while the enable signal is active
//      ila_set_circular_buffer(1);
//  #endif
//  
//    // init dma
//    dma_init(DMA0_BASE);
//    // init console eth
//    eth_init(ETH0_BASE, &clear_cache);
//  
//    uart16550_puts("\n[Tester]: Waiting for ethernet PHY reset to finish...\n\n");
//    eth_wait_phy_rst();
//  
//  #ifndef SIMULATION
//    // Receive data from console via Ethernet
//    file_size = uart_recvfile_ethernet("../src/eth_example.txt");
//    eth_rcv_file(buffer,file_size);
//    uart16550_puts("\n[Tester]: File received from console via ethernet:\n");
//    for(i=0; i<file_size; i++)
//      uart16550_putc(buffer[i]);
//  #endif
//  
//    // init SUT eth
//    eth_init_mac(ETH1_BASE, ETH_RMAC_ADDR, ETH_MAC_ADDR);
//  
  puts("\n\n[Tester]: Hello from tester!\n\n\n");
//  
//    // Write data to the registers of the SUT to be read by it.
//    IOB_SOC_SUT_SET_REG1(64);
//    IOB_SOC_SUT_SET_REG2(1024);
//    uart16550_puts("[Tester]: Stored values 64 and 1024 in registers 1 and 2 of the "
//              "SUT.\n\n");
//  
//    // Write a test pattern to the GPIO outputs to be read by the SUT.
//    gpio_set(0x1234abcd);
//    uart16550_puts("[Tester]: Placed test pattern 0x1234abcd in GPIO outputs.\n\n");
//  
//  #ifdef USE_ILA_PFSM
//      // Program PFSM
//      pfsm_program(buffer);
//  
//      // Program Monitor PFSM (internal to ILA)
//      ila_monitor_program(buffer);
//  
//      // Enable all ILA triggers
//      ila_enable_all_triggers();
//  #endif
//  
  puts("[Tester]: Initializing SUT via UART...\n");
  // Init to uart1 (connected to the SUT)
  uart16550_init(UART1_BASE);

  // Wait for ENQ signal from SUT
  //while ((c = uart16550_getc()) != ENQ)
  //  if (DEBUG)
  //    putchar(c);
    
  // Send ack to sut
  uart16550_puts("\nTester ACK");

  puts("[Tester]: Received SUT UART enquiry and sent acknowledge.\n");
  
#ifndef IOB_SOC_TESTER_INIT_MEM
  puts("[Tester]: SUT memory is not initalized. Waiting for config file "
            "transfer request from SUT...\n");

  relay_file_transfer_to_sut(buffer, sut_firmware);

  puts("[Tester]: Waiting for firmware transfer request from SUT...\n");

  relay_file_transfer_to_sut(buffer, sut_firmware);

  puts("[Tester]: SUT firmware transfered.");

#endif //ifndef IOB_SOC_TESTER_INIT_MEM
  
//    uart16550_base(UART0_BASE);
//  
//    //Delay to allow time for sut to run bootloader and enable its axistream
//    for ( i = 0; i < (FREQ/BAUD)*256; i++)asm("nop");
//  
//    // Send byte stream via AXI stream
//    send_axistream();
//    
//  #ifdef USE_ILA_PFSM
//      // Disable all ILA triggers
//      ila_disable_all_triggers();
//      
//      // Print sampled ILA values
//      print_ila_samples();
//  #endif
//  
//    // Test sending data to SUT via ethernet
//    uart16550_puts("[Tester]: Sending data to SUT via ethernet:\n");
//    for(i=0; i<64; i++) {
//      buffer[i] = i; 
//      printf("%d ", buffer[i]);
//    }
//    uart16550_putc('\n'); uart16550_putc('\n');
//    // Send file
//    eth_send_file(buffer, 64);
  puts("\n[Tester]: Reading SUT messages...\n");

  i = 0;
  // Read and store messages sent from SUT
  // Up until it sends the test.log file
  while ((c = uart16550_getc()) != FTX) {
    buffer[i] = c;
    if (DEBUG) {
      putchar(c);
    }
    i++;
  }
  buffer[i] = EOT;

  // Receive filename (test.log)
  for (i = 0; uart16550_getc() != '\0'; i++)
    ;

  // receive file size (test.log)
  file_size = uart16550_getc();
  file_size |= ((uint32_t)uart16550_getc()) << 8;
  file_size |= ((uint32_t)uart16550_getc()) << 16;
  file_size |= ((uint32_t)uart16550_getc()) << 24;

  // ignore file contents received (test.log)
  for (i = 0; i < file_size; i++)
    uart16550_getc();

  // End UART1 connection with SUT
  uart16550_finish();

  // Send messages previously stored from SUT
  puts("[Tester]: #### Messages received from SUT: ####\n\n");
  for (i = 0; buffer[i] != EOT; i++) {
    putchar(buffer[i]);
  }
  puts("\n[Tester]: #### End of messages received from SUT ####\n\n");
//  
//    // Read data from the SUT's registers
//    uart16550_puts("[Tester]: Reading SUT's register contents:\n");
//    printf("[Tester]: Register 3: %d \n", IOB_SOC_SUT_GET_REG3());
//    printf("[Tester]: Register 4: %d \n", IOB_SOC_SUT_GET_REG4());
//  
//    // Read pattern from GPIO inputs (was set by the SUT)
//    printf("\n[Tester]: Pattern read from GPIO inputs: 0x%x\n\n", gpio_get());
//  
//    // Read byte stream via AXI stream
//    receive_axistream();
//  
//    uart16550_puts("\n[Tester] Using shared external memory. Obtain SUT memory string "
//              "pointer via SUT's register 5...\n");
//    uart16550_puts("[Tester]: String pointer is: ");
//    printf("0x%x", IOB_SOC_SUT_GET_REG5());
//    uart16550_putc('\n');
//    // Get address of string stored in SUT's memory
//    // and invert the highest bit of MEM_ADDR_W to access the SUT's memory zone
//    sutStr = (char *)(IOB_SOC_SUT_GET_REG5() ^ (1 << (IOB_SOC_TESTER_MEM_ADDR_W - 1)));
//  
//    // Print the string by accessing that address
//    uart16550_puts("[Tester]: String read from SUT's memory via shared memory:\n");
//    for (i = 0; sutStr[i] != '\0'; i++) {
//      uart16550_putc(sutStr[i]);
//    }
//    uart16550_putc('\n');
//  
//  #ifdef USE_ILA_PFSM
//      // Allocate memory for ILA output data
//      const uint32_t ila_n_samples = (1<<4); //Same as buffer size
//      uint32_t ila_data_size = ila_output_data_size(ila_n_samples, ILA0_DWORD_SIZE);
//  
//      // Write data to allocated memory
//      uint32_t latest_sample_index = ila_number_samples();
//      ila_output_data(buffer, latest_sample_index, (latest_sample_index-1)%ila_n_samples, ila_n_samples, ILA0_DWORD_SIZE);
//  
//      // Send ila data to file via UART
//      uart16550_sendfile("ila_data.bin", ila_data_size-1, buffer); //Don't send last byte (\0)
//  #endif
//  
  puts("\n[Tester]: Verification successful!\n\n");
  sprintf(buffer+1000, "echo '%s' > test.log; sz -e test.log", pass_string);
  i = system(buffer+1000);
  if (i != 0) puts("[Tester]: File transfer of test.log via sz failed!\n");
}

// #ifdef USE_ILA_PFSM
// // Program independent PFSM peripheral of the Tester
// void pfsm_program(char *bitstreamBuffer){
//   // init Programmable Finite State Machine
//   pfsm_init(PFSM0_BASE, 2, 1, 1);
//   uint32_t file_size = 0;
//   // Receive pfsm bitstream
//   file_size = uart16550_recvfile("pfsm.bit", bitstreamBuffer);
//   // Program PFSM
//   uart16550_puts("[Tester]: Programming PFSM...\n");
//   printf("[Tester]: Programmed PFSM with %d bytes.\n\n",
//          pfsm_bitstream_program(bitstreamBuffer)
//          );
// }
// 
// // Program Monitor PFSM internal to ILA.
// void ila_monitor_program(char *bitstreamBuffer){
//   // init ILA Monitor (PFSM)
//   pfsm_init(ila_get_monitor_base_addr(ILA0_BASE), 2, 1, 1);
//   uint32_t file_size = 0;
//   // Receive pfsm bitstream
//   file_size = uart16550_recvfile("monitor_pfsm.bit", bitstreamBuffer);
//   // Program PFSM
//   uart16550_puts("[Tester]: Programming Monitor PFSM...\n");
//   printf("[Tester]: Programmed Monitor PFSM with %d bytes.\n\n",
//          pfsm_bitstream_program(bitstreamBuffer)
//          );
// }
// 
// void print_ila_samples() {
//   // From the ILA0 configuration: bits 0-15 are the timestamp; bits 16-47 are fifo_value; bits 48-52 are the fifo_level; bit 53 is PFSM output
//   uint32_t j, i, fifo_value;
//   uint16_t initial_time = (uint16_t)ila_get_large_value(0,0);
//   uint32_t latest_sample_index = ila_number_samples();
//   const uint32_t ila_buffer_size = (1<<4);
// 
//   // Allocate memory for samples
//   // Each buffer sample has 2 * 32 bit words
//   volatile uint32_t *samples = (volatile uint32_t *)malloc((ila_buffer_size*2)*sizeof(uint32_t));
// 
//   // Point ila cursor to the latest sample
//   ila_set_cursor(latest_sample_index,0);
// 
//   uart16550_puts("[Tester]: Storing ILA samples into memory via DMA...\n");
//   dma_start_transfer((uint32_t *)samples+i, ila_buffer_size*2, 1, 1);
// 
//   clear_cache();
// 
//   uart16550_puts("[Tester]: ILA values sampled from the AXI input FIFO of SUT: \n");
//   uart16550_puts("[Tester]: | Timestamp | FIFO level | AXI input value | PFSM output |\n");
//   // For every sample in the buffer
//   for(i=0; i<ila_buffer_size*2; i+=2){
//     fifo_value = samples[i+1]<<16 | samples[i]>>16;
//     printf("[Tester]: | %06d    | 0x%02x       | 0x%08x      | %d           |\n",(uint16_t)(samples[i]-initial_time), samples[i+1]>>16 & 0x1f, fifo_value, samples[i+1]>>21 & 0x1);
//   }
//   uart16550_putc('\n');
// 
//   free((uint32_t *)samples);
// }
// #endif //USE_ILA_PFSM
// 
// void send_axistream() {
//   uint8_t i;
//   uint8_t words_in_byte_stream = 4; 
//   // Allocate memory for byte stream
//   uint32_t *byte_stream = (uint32_t *)malloc(words_in_byte_stream*sizeof(uint32_t));
//   // Fill byte stream to send
//   byte_stream[0] = 0x03020100;
//   byte_stream[1] = 0x07060504;
//   byte_stream[2] = 0xbbaa0908;
//   byte_stream[3] = 0xffeeddcc;
// 
//   // Print byte stream to send
//   uart16550_puts("[Tester]: Sending AXI stream bytes: ");
//   for (i = 0; i < words_in_byte_stream*4; i++)
//     printf("0x%02x ", ((uint8_t *)byte_stream)[i]);
//   uart16550_puts("\n");
// 
//   // Send bytes to AXI stream output via DMA, except the last word.
//   uart16550_puts("[Tester]: Loading AXI words via DMA...\n");
//   dma_start_transfer(byte_stream, words_in_byte_stream-1, 0, 0);
//   // Send the last word with via SWregs with the TLAST signal.
//   uart16550_puts("[Tester]: Loading last AXI word via SWregs...\n\n");
//   axistream_out_push(byte_stream[words_in_byte_stream-1], 1, 1);
// 
//   free(byte_stream);
// }
// 
// void receive_axistream() {
//   uint8_t i;
//   uint8_t n_received_words = axistream_in_fifo_level();
//   
//   // Allocate memory for byte stream
//   volatile uint32_t *byte_stream = (volatile uint32_t *)malloc((n_received_words)*sizeof(uint32_t));
// 
//   // Transfer bytes from AXI stream input via DMA
//   uart16550_puts("[Tester]: Storing AXI words via DMA...\n");
//   dma_start_transfer((uint32_t *)byte_stream+i, n_received_words, 1, 0);
// 
//   clear_cache();
// 
//   // Print byte stream received
//   uart16550_puts("[Tester]: Received AXI stream bytes: ");
//   for (i = 0; i < n_received_words*4; i++)
//     printf("0x%02x ", ((volatile uint8_t *)byte_stream)[i]);
//   uart16550_puts("\n\n");
// 
//   free((uint32_t *)byte_stream);
// }
// 
// void clear_cache(){
//   // Delay to ensure all data is written to memory
//   for ( unsigned int i = 0; i < 10; i++)asm volatile("nop");
//   // Flush VexRiscv CPU internal cache
//   asm volatile(".word 0x500F" ::: "memory");
// }
