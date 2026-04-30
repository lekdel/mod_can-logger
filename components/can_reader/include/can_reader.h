#ifndef CAN_READER_H
#define CAN_READER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    const char *name;
    const char *unit;
    float value;
    bool valid;
} can_signal_value_t;

void can_reader_start(void);
const char *can_reader_get_dbc_name(void);
size_t can_reader_get_signal_count(void);
void can_reader_copy_signals(can_signal_value_t *out, size_t max_count, uint64_t *ts_ms_out);

#endif
