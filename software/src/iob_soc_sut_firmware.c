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
#include "iob-timer.h"
#include "iob_regfileif_inverted_swreg.h"
#include "iob-axistream-in.h"
#include "iob-axistream-out.h"
#if __has_include("iob_soc_tester_conf.h")
#define USE_TESTER
#endif

#include "arena.h"
#include "versat_accel.h"
#include "versat_crypto.h"
#include "versat_crypto_tests.h"
#include "versat_interface.h"

#include "api.h"
void nist_kat_init(unsigned char *entropy_input, unsigned char *personalization_string, int security_strength);

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

uint32_t uart_recvfile_ethernet(const char *file_name){
  return uart16550_recvfile_ethernet(file_name);
}

int GetTime(){
  return timer_get_count();
}

void getc_until(char toSee){
  char ch = 0;
  bool seenOne = false;
  while((ch = uart16550_getc()) != toSee){
    if(!seenOne){
      //SUT cannot report because the tester cannot handle receiving unexpected messages from the SUT
      //At least while the SUT is inside the Versat loop.
      //printf("Stream got out of sync, got unexpected data\n");
    }
    seenOne = true;
  }
}

int send_small_string(const char* string){
  uart16550_putc(STX);

  int bytesSent = 0;
  while(bytesSent < 255 && string[bytesSent] != '\0'){
    uart16550_putc(string[bytesSent]);
    bytesSent += 1;
  }

  uart16550_putc(ETX);
}

void send_int(int integer){
  uart16550_putc(integer & 0x000000FF);
  uart16550_putc((integer >> 8) & 0x000000FF);
  uart16550_putc((integer >> 16) & 0x000000FF);
  uart16550_putc((integer >> 24) & 0x000000FF);
}

void send_large_data(const char* buffer,int size){
  uart16550_putc(STX);

  send_int(size);
  
  for(int i = 0; i < size; i++){
    uart16550_putc(buffer[i]);
  }  

#if 0
  connect_terminal();
  eth_send_file(buffer, size);
  connect_sut();
#endif

  uart16550_putc(ETX);
}

int receive_small_string(char* buffer){
  getc_until(STX);

  int bytesSeen = 0;
  char ch = 0;
  while((ch = uart16550_getc()) != ETX){
    buffer[bytesSeen] = ch;
    bytesSeen += 1;
  }

  buffer[bytesSeen] = '\0';
  return bytesSeen;
}

int receive_int(){
  int result = uart16550_getc();
  result |= ((uint32_t)uart16550_getc()) << 8;
  result |= ((uint32_t)uart16550_getc()) << 16;
  result |= ((uint32_t)uart16550_getc()) << 24;

  return result;
}

void read_remaining(){
  char ch = 0;
  while((ch = uart16550_getc()) != EOT){
    printf("%02x ",ch);
  }
  printf("\n");
}

String receive_large_data(Arena* out){
  getc_until(STX);

  int size = receive_int();
  char* res = PushBytes(out,size + 1);

  //printf("Got %d\n",size);

#if 1
  for(int i = 0; i < size; i++){
    res[i] = uart16550_getc();
  }
#endif

//  eth_rcv_file(res,size);
  res[size] = '\0';

  getc_until(ETX);
  return (String){.str = res,.size = size};
}

