/*********************************************************
 *             Tester Verification Program               *
 *********************************************************/
 // Place the compiled source in /etc/init.d/S99IObundleVerification

// #include "bsp.h"
#include "iob-gpio-user.h"
#include "iob-uart16550.h"
#include "iob-timer-user.h"
#include "iob-soc-sut-user.h"
#include "iob-axistream-in-user.h"
#include "iob-axistream-out-user.h"

//// System may not use ILA/PFSM for Quartus boards
// how to fix this for linux program?
#include "iob-pfsm-user.h"
#include "iob-ila-user.h"
#include "iob-dma-user.h"
#define USE_ILA_PFSM
#define ILA0_BUFFER_W 3

#include <stdbool.h>

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

#define SUT_FIRMWARE_SIZE 150000

#define BUFFER_SIZE 5096

// copied from bsp.h
#define BAUD 115200
#define FREQ 100000000

// We should ideally receive the INIT_MEM information from the PC
// However, since most of the time we run linux on the FPGA,
// the INIT_MEM will be 0, therefore we hardcode it for now.
#undef IOB_SOC_TESTER_INIT_MEM

// Serial port connected to SUT
#define UART1_BASE "/dev/ttyS0"

// DEVICE MINOR FOR PFSM DRIVER DEVICES
// check logs after running: ./insmod iob_pfsm.ko
#define PFSM_MINOR 0
#define MONITOR_MINOR 1

void print_ila_samples();
void send_axistream();
void receive_axistream();
void pfsm_program(char *, uint32_t);
void ila_monitor_program(char *, uint32_t);
//void clear_cache();


/*
 * Request file via rz protocol:
 *  - fname: file name
 *  - buffer: byte array to read file contents into
 *  - fsize: size of file to read.
 * return: size of file read.
 */
uint32_t rz_request_file_by_path(const char *filepath,const char *fname, char *buffer, uint32_t fsize){
  FILE *fp = NULL;
  uint32_t file_size = 0;
  int i = 0;

  // Make request to host
  puts(filepath);
  i = system("rz");
  if (i != 0) puts("[Tester]: File transfer via rz failed!\n");
  fp = fopen(fname, "r");
  if(fp == NULL){
    puts("fp is null\n");
  }
  file_size = fread(buffer, 1, fsize, fp);
  fclose(fp);

  return file_size;
}

uint32_t rz_request_file(const char *fname, char *buffer, uint32_t fsize){
  FILE *fp = NULL;
  uint32_t file_size = 0;
  int i = 0;

  // Make request to host
  puts(fname);
  i = system("rz");
  if (i != 0) puts("[Tester]: File transfer via rz failed!\n");
  fp = fopen(fname, "r");
  if(fp == NULL){
    puts("fp is null\n");
  }
  file_size = fread(buffer, 1, fsize, fp);
  fclose(fp);
  
  return file_size;
}

// ASCII characters are the safest to send
#define PERFORM_SHA 'a'
#define PERFORM_AES 'b'
#define PERFORM_MCELIECE 'c'
#define PERFORM_MCELIECE_SHORT 'd'

#define PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_PUBLICKEYBYTES 261120
#define PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_SECRETKEYBYTES 6492
#define PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_CIPHERTEXTBYTES 96
#define PQCLEAN_MCELIECE348864_CLEAN_CRYPTO_BYTES 32

typedef struct{
  char* ptr;
  int used;
  int allocated;
} Arena;

//Arena globalArenaInst = {};
static Arena* globalArena = NULL;

Arena InitArena(int size){
  Arena arena = {};
  arena.ptr = (char*) malloc(size * sizeof(char));
  arena.allocated = size;

  return arena;
}

void* PushBytes(Arena* arena,int size){
  char* ptr = &arena->ptr[arena->used];

  size = (size + 3) & (~3); // Align to 4 byte boundary
  arena->used += size;

  if(arena->used > arena->allocated){
    printf("Arena overflow\n");
    printf("Size: %d,Used: %d, Allocated: %d\n",size,arena->used,arena->allocated);
  }

  return ptr;
}

