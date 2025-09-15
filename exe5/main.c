#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;
const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueBtn;
SemaphoreHandle_t xSemLedR, xSemLedY;

void btn_isr(uint gpio, uint32_t events) {
  if (!(events & GPIO_IRQ_EDGE_FALL)) return;
  int id = (gpio == BTN_PIN_R) ? 1 : 2;
  BaseType_t w = pdFALSE;
  xQueueSendFromISR(xQueueBtn, &id, &w);
  portYIELD_FROM_ISR(w);
}

void btn_task(void *p) {
  int id;
  while (1) {
    if (xQueueReceive(xQueueBtn, &id, portMAX_DELAY)) {
      if (id == 1) xSemaphoreGive(xSemLedR);
      else         xSemaphoreGive(xSemLedY);
    }
  }
}

void led_r_task(void *p) {
  gpio_init(LED_PIN_R);
  gpio_set_dir(LED_PIN_R, GPIO_OUT);
  int blink = 0, d = 100;
  while (1) {
    if (xSemaphoreTake(xSemLedR, 0) == pdTRUE) blink = !blink;
    if (blink) {
      gpio_put(LED_PIN_R, 1); vTaskDelay(pdMS_TO_TICKS(d));
      if (xSemaphoreTake(xSemLedR, 0) == pdTRUE) { blink = !blink; gpio_put(LED_PIN_R, 0); continue; }
      gpio_put(LED_PIN_R, 0); vTaskDelay(pdMS_TO_TICKS(d));
    } else {
      gpio_put(LED_PIN_R, 0);
      xSemaphoreTake(xSemLedR, portMAX_DELAY);
      blink = !blink;
    }
  }
}

void led_y_task(void *p) {
  gpio_init(LED_PIN_Y);
  gpio_set_dir(LED_PIN_Y, GPIO_OUT);
  int blink = 0, d = 100;
  while (1) {
    if (xSemaphoreTake(xSemLedY, 0) == pdTRUE) blink = !blink;
    if (blink) {
      gpio_put(LED_PIN_Y, 1); vTaskDelay(pdMS_TO_TICKS(d));
      if (xSemaphoreTake(xSemLedY, 0) == pdTRUE) { blink = !blink; gpio_put(LED_PIN_Y, 0); continue; }
      gpio_put(LED_PIN_Y, 0); vTaskDelay(pdMS_TO_TICKS(d));
    } else {
      gpio_put(LED_PIN_Y, 0);
      xSemaphoreTake(xSemLedY, portMAX_DELAY);
      blink = !blink;
    }
  }
}

int main() {
  stdio_init_all();

  gpio_init(BTN_PIN_R); gpio_set_dir(BTN_PIN_R, GPIO_IN); gpio_pull_up(BTN_PIN_R);
  gpio_init(BTN_PIN_Y); gpio_set_dir(BTN_PIN_Y, GPIO_IN); gpio_pull_up(BTN_PIN_Y);

  xQueueBtn = xQueueCreate(8, sizeof(int));
  xSemLedR  = xSemaphoreCreateBinary();
  xSemLedY  = xSemaphoreCreateBinary();

  gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_isr);
  gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

  xTaskCreate(btn_task,  "BTN",  256, NULL, 2, NULL);
  xTaskCreate(led_r_task,"LED_R",256, NULL, 1, NULL);
  xTaskCreate(led_y_task,"LED_Y",256, NULL, 1, NULL);

  vTaskStartScheduler();
  while (1) {}
}