#include "iob-ila-user.h"

// The values of the defines can be found in the beginning of ila_core.v
#define RST_SOFT_BIT 0
#define DIFF_SIGNAL_BIT 1
#define CIRCULAR_BUFFER_BIT 2
#define DELAY_TRIGGER_BIT 3
#define DELAY_SIGNAL_BIT 4
#define REDUCE_TYPE_BIT 5

// The software module keeps track of the register values
static uint32_t triggerType;
static uint32_t triggerNegate;
static uint32_t triggerMask;
static uint32_t miscValue;

static inline int setBit(int bitfield, int bit, int value) {
  value = (value ? 1 : 0);    // (value > 0): set bit, otherwise reset bit
  bitfield &= ~(1 << bit);    // Clear bit
  bitfield |= (value << bit); // Set to value
  return bitfield;
}

// Init the ila module
void ila_init(int base_address) {

  triggerType = 0;
  triggerNegate = 0;
  triggerMask = 0;
  miscValue = 0;

  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_TYPE, triggerType);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_NEGATE, triggerNegate);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_MASK, triggerMask);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);

  ila_set_time_offset(0);
  ila_set_reduce_type(ILA_REDUCE_TYPE_OR);

  ila_reset();
}

// Reset the ILA buffer
void ila_reset() {
  miscValue = setBit(miscValue, RST_SOFT_BIT, 1);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);
  miscValue = setBit(miscValue, RST_SOFT_BIT, 0);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_TYPE, triggerType);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_MASK, triggerMask);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_NEGATE, triggerNegate);
}

// Set the trigger type
void ila_set_trigger_type(int trigger, int type) {
  triggerType = setBit(triggerType, trigger, type);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_TYPE, triggerType);
}

// Enable or disable a particular trigger
void ila_set_trigger_enabled(int trigger, int enabled_bool) {
  triggerMask = setBit(triggerMask, trigger, enabled_bool);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_MASK, triggerMask);
}

// Set whether the trigger is to be negated
void ila_set_trigger_negated(int trigger, int negate_bool) {
  triggerNegate = setBit(triggerNegate, trigger, negate_bool);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_NEGATE, triggerNegate);
}

// Enables every trigger
void ila_enable_all_triggers() {
  triggerMask = 0xffffffff;
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_MASK, triggerMask);
}

// Disable every trigger (equivalent to disabling ila)
void ila_disable_all_triggers() {
  triggerMask = 0x00000000;
  iob_sysfs_write_file(IOB_ILA_SYSFILE_TRIGGER_MASK, triggerMask);
}

void ila_set_time_offset(int amount) {
  if (amount == 0) {
    miscValue = setBit(miscValue, DELAY_TRIGGER_BIT, 0);
    miscValue = setBit(miscValue, DELAY_SIGNAL_BIT, 0);
  } else if (amount >= 1) {
    miscValue = setBit(miscValue, DELAY_TRIGGER_BIT, 1);
    miscValue = setBit(miscValue, DELAY_SIGNAL_BIT, 0);
  } else {
    miscValue = setBit(miscValue, DELAY_TRIGGER_BIT, 0);
    miscValue = setBit(miscValue, DELAY_SIGNAL_BIT, 1);
  }
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);
}

// Set the reduce type for multiple triggers
void ila_set_reduce_type(int reduceType) {
  miscValue = setBit(miscValue, REDUCE_TYPE_BIT, reduceType);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);
}

// If CIRCULAR_BUFFER=0: Returns the number of samples currently stored in the
// ila buffer If CIRCULAR_BUFFER=1: Returns the index of the last sample stored
// in the ila buffer
int ila_number_samples() {
  uint32_t num_samples = 0;
  iob_sysfs_read_file(IOB_ILA_SYSFILE_N_SAMPLES, &num_samples);
  return num_samples;
}

// Returns the value as a 32 bit of the sample at position index
int ila_get_value(int index) {
  uint32_t sample_data = 0;
  iob_sysfs_write_file(IOB_ILA_SYSFILE_SIGNAL_SELECT, 0);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_INDEX, index);
  iob_sysfs_read_file(IOB_ILA_SYSFILE_SAMPLE_DATA, &sample_data);
  return sample_data;
}

uint32_t ila_get_large_value(int index, int partSelect) {
  uint32_t sample_data = 0;
  iob_sysfs_write_file(IOB_ILA_SYSFILE_SIGNAL_SELECT, partSelect);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_INDEX, index);
  iob_sysfs_read_file(IOB_ILA_SYSFILE_SAMPLE_DATA, &sample_data);
  return sample_data;
}

