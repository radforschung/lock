#include "button.h"
#include "globals.h"

static const char *TAG = "button";

QueueHandle_t buttonQueue = NULL;

static void IRAM_ATTR button_isr_handler(void *args) {
  gpio_isr_handler_remove(PIN_BUTTON);
  void *nothing;
  xQueueSendToBackFromISR(buttonQueue, &nothing, NULL);
}

void button_task(void *ignore) {
  ESP_LOGD(TAG, "task=button_task state=enter");

  buttonQueue = xQueueCreate(3, 1);
  if (buttonQueue == NULL) {
    ESP_LOGE(TAG, "error=\"error creating button queue\"");
  }

  Bounce debounceButton = Bounce();
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  debounceButton.attach(PIN_BUTTON);
  debounceButton.interval(5);

  // TODO clean this up, mode etc is already set in button.cpp
  //     only thing to be done is to set INTR_ANYEDGE
  gpio_config_t gpioConfig;
  gpioConfig.pin_bit_mask = GPIO_SEL_26;
  gpioConfig.mode = GPIO_MODE_INPUT;
  gpioConfig.pull_up_en = GPIO_PULLUP_ENABLE;
  gpioConfig.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpioConfig.intr_type = GPIO_INTR_ANYEDGE;
  gpio_config(&gpioConfig);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(PIN_BUTTON, button_isr_handler, NULL);

  boolean lastState = false;
  void *nothing;
  while (1) {
    // wait for activation by queue
    ESP_LOGD(TAG, "task=button_task state=waiting");
    if (xQueueReceive(buttonQueue, &nothing, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    ESP_LOGD(TAG, "task=button_task state=active");

    debounceButton.update();
    bool pressed = debounceButton.read();
    if (lastState != pressed) {
      ESP_LOGI(TAG, "Button state changed: %d", pressed);
      xQueueSend(taskQueue, &TASK_SEND_LOCK_STATUS, portMAX_DELAY);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    lastState = pressed;

    gpio_isr_handler_add(PIN_BUTTON, button_isr_handler, NULL);

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
