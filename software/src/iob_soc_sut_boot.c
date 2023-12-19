#include "bsp.h"
#include "iob-uart.h"
#include "iob_soc_sut_system.h"
#include "iob_soc_sut_conf.h"

#ifdef IOB_SOC_SUT_USE_EXTMEM
#include "iob_cache_swreg.h"
#endif

// defined here (and not in periphs.h) because it is the only peripheral used
// by the bootloader
#define UART_BASE (IOB_SOC_SUT_UART0 << (31 - IOB_SOC_SUT_N_SLAVES_W))

#define PROGNAME "IOb-Bootloader"

int main() {
  uint32_t i;

  // init uart
  uart_init(UART_BASE, FREQ / BAUD);

#ifdef IOB_SOC_SUT_USE_EXTMEM
  IOB_CACHE_INIT_BASEADDR((1 << IOB_SOC_SUT_E) + (1 << IOB_SOC_SUT_MEM_ADDR_W));
#endif

  // connect with console
  do {
    if (IOB_UART_GET_TXREADY())
      // Send 0xff (to allow start bit synchronization) followed by ENQ
      uart_puts((char [3]){0xff, ENQ, 0x00});
  } while (!IOB_UART_GET_RXREADY());

  for ( i = 0; i < 5000; i++)asm("nop"); //Delay to allow time for tester to print debug messages


  // welcome message
  uart_puts(PROGNAME);
  uart_puts(": connected!\n");

#ifdef IOB_SOC_SUT_USE_EXTMEM
  uart_puts(PROGNAME);
  uart_puts(": DDR in use and program runs from DDR\n");
#endif

  // address to copy firmware to
  char *prog_start_addr;
#ifdef IOB_SOC_SUT_USE_EXTMEM
  prog_start_addr = (char *)EXTRA_BASE;
#else
  prog_start_addr = (char *)(1 << IOB_SOC_SUT_BOOTROM_ADDR_W);
#endif

  while (uart_getc() != ACK) {
    uart_puts(PROGNAME);
    uart_puts(": Waiting for Console ACK.\n");
  }

#ifndef IOB_SOC_SUT_INIT_MEM
  for ( i = 0; i < 45000; i++)asm("nop"); //Delay to allow time for tester to print debug messages
  // receive firmware from host
  int file_size = 0;
  char r_fw[] = "iob_soc_sut_firmware.bin";
  file_size = uart_recvfile(r_fw, prog_start_addr);
  uart_puts(PROGNAME);
  uart_puts(": Loading firmware...\n");

  // sending firmware back for debug
  char s_fw[] = "s_fw.bin";

  for ( i = 0; i < 5000; i++)asm("nop"); //Delay to allow time for tester to print debug messages
  if (file_size)
    uart_sendfile(s_fw, file_size, prog_start_addr);
  else {
    uart_puts (PROGNAME);
    uart_puts (": ERROR loading firmware\n");
  }
  for ( i = 0; i < 5000; i++)asm("nop"); //Delay to allow time for tester to print debug messages
#endif

  // run firmware
  uart_puts(PROGNAME);
  uart_puts(": Restart CPU to run user program...\n");
  uart_txwait();

#ifdef IOB_SOC_SUT_USE_EXTMEM
  while (!IOB_CACHE_GET_WTB_EMPTY())
    ;
#endif
}
