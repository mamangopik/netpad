#ifndef global_h
#define global_h
#include <Arduino.h>
#include <EEPROM.h>
#include <EspUsbHost.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define CPU_0 0
#define CPU_1 1

#define SCREEN_WAKEUP_TIMEOUT_MS 60000

#define PRIORITY_REALTIME 6
#define PRIORITY_VERY_HIGH 5
#define PRIORITY_HIGH 4
#define PRIORITY_NORMAL 3
#define PRIORITY_LOW 2
#define PRIORITY_VERY_LOW 1
#define PRIORITY_IDLE 0

#define MENU_UDP 0
#define MENU_WIFI_CLIENT 1
#define MENU_WIFI_HOTSPOT 2
#define MENU_SCANNER 3
#define MENU_DEVICE_IP 4
#define MENU_CHOICE_END 5
#define MENU_GOING_RESET 100 // MUST BE greater than MENU_CHOICE_END

// Wi-Fi credentials
const char *ssid = "esp8266Tester-1";
const char *password = "makanbang";

const char *udpAddress = "192.168.15.98";
const int udpPort = 5555;
String keyboardBuffer = "";
String last_send_scanned;
byte ready_to_send = 0;
byte wakeup = 1;
byte conn_mode;
byte message_info_index = 0;
byte going_to_reset = 0;
uint32_t reset_cnt = 0;
unsigned long timer_wakeup = 0;

WebServer server(80);
WiFiUDP udp;

void WiFi_manager(void *param);
void reset_setup();
void logoHandler();
void setupHandler();
void syncHandler();
void setSetupHandler();
String get_last_setup();
String get_value(String json, String key);

void keyboardHandler(void *param);

void init_network();
void UDP_client(void *param);

#include "wifi_manager_task.h"
#include "keyboard_task.h"
#include "udp_tcp_task.h"
#include "oled.h"

#endif