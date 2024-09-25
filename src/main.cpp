#include <Arduino.h>
#include <EspUsbHost.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"

String get_value(String json, String key);
String get_last_setup();
void reset_setup();

// Wi-Fi credentials
const char *ssid = "esp8266Tester-1";
const char *password = "makanbang";

const char *udpAddress = "192.168.15.98";
const int udpPort = 5555;

#define CPU_0 0
#define CPU_1 1

#define PRIORITY_REALTIME 6
#define PRIORITY_VERY_HIGH 5
#define PRIORITY_HIGH 4
#define PRIORITY_NORMAL 3
#define PRIORITY_LOW 2
#define PRIORITY_VERY_LOW 1
#define PRIORITY_IDLE 0

TaskHandle_t TASK_KEYBOARD;
TaskHandle_t TASK_UDP;
TaskHandle_t TASK_WMANAGER;
TaskHandle_t TASK_BUTTON;

String keyboardBuffer = "";
byte ready_to_send = 0;

class MyEspUsbHost : public EspUsbHost
{
  void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier)
  {
    if (' ' <= ascii && ascii <= '~')
    {
      keyboardBuffer += String((char)ascii);
    }
    else if (ascii == '\r' || ascii == '\n')
    {
      ready_to_send = 1;
    }
  };
};

MyEspUsbHost usbHost;
WiFiUDP udp;
WebServer server(80);

void keyboardHandler(void *param)
{
  Serial.begin(115200);
  usbHost.begin();
  usbHost.setHIDLocal(HID_LOCAL_US);
  while (1)
  {
    usbHost.task();
    esp_task_wdt_reset();
  }
}

void reset_setup()
{
  File f = SPIFFS.open("/default_setup.json", "r");
  String default_config = "";
  while (f.available())
  {
    default_config += String((char)f.read());
  }
  f.close();
  f = SPIFFS.open("/hw_config.json", "w");
  for (uint32_t i = 0; i < default_config.length(); i++)
  {
    f.write((uint8_t)default_config[i]);
  }
  f.close();
}

void init_network()
{
  String last_config = get_last_setup();
  IPAddress local_IP(get_value(last_config, "static_ip_1").toInt(),
                     get_value(last_config, "static_ip_2").toInt(),
                     get_value(last_config, "static_ip_3").toInt(),
                     get_value(last_config, "static_ip_4").toInt()); // static IP
  IPAddress gateway(get_value(last_config, "gateway_1").toInt(),
                    get_value(last_config, "gateway_2").toInt(),
                    get_value(last_config, "gateway_3").toInt(),
                    get_value(last_config, "gateway_4").toInt()); // gateway
  IPAddress subnet(get_value(last_config, "subnet_1").toInt(),
                   get_value(last_config, "subnet_2").toInt(),
                   get_value(last_config, "subnet_3").toInt(),
                   get_value(last_config, "subnet_4").toInt()); // subnet
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }
  // Connect to Wi-Fi

  String config = get_last_setup();
  String router_ssid = get_value(config, "SSID_client");
  String router_password = get_value(config, "password");

  WiFi.begin(router_ssid.c_str(), router_password.c_str());
  // WiFi.begin(ssid, password);
  Serial.print("Connecting to SSID : ");
  Serial.println(router_ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Start UDP connection
  udp.begin(udpPort);
  Serial.println("UDP Client started.");
}

String get_last_setup()
{
  File f = SPIFFS.open("/hw_config.json", "r");
  String last_config = "";
  while (f.available())
  {
    last_config += String((char)f.read());
  }
  f.close();
  return last_config;
}

