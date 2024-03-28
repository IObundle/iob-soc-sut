#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "iob-pfsm-user.h"

#define IOB_PFSM_SYSFS "/sys/devices/virtual/iob_pfsm/iob_pfsm"
#define IOB_PFSM_DEVICE "/dev/iob_pfsm"
#define MAX_BUF 256

// Global variables to store parameters of the current PFSM
static uint32_t g_state_w;
static uint8_t g_input_w;
static uint32_t g_output_w;
static uint32_t g_dev_minor;

static char fname[MAX_BUF] = {0};

void iob_pfsm_set_memory(uint32_t value, uint32_t address) {
  int fd = 0;
  sprintf(fname, "%s%d", IOB_PFSM_DEVICE, g_dev_minor);

  fd = open(fname, O_RDWR);
  if (fd == -1) {
    perror("[Tester|User] Failed to open device file");
    return;
  }

  // check for invalid memory address
  if ((address < 0) || (address >= IOB_PFSM_MEM_WORD_SELECT_ADDR)) {
    perror("[Tester|User] Invalid memory address");
    close(fd);
    return;
  }

  // Point to memory address
  if (lseek(fd, IOB_PFSM_MEMORY_ADDR + address, SEEK_SET) == -1) {
    perror("[Tester|User] Failed to seek memory address");
    close(fd);
    return;
  }

  // Write value to memory
  if (write(fd, &value, sizeof(value)) == -1) {
    perror("[Tester|User] Failed to write to device");
  }
  close(fd);
  return;
}
// PFSM functions

// Set PFSM parameters
void pfsm_init(uint32_t state_w, uint8_t input_w, uint32_t output_w,
               uint32_t dev_minor) {
  g_state_w = state_w;
  g_input_w = input_w;
  g_output_w = output_w;
  g_dev_minor = dev_minor;
}

// Write a 32-bit word to the LUT memory
// The LUT may have words larger than 32-bits. Use the word_select to select the
// which 32-bit word to write.
void pfsm_insert_word_lut(int addr, uint8_t word_select, uint32_t value) {
  iob_sysfs_gen_fname(fname, IOB_PFSM_SYSFS, g_dev_minor, "mem_word_select");
  iob_sysfs_write_file(fname, word_select);
  iob_pfsm_set_memory(value, addr);
}

// Pulse soft reset to go back to state 0
void pfsm_reset() {
  iob_sysfs_gen_fname(fname, IOB_PFSM_SYSFS, g_dev_minor, "softreset");
  iob_sysfs_write_file(fname, 1);
  iob_sysfs_write_file(fname, 0);
}

// Get current state of PFSM
uint32_t pfsm_get_state() {
  uint32_t current_state = 0;
  iob_sysfs_gen_fname(fname, IOB_PFSM_SYSFS, g_dev_minor, "current_state");
  iob_sysfs_read_file(fname, &current_state);
  return current_state;
}

// Program PFSM given a bitstream with data to program the States LUT and every
// condition LUT. Returns number of bytes written (should be equal to the
// bitstream size)
uint32_t pfsm_bitstream_program(char *bitstream) {
  uint8_t i;
  uint32_t j, byteCounter = 0;
  uint32_t data_words_per_lut_word =
      (g_state_w + g_output_w + 32 - 1) /
      32; // Ceiling division of (state_w+output_w)/32
  // Program LUT memory
  for (j = 0; j < (1 << (g_input_w + g_state_w)); j++) {
    // Program each 32-bit word in this LUT address
    for (i = 0; i < data_words_per_lut_word; i++) {
      pfsm_insert_word_lut(
          j, i,
          bitstream[byteCounter] << 24 | bitstream[byteCounter + 1] << 16 |
              bitstream[byteCounter + 2] << 8 | bitstream[byteCounter + 3]);
      byteCounter += 4;
    }
  }

  // Reset PFSM back to state 0
  pfsm_reset();

  return byteCounter;
}