void* PushAndZeroBytes(Arena* arena,int size){
  char* ptr = PushBytes(arena,size);

  for (int i = 0; i < size; i++) {
    ptr[i] = 0;
  }

  return ptr;
}

int MarkArena(Arena* arena){
  return arena->used;
}

void PopArena(Arena* arena,int mark){
  arena->used = mark;
}

#define PushArray(ARENA,N_ELEM,TYPE) (TYPE*) PushBytes(ARENA,(N_ELEM) * sizeof(TYPE))

typedef struct{
  char* str;
  int size;
} String;

#include <string.h>
#define STRING(str) (String){str,strlen(str)}

String PushFile(const char* filepath,const char* filename){
  // Make request to host
  int file_size = rz_request_file_by_path(filepath,filename, globalArena->ptr,globalArena->allocated - globalArena->used);
  char* content = PushBytes(globalArena,file_size + 1);
  content[file_size] = '\0';

  return (String){.str=content,.size=file_size};
}

void getc_until(char toSee){
  char ch = 0;
  bool seenOne = false;
  while((ch = uart16550_getc()) != toSee){
    if(!seenOne){
      puts("Stream got out of sync, got unexpected data:");
    }
    printf("%02x ",ch);

    seenOne = true;
  }
  if(seenOne){
    puts("\n");    
  }
}

String receive_small_string(){
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

  return (String){.str = buffer,.size = bytesSeen};
}

int receive_int(){
  int result = uart16550_getc();
  result |= ((uint32_t)uart16550_getc()) << 8;
  result |= ((uint32_t)uart16550_getc()) << 16;
  result |= ((uint32_t)uart16550_getc()) << 24;

  return result;
}

String receive_large_data(Arena* out){
  getc_until(STX);

  int size = receive_int();
  char* res = PushBytes(out,size + 1);

  for(int i = 0; i < size; i++){
    res[i] = uart16550_getc();
  }

  res[size] = '\0';

  getc_until(ETX);
  return (String){.str = res,.size = size};
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

  uart16550_putc(ETX);
}

