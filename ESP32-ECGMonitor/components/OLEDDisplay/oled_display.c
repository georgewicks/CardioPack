#include "oled_display.h"

#include <stdio.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OLED_WIDTH                  (128)
#define OLED_HEIGHT                 (64)
#define OLED_FRAME_BYTES            (OLED_WIDTH * OLED_HEIGHT / 8)
#define OLED_RR_HISTORY             (30)
#define OLED_SAMPLE_RATE_HZ         (300)
#define OLED_TRACE_SECONDS          (3)
#define OLED_TRACE_SAMPLES          (OLED_SAMPLE_RATE_HZ * OLED_TRACE_SECONDS)
#define OLED_REFRESH_MS             (1000)
#define OLED_METRICS_PAGE_MS        (5000)
#define OLED_PAGE_CYCLE_MS          (8000)
#define OLED_I2C_CLOCK_HZ           (400000)

static const char *TAG = "oled_display";

typedef struct {
    uint32_t rr[OLED_RR_HISTORY];
    size_t count;
    size_t next;
    uint32_t latest;
} rr_history_t;

static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_panel_handle_t s_panel;
static bool s_initialized;
static portMUX_TYPE s_state_mux = portMUX_INITIALIZER_UNLOCKED;
static rr_history_t s_rr_history;
static uint16_t s_wave_samples[OLED_TRACE_SAMPLES];
static size_t s_wave_count;
static size_t s_wave_next;
static uint8_t s_frame[OLED_FRAME_BYTES];
static int16_t s_render_samples[OLED_TRACE_SAMPLES];

static const uint8_t s_digits[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
};

static const uint8_t s_letters[26][5] = {
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
};

static uint8_t glyph_column(char ch, int column)
{
    if (ch >= '0' && ch <= '9') {
        return s_digits[ch - '0'][column];
    }
    if (ch >= 'A' && ch <= 'Z') {
        return s_letters[ch - 'A'][column];
    }
    switch (ch) {
    case '-':
        return 0x08;
    case '/':
        return (uint8_t[]){0x20, 0x10, 0x08, 0x04, 0x02}[column];
    case ':':
        return (uint8_t[]){0x00, 0x36, 0x36, 0x00, 0x00}[column];
    case '=':
        return 0x14;
    case '.':
        return (uint8_t[]){0x00, 0x60, 0x60, 0x00, 0x00}[column];
    default:
        return 0;
    }
}

static void draw_pixel(int x, int y)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }
    s_frame[(y / 8) * OLED_WIDTH + x] |= (uint8_t)(1U << (y % 8));
}

static void draw_text(int x, int y, const char *text, int scale)
{
    for (size_t i = 0; text[i] != '\0'; i++) {
        char ch = text[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - 'a' + 'A');
        }

        const int char_x = x + (int)i * 6 * scale;
        if (char_x >= OLED_WIDTH) {
            break;
        }
        for (int col = 0; col < 5; col++) {
            const uint8_t bits = glyph_column(ch, col);
            for (int row = 0; row < 7; row++) {
                if ((bits & (1U << row)) == 0) {
                    continue;
                }
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        draw_pixel(char_x + col * scale + dx,
                                   y + row * scale + dy);
                    }
                }
            }
        }
    }
}

