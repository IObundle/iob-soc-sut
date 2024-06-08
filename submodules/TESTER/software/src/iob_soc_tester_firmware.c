/*********************************************************
 *                    Tester Firmware                    *
 *********************************************************/
#include "bsp.h"
#include "iob-axistream-in.h"
#include "iob-axistream-out.h"
#include "iob-gpio.h"
#include "iob-uart16550.h"

// System may not use ILA/PFSM for Quartus boards
#if __has_include("ILA0.h")
#include "ILA0.h" // ILA0 instance specific defines
#include "iob-ila.h"
#include "iob-pfsm.h"
#define USE_ILA_PFSM
#define ILA0_BUFFER_W IOB_SOC_TESTER_ILA0_BUFFER_W
#endif

#include "iob-dma.h"
#include "iob-eth.h"
#include "iob_soc_sut_swreg.h"
#include "iob_soc_tester_conf.h"
#include "iob_soc_tester_periphs.h"
#include "iob_soc_tester_system.h"
#include "printf.h"
#include "stdlib.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "iob_eth_rmac.h"
#define ETH_MAC_ADDR 0x01606e11020f

#include "versat_interface.h"

// Enable debug messages.
#define DEBUG 1

#define SUT_FIRMWARE_SIZE 150000

#if 0
#define printf_terminal(...) \
  { \
    int last = connect_terminal(); \
    printf(__ARGS__); \
    restore_connection(last); \
  }

#define printf_sut(...) \
  { \
    int last = connect_sut(); \
    printf(__ARGS__); \
    restore_connection(last); \
  }

#undef printf // Do not use a naked printf.
#endif

void print_ila_samples();
void send_axistream();
void receive_axistream();
void pfsm_program(char *);
void ila_monitor_program(char *);

void clear_cache(){
  // Delay to ensure all data is written to memory
  for ( unsigned int i = 0; i < 10; i++)asm volatile("nop");
  // Flush VexRiscv CPU internal cache
  asm volatile(".word 0x500F" ::: "memory");
}

