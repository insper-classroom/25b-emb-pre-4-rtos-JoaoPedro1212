#include <FreeRTOS.h>
#include <task.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_R 4
#define LED_G 6
#define BLINK_MS 250

static void led_task(void *p) {
  int pin = (int)(uintptr_t)p;
  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, 0);

  while (1) {
    gpio_put(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
    gpio_put(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
  }
}

int main(void) {
  xTaskCreate(led_task, "LED_R", 256, (void*)(uintptr_t)LED_R, 1, NULL);
  xTaskCreate(led_task, "LED_G", 256, (void*)(uintptr_t)LED_G, 1, NULL);
  vTaskStartScheduler();
  while (1) {}
}