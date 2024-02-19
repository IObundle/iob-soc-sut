#include <stdint.h>
#include <stdio.h>

#include "iob-sysfs-common.h"
/* IOb-Soc-SUT header automatically generated from:
 * /path/to/iob-linux/scripts/drivers.py iob_soc_sut
 * contains:
 * - sysfs file paths
 * - Register address and width definitions
 */
#include "iob_soc_sut.h"

int iob_soc_sut_set_reg(uint32_t num, uint32_t value);
int iob_soc_sut_get_reg(uint32_t num, uint32_t *value);
