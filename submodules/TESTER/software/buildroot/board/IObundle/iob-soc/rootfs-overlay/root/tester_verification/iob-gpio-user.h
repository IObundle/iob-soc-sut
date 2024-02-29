#include <stdint.h>
#include <stdio.h>

#include "iob-sysfs-common.h"
/* IOb-GPIO header automatically generated from:
 * /path/to/iob-linux/scripts/drivers.py iob_gpio
 * contains:
 * - sysfs file paths
 * - Register address and width definitions
 */
#include "iob_gpio.h"

uint32_t gpio_get();
int gpio_set(uint32_t value);
int gpio_output_enable(uint32_t value);
int iob_gpio_test();
