#ifndef DBC_SCHEMA_H
#define DBC_SCHEMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *name;
    const char *unit;
    uint32_t frame_id;
    uint8_t start_bit;
    uint8_t bit_length;
    uint8_t byte_order;
    uint8_t is_signed;
    float factor;
    float offset;
} dbc_signal_desc_t;

extern const char *g_dbc_name;
extern const dbc_signal_desc_t g_dbc_signals[];
extern const size_t g_dbc_signal_count;

#endif
