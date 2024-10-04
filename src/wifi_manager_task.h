#ifndef wifi_manager_task_h
#define wifi_manager_task_h

#include "global.h"
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

#endif