// Send signal by uart to receive file by ethernet
uint32_t uart_recvfile_ethernet(char *file_name) {

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

int GetTime(){
  return 0;
}

#include "arena.h"

typedef struct{
  char* str;
  int size;
} String;

int connect_terminal(){
  return uart16550_base(UART0_BASE);
}

int connect_sut(){
  return uart16550_base(UART1_BASE);
}

void restore_connection(int last_connection){
  uart16550_base(last_connection);
}

String PushFile(const char* filepath){
  int last = connect_terminal();
  // Make request to host
  int file_size = uart16550_recvfile(filepath, globalArena->ptr);
  char* content = PushBytes(globalArena,file_size + 1);
  content[file_size] = '\0';

  restore_connection(last);
  return (String){.str=content,.size=file_size};
}

// When sending multiple amounts of data to sut
// Need to call end_message afterwards
void start_message(){
  int connection = connect_sut();
  uart16550_putc(STX);
  restore_connection(connection);
}

void end_message(){
  int connection = connect_sut();
  uart16550_putc(ETX);
  restore_connection(connection);
}

void report(const char* message){
  int last = connect_terminal();
  uart16550_puts(message);
  restore_connection(last);
}

void report_string(String str){
  int last = connect_terminal();
  printf("%.*s",str.size,str.str);
  restore_connection(last);
}

// A small string contains a maximum of 255 bytes (256 if counting '\0')
// Anything bigger is cutoff
void send_small_string(const char* string){
  start_message();

  int bytesSent = 0;
  while(bytesSent < 255 && string[bytesSent] != '\0'){
    uart16550_putc(string[bytesSent]);
    bytesSent += 1;
  }

  if(string[bytesSent] != '\0'){
    report("[TESTER] send_small_string was cutoff\n");
  }

  end_message();
}

void getc_until(char toSee){
  char ch = 0;
  bool seenOne = false;
  while((ch = uart16550_getc()) != toSee){
    if(!seenOne){
      report("Stream got out of sync, got unexpected data:");
    }
    int last = connect_terminal();
    printf("%02x ",ch);
    restore_connection(last);

    seenOne = true;
  }
  if(seenOne){
    report("\n");    
  }
}

String receive_small_string(){
  int last = connect_sut();
  getc_until(STX);

  int bytesSeen = 0;
  char ch = 0;
  char* buffer = globalArena->ptr;
  while((ch = uart16550_getc()) != ETX){
    buffer[bytesSeen] = ch;
    bytesSeen += 1;
  }
  PushBytes(globalArena,bytesSeen + 1);

  buffer[bytesSeen] = '\0';

  restore_connection(last);
  return (String){.str = buffer,.size = bytesSeen};
}

void send_int(int integer){
  int last = connect_sut();
  uart16550_putc(integer & 0x000000FF);
  uart16550_putc((integer >> 8) & 0x000000FF);
  uart16550_putc((integer >> 16) & 0x000000FF);
  uart16550_putc((integer >> 24) & 0x000000FF);
  restore_connection(last);
}

void send_large_data(const char* buffer,int size){
  start_message();

  send_int(size);
  
#if 1
  int last = connect_sut();
  for(int i = 0; i < size; i++){
    uart16550_putc(buffer[i]);
  }  
  restore_connection(last);
#endif

#if 0
  connect_terminal();
  eth_send_file(buffer, size);
  connect_sut();
#endif

  end_message();
}

#include <string.h>
#define STRING(str) (String){str,strlen(str)}

char* SearchAndAdvance(char* ptr,String str){
  char* firstChar = strstr(ptr,str.str);
  if(firstChar == NULL){
    return NULL;
  }

  char* advance = firstChar + str.size;
  return advance;
}

int ParseNumber(char* ptr){
  int count = 0;

  while(ptr != NULL){
    char ch = *ptr;

    if(ch >= '0' && ch <= '9'){
      count *= 10;
      count += ch - '0';
      ptr += 1;
      continue;
    }

    break;
  }

  return count;
}

char GetHexadecimalChar(unsigned char value){
  if(value < 10){
    return '0' + value;
  } else{
    return 'A' + (value - 10);
  }
}

char* GetHexadecimal(const char* text,char* buffer,int str_size){
  int i = 0;
  unsigned char* view = (unsigned char*) text;
  for(; i< str_size; i++){
    buffer[i*2] = GetHexadecimalChar(view[i] / 16);
    buffer[i*2+1] = GetHexadecimalChar(view[i] % 16);
  }

  buffer[i*2] = '\0';

  return buffer;
}

static char HexToInt(char ch){
   if('0' <= ch && ch <= '9'){
      return (ch - '0');
   } else if('a' <= ch && ch <= 'f'){
      return ch - 'a' + 10;
   } else if('A' <= ch && ch <= 'F'){
      return ch - 'A' + 10;
   } else {
      return 0x7f;
   }
}

// Make sure that buffer is capable of storing the whole thing. Returns number of bytes inserted
int HexStringToHex(char* buffer,const char* str){
   int inserted = 0;
   for(int i = 0; ; i += 2){
      char upper = HexToInt(str[i]);
      char lower = HexToInt(str[i+1]);

      if(upper >= 16 || lower >= 16){
         if(upper < 16){ // Upper is good but lower is not
            report("Warning: HexString was not divisible by 2\n");
         }
         break;
      }

      buffer[inserted++] = upper * 16 + lower;
   }

   return inserted;
}

typedef struct {
  int initTime;
  int tests;
  int goodTests;
  int versatTimeAccum;
  int softwareTimeAccum;
  int earlyExit;
} TestState;

TestState VersatCommonSHATests(String content){
  TestState testResult = {};

  int mark = MarkArena(globalArena);

  static const int HASH_SIZE = (256/8);

  char* ptr = content.str;
  while(1){
    int testMark = MarkArena(globalArena);

    ptr = SearchAndAdvance(ptr,STRING("LEN = "));
    if(ptr == NULL){
      break;
    }

    int len = ParseNumber(ptr);

    ptr = SearchAndAdvance(ptr,STRING("MSG = "));
    if(ptr == NULL){ // Note: It's only a error if any check after the first one fails, because we are assuming that if the first passes then that must mean that the rest should pass as well.
      testResult.earlyExit = 1;
      break;
    }

    unsigned char* message = PushArray(globalArena,len,unsigned char);
    int bytes = HexStringToHex(message,ptr);

    ptr = SearchAndAdvance(ptr,STRING("MD = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* expected = ptr;

    int last = connect_sut();
    uart16550_putc(PERFORM_SHA);

    if(len == 0){
      send_large_data("",0);
    } else {
      send_large_data(message,bytes);
    }

    String result = receive_small_string();
    restore_connection(last);

    bool good = true;
    for(int i = 0; i < 64; i++){
      if(result.str[i] != expected[i]){
        good = false;
        break;
      }
    }

    if(good){
      testResult.goodTests += 1;
    } else {
      int last = connect_terminal();
      printf("SHA Test %02d: Error\n",testResult.tests);
      printf("  Expected: %.64s\n",expected); 
      printf("  Versat:   %.*s\n",result.size,result.str);
      restore_connection(last);
    }

    testResult.tests += 1;
    PopArena(globalArena,testMark);
  }

  PopArena(globalArena,mark);

  return testResult;
}

TestState VersatCommonAESTests(String content){
  TestState testResult = {};

  int mark = MarkArena(globalArena);

  char* ptr = content.str;
  while(1){
    int testMark = MarkArena(globalArena);

    ptr = SearchAndAdvance(ptr,STRING("COUNT = "));
    if(ptr == NULL){
      break;
    }

    int count = ParseNumber(ptr);

    ptr = SearchAndAdvance(ptr,STRING("KEY = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    unsigned char* key = PushArray(globalArena,32 + 1,unsigned char);
    HexStringToHex(key,ptr);

    ptr = SearchAndAdvance(ptr,STRING("PLAINTEXT = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }
  
    unsigned char* plain = PushArray(globalArena,16 + 1,unsigned char);
    HexStringToHex(plain,ptr);

    ptr = SearchAndAdvance(ptr,STRING("CIPHERTEXT = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* cypher = ptr;

    int last = connect_sut();
    uart16550_putc(PERFORM_AES);    

    send_large_data(key,32);
    send_large_data(plain,16);

    String result = receive_small_string();
    restore_connection(last);

    bool good = true;
    for(int i = 0; i < 16; i++){
      if(result.str[i] != cypher[i]){
        good = false;
        break;
      }
    }

    if(good){
      testResult.goodTests += 1;
    } else {
      int last = connect_terminal();
      printf("AES Test %02d: Error\n",testResult.tests);
      printf("  Expected: %.32s\n",cypher); 
      printf("  Versat:   %.*s\n",result.size,result.str);
      restore_connection(last);
    }

    testResult.tests += 1;
    PopArena(globalArena,testMark);
  }

  PopArena(globalArena,mark);

  return testResult;
}

/*
 * Receive a file transfer request from SUT;
 * relay that request to the console;
 * receive file contents from console; 
 * and send them to the SUT.
 *
 * file_name_buffer: buffer to store file name
 * file_content_buffer: buffer to store file content
*/
void relay_file_transfer_to_sut(char *file_name_buffer, char *file_content_buffer){
  int i;
  uint32_t file_size = 0;

  uart16550_base(UART1_BASE);
  // receive file transfer request from SUT
  // Wait for FRX signal from SUT
  while (uart16550_getc() != FRX)
    ;
  // Receive filename
  for (i = 0; (file_name_buffer[i] = uart16550_getc()) != '\0'; i++);

  uart16550_base(UART0_BASE);
  uart16550_puts("[Tester]: Received file transfer request with filename: ");
  uart16550_puts(file_name_buffer);
  uart16550_putc('\n');
  uart16550_puts("[Tester]: Sending transfer request to console...\n");

  // Make request to host
  file_size = uart16550_recvfile(file_name_buffer, file_content_buffer);

  uart16550_puts(
      "[Tester]: SUT file obtained. Transfering it to SUT via UART...\n");

  uart16550_base(UART1_BASE);
  // send file size
  uart16550_putc((char)(file_size & 0x0ff));
  uart16550_putc((char)((file_size & 0x0ff00) >> 8));
  uart16550_putc((char)((file_size & 0x0ff0000) >> 16));
  uart16550_putc((char)((file_size & 0x0ff000000) >> 24));
  // Wait for ACK signal from SUT
  while (uart16550_getc() != ACK);

  if (DEBUG) {
    uart16550_base(UART0_BASE);
    uart16550_puts("[Tester] Got ack! Sending firmware to SUT...\n");
    uart16550_base(UART1_BASE);
  }

  // send file contents to SUT
  for (i = 0; i < file_size; i++)
    uart16550_putc(file_content_buffer[i]);
}

int main() {
  char pass_string[] = "Test passed!";
  char fail_string[] = "Test failed!";
  uint32_t file_size = 0;
  char c, buffer[5096], *sutStr;
  int i;
#ifndef IOB_SOC_TESTER_INIT_MEM
  char* sut_firmware = (char*) malloc(SUT_FIRMWARE_SIZE);
  //char sut_firmware[SUT_FIRMWARE_SIZE];
#endif

  // Init uart0
  uart16550_init(UART0_BASE, FREQ/(16*BAUD));
  printf_init(&uart16550_putc);
  // Init SUT (connected through REGFILEIF)
  IOB_SOC_SUT_INIT_BASEADDR(SUT0_BASE);
  // Init gpio
  gpio_init(GPIO0_BASE);
  // init axistream
  IOB_AXISTREAM_IN_INIT_BASEADDR(AXISTREAMIN0_BASE);
  IOB_AXISTREAM_OUT_INIT_BASEADDR(AXISTREAMOUT0_BASE);
  IOB_AXISTREAM_IN_SET_ENABLE(1);
  IOB_AXISTREAM_OUT_SET_ENABLE(1);

#ifdef USE_ILA_PFSM
    // init integrated logic analyzer
    ila_init(ILA0_BASE);
    // Enable ILA circular buffer
    // This allows for continuous sampling while the enable signal is active
    ila_set_circular_buffer(1);
#endif

  // init dma
  dma_init(DMA0_BASE);
  // init console eth
  eth_init(ETH0_BASE, &clear_cache);

  uart16550_puts("\n[Tester]: Waiting for ethernet PHY reset to finish...\n\n");
  eth_wait_phy_rst();

#ifndef SIMULATION
  // Receive data from console via Ethernet
  file_size = uart_recvfile_ethernet("../src/eth_example.txt");
  eth_rcv_file(buffer,file_size);
  uart16550_puts("\n[Tester]: File received from console via ethernet:\n");
  for(i=0; i<file_size; i++)
    uart16550_putc(buffer[i]);
#endif

  // init SUT eth
  eth_init_mac(ETH1_BASE, ETH_RMAC_ADDR, ETH_MAC_ADDR);

  uart16550_puts("\n\n[Tester]: Hello from tester!\n\n\n");

  // Write data to the registers of the SUT to be read by it.
  IOB_SOC_SUT_SET_REG1(64);
  IOB_SOC_SUT_SET_REG2(1024);
  uart16550_puts("[Tester]: Stored values 64 and 1024 in registers 1 and 2 of the "
            "SUT.\n\n");

  // Write a test pattern to the GPIO outputs to be read by the SUT.
  gpio_set(0x1234abcd);
  uart16550_puts("[Tester]: Placed test pattern 0x1234abcd in GPIO outputs.\n\n");

#ifdef USE_ILA_PFSM
    // Program PFSM
    pfsm_program(buffer);

    // Program Monitor PFSM (internal to ILA)
    ila_monitor_program(buffer);

    // Enable all ILA triggers
    ila_enable_all_triggers();
#endif

  uart16550_puts("[Tester]: Initializing SUT via UART...\n");
  // Init and switch to uart1 (connected to the SUT)
  uart16550_init(UART1_BASE, FREQ/(16*BAUD));

  // Wait for ENQ signal from SUT
  while ((c = uart16550_getc()) != ENQ)
    if (DEBUG) {
      uart16550_base(UART0_BASE);
      uart16550_putc(c);
      uart16550_base(UART1_BASE);
    };
    
  // Send ack to sut
  uart16550_puts("\nTester ACK");

  uart16550_base(UART0_BASE);
  uart16550_puts("[Tester]: Received SUT UART enquiry and sent acknowledge.\n");
  
#ifndef IOB_SOC_TESTER_INIT_MEM
  uart16550_base(UART0_BASE);
  uart16550_puts("[Tester]: SUT memory is not initalized. Waiting for config file "
            "transfer request from SUT...\n");
  uart16550_base(UART1_BASE);

  relay_file_transfer_to_sut(buffer, sut_firmware);

  uart16550_base(UART0_BASE);
  uart16550_puts("[Tester]: Waiting for firmware transfer request from SUT...\n");
  uart16550_base(UART1_BASE);

  relay_file_transfer_to_sut(buffer, sut_firmware);

  uart16550_base(UART0_BASE);
  uart16550_puts("[Tester]: SUT firmware transfered.\n\n");

#endif //IOB_SOC_TESTER_INIT_MEM

  //Delay to allow time for sut to run bootloader and enable its axistream
  for ( i = 0; i < (FREQ/BAUD)*256; i++)asm("nop");

  // Send byte stream via AXI stream
  send_axistream();

#ifdef USE_ILA_PFSM
    // Disable all ILA triggers
    ila_disable_all_triggers();
    
    // Print sampled ILA values
    print_ila_samples();
#endif

  // Tell SUT that the Tester is running baremetal
  uart16550_base(UART1_BASE);
  send_small_string("TESTER_RUN_BAREMETAL");
  uart16550_base(UART0_BASE);

  // Test sending data to SUT via ethernet
  uart16550_puts("[Tester]: Sending data to SUT via ethernet:\n");
  for(i=0; i<64; i++) {
    buffer[i] = i; 
    printf("%d ", buffer[i]);
  }
  uart16550_putc('\n'); uart16550_putc('\n');
  // Send file
  eth_send_file(buffer, 64);

  uart16550_puts("\n[Tester]: Reading SUT messages...\n");
  uart16550_base(UART1_BASE);

  i = 0;
  // Read and store messages sent from SUT
  // Up until it sends the test.log file
  while ((c = uart16550_getc()) != FTX) {
    buffer[i] = c;
    if (DEBUG) {
      uart16550_base(UART0_BASE);
      uart16550_putc(c);
      uart16550_base(UART1_BASE);
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

  while(uart16550_getc() != ENQ); // Read the messages outputted by file transfer function

  // At this point the SUT is in waiting for Versat requests mode
  // UART is used to send requests and to receive results

  // Request SUT to perform various cryptographic operations
  Arena globalArenaInst = InitArena(1 * 1024 * 1024); 
  globalArena = &globalArenaInst;
#if 1
  connect_sut();
  
  #if 0
  String content = PushFile("../../software/KAT/SHA256ShortMsg.rsp");
  VersatCommonSHATests(content);
  #endif

  String content = PushFile("../../software/KAT/AESECB256.rsp");
  VersatCommonAESTests(content);

#if 0
  uart16550_putc(PERFORM_SHA);
  send_large_data("OLA",3);

  String result = receive_small_string();
  report_string(result);
#endif

#if 0
  uart16550_putc(PERFORM_AES);

  send_large_data("CC22DA787F375711C76302BEF0979D8EDDF842829C2B99EF3DD04E23E54CC24B",64);
  send_large_data("CCC62C6B0A09A671D64456818DB29A4D",32);
#endif

  uart16550_putc(ETX);

  connect_terminal();
#endif

  // End UART1 connection with SUT
  uart16550_finish();

  // Switch back to UART0
  uart16550_base(UART0_BASE);

  // Send messages previously stored from SUT
  uart16550_puts("[Tester]: #### Messages received from SUT: ####\n\n");
  if(!DEBUG){
    for (i = 0; buffer[i] != EOT; i++) {
      uart16550_putc(buffer[i]);
    }
  }
  uart16550_puts("\n[Tester]: #### End of messages received from SUT ####\n\n");

  // Read data from the SUT's registers
  uart16550_puts("[Tester]: Reading SUT's register contents:\n");
  printf("[Tester]: Register 3: %d \n", IOB_SOC_SUT_GET_REG3());
  printf("[Tester]: Register 4: %d \n", IOB_SOC_SUT_GET_REG4());

  // Read pattern from GPIO inputs (was set by the SUT)
  printf("\n[Tester]: Pattern read from GPIO inputs: 0x%x\n\n", gpio_get());

  // Read byte stream via AXI stream
  receive_axistream();

  uart16550_puts("\n[Tester] Using shared external memory. Obtain SUT memory string "
            "pointer via SUT's register 5...\n");
  uart16550_puts("[Tester]: String pointer is: ");
  printf("0x%x", IOB_SOC_SUT_GET_REG5());
  uart16550_putc('\n');
  // Get address of string stored in SUT's memory
  // and invert the highest bit of MEM_ADDR_W to access the SUT's memory zone
  sutStr = (char *)(IOB_SOC_SUT_GET_REG5() ^ (1 << (IOB_SOC_TESTER_MEM_ADDR_W - 1)));

  // Print the string by accessing that address
  uart16550_puts("[Tester]: String read from SUT's memory via shared memory:\n");
  for (i = 0; sutStr[i] != '\0'; i++) {
    uart16550_putc(sutStr[i]);
  }
  uart16550_putc('\n');

#ifdef USE_ILA_PFSM
    // Allocate memory for ILA output data
    uint32_t ila_data_size = ila_output_data_size(ILA0_BUFFER_SIZE, ILA0_DWORD_SIZE);

    // Write data to allocated memory
    uint32_t latest_sample_index = ila_number_samples();
    ila_output_data(buffer, latest_sample_index, (latest_sample_index-1)%ILA0_BUFFER_SIZE, ILA0_BUFFER_SIZE, ILA0_DWORD_SIZE);

    // Send ila data to file via UART
    uart16550_sendfile("ila_data.bin", ila_data_size-1, buffer); //Don't send last byte (\0)
#endif

  uart16550_puts("\n[Tester]: Verification successful!\n\n");
  uart16550_sendfile("test.log", strlen(pass_string), pass_string);

  // End UART0 connection
  uart16550_finish();
}

#ifdef USE_ILA_PFSM
// Program independent PFSM peripheral of the Tester
void pfsm_program(char *bitstreamBuffer){
  // init Programmable Finite State Machine
  pfsm_init(PFSM0_BASE, 2, 1, 1);
  uint32_t file_size = 0;
  // Receive pfsm bitstream
  file_size = uart16550_recvfile("pfsm.bit", bitstreamBuffer);
  // Program PFSM
  uart16550_puts("[Tester]: Programming PFSM...\n");
  printf("[Tester]: Programmed PFSM with %d bytes.\n\n",
         pfsm_bitstream_program(bitstreamBuffer)
         );
}

// Program Monitor PFSM internal to ILA.
void ila_monitor_program(char *bitstreamBuffer){
  // init ILA Monitor (PFSM)
  pfsm_init(ila_get_monitor_base_addr(ILA0_BASE), 2, 2, 1);
  uint32_t file_size = 0;
  // Receive pfsm bitstream
  file_size = uart16550_recvfile("monitor_pfsm.bit", bitstreamBuffer);
  // Program PFSM
  uart16550_puts("[Tester]: Programming Monitor PFSM...\n");
  printf("[Tester]: Programmed Monitor PFSM with %d bytes.\n\n",
         pfsm_bitstream_program(bitstreamBuffer)
         );
}

void print_ila_samples() {
  // From the ILA0 configuration: bits 0-15 are the timestamp; bits 16-47 are fifo_value; bits 48-52 are the fifo_level; bit 53 is PFSM output
  uint32_t j, i, fifo_value;
  uint16_t initial_time = (uint16_t)ila_get_large_value(0,0);
  uint32_t latest_sample_index = ila_number_samples();

  // Allocate memory for samples
  // Each buffer sample has 2 * 32 bit words
  volatile uint32_t *samples = (volatile uint32_t *)malloc((ILA0_BUFFER_SIZE*2)*sizeof(uint32_t));

  // Point ila cursor to the latest sample
  ila_set_cursor(latest_sample_index,0);

  uart16550_puts("[Tester]: Storing ILA samples into memory via DMA...\n");
  dma_start_transfer((uint32_t *)samples, ILA0_BUFFER_SIZE*2, 1, 1);

  clear_cache();

  uart16550_puts("[Tester]: ILA values sampled from the AXI input FIFO of SUT: \n");
  uart16550_puts("[Tester]: | Timestamp | FIFO level | AXI input value | PFSM output |\n");
  // For every sample in the buffer
  for(i=(ILA0_BUFFER_SIZE-latest_sample_index)*2; i<ILA0_BUFFER_SIZE*2; i+=2){
    fifo_value = samples[i+1]<<16 | samples[i]>>16;
    printf("[Tester]: | %06d    | 0x%02x       | 0x%08x      | %d           |\n",(uint16_t)(samples[i]-initial_time), samples[i+1]>>16 & 0x1f, fifo_value, samples[i+1]>>21 & 0x1);
  }
  uart16550_putc('\n');

  free((uint32_t *)samples);
}
#endif //USE_ILA_PFSM

void send_axistream() {
  uint8_t i;
  uint8_t words_in_byte_stream = 4; 
  // Allocate memory for byte stream
  uint32_t *byte_stream = (uint32_t *)malloc(words_in_byte_stream*sizeof(uint32_t));
  // Fill byte stream to send
  byte_stream[0] = 0x03020100;
  byte_stream[1] = 0x07060504;
  byte_stream[2] = 0xbbaa0908;
  byte_stream[3] = 0xffeeddcc;

  // Print byte stream to send
  uart16550_puts("[Tester]: Sending AXI stream bytes: ");
  for (i = 0; i < words_in_byte_stream*4; i++)
    printf("0x%02x ", ((uint8_t *)byte_stream)[i]);
  uart16550_puts("\n");

  // Send bytes to AXI stream output via DMA, except the last word.
  uart16550_puts("[Tester]: Loading AXI words via DMA...\n");
  iob_axis_out_reset();
  IOB_AXISTREAM_OUT_SET_ENABLE(1);
  IOB_AXISTREAM_OUT_SET_MODE(1);
  IOB_AXISTREAM_OUT_SET_NWORDS(words_in_byte_stream);
  dma_start_transfer(byte_stream, words_in_byte_stream-1, 0, 0);
  // Send the last word with via SWregs with the TLAST signal.
  uart16550_puts("[Tester]: Loading last AXI word via SWregs...\n\n");
  IOB_AXISTREAM_OUT_SET_MODE(0);
  iob_axis_write(byte_stream[words_in_byte_stream-1]);

  free(byte_stream);
}

void receive_axistream() {
  uint8_t i;
  uint8_t n_received_words = IOB_AXISTREAM_IN_GET_NWORDS();
  
  // Allocate memory for byte stream
  volatile uint32_t *byte_stream = (volatile uint32_t *)malloc((n_received_words)*sizeof(uint32_t));

  // Transfer bytes from AXI stream input via DMA
  uart16550_puts("[Tester]: Storing AXI words via DMA...\n");
  IOB_AXISTREAM_IN_SET_MODE(1);
  dma_start_transfer((uint32_t *)byte_stream, n_received_words, 1, 0);

  clear_cache();

  // Print byte stream received
  uart16550_puts("[Tester]: Received AXI stream bytes: ");
  for (i = 0; i < n_received_words*4; i++)
    printf("0x%02x ", ((volatile uint8_t *)byte_stream)[i]);
  uart16550_puts("\n\n");

  free((uint32_t *)byte_stream);
}
