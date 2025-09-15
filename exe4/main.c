#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

const int BTN_PIN_R = 28;
const int BTN_PIN_G = 26;
const int LED_PIN_R = 4;
const int LED_PIN_G = 6;

QueueHandle_t xQueueButId;      // fila do LED R
QueueHandle_t xQueueButId_g;    // fila do LED G
SemaphoreHandle_t xSemaphore_r; // semáforo do botão R

void btn_callback(uint gpio, uint32_t events) {
  if (!(events & GPIO_IRQ_EDGE_FALL)) return;

  if (gpio == BTN_PIN_R) {
    xSemaphoreGiveFromISR(xSemaphore_r, 0);
  } else if (gpio == BTN_PIN_G) {
    static int delay_g = 0;
    delay_g = (delay_g < 1000) ? delay_g + 100 : 100;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(xQueueButId_g, &delay_g, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void led_1_task(void *p) {
  gpio_init(LED_PIN_R);
  gpio_set_dir(LED_PIN_R, GPIO_OUT);

  int delay = 0;
  while (true) {
    if (xQueueReceive(xQueueButId, &delay, 0)) {}

    if (delay > 0) {
      gpio_put(LED_PIN_R, 1);
      vTaskDelay(pdMS_TO_TICKS(delay));
      gpio_put(LED_PIN_R, 0);
      vTaskDelay(pdMS_TO_TICKS(delay));
    }
  }
}

void btn_1_task(void *p) {
  gpio_init(BTN_PIN_R);
  gpio_set_dir(BTN_PIN_R, GPIO_IN);
  gpio_pull_up(BTN_PIN_R);
  gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &btn_callback);

  int delay = 0;
  while (true) {
    if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500)) == pdTRUE) {
      delay = (delay < 1000) ? delay + 100 : 100;
      xQueueSend(xQueueButId, &delay, 0);
    }
  }
}

void led_2_task(void *p) {
  gpio_init(LED_PIN_G);
  gpio_set_dir(LED_PIN_G, GPIO_OUT);

  int delay = 0;
  while (true) {
    if (xQueueReceive(xQueueButId_g, &delay, portMAX_DELAY)) {}

    if (delay > 0) {
      gpio_put(LED_PIN_G, 1);
      vTaskDelay(pdMS_TO_TICKS(delay));
      gpio_put(LED_PIN_G, 0);
      vTaskDelay(pdMS_TO_TICKS(delay));
    }
  }
}

int main() {
  stdio_init_all();
  printf("Start RTOS\n");

  xQueueButId   = xQueueCreate(32, sizeof(int));
  xQueueButId_g = xQueueCreate(32, sizeof(int));
  xSemaphore_r  = xSemaphoreCreateBinary();

  // botão G também usa a MESMA callback (apenas habilita a IRQ)
  gpio_init(BTN_PIN_G);
  gpio_set_dir(BTN_PIN_G, GPIO_IN);
  gpio_pull_up(BTN_PIN_G);
  gpio_set_irq_enabled(BTN_PIN_G, GPIO_IRQ_EDGE_FALL, true);

  xTaskCreate(led_1_task, "LED_R", 256, NULL, 1, NULL);
  xTaskCreate(btn_1_task, "BTN_R", 256, NULL, 2, NULL);
  xTaskCreate(led_2_task, "LED_G", 256, NULL, 1, NULL);

  vTaskStartScheduler();
  while (true) {}
}