void send_small_string(const char* string){
  uart16550_putc(STX);

  int bytesSent = 0;
  while(bytesSent < 255 && string[bytesSent] != '\0'){
    uart16550_putc(string[bytesSent]);
    bytesSent += 1;
  }

  if(string[bytesSent] != '\0'){
    puts("[TESTER] send_small_string was cutoff\n");
  }

  uart16550_putc(ETX);
}

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
            puts("Warning: HexString was not divisible by 2\n");
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

    char* message = PushArray(globalArena,len,char);
    int bytes = HexStringToHex(message,ptr);

    ptr = SearchAndAdvance(ptr,STRING("MD = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* expected = ptr;

    uart16550_putc(PERFORM_SHA);

    if(len == 0){
      send_large_data("",0);
    } else {
      send_large_data(message,bytes);
    }

    String result = receive_small_string();

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
      printf("SHA Test %02d: Error\n",testResult.tests);
      printf("  Expected: %.64s\n",expected); 
      printf("  Versat:   %.*s\n",result.size,result.str);
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

    ParseNumber(ptr);

    ptr = SearchAndAdvance(ptr,STRING("KEY = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* key = PushArray(globalArena,32 + 1,char);
    HexStringToHex(key,ptr);

    ptr = SearchAndAdvance(ptr,STRING("PLAINTEXT = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }
  
    char* plain = PushArray(globalArena,16 + 1,char);
    HexStringToHex(plain,ptr);

    ptr = SearchAndAdvance(ptr,STRING("CIPHERTEXT = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* cypher = ptr;

    uart16550_putc(PERFORM_AES);    

    send_large_data(key,32);
    send_large_data(plain,16);

    String result = receive_small_string();

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
      printf("AES Test %02d: Error\n",testResult.tests);
      printf("  Expected: %.32s\n",cypher); 
      printf("  Versat:   %.*s\n",result.size,result.str);
    }

    testResult.tests += 1;
    PopArena(globalArena,testMark);
  }

  PopArena(globalArena,mark);

  return testResult;
}

TestState VersatMcElieceShortTests(String content){
  TestState testResult = {};

  int mark = MarkArena(globalArena);

  char* ptr = content.str;
  while(1){
    int testMark = MarkArena(globalArena);

    ptr = SearchAndAdvance(ptr,STRING("COUNT = "));
    if(ptr == NULL){
      break;
    }

    ParseNumber(ptr);

    ptr = SearchAndAdvance(ptr,STRING("SEED = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char seed[49];
    HexStringToHex(seed,ptr);

    ptr = SearchAndAdvance(ptr,STRING("PK = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }
  
    char* good_pk_start = ptr;

    ptr = SearchAndAdvance(ptr,STRING("PKL = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }
  
    char* good_pk_end = ptr;

    ptr = SearchAndAdvance(ptr,STRING("SK = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* good_sk_start = ptr;

    ptr = SearchAndAdvance(ptr,STRING("SKL = "));
    if(ptr == NULL){
      testResult.earlyExit = 1;
      break;
    }

    char* good_sk_end = ptr;

    uart16550_putc(PERFORM_MCELIECE_SHORT);
    send_large_data(seed,48);    

    String public_key_start = receive_large_data(globalArena);
    String public_key_end = receive_large_data(globalArena);
    String secret_key_start = receive_large_data(globalArena);
    String secret_key_end = receive_large_data(globalArena);

    char* public_key_start_hex = PushArray(globalArena,512 * 2 + 1,char);
    char* public_key_end_hex = PushArray(globalArena,512 * 2 + 1,char);
    char* secret_key_start_hex = PushArray(globalArena,512 * 2 + 1,char);
    char* secret_key_end_hex = PushArray(globalArena,512 * 2 + 1,char);

    GetHexadecimal(public_key_start.str,public_key_start_hex,512);
    GetHexadecimal(public_key_end.str,public_key_end_hex,512);
    GetHexadecimal(secret_key_start.str,secret_key_start_hex,512);
    GetHexadecimal(secret_key_end.str,secret_key_end_hex,512);

    bool good = true;
    for(int i = 0; i < 1024; i++){
      if(public_key_start_hex[i] != good_pk_start[i]){
        good = false;
        break;
      }
    }
    for(int i = 0; i < 1024; i++){
      if(public_key_end_hex[i] != good_pk_end[i]){
        good = false;
        break;
      }
    }
    for(int i = 0; i < 1024; i++){
      if(secret_key_start_hex[i] != good_sk_start[i]){
        good = false;
        break;
      }
    }
    for(int i = 0; i < 1024; i++){
      if(secret_key_end_hex[i] != good_sk_end[i]){
        good = false;
        break;
      }
    }

    if(good){
      testResult.goodTests += 1;
    } else {
      printf("McEliece Test %02d: Error\n",testResult.tests);
      printf("  Expected Public first (first 32 chars): %.32s\n",good_pk_start); 
      printf("  Got Public first (first 32 chars):      %.32s\n",public_key_start_hex);
      printf("  Expected Public last  (first 32 chars): %.32s\n",good_pk_end); 
      printf("  Got Public last  (first 32 chars):      %.32s\n",public_key_end_hex);
      printf("  Expected Secret first (first 32 chars): %.32s\n",good_sk_start); 
      printf("  Got Secret first (first 32 chars):      %.32s\n",secret_key_start_hex);
      printf("  Expected Secret last  (first 32 chars): %.32s\n",good_sk_end); 
      printf("  Got Secret last  (first 32 chars):      %.32s\n",secret_key_end_hex);
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
 * small_buffer: buffer to store strings
 * file_content_buffer: buffer to store file content
*/
void relay_file_transfer_to_sut(char *small_buffer, char *file_content_buffer){
  int i;
  uint32_t file_size = 0;
  FILE *fptr;

  // receive file transfer request from SUT
  // Wait for FRX signal from SUT
  while (uart16550_getc() != FRX);
  // Receive filename
  for (i = 0; (small_buffer[i] = uart16550_getc()) != '\0'; i++);

  puts("[Tester]: Received file transfer request with filename: ");
  puts(small_buffer);
  putchar('\n');
  puts("[Tester]: Sending transfer request to console...\n");

  // Make request to host
  //file_size = uart16550_recvfile(small_buffer, file_content_buffer);
  puts(small_buffer);
  i = system("rz");
  if (i != 0) puts("[Tester]: File transfer via rz failed!\n");
  fptr = fopen(small_buffer, "r");
  file_size = fread(file_content_buffer, 1, SUT_FIRMWARE_SIZE, fptr);
  fclose(fptr);

  puts(
      "\n[Tester]: SUT file obtained. Transfering it to SUT via UART...\n");

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
  char c, buffer[BUFFER_SIZE];//, *sutStr;
  int i;
#ifndef IOB_SOC_TESTER_INIT_MEM
  char* sut_firmware = (char*) malloc(SUT_FIRMWARE_SIZE);
#endif

//    // Init uart0
//    uart16550_init(UART0_BASE, FREQ/(16*BAUD));
//    printf_init(&uart16550_putc);
//    // Init SUT (connected through REGFILEIF)
//    IOB_SOC_SUT_INIT_BASEADDR(SUT0_BASE);
    // init axistream
    iob_sysfs_write_file(IOB_AXISTREAM_IN_SYSFILE_ENABLE, 1);
    iob_sysfs_write_file(IOB_AXISTREAM_OUT_SYSFILE_ENABLE, 1);


 #ifdef USE_ILA_PFSM
     // init integrated logic analyzer
     ila_init();
     // Enable ILA circular buffer
     // This allows for continuous sampling while the enable signal is active
     ila_set_circular_buffer(1);
 #endif
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
  // Write data to the registers of the SUT to be read by it.
  iob_soc_sut_set_reg(1, 64);
  iob_soc_sut_set_reg(2, 1024);
  puts("[Tester]: Stored values 64 and 1024 in registers 1 and 2 of the "
             "SUT.\n\n");
//
    // Write a test pattern to the GPIO outputs to be read by the SUT.
    gpio_set(0x1234abcd);
    puts("[Tester]: Placed test pattern 0x1234abcd in GPIO outputs.\n\n");
//
 #ifdef USE_ILA_PFSM
     // Program PFSM
     pfsm_program(buffer, BUFFER_SIZE);

     // // Program Monitor PFSM (internal to ILA)
     ila_monitor_program(buffer, BUFFER_SIZE);

     // Enable all ILA triggers
     ila_enable_all_triggers();
 #endif
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

   //Delay to allow time for sut to run bootloader and enable its axistream
   for ( i = 0; i < (FREQ/BAUD)*256; i++)asm("nop");

    // Send byte stream via AXI stream
    send_axistream();

 #ifdef USE_ILA_PFSM

      while(ila_number_samples()<4);

     // Disable all ILA triggers
     ila_disable_all_triggers();

     // Print sampled ILA values
     print_ila_samples();
 #endif

  // Tell SUT that the Tester is running linux
  send_small_string("TESTER_RUN_LINUX");
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

#if 1
  while(uart16550_getc() != ENQ); // Read the messages outputted by file transfer function
  // Request SUT to perform various cryptographic operations
  Arena globalArenaInst = InitArena(2 * 1024 * 1024); 
  globalArena = &globalArenaInst;
  
  {
  int mark = MarkArena(globalArena);
  String content = PushFile("../../software/KAT/SHA256ShortMsg.rsp","SHA256ShortMsg.rsp");
  TestState result = VersatCommonSHATests(content);

  if(result.earlyExit){
    printf("SHA early exit. Check testcases to see if they follow the expected format\n");
  } else {
    printf("\n\n=======================================================\n");
    printf("SHA tests: %d passed out of %d\n",result.goodTests,result.tests);
    printf("=======================================================\n\n");
  }

  PopArena(globalArena,mark);
  }

  {
  int mark = MarkArena(globalArena);
  String content = PushFile("../../software/KAT/AESECB256.rsp","AESECB256.rsp");
  TestState result = VersatCommonAESTests(content);

  if(result.earlyExit){
    printf("AES early exit. Check testcases to see if they follow the expected format\n");
  } else {
    printf("\n\n=======================================================\n");
    printf("AES tests: %d passed out of %d\n",result.goodTests,result.tests);
    printf("=======================================================\n\n");
  }

  PopArena(globalArena,mark);
  }

  {
  int mark = MarkArena(globalArena);
  String content = PushFile("../../software/KAT/McElieceRound4kat_kem_short.rsp","McElieceRound4kat_kem_short.rsp");
  TestState result = VersatMcElieceShortTests(content);

  if(result.earlyExit){
    printf("McEliece early exit. Check testcases to see if they follow the expected format\n");
  } else {
    printf("\n\n=======================================================\n");
    printf("McEliece tests: %d passed out of %d\n",result.goodTests,result.tests);
    printf("=======================================================\n\n");
  }
  PopArena(globalArena,mark);
  }

  uart16550_putc(ETX); // Terminate loop on SUT side
#endif

  // End UART1 connection with SUT
  uart16550_finish();

  // Send messages previously stored from SUT
  puts("[Tester]: #### Messages received from SUT: ####\n\n");
  for (i = 0; buffer[i] != EOT; i++) {
    putchar(buffer[i]);
  }
  puts("\n[Tester]: #### End of messages received from SUT ####\n\n");

  // Read data from the SUT's registers
  uart16550_puts("[Tester]: Reading SUT's register contents:\n");
  uint32_t sut_reg = 0;
  iob_soc_sut_get_reg(3, &sut_reg);
  printf("[Tester]: Register 3: %d \n", sut_reg);
  iob_soc_sut_get_reg(4, &sut_reg);
  printf("[Tester]: Register 4: %d \n", sut_reg);

    // Read pattern from GPIO inputs (was set by the SUT)
    printf("\n[Tester]: Pattern read from GPIO inputs: 0x%x\n\n", gpio_get());
//
  // Read byte stream via AXI stream
  receive_axistream();
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

 #ifdef USE_ILA_PFSM
     // Allocate memory for ILA output data
     uint32_t ila_data_size = ila_output_data_size(ILA0_BUFFER_SIZE, ILA0_DWORD_SIZE);

     // Write data to allocated memory
     uint32_t latest_sample_index = ila_number_samples();
     ila_output_data(buffer, latest_sample_index, (latest_sample_index-1)%ILA0_BUFFER_SIZE, ILA0_BUFFER_SIZE, ILA0_DWORD_SIZE);

     // Send ila data to file via UART
     uart16550_sendfile("ila_data.bin", ila_data_size-1, buffer); //Don't send last byte (\0)
 #endif
//
  // Test iob-timer with drivers
  if (iob_timer_test() == -1){
      puts("[Tester]: iob-timer test failed!\n");
      return -1;
  }

  // Test iob-gpio with drivers
  if (iob_gpio_test() == -1){
      puts("[Tester]: iob-gpio test failed!\n");
      return -1;
  }

  puts("\n[Tester]: Verification successful!\n\n");
  sprintf(buffer+1000, "echo '%s' > test.log; sz -e test.log", pass_string);
  i = system(buffer+1000);
  if (i != 0) puts("[Tester]: File transfer of test.log via sz failed!\n");
}

#ifdef USE_ILA_PFSM
// Program independent PFSM peripheral of the Tester
void pfsm_program(char *bitstreamBuffer, uint32_t read_size){
  // init Programmable Finite State Machine
  pfsm_init(2, 1, 1, PFSM_MINOR);
  uint32_t file_size = 0;
  // Receive pfsm bitstream
  // file_size = uart16550_recvfile("pfsm.bit", bitstreamBuffer);
  file_size = rz_request_file("pfsm.bit", bitstreamBuffer, read_size);

  // Program PFSM
  printf("[Tester]: Programming PFSM...with %d bytes\n", file_size);
  printf("[Tester]: Programmed PFSM with %d bytes.\n\n",
         pfsm_bitstream_program(bitstreamBuffer)
         );
}

// Program Monitor PFSM internal to ILA.
void ila_monitor_program(char *bitstreamBuffer, uint32_t read_size){
  // init ILA Monitor (PFSM)
  pfsm_init(2, 2, 1, MONITOR_MINOR);
  uint32_t file_size = 0;
  // Receive pfsm bitstream
  // file_size = uart16550_recvfile("monitor_pfsm.bit", bitstreamBuffer);
  file_size = rz_request_file("monitor_pfsm.bit", bitstreamBuffer, read_size);
  // Program PFSM
  printf("[Tester]: Programming MONITOR...with %d bytes\n", file_size);
  printf("[Tester]: Programmed Monitor PFSM with %d bytes.\n\n",
         pfsm_bitstream_program(bitstreamBuffer)
         );
}

void print_ila_samples() {
  // From the ILA0 configuration: bits 0-15 are the timestamp; bits 16-47 are fifo_value; bits 48-52 are the fifo_level; bit 53 is PFSM output
  uint32_t i=0, fifo_value;
  uint16_t initial_time = (uint16_t)ila_get_large_value(0,0);
  uint32_t latest_sample_index = ila_number_samples();

  // Allocate memory for samples
  // Each buffer sample has 2 * 32 bit words
  volatile uint32_t *samples = (volatile uint32_t *)malloc((ILA0_BUFFER_SIZE*2)*sizeof(uint32_t));

  // Point ila cursor to the latest sample
  ila_set_cursor(latest_sample_index,0);

  printf("[Tester]: Storing ILA samples into memory via DMA...\n");
  dma_start_transfer((uint32_t *)samples, ILA0_BUFFER_SIZE*2, 1, 1);

  // clear_cache(); // linux command: sync; echo 3 > /proc/sys/vm/drop_caches

  printf("[Tester]: ILA values sampled from the AXI input FIFO of SUT: \n");
  printf("[Tester]: | Timestamp | FIFO level | AXI input value | PFSM output |\n");
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
  printf("[Tester]: Sending AXI stream bytes: ");
  for (i = 0; i < words_in_byte_stream; i++)
    printf("0x%08x ", byte_stream[i]);
  printf("\n");

  // Send bytes to AXI stream output via DMA, except the last word.
  printf("[Tester]: Loading AXI words via DMA...\n");
  iob_axis_out_reset();
  iob_sysfs_write_file(IOB_AXISTREAM_OUT_SYSFILE_ENABLE, 1);
  iob_sysfs_write_file(IOB_AXISTREAM_OUT_SYSFILE_MODE, 1);
  iob_sysfs_write_file(IOB_AXISTREAM_OUT_SYSFILE_NWORDS, words_in_byte_stream);
  dma_start_transfer(byte_stream, words_in_byte_stream-1, 0, 0);
  // Send the last word with via SWregs with the TLAST signal.
  printf("[Tester]: Loading last AXI word via SWregs...\n\n");
  iob_sysfs_write_file(IOB_AXISTREAM_OUT_SYSFILE_MODE, 0);
  iob_axis_write(byte_stream[words_in_byte_stream-1]);

  free(byte_stream);
}

void receive_axistream() {
  uint8_t i;
  uint32_t n_received_words = 0;
  iob_sysfs_read_file(IOB_AXISTREAM_IN_SYSFILE_NWORDS, &n_received_words);

  // Allocate memory for byte stream
  volatile uint32_t *byte_stream = (volatile uint32_t *)malloc((n_received_words)*sizeof(uint32_t));
  for (i = 0; i < n_received_words; i++)
      byte_stream[i] = 0;

  // Transfer bytes from AXI stream input via DMA
  printf("[Tester]: Storing AXI words via DMA...\n");
  iob_sysfs_write_file(IOB_AXISTREAM_IN_SYSFILE_MODE, 1);
  dma_start_transfer((uint32_t *)byte_stream, n_received_words, 1, 0);

  // clear_cache();

  // Print byte stream received
  printf("[Tester]: Received AXI stream %d bytes: ", n_received_words*4);
  for (i = 0; i < n_received_words; i++)
    printf("0x%08x ", byte_stream[i]);
  printf("\n\n");

  free((uint32_t *)byte_stream);
}

// void clear_cache(){
//   // Delay to ensure all data is written to memory
//   for ( unsigned int i = 0; i < 10; i++)asm volatile("nop");
//   // Flush VexRiscv CPU internal cache
//   asm volatile(".word 0x500F" ::: "memory");
// }
