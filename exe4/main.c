// exe4/main.c
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BTN_R 28
#define BTN_G 26
#define LED_R 4
#define LED_G 6

static QueueHandle_t qR, qG;     // R recebe delay pela task, G recebe pela ISR
static SemaphoreHandle_t semR;   // semáforo liberado pela ISR do botão R

static void isr_btn(uint gpio, uint32_t events) {
  if (!(events & GPIO_IRQ_EDGE_FALL)) return;

  if (gpio == BTN_R) {
    xSemaphoreGiveFromISR(semR, NULL);
  } else if (gpio == BTN_G) {
    static int dG = 0;
    dG = (dG < 1000) ? dG + 100 : 100;
    BaseType_t w = pdFALSE;
    xQueueSendFromISR(qG, &dG, &w);
    portYIELD_FROM_ISR(w);
  }
}

static void led_r_task(void *p) {
  gpio_init(LED_R);
  gpio_set_dir(LED_R, GPIO_OUT);
  gpio_put(LED_R, 0);

  int d = 0;
  while (1) {
    if (xQueueReceive(qR, &d, 0)) {}
    if (xSemaphoreTake(semR, portMAX_DELAY) == pdTRUE && d > 0) {
      gpio_put(LED_R, 1); vTaskDelay(pdMS_TO_TICKS(d));
      gpio_put(LED_R, 0); vTaskDelay(pdMS_TO_TICKS(d));
    }
  }
}

static void btn_r_task(void *p) {
  gpio_init(BTN_R);
  gpio_set_dir(BTN_R, GPIO_IN);
  gpio_pull_up(BTN_R);
  gpio_set_irq_enabled_with_callback(BTN_R, GPIO_IRQ_EDGE_FALL, true, &isr_btn);

  int d = 0;
  while (1) {
    if (xSemaphoreTake(semR, portMAX_DELAY) == pdTRUE) {
      d = (d < 1000) ? d + 100 : 100;
      xQueueSend(qR, &d, 0);
    }
  }
}

static void led_g_task(void *p) {
  gpio_init(LED_G);
  gpio_set_dir(LED_G, GPIO_OUT);
  gpio_put(LED_G, 0);

  int d = 0;
  while (1) {
    if (xQueueReceive(qG, &d, portMAX_DELAY)) {
      gpio_put(LED_G, 1); vTaskDelay(pdMS_TO_TICKS(d));
      gpio_put(LED_G, 0); vTaskDelay(pdMS_TO_TICKS(d));
    }
  }
}

int main(void) {
  // sem prints

  qR   = xQueueCreate(8, sizeof(int));
  qG   = xQueueCreate(8, sizeof(int));
  semR = xSemaphoreCreateBinary();

  gpio_init(BTN_G); gpio_set_dir(BTN_G, GPIO_IN); gpio_pull_up(BTN_G);
  gpio_set_irq_enabled(BTN_G, GPIO_IRQ_EDGE_FALL, true); // mesma callback já registrada

  xTaskCreate(led_r_task, "LED_R", 256, NULL, 1, NULL);
  xTaskCreate(btn_r_task, "BTN_R", 256, NULL, 2, NULL);
  xTaskCreate(led_g_task, "LED_G", 256, NULL, 1, NULL);

  vTaskStartScheduler();
  while (1) {}
}