void UDP_client(void *param)
{
  init_network();
  while (1)
  {
    // Send a message to the UDP server
    if (ready_to_send == 1)
    {
      keyboardBuffer += "\r\n";                      // add end line char to the UDP packet (optional)
      uint8_t buffer_kirim[keyboardBuffer.length()]; // array buffer for UDP packets
      // copy keyboardBuffer for UDP packets buffer array
      for (uint32_t i = 0; i < keyboardBuffer.length(); i++)
      {
        buffer_kirim[i] = keyboardBuffer[i];
      }
      udp.beginPacket(udpAddress, udpPort);
      udp.write(buffer_kirim, keyboardBuffer.length());
      udp.endPacket();
      Serial.println(keyboardBuffer);
      keyboardBuffer = ""; // reset keyboard buffer
      ready_to_send = 0;   // reset buffer ready status
    }
    esp_task_wdt_reset(); // reset Watchdog timer
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void logoHandler()
{
  File f = SPIFFS.open("/logo.png", "r");
  server.streamFile(f, "image/png");
  f.close();
}
void setupHandler()
{
  File f = SPIFFS.open("/setup.html", "r");
  server.streamFile(f, "text/html");
  f.close();
}

void syncHandler()
{
  File f = SPIFFS.open("/hw_config.json", "r");
  server.streamFile(f, "application/json");
  f.close();
}

void setSetupHandler()
{
  if (server.hasArg("plain") == false)
  {
    // If no JSON payload, send 400 Bad Request
    server.send(400, "application/json", "{\"message\":\"No payload received\"}");
    return;
  }

  String jsonPayload = server.arg("plain"); // Get the POST body content
  Serial.println("Received JSON: ");
  Serial.println(jsonPayload);

  // Parse the JSON payload
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonPayload);

  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"message\":\"Invalid JSON format\"}");
    return;
  }

  // Extract values from the JSON object
  const char *static_IP = doc["static_IP"];
  const char *subnet = doc["subnet"];
  const char *gateway = doc["gateway"];
  const char *udp_server_address = doc["udp_server_address"];
  const char *udp_server_port = doc["udp_server_port"];
  const char *max_udp_buffer = doc["max_udp_buffer"];
  const char *SSID_client = doc["SSID_client"];
  const char *password_client = doc["password_client"];
  const char *device_name = doc["device_name"];
  const char *wifi_manager_password = doc["wifi_manager_password"];

  String last_config = get_last_setup();
  String last_SN = get_value(last_config, "sn");

  // Create a new JSON object with the received data
  StaticJsonDocument<300> newConfig;
  newConfig["sn"] = last_SN;
  newConfig["name"] = device_name;
  newConfig["password"] = password_client;
  newConfig["wifi_manager_password"] = wifi_manager_password;
  newConfig["SSID_client"] = SSID_client;

  int static_ip_1, static_ip_2, static_ip_3, static_ip_4;
  int subnet_1, subnet_2, subnet_3, subnet_4;
  int gateway_1, gateway_2, gateway_3, gateway_4;
  // Parse static IP into four parts and store
  sscanf(static_IP, "%d.%d.%d.%d", &static_ip_1, &static_ip_2, &static_ip_3, &static_ip_4);

  // Parse subnet into four parts and store
  sscanf(subnet, "%d.%d.%d.%d", &subnet_1, &subnet_2, &subnet_3, &subnet_4);

  // Parse gateway into four parts and store
  sscanf(gateway, "%d.%d.%d.%d", &gateway_1, &gateway_2, &gateway_3, &gateway_4);

  // Assign values to the JSON object
  newConfig["static_ip_1"] = static_ip_1;
  newConfig["static_ip_2"] = static_ip_2;
  newConfig["static_ip_3"] = static_ip_3;
  newConfig["static_ip_4"] = static_ip_4;

  newConfig["subnet_1"] = subnet_1;
  newConfig["subnet_2"] = subnet_2;
  newConfig["subnet_3"] = subnet_3;
  newConfig["subnet_4"] = subnet_4;

  newConfig["gateway_1"] = gateway_1;
  newConfig["gateway_2"] = gateway_2;
  newConfig["gateway_3"] = gateway_3;
  newConfig["gateway_4"] = gateway_4;

  // Add remaining configuration fields
  newConfig["udp_ip_address"] = udp_server_address;
  newConfig["udp_port"] = udp_server_port;
  newConfig["max_udp_buffer"] = max_udp_buffer;

  // Open the "hw_config.js" file for writing on SPIFFS
  File configFile = SPIFFS.open("/hw_config.json", "w");
  String new_config_value;
  serializeJson(newConfig, new_config_value);
  for (uint32_t i = 0; i < new_config_value.length(); i++)
  {
    configFile.write((uint8_t)new_config_value[i]);
  }
  configFile.close();

  Serial.print("================");
  Serial.print(new_config_value);
  Serial.println("================");
  // Send a success response
  server.send(200, "application/json", "{\"status\":\"success update config, target going to Reboot\",\"data\":" + new_config_value + "}");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP.restart();
}

String get_value(String json, String key)
{
  int keyIndex = json.indexOf('"' + key + '"');
  if (keyIndex == -1)
    return "";

  int colonIndex = json.indexOf(':', keyIndex);
  int startQuoteIndex = json.indexOf('"', colonIndex);
  int endQuoteIndex = json.indexOf('"', startQuoteIndex + 1);

  if (startQuoteIndex == -1 || endQuoteIndex == -1)
    return "";

  return json.substring(startQuoteIndex + 1, endQuoteIndex);
}

void WiFi_manager(void *param)
{
  String last_config = get_last_setup();
  String ssid_wifi_manager = get_value(last_config, "sn") + "-" + get_value(last_config, "name");
  String password_wifi_manager = get_value(last_config, "wifi_manager_password");

  WiFi.softAP(ssid_wifi_manager.c_str(), password_wifi_manager.c_str());
  Serial.print("softAP begin : ");
  Serial.println(ssid_wifi_manager);
  server.on("/", setupHandler);
  server.on("/home", setupHandler);
  server.on("/setup", setupHandler);
  server.on("/logo", logoHandler);
  server.on("/sync_setup", syncHandler);
  server.on("/update_setup", HTTP_POST, setSetupHandler);
  server.enableCORS();
  server.begin();
  while (1)
  {
    server.handleClient();
    vTaskDelay(1 / portTICK_PERIOD_MS);
    esp_task_wdt_reset();
  }
}

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
  /////////////////// SPIFFS init///////////////////
  if (!SPIFFS.begin(true))
  {
    return;
  }
  //////////////TASK Action Button///////////////////
  xTaskCreatePinnedToCore(action_button, "Action Button", 4096, NULL, PRIORITY_HIGH, &TASK_BUTTON, CPU_0);
  //////////////TASK KEYBOARD///////////////////
  xTaskCreatePinnedToCore(keyboardHandler, "keyboard task", 2048, NULL, PRIORITY_REALTIME, &TASK_KEYBOARD, CPU_0);
  //////////////TASK UDP///////////////////
  xTaskCreatePinnedToCore(UDP_client, "UDP task", 8192, NULL, PRIORITY_NORMAL, &TASK_UDP, CPU_1);
  //////////////TASK UDP///////////////////
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(WiFi_manager, "WiFi Manager", 8192, NULL, PRIORITY_VERY_HIGH, &TASK_WMANAGER, CPU_1);
}

void loop()
{
}