static void draw_line(int x0, int y0, int x1, int y1)
{
    const int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    const int sx = (x0 < x1) ? 1 : -1;
    const int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    const int sy = (y0 < y1) ? 1 : -1;
    int error = dx + dy;

    while (true) {
        draw_pixel(x0, y0);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twice_error = 2 * error;
        if (twice_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void snapshot_rr_history(rr_history_t *snapshot)
{
    portENTER_CRITICAL(&s_state_mux);
    *snapshot = s_rr_history;
    portEXIT_CRITICAL(&s_state_mux);
}

static size_t snapshot_waveform(void)
{
    portENTER_CRITICAL(&s_state_mux);
    const size_t count = s_wave_count;
    const size_t start = (s_wave_next + OLED_TRACE_SAMPLES - count) % OLED_TRACE_SAMPLES;
    for (size_t i = 0; i < count; i++) {
        s_render_samples[i] = (int16_t)s_wave_samples[(start + i) % OLED_TRACE_SAMPLES];
    }
    portEXIT_CRITICAL(&s_state_mux);
    return count;
}

static esp_err_t flush_frame(void)
{
    return esp_lcd_panel_draw_bitmap(s_panel, 0, 0, OLED_WIDTH, OLED_HEIGHT, s_frame);
}

static void render_metrics(void)
{
    rr_history_t history;
    snapshot_rr_history(&history);
    memset(s_frame, 0, sizeof(s_frame));

    draw_text(0, 0, "ECG METRICS", 1);
    if (history.count == 0 || history.latest == 0) {
        draw_text(1, 8, "HR -- BPM", 2);
        draw_text(0, 26, "RR NOW -- MS", 1);
        draw_text(0, 34, "AVG -- MS", 1);
        draw_text(0, 42, "MIN/MAX --/--", 1);
        draw_text(0, 50, "N 0 RR", 1);
        draw_text(0, 57, "NOT DIAGNOSTIC", 1);
        return;
    }

    uint64_t sum = 0;
    uint32_t minimum = UINT32_MAX;
    uint32_t maximum = 0;
    for (size_t i = 0; i < history.count; i++) {
        const size_t index = (history.next + OLED_RR_HISTORY - history.count + i) % OLED_RR_HISTORY;
        const uint32_t rr = history.rr[index];
        sum += rr;
        if (rr < minimum) {
            minimum = rr;
        }
        if (rr > maximum) {
            maximum = rr;
        }
    }

    const uint32_t bpm = (60000U + history.latest / 2U) / history.latest;
    const uint32_t average = (uint32_t)((sum + history.count / 2U) / history.count);
    char line[24];

    snprintf(line, sizeof(line), "HR %lu BPM", (unsigned long)bpm);
    draw_text(1, 8, line, 2);
    snprintf(line, sizeof(line), "RR NOW %lu MS", (unsigned long)history.latest);
    draw_text(0, 26, line, 1);
    snprintf(line, sizeof(line), "AVG %lu MS", (unsigned long)average);
    draw_text(0, 34, line, 1);
    snprintf(line, sizeof(line), "MIN/MAX %lu/%lu",
             (unsigned long)minimum, (unsigned long)maximum);
    draw_text(0, 42, line, 1);
    snprintf(line, sizeof(line), "N %u RR", (unsigned)history.count);
    draw_text(0, 50, line, 1);
    draw_text(0, 57, "NOT DIAGNOSTIC", 1);
}

static void render_trace(void)
{
    const size_t count = snapshot_waveform();
    memset(s_frame, 0, sizeof(s_frame));
    draw_text(0, 0, "ECG TRACE", 1);

    const uint32_t duration_tenths =
        (uint32_t)((count * 10U + OLED_SAMPLE_RATE_HZ / 2U) / OLED_SAMPLE_RATE_HZ);
    char line[24];
    snprintf(line, sizeof(line), "DUR %u.%u S",
             (unsigned)(duration_tenths / 10U),
             (unsigned)(duration_tenths % 10U));
    draw_text(0, 8, line, 1);

    if (count < 2) {
        draw_text(0, 28, "WAITING FOR ECG", 1);
    } else {
        int16_t overall_min = INT16_MAX;
        int16_t overall_max = INT16_MIN;
        for (size_t i = 0; i < count; i++) {
            if (s_render_samples[i] < overall_min) {
                overall_min = s_render_samples[i];
            }
            if (s_render_samples[i] > overall_max) {
                overall_max = s_render_samples[i];
            }
        }
        const int32_t range = (int32_t)overall_max - (int32_t)overall_min;
        const int plot_top = 18;
        const int plot_height = 34;
        int previous_y = plot_top + plot_height / 2;

        for (int x = 0; x < OLED_WIDTH; x++) {
            size_t start = ((size_t)x * count) / OLED_WIDTH;
            size_t end = ((size_t)(x + 1) * count) / OLED_WIDTH;
            if (end <= start) {
                end = start + 1;
            }
            if (start >= count) {
                start = count - 1;
                end = count;
            }
            if (end > count) {
                end = count;
            }

            int16_t sample_min = INT16_MAX;
            int16_t sample_max = INT16_MIN;
            int64_t sample_sum = 0;
            for (size_t i = start; i < end; i++) {
                const int16_t sample = s_render_samples[i];
                if (sample < sample_min) {
                    sample_min = sample;
                }
                if (sample > sample_max) {
                    sample_max = sample;
                }
                sample_sum += sample;
            }
            const int32_t mean = (int32_t)(sample_sum / (int64_t)(end - start));
            const int y_min = plot_top + plot_height -
                              (int)(((int32_t)sample_min - overall_min) * plot_height /
                                    (range == 0 ? 1 : range));
            const int y_max = plot_top + plot_height -
                              (int)(((int32_t)sample_max - overall_min) * plot_height /
                                    (range == 0 ? 1 : range));
            const int y_mean = plot_top + plot_height -
                               (int)((mean - overall_min) * plot_height /
                                     (range == 0 ? 1 : range));
            draw_line(x, y_max, x, y_min);
            if (x > 0) {
                draw_line(x - 1, previous_y, x, y_mean);
            }
            previous_y = y_mean;
        }
    }

    draw_text(0, 56, "NOT DIAGNOSTIC", 1);
}

static void oled_display_task(void *arg)
{
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t page_phase_ms = 0;
    unsigned consecutive_errors = 0;

    for (;;) {
        if (page_phase_ms < OLED_METRICS_PAGE_MS) {
            render_metrics();
        } else {
            render_trace();
        }

        const esp_err_t err = flush_frame();
        if (err != ESP_OK) {
            if (consecutive_errors == 0 || consecutive_errors % 30U == 0) {
                ESP_LOGW(TAG, "OLED frame update failed: %s", esp_err_to_name(err));
            }
            consecutive_errors++;
        } else {
            consecutive_errors = 0;
        }

        page_phase_ms = (page_phase_ms + OLED_REFRESH_MS) % OLED_PAGE_CYCLE_MS;
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(OLED_REFRESH_MS));
    }
}

esp_err_t oled_display_init(i2c_master_bus_handle_t bus, uint8_t i2c_address)
{
    if (bus == NULL || i2c_address > 0x7F || s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = i2c_master_probe(bus, i2c_address, 100);
    if (err != ESP_OK) {
        return err;
    }

    const esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = i2c_address,
        .scl_speed_hz = OLED_I2C_CLOCK_HZ,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    err = esp_lcd_new_panel_io_i2c(bus, &io_config, &s_io);
    if (err != ESP_OK) {
        goto fail;
    }

    const esp_lcd_panel_dev_config_t panel_config = {
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 1,
        .reset_gpio_num = GPIO_NUM_NC,
        .vendor_config = NULL,
    };
    err = esp_lcd_new_panel_ssd1306(s_io, &panel_config, &s_panel);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_lcd_panel_reset(s_panel);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_lcd_panel_init(s_panel);
    if (err != ESP_OK) {
        goto fail;
    }
    err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err != ESP_OK) {
        goto fail;
    }

    if (xTaskCreate(oled_display_task, "oled_display", 4096, NULL, 2, NULL) != pdPASS) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    s_initialized = true;
    ESP_LOGI(TAG, "SSD1306 OLED initialized at I2C address 0x%02X", i2c_address);
    return ESP_OK;

fail:
    if (s_panel != NULL) {
        esp_lcd_panel_del(s_panel);
        s_panel = NULL;
    }
    if (s_io != NULL) {
        esp_lcd_panel_io_del(s_io);
        s_io = NULL;
    }
    return err;
}

void oled_display_push_samples(const uint16_t *samples, size_t count)
{
    if (samples == NULL) {
        return;
    }

    portENTER_CRITICAL(&s_state_mux);
    for (size_t i = 0; i < count; i++) {
        s_wave_samples[s_wave_next] = samples[i];
        s_wave_next = (s_wave_next + 1U) % OLED_TRACE_SAMPLES;
        if (s_wave_count < OLED_TRACE_SAMPLES) {
            s_wave_count++;
        }
    }
    portEXIT_CRITICAL(&s_state_mux);
}

void oled_display_report_rr(uint32_t rr_ms)
{
    if (rr_ms == 0) {
        return;
    }

    portENTER_CRITICAL(&s_state_mux);
    s_rr_history.rr[s_rr_history.next] = rr_ms;
    s_rr_history.next = (s_rr_history.next + 1U) % OLED_RR_HISTORY;
    if (s_rr_history.count < OLED_RR_HISTORY) {
        s_rr_history.count++;
    }
    s_rr_history.latest = rr_ms;
    portEXIT_CRITICAL(&s_state_mux);
}
