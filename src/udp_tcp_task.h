#ifndef udp_tcp_task_h
#define udp_tcp_task_h

#include "global.h"

String config = get_last_setup();
String router_ssid = get_value(config, "SSID_client");
String router_password = get_value(config, "password");
String saved_udp_port = get_value(config, "udp_port");
String saved_udp_host = get_value(config, "udp_ip_address");

void init_network()
{
    config = get_last_setup();
    router_ssid = get_value(config, "SSID_client");
    router_password = get_value(config, "password");
    saved_udp_port = get_value(config, "udp_port");
    saved_udp_host = get_value(config, "udp_ip_address");

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
    udp.begin(saved_udp_port.toInt());
    Serial.println("UDP Client started.");
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
            udp.beginPacket(saved_udp_host.c_str(), saved_udp_port.toInt());
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
#endif