#include <Arduino.h>
#include "global.h"

TaskHandle_t TASK_KEYBOARD;
TaskHandle_t TASK_UDP;
TaskHandle_t TASK_WMANAGER;
TaskHandle_t TASK_BUTTON;
TaskHandle_t TASK_OLED;

void action_button(void *param)
{
  /*
  if the screen is off, then just turn on the screen without change the menu
  */
  pinMode(0, INPUT_PULLUP);
  while (1)
  {
    reset_cnt = 0;
    if (digitalRead(0) == 0)
    {
      if (wakeup == 1)
      {
        while (digitalRead(0) == 0)
        {
          if (reset_cnt >= 100)
          {
            // reset_setup();
            Serial.println("Setup reset to default value");
            ESP.restart();
          }
          if (reset_cnt >= 10)
          {
            message_info_index = MENU_GOING_RESET;
          }
          reset_cnt++;
          Serial.print("=");
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        message_info_index = last_message;
        message_info_index++;
        if (message_info_index >= MENU_CHOICE_END)
        {
          message_info_index = 0;
        }
      }
      else
      {
        while (digitalRead(0) == 0)
        {
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        wakeup = 1;
        timer_wakeup = millis();
      }
    }
  }
}
void setup()
{

  Serial.begin(115200);
  /////////////config Watchdog timer/////////////////
  esp_task_wdt_init(0xffffffff, true);
  esp_task_wdt_reset();

  ////////////EEPROM setup////////////////////
  EEPROM.begin(32);
  if (EEPROM.read(0) > 1)
  {
    EEPROM.write(0, 1);
    EEPROM.commit();
    conn_mode = EEPROM.read(0);
  }
  else
  {
    conn_mode = EEPROM.read(0);
  }
  ////////conn_mode//////
  // 0 = TCP
  // 1 = UDP
  Serial.println(conn_mode);

  timer_wakeup = millis();
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
