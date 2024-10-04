#include <Arduino.h>
#include "global.h"

TaskHandle_t TASK_KEYBOARD;
TaskHandle_t TASK_UDP;
TaskHandle_t TASK_WMANAGER;
TaskHandle_t TASK_BUTTON;
TaskHandle_t TASK_OLED;

void action_button(void *param)
{
  pinMode(0, INPUT_PULLUP);
  while (1)
  {
    uint32_t cnt = 0;
    while (digitalRead(0) == 0)
    {
      if (cnt == 10)
      {
        reset_setup();
        Serial.println("Setup reset to default value");
        ESP.restart();
      }
      cnt++;
      Serial.print("=");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}
void setup()
{

  /////////////config Watchdog timer/////////////////
  esp_task_wdt_init(0xffffffff, true);
  esp_task_wdt_reset();

  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  /////////////////// SPIFFS init///////////////////
  if (!SPIFFS.begin(true))
  {
    return;
  }
  //////////////TASK Action Button///////////////////
  xTaskCreatePinnedToCore(action_button, "Action Button", 8192, NULL, PRIORITY_NORMAL, &TASK_BUTTON, CPU_0);
  //////////////TASK KEYBOARD///////////////////
  xTaskCreatePinnedToCore(keyboardHandler, "keyboard task", 8192, NULL, PRIORITY_REALTIME, &TASK_KEYBOARD, CPU_0);
  //////////////TASK UDP///////////////////
  xTaskCreatePinnedToCore(UDP_client, "UDP task", 8192, NULL, PRIORITY_NORMAL, &TASK_UDP, CPU_1);
  //////////////TASK UDP///////////////////
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(WiFi_manager, "WiFi Manager", 8192, NULL, PRIORITY_VERY_HIGH, &TASK_WMANAGER, CPU_1);
  //////////////TASK KEYBOARD///////////////////
  xTaskCreatePinnedToCore(display_oled_loop, "oled task", 8192, NULL, PRIORITY_HIGH, &TASK_OLED, CPU_0);
}

void loop()
{
}
