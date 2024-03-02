#include "iob-gpio-user.h"

uint32_t gpio_get() {
  uint32_t value = 0;
  iob_sysfs_read_file(IOB_GPIO_SYSFILE_GPIO_INPUT, &value);

  return value;
}

int gpio_set(uint32_t value) {
  return iob_sysfs_write_file(IOB_GPIO_SYSFILE_GPIO_OUTPUT, value);
}

int gpio_set_output_enable(uint32_t value) {
  return iob_sysfs_write_file(IOB_GPIO_SYSFILE_GPIO_OUTPUT_ENABLE, value);
}

int iob_gpio_test() {
  printf("[Tester|User] IOb-GPIO test\n");

  if (iob_sysfs_print_version(IOB_GPIO_SYSFILE_VERSION) == -1) {
    perror("[Tester|User] Failed to print version");

    return -1;
  }

  // Attempt to set GPIO registers
  if (gpio_set_output_enable(0x0) == -1 || gpio_set(0x0) == -1) {
    perror("[Tester|User] Failed to set outputs");

    return -1;
  }

  return 0;
}
