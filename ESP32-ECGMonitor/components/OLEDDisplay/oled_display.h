#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t oled_display_init(i2c_master_bus_handle_t bus, uint8_t i2c_address);
void oled_display_push_samples(const uint16_t *samples, size_t count);
void oled_display_report_rr(uint32_t rr_ms);
