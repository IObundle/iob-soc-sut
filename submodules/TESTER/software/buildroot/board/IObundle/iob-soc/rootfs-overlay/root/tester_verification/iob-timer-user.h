#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/* IOb-Timer header automatically generated from:
 * ./bootstrap.py iob_timer -f gen_linux_driver_header
 * contains:
 * - device file path
 * - Register address and width definitions
 */
#include "iob_timer.h"

uint32_t iob_timer_read_reg(int fd, uint32_t addr, uint32_t nbits, uint32_t *value);
uint32_t iob_timer_write_reg(int fd, uint32_t addr, uint32_t nbits, uint32_t value);
uint32_t iob_timer_print_version();
uint32_t iob_timer_reset();
uint32_t iob_timer_init();
void iob_timer_finish();
uint64_t iob_timer_get_count();
int iob_timer_test();


