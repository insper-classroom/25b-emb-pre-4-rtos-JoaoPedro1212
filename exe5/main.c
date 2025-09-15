#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN_R 28
#define BTN_Y 21
#define LED_R 5
#define LED_Y 10
#define BLINK_MS 100

static QueueHandle_t qBtn;     
static SemaphoreHandle_t semR;
static SemaphoreHandle_t semY;

static void gpio_btn_isr(uint gpio, uint32_t events) {
  if (!(events & GPIO_IRQ_EDGE_FALL)) return;
  int id = (gpio == BTN_R) ? 1 : 2;
  BaseType_t w = pdFALSE;
  xQueueSendFromISR(qBtn, &id, &w);
  portYIELD_FROM_ISR(w);
}

static void btn_task(void *p) {
  int id;
  while (1) {
    if (xQueueReceive(qBtn, &id, portMAX_DELAY)) {
      if (id == 1) xSemaphoreGive(semR);
      else         xSemaphoreGive(semY);
    }
  }
}

static void led_r_task(void *p) {
  gpio_init(LED_R);
  gpio_set_dir(LED_R, GPIO_OUT);
  gpio_put(LED_R, 0);

  int blink = 0;
  while (1) {
    if (xSemaphoreTake(semR, 0) == pdTRUE) blink = !blink;

    if (blink) {
      gpio_put(LED_R, 1); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
      if (xSemaphoreTake(semR, 0) == pdTRUE) { blink = !blink; gpio_put(LED_R, 0); continue; }
      gpio_put(LED_R, 0); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
    } else {
      gpio_put(LED_R, 0);
      xSemaphoreTake(semR, portMAX_DELAY);
      blink = !blink;
    }
  }
}

static void led_y_task(void *p) {
  gpio_init(LED_Y);
  gpio_set_dir(LED_Y, GPIO_OUT);
  gpio_put(LED_Y, 0);

  int blink = 0;
  while (1) {
    if (xSemaphoreTake(semY, 0) == pdTRUE) blink = !blink;

    if (blink) {
      gpio_put(LED_Y, 1); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
      if (xSemaphoreTake(semY, 0) == pdTRUE) { blink = !blink; gpio_put(LED_Y, 0); continue; }
      gpio_put(LED_Y, 0); vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
    } else {
      gpio_put(LED_Y, 0);
      xSemaphoreTake(semY, portMAX_DELAY);
      blink = !blink;
    }
  }
}

int main() {

  gpio_init(BTN_R); gpio_set_dir(BTN_R, GPIO_IN); gpio_pull_up(BTN_R);
  gpio_init(BTN_Y); gpio_set_dir(BTN_Y, GPIO_IN); gpio_pull_up(BTN_Y);

  qBtn = xQueueCreate(8, sizeof(int));
  semR = xSemaphoreCreateBinary();
  semY = xSemaphoreCreateBinary();

  gpio_set_irq_enabled_with_callback(BTN_R, GPIO_IRQ_EDGE_FALL, true, &gpio_btn_isr);
  gpio_set_irq_enabled(BTN_Y, GPIO_IRQ_EDGE_FALL, true);

  xTaskCreate(btn_task,  "BTN",   256, NULL, 2, NULL);
  xTaskCreate(led_r_task,"LED_R", 256, NULL, 1, NULL);
  xTaskCreate(led_y_task,"LED_Y", 256, NULL, 1, NULL);

  vTaskStartScheduler();
  while (1) {}
}