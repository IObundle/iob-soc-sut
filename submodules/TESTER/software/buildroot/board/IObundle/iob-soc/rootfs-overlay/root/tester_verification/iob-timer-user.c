#include "iob-timer-user.h"

static int iob_timer_fd = 0;

uint32_t iob_timer_read_reg(int fd, uint32_t addr, uint32_t nbits,
                            uint32_t *value) {
  ssize_t ret = -1;

  if (fd == 0) {
    perror("[Tester|User] Invalid file descriptor");
    return -1;
  }

  // Point to register address
  if (lseek(fd, addr, SEEK_SET) == -1) {
    perror("[Tester|User] Failed to seek to register");
    return -1;
  }

  // Read value from device
  switch (nbits) {
  case 8:
    uint8_t value8 = 0;
    ret = read(fd, &value8, sizeof(value8));
    if (ret == -1) {
      perror("[Tester|User] Failed to read from device");
    }
    *value = (uint32_t)value8;
    break;
  case 16:
    uint16_t value16 = 0;
    ret = read(fd, &value16, sizeof(value16));
    if (ret == -1) {
      perror("[Tester|User] Failed to read from device");
    }
    *value = (uint32_t)value16;
    break;
  case 32:
    uint32_t value32 = 0;
    ret = read(fd, &value32, sizeof(value32));
    if (ret == -1) {
      perror("[Tester|User] Failed to read from device");
    }
    *value = (uint32_t)value32;
    break;
  default:
    // unsupported nbits
    ret = -1;
    *value = 0;
    perror("[Tester|User] Unsupported nbits");
    break;
  }

  return ret;
}

uint32_t iob_timer_write_reg(int fd, uint32_t addr, uint32_t nbits,
                             uint32_t value) {
  ssize_t ret = -1;

  if (fd == 0) {
    perror("[Tester|User] Invalid file descriptor");
    return -1;
  }

  // Point to register address
  if (lseek(fd, addr, SEEK_SET) == -1) {
    perror("[Tester|User] Failed to seek to register");
    return -1;
  }

  // Write value to device
  switch (nbits) {
  case 8:
    uint8_t value8 = (uint8_t)value;
    ret = write(fd, &value8, sizeof(value8));
    if (ret == -1) {
      perror("[Tester|User] Failed to write to device");
    }
    break;
  case 16:
    uint16_t value16 = (uint16_t)value;
    ret = write(fd, &value16, sizeof(value16));
    if (ret == -1) {
      perror("[Tester|User] Failed to write to device");
    }
    break;
  case 32:
    ret = write(fd, &value, sizeof(value));
    if (ret == -1) {
      perror("[Tester|User] Failed to write to device");
    }
    break;
  default:
    break;
  }

  return ret;
}

uint32_t iob_timer_print_version() {
  uint32_t ret = -1;
  uint32_t version = 0;

  ret = iob_timer_read_reg(iob_timer_fd, IOB_TIMER_VERSION_ADDR,
                           IOB_TIMER_VERSION_W, &version);
  if (ret == -1) {
    return ret;
  }

  printf("[Tester|User] Version: 0x%x\n", version);
  return ret;
}

uint32_t iob_timer_reset() {
  if (iob_timer_write_reg(iob_timer_fd, IOB_TIMER_RESET_ADDR, IOB_TIMER_RESET_W,
                          1) == -1) {
    return -1;
  }
  if (iob_timer_write_reg(iob_timer_fd, IOB_TIMER_RESET_ADDR, IOB_TIMER_RESET_W,
                          0) == -1) {
    return -1;
  }
  return 0;
}

uint32_t iob_timer_init() {

  // Open device for read and write
  iob_timer_fd = open(IOB_TIMER_DEVICE_FILE, O_RDWR);
  if (iob_timer_fd == -1) {
    perror("[Tester|User] Failed to open the device file");
    return -1;
  }

  if (iob_timer_reset() == -1) {
    return -1;
  }

  if (iob_timer_write_reg(iob_timer_fd, IOB_TIMER_ENABLE_ADDR,
                          IOB_TIMER_ENABLE_W, 1) == -1) {
    return -1;
  }
  return 0;
}

void iob_timer_finish() {
  if (iob_timer_fd != 0) {
    close(iob_timer_fd);
    iob_timer_fd = 0;
  }
  return;
}

uint64_t iob_timer_get_count() {
  uint32_t data = 0;
  uint64_t count = 0;

  // Sample timer counter
  if (iob_timer_write_reg(iob_timer_fd, IOB_TIMER_SAMPLE_ADDR,
                          IOB_TIMER_SAMPLE_W, 1) == -1) {
    return -1;
  }
  if (iob_timer_write_reg(iob_timer_fd, IOB_TIMER_SAMPLE_ADDR,
                          IOB_TIMER_SAMPLE_W, 0) == -1) {
    return -1;
  }

  // Read sampled timer counter
  if (iob_timer_read_reg(iob_timer_fd, IOB_TIMER_DATA_HIGH_ADDR,
                         IOB_TIMER_DATA_HIGH_W, &data) == -1) {
    return -1;
  }
  count = ((uint64_t)data) << IOB_TIMER_DATA_LOW_W;
  if (iob_timer_read_reg(iob_timer_fd, IOB_TIMER_DATA_LOW_ADDR,
                         IOB_TIMER_DATA_LOW_W, &data) == -1) {
    return -1;
  }
  count |= (uint64_t)data;

  return count;
}

int iob_timer_test() {
  printf("[Tester|User] IOb-Timer test\n");

  if (iob_timer_init() == -1) {
    perror("[Tester|User] Failed to initialize timer");
    iob_timer_finish();
    return -1;
  }

  if (iob_timer_print_version() == -1) {
    perror("[Tester|User] Failed to print version");
    iob_timer_finish();
    return -1;
  }

  // read current timer count
  uint64_t elapsed = iob_timer_get_count();
  printf("\nExecution time: %lld clock cycles\n", elapsed);

  iob_timer_finish();
  return 0;
}