int main()
{
  char pass_string[] = "Test passed!";
  char fail_string[] = "Test failed!";
  int i;
  char buffer[64];
  char file_buffer[256];
  char misc_buffer[256];
  int ethernet_connected = 0;
  int test_result = 0;

  //init uart
  uart16550_init(UART0_BASE, FREQ/(16*BAUD));
  printf_init(&uart16550_putc);

  timer_init(TIMER0_BASE);

  //init regfileif
  IOB_REGFILEIF_INVERTED_INIT_BASEADDR(REGFILEIF0_BASE);
  //init gpio
  gpio_init(GPIO0_BASE);   
  //init axistream
  IOB_AXISTREAM_IN_INIT_BASEADDR(AXISTREAMIN0_BASE);
  IOB_AXISTREAM_OUT_INIT_BASEADDR(AXISTREAMOUT0_BASE);
  IOB_AXISTREAM_IN_SET_ENABLE(1);
  IOB_AXISTREAM_OUT_SET_ENABLE(1);
  // init eth
  eth_init(ETH0_BASE, &clear_cache);

  // Wait for PHY reset to finish
  eth_wait_phy_rst();

#ifdef USE_TESTER
  // Receive a special string message from tester to tell if its running linux
  receive_small_string(misc_buffer);

  bool tester_run_linux = false;
  if(strcmp(misc_buffer,"TESTER_RUN_LINUX") == 0){
    tester_run_linux = true;
  } else if(strcmp(misc_buffer,"TESTER_RUN_BAREMETAL") == 0){
    tester_run_linux = false;
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

#ifdef USE_TESTER
  uart16550_putc(ENQ);

  versat_init(VERSAT0_BASE);
  ConfigEnableDMA(false);
  Arena arena = InitArena(2*1024*1024);
  globalArena = &arena;

  char ch = 0;
  while((ch = uart16550_getc()) != ETX){
    int mark = MarkArena(globalArena);

    switch(ch){
    case PERFORM_SHA:{

      String received = receive_large_data(globalArena);
      InitVersatSHA();

      unsigned char digest[256];
      VersatSHA(digest,received.str,received.size);

      static const int HASH_SIZE = (256/8);
      char versat_buffer[2048];
      GetHexadecimal((char*) digest,versat_buffer, HASH_SIZE);
      
      send_small_string(versat_buffer);
    } break;
    case PERFORM_AES:{
      String key = receive_large_data(globalArena);
      String plaintext = receive_large_data(globalArena);

      InitVersatAES();
      InitAES();

      unsigned char chiperBuffer[16 + 1];
      AES_ECB256(key.str,plaintext.str,chiperBuffer);
      
      char versat_buffer[2048];
      GetHexadecimal((char*) chiperBuffer,versat_buffer, 16);      
      send_small_string(versat_buffer);
    } break;
    case PERFORM_MCELIECE:{
      String seed = receive_large_data(globalArena);

      nist_kat_init(seed.str, NULL, 256);

      unsigned char* public_key = PushArray(globalArena,PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_PUBLICKEYBYTES,unsigned char);
      unsigned char* secret_key = PushArray(globalArena,PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_SECRETKEYBYTES,unsigned char);

      VersatMcEliece(public_key, secret_key);

      send_large_data(public_key,PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_PUBLICKEYBYTES);
      send_large_data(secret_key,PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_SECRETKEYBYTES);
    } break;
    case PERFORM_MCELIECE_SHORT:{
      String seed = receive_large_data(globalArena);

      nist_kat_init(seed.str, NULL, 256);

      unsigned char* public_key = PushArray(globalArena,PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_PUBLICKEYBYTES,unsigned char);
      unsigned char* secret_key = PushArray(globalArena,PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_SECRETKEYBYTES,unsigned char);

      VersatMcEliece(public_key, secret_key);

      int pkOffset = PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_PUBLICKEYBYTES - 512;
      int skOffset = PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_SECRETKEYBYTES - 512;

      send_large_data(public_key,512);
      send_large_data(public_key + pkOffset,512);
      send_large_data(secret_key,512);
      send_large_data(secret_key + skOffset,512);
    } break;
    default: goto end; // Something is wrong 
    }

    PopArena(globalArena,mark);
  }
end:
#endif // USE_TESTER

  uart16550_finish();
}

// Read AXI stream input, print, and relay data to AXI stream output
void axistream_loopback(){
  uint32_t byte_stream[16] = {0};
  uint8_t i, received_words;
  
  //Check if we are receiving an AXI stream
  if(!IOB_AXISTREAM_IN_GET_FIFO_EMPTY()){
    // Read up to NWORD 32-bit words, or up to 16 32-bit words
    received_words = IOB_AXISTREAM_IN_GET_NWORDS();
    for(i=0; i<received_words && i<16; i++){
      byte_stream[i] = IOB_AXISTREAM_IN_GET_DATA();
    }
    
    // Print received bytes
    // uart16550_puts("[SUT]: Received AXI stream bytes: ");
    printf("[SUT]: Received AXI stream %d bytes: ",received_words*4);
    // for(i=0;i<received_words*4;i++)
    //   printf("0x%02x ", ((uint8_t *)byte_stream)[i]);
    for(i=0;i<received_words;i++)
      printf("0x%08x ", byte_stream[i]);

    // Send bytes to AXI stream output
    IOB_AXISTREAM_OUT_SET_NWORDS(received_words);
    for(i=0;i<received_words;i++)
      iob_axis_write(byte_stream[i]);

    uart16550_puts("\n[SUT]: Sent AXI stream bytes back via output interface.\n\n");
  } else {
    // Input AXI stream queue is empty
    uart16550_puts("[SUT]: AXI stream input is empty. Skipping AXI stream transfer.\n\n");
  }

}