// Returns the value of the signal right now (does not mean it is stored in the
// buffer)
int ila_get_current_value() {
  uint32_t sample_data = 0;
  iob_sysfs_write_file(IOB_ILA_SYSFILE_SIGNAL_SELECT, 0);
  iob_sysfs_read_file(IOB_ILA_SYSFILE_SAMPLE_DATA, &sample_data);
  return sample_data;
}

// Returns 32 bits of the value of the signal right now
uint32_t ila_get_current_large_value(int partSelect) {
  uint32_t sample_data = 0;
  iob_sysfs_write_file(IOB_ILA_SYSFILE_SIGNAL_SELECT, partSelect);
  iob_sysfs_read_file(IOB_ILA_SYSFILE_SAMPLE_DATA, &sample_data);
  return sample_data;
}

// Returns the value of the trigger signal, directly (does not take into account
// negation or type)
int ila_get_current_triggers() {
  uint32_t cur_triggers = 0;
  iob_sysfs_read_file(IOB_ILA_SYSFILE_CURRENT_TRIGGERS, &cur_triggers);
  return cur_triggers;
}

// Returns the value of the triggers taking into account negation and type (For
// continuous triggers, the bit is asserted if the trigger has been activated)
int ila_get_current_active_triggers() {
  uint32_t active_triggers = 0;
  iob_sysfs_read_file(IOB_ILA_SYSFILE_CURRENT_ACTIVE_TRIGGERS,
                      &active_triggers);
  return active_triggers;
}

void ila_set_different_signal_storing(int enabled_bool) {
  miscValue = setBit(miscValue, DIFF_SIGNAL_BIT, enabled_bool);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);
}

void ila_print_current_configuration() {
  printf("Trigger Type:   %08x\n", triggerType);
  printf("Trigger Negate: %08x\n", triggerNegate);
  printf("Trigger Mask:   %08x\n", triggerMask);
  printf("Misc Value:     %08x\n\n", miscValue);
}

// TODO: how to replicate this with linux drivers?
// Returns Monitor base address based on ILA base address.
// ila_init() must be called first.
// You can use the iob-pfsm drivers to control the ILA Monitor using its base
// address.
uint32_t ila_get_monitor_base_addr(int base_address) {
  return base_address | 1 << (IOB_ILA_SWREG_ADDR_W - 1);
}

// Enable/Disable circular buffer
void ila_set_circular_buffer(int value) {
  miscValue = setBit(miscValue, CIRCULAR_BUFFER_BIT, value);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_MISCELLANEOUS, miscValue);
}

// Set index and partSelect for start of reading by DMA
void ila_set_cursor(int index, int partSelect) {
  iob_sysfs_write_file(IOB_ILA_SYSFILE_SIGNAL_SELECT, partSelect);
  iob_sysfs_write_file(IOB_ILA_SYSFILE_INDEX, index);
}

static inline char *OutputHex(char *buffer, int value) {
  char hex_table[] = "0123456789ABCDEF";
  int lw = hex_table[value & 0xFF]; // value % 16
  int hw = hex_table[value >> 4];   // value / 16

  (*buffer++) = hw;
  (*buffer++) = lw;

  return buffer;
}

int ila_output_data_size(int number_samples, int ila_dword_size) {
  int size_per_line = ila_dword_size * 8 + 1;    // +1 for '\n'
  int size = size_per_line * number_samples + 1; // +1 for '\0'

  return size;
}

int ila_output_data(char *buffer, int start, int end, int buffer_size,
                    int ila_dword_size) {
  union {
    char i8[4];
    int i32;
  } data;

  buffer[0] = '\0'; // For the cases where end - start == 0

  for (int i = start;; i = (i + 1) % buffer_size) {
    for (int ii = ila_dword_size - 1; ii >= 0; ii--) {
      data.i32 = ila_get_large_value(i, ii);
      buffer = OutputHex(buffer, data.i8[3]);
      buffer = OutputHex(buffer, data.i8[2]);
      buffer = OutputHex(buffer, data.i8[1]);
      buffer = OutputHex(buffer, data.i8[0]);
    }

    (*buffer++) = '\n';

    if (i == end)
      break;
  }
  *buffer = '\0';

  int res = end < start ? end + buffer_size - start : end - start;
  return res;
}

void ila_output_everything(int ila_dword_size, int buffer_size) {
  char buffer[1024]; // TODO: Make sure buffer size is big enough (can calculate
                     // at generate time)

  for (int i = 1; i <= buffer_size;) {
    i += ila_output_data(buffer, i - 1, i, buffer_size, ila_dword_size);
    printf("%s", buffer);
  }
}
