// exe2/main.c
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN_R 28
#define BTN_G 26
#define LED_R 4
#define LED_G 6
#define BLINK_MS 250

static SemaphoreHandle_t semR, semG;

static void led_task(void *p) {
  int pin = (int)(uintptr_t)p;
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, 0);
  SemaphoreHandle_t sem = (pin == LED_R) ? semR : semG;

  while (1) {
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
      gpio_put(pin, 1); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
      gpio_put(pin, 0); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
    }
  }
}

static void btn_task(void *p) {
  int pin = (int)(uintptr_t)p;
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_IN);
  gpio_pull_up(pin);
  SemaphoreHandle_t sem = (pin == BTN_R) ? semR : semG;

  while (1) {
    if (!gpio_get(pin)) {
      while (!gpio_get(pin)) vTaskDelay(pdMS_TO_TICKS(1)); 
      xSemaphoreGive(sem);
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

int main(void) {
  semR = xSemaphoreCreateBinary();
  semG = xSemaphoreCreateBinary();

  xTaskCreate(led_task, "LED_R", 256, (void*)(uintptr_t)LED_R, 1, NULL);
  xTaskCreate(led_task, "LED_G", 256, (void*)(uintptr_t)LED_G, 1, NULL);
  xTaskCreate(btn_task, "BTN_R", 256, (void*)(uintptr_t)BTN_R, 2, NULL);
  xTaskCreate(btn_task, "BTN_G", 256, (void*)(uintptr_t)BTN_G, 2, NULL);

  vTaskStartScheduler();
  while (1) {}
}