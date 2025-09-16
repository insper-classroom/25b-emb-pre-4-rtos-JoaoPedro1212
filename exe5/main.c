/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

#define BLINK_MS 100

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemR, xSemY;

static void btn_isr(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) return;
    int id = (gpio == BTN_PIN_R) ? 1 : 2;
    BaseType_t w = pdFALSE;
    xQueueSendFromISR(xQueueBtn, &id, &w);
    portYIELD_FROM_ISR(w);
}

static void led_task(void *p) {
    int pin = (int)(uintptr_t)p;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);

    SemaphoreHandle_t sem = (pin == LED_PIN_R) ? xSemR : xSemY;
    int blinking = 0;

    while (true) {
        if (xSemaphoreTake(sem, 0) == pdTRUE) blinking = !blinking;

        if (blinking) {
            gpio_put(pin, 1); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
            if (xSemaphoreTake(sem, 0) == pdTRUE) { blinking = !blinking; gpio_put(pin, 0); continue; }
            gpio_put(pin, 0); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
        } else {
            gpio_put(pin, 0);
            xSemaphoreTake(sem, portMAX_DELAY);
            blinking = !blinking;
        }
    }
}

void btn_task(void* p) {

    gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
    gpio_init(BTN_PIN_Y); gpio_set_dir(BTN_PIN_Y, GPIO_IN); gpio_pull_up(BTN_PIN_Y);

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_isr);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int id;

    while (true) {

        if (xQueueReceive(xQueueBtn, &id, portMAX_DELAY)) {
            if (id == 1) xSemaphoreGive(xSemR);
            else         xSemaphoreGive(xSemY);
        }
    }
}

int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(8, sizeof(int));
    xSemR = xSemaphoreCreateBinary();
    xSemY = xSemaphoreCreateBinary();

    xTaskCreate(led_task, "LED_Task 1", 256, (void*)(uintptr_t)LED_PIN_R, 1, NULL);
    xTaskCreate(led_task, "LED_Task 2", 256, (void*)(uintptr_t)LED_PIN_Y, 1, NULL);

    xTaskCreate(btn_task, "BTN_Task 1", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}