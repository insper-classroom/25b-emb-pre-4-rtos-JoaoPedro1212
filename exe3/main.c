// exe3/main.c
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN_R 28
#define BTN_G 26
#define LED_R 4
#define LED_G 6

static QueueHandle_t qR, qG;

static void led_task(void *p) {
  int pin = (int)(uintptr_t)p;
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, 0);

  QueueHandle_t q = (pin == LED_R) ? qR : qG;
  int d = 0;

  while (1) {
    if (d == 0) xQueueReceive(q, &d, portMAX_DELAY);     // espera primeiro valor
    gpio_put(pin, 1); vTaskDelay(pdMS_TO_TICKS(d));
    xQueueReceive(q, &d, 0);                             // atualiza se chegar
    gpio_put(pin, 0); vTaskDelay(pdMS_TO_TICKS(d));
    xQueueReceive(q, &d, 0);                             // atualiza se chegar
  }
}

static void btn_task(void *p) {
  int pin = (int)(uintptr_t)p;
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_up(pin);

  QueueHandle_t q = (pin == BTN_R) ? qR : qG;
  int d = 0;

  while (1) {
    if (!gpio_get(pin)) {
      while (!gpio_get(pin)) vTaskDelay(pdMS_TO_TICKS(1)); 
      d = (d < 1000) ? d + 100 : 100;
      xQueueSend(q, &d, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

int main(void) {
  qR = xQueueCreate(8, sizeof(int));
  qG = xQueueCreate(8, sizeof(int));

  xTaskCreate(led_task, "LED_R", 256, (void*)(uintptr_t)LED_R, 1, NULL);
  xTaskCreate(led_task, "LED_G", 256, (void*)(uintptr_t)LED_G, 1, NULL);
  xTaskCreate(btn_task, "BTN_R", 256, (void*)(uintptr_t)BTN_R, 2, NULL);
  xTaskCreate(btn_task, "BTN_G", 256, (void*)(uintptr_t)BTN_G, 2, NULL);

  vTaskStartScheduler();
  while (1) {}
}