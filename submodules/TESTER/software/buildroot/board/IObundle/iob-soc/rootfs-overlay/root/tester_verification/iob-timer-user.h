#include <stdint.h>
#include <stdio.h>

/* IOb-Timer header automatically generated from:
 * ./bootstrap.py iob_timer -f gen_linux_driver_headers
 * contains:
 * - sysfs file paths
 * - Register address and width definitions
 */
#include "iob_timer.h"

int iob_timer_sysfs_read_file(const char *filename, uint32_t *read_value);
int iob_timer_sysfs_write_file(const char *filename, uint32_t write_value);
int iob_timer_reset();
int iob_timer_init();
int iob_timer_print_version();
int iob_timer_get_count(uint64_t *count);
int iob_timer_test();
