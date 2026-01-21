/*
 *
 * Copyright (c) [2022] by InvenSense, Inc.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void inv_imu_sleep_us(uint32_t us)
{
    //delayMicroseconds(us);
/*     const TickType_t ticks_to_wait = us / 1000 * portTICK_PERIOD_MS;
    vTaskDelay(ticks_to_wait); */
    vTaskDelay(pdMS_TO_TICKS(us / 1000));
}

uint64_t inv_imu_get_time_us(void)
{
    //return (uint64_t)micros();
    return (uint64_t)(esp_timer_get_time() / 1000);
}