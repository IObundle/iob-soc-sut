#include <stdint.h>
#include <stdio.h>

#include "iob-sysfs-common.h"
/* IOb-ILA header automatically generated from:
 * /path/to/iob-linux/scripts/drivers.py iob_ila
 * contains:
 * - sysfs file paths
 * - Register address and width definitions
 */
#include "iob_ila.h"

// Defines from ILA/software/src/iob-ila.h
#define ILA_REDUCE_TYPE_OR 0
#define ILA_REDUCE_TYPE_AND 1

// Defines from generated file: ILA0.h
#define ILA0_DWORD_SIZE 2
#define ILA0_BYTE_SIZE 7
#define ILA0_BUFFER_SIZE (2 * *ILA0_BUFFER_W)

void ila_init(int base_address);
void ila_reset();
void ila_set_trigger_type(int trigger, int type);
void ila_set_trigger_enabled(int trigger, int enabled_bool);
void ila_set_trigger_negated(int trigger, int negate_bool);
void ila_enable_all_triggers();
void ila_disable_all_triggers();
void ila_set_time_offset(int amount);
void ila_set_reduce_type(int reduceType);
int ila_number_samples();
int ila_get_value(int index);
uint32_t ila_get_large_value(int index, int partSelect);
int ila_get_current_value();
uint32_t ila_get_current_large_value(int partSelect);
int ila_get_current_triggers();
int ila_get_current_active_triggers();
void ila_set_different_signal_storing(int enabled_bool);
void ila_print_current_configuration();
uint32_t ila_get_monitor_base_addr(int base_address);
void ila_set_circular_buffer(int value);
void ila_set_cursor(int index, int partSelect);

// ila-static-generate functions
int ila_output_data_size(int number_samples, int ila_dword_size);
int ila_output_data(char *buffer, int start, int end, int buffer_size,
                    int ila_dword_size);
void ila_output_everything(int ila_dword_size, int buffer_size);
