#include "can_reader.h"
#include "config.h"

#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "dbc_schema.h"

static const char *TAG = "CAN_READER";

typedef struct {
    float value;
    bool valid;
} signal_state_t;

static SemaphoreHandle_t s_lock;
static signal_state_t s_state[64];
static uint64_t s_last_ts_ms;

static twai_timing_config_t get_twai_timing_from_config(void)
{
    twai_timing_config_t t;
#if CAN_BITRATE_KBPS == 125
    t = (twai_timing_config_t)TWAI_TIMING_CONFIG_125KBITS();
#elif CAN_BITRATE_KBPS == 250
    t = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
#elif CAN_BITRATE_KBPS == 500
    t = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
#elif CAN_BITRATE_KBPS == 1000
    t = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
#else
#error "CAN_BITRATE_KBPS non supporte. Utiliser 125, 250, 500 ou 1000."
#endif
    return t;
}

static uint8_t get_bit(const uint8_t *data, int bit_index)
{
    return (uint8_t)((data[bit_index / 8] >> (bit_index % 8)) & 0x1);
}

static uint64_t extract_raw(const uint8_t *data, const dbc_signal_desc_t *d)
{
    uint64_t raw = 0;
    if (d->bit_length == 0 || d->bit_length > 64) {
        return 0;
    }

    if (d->byte_order == 1) {
        for (int i = 0; i < d->bit_length; i++) {
            int bit = d->start_bit + i;
            raw |= ((uint64_t)get_bit(data, bit) << i);
        }
    } else {
        int bit = d->start_bit;
        for (int i = 0; i < d->bit_length; i++) {
            raw = (raw << 1) | get_bit(data, bit);
            if ((bit % 8) == 0) {
                bit += 15;
            } else {
                bit -= 1;
            }
        }
    }
    return raw;
}

static float decode_value(const uint8_t *data, const dbc_signal_desc_t *d)
{
    uint64_t raw = extract_raw(data, d);

    if (d->is_signed && d->bit_length < 64) {
        uint64_t sign_mask = (uint64_t)1 << (d->bit_length - 1);
        if (raw & sign_mask) {
            uint64_t extend_mask = ~(((uint64_t)1 << d->bit_length) - 1);
            int64_t s = (int64_t)(raw | extend_mask);
            return ((float)s * d->factor) + d->offset;
        }
    }

    return ((float)raw * d->factor) + d->offset;
}

static void can_task(void *arg)
{
    twai_message_t rx;

    while (1) {
        if (twai_receive(&rx, portMAX_DELAY) != ESP_OK) {
            continue;
        }
        if (rx.rtr || rx.extd || rx.data_length_code < 1) {
            continue;
        }

        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_last_ts_ms = (uint64_t)(esp_timer_get_time() / 1000);

        for (size_t i = 0; i < g_dbc_signal_count && i < 64; i++) {
            const dbc_signal_desc_t *d = &g_dbc_signals[i];
            if (d->frame_id == rx.identifier) {
                s_state[i].value = decode_value(rx.data, d);
                s_state[i].valid = true;
            }
        }

        xSemaphoreGive(s_lock);
    }
}

void can_reader_start(void)
{
    const twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
    const twai_timing_config_t t = get_twai_timing_from_config();
    const twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    s_lock = xSemaphoreCreateMutex();
    memset(s_state, 0, sizeof(s_state));
    s_last_ts_ms = 0;

    ESP_ERROR_CHECK(twai_driver_install(&g, &t, &f));
    ESP_ERROR_CHECK(twai_start());

    ESP_LOGI(TAG, "TWAI started at %d kbps | TX=%d RX=%d | DBC=%s | signals=%u",
             CAN_BITRATE_KBPS, CAN_TX_GPIO, CAN_RX_GPIO, g_dbc_name, (unsigned)g_dbc_signal_count);
    xTaskCreate(can_task, "can_task", 4096, NULL, 8, NULL);
}

const char *can_reader_get_dbc_name(void)
{
    return g_dbc_name;
}

size_t can_reader_get_signal_count(void)
{
    return g_dbc_signal_count;
}

void can_reader_copy_signals(can_signal_value_t *out, size_t max_count, uint64_t *ts_ms_out)
{
    if (!out) {
        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);

    size_t n = g_dbc_signal_count < max_count ? g_dbc_signal_count : max_count;
    for (size_t i = 0; i < n; i++) {
        out[i].name = g_dbc_signals[i].name;
        out[i].unit = g_dbc_signals[i].unit;
        out[i].value = s_state[i].value;
        out[i].valid = s_state[i].valid;
    }

    if (ts_ms_out) {
        *ts_ms_out = s_last_ts_ms;
    }

    xSemaphoreGive(s_lock);
}
