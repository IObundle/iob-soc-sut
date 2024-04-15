#include <stdint.h>
#include <stdio.h>

#include "iob-sysfs-common.h"
/* IOb-PFSM header automatically generated from:
 * /path/to/iob-linux/scripts/drivers.py iob_pfsm
 * contains:
 * - sysfs file paths
 * - Register address and width definitions
 */
#include "iob_pfsm.h"

void iob_pfsm_set_memory(uint32_t value, uint32_t address);
void pfsm_init(uint32_t state_w, uint8_t input_w, uint32_t output_w,
               uint32_t dev_minor);
void pfsm_insert_word_lut(int addr, uint8_t word_select, uint32_t value);
void pfsm_reset();
uint32_t pfsm_get_state();
uint32_t pfsm_bitstream_program(char *bitstream);
