#ifndef oled_h
#define oled_h
#include "global.h"
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SCL_1 39
#define SDA_1 40

#define SCL_2 41
#define SDA_2 42

#define FONT_SIZE 2

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

uint8_t last_message = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cone);

unsigned int sortAndReturnBiggest(unsigned int array[])
{
    int n = 3; // Number of elements in the array

    // Simple Bubble Sort for 3 elements
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                // Swap array[j] and array[j + 1]
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }

    return array[2]; // Return the biggest element (last element in the sorted array)
}

String adjust_text_to_screen(String text)
{
    uint16_t text_len = text.length();
    uint16_t text_capacity = SCREEN_WIDTH / (6 * FONT_SIZE);
    uint16_t delta_size = (text_capacity - (text_len % text_capacity)) + (text_capacity / 2);
    String tmp = text;
    Serial.print("Capacity:");
    Serial.println(text_capacity);
    for (uint16_t i = 0; i < delta_size; i++)
    {
        tmp += " ";
    }
    return tmp;
}

String adjust_text_to_another(uint16_t longest, String text)
{
    uint16_t delta_size = longest - text.length();
    String tmp = text;
    for (uint16_t i = 0; i < delta_size; i++)
    {
        tmp += " ";
    }
    return tmp;
}

void running_text(byte y, String text1, String text2, String text3, uint32_t speed)
{
    unsigned int arr[3] = {text1.length(), text2.length(), text3.length()};
    unsigned longest = sortAndReturnBiggest(arr);
    text1 = adjust_text_to_another(longest, text1);
    text2 = adjust_text_to_another(longest, text2);
    text3 = adjust_text_to_another(longest, text3);

    text1 = adjust_text_to_screen(text1);
    text2 = adjust_text_to_screen(text2);
    text3 = adjust_text_to_screen(text3);

    Serial.print("t1:");
    Serial.println(text1.length());
    Serial.print("t2:");
    Serial.println(text2.length());
    Serial.print("t3:");
    Serial.println(text3.length());

    for (uint16_t i = 0; i < text1.length(); i++)
    {
        if (last_message != message_info_index)
        {
            last_message = message_info_index;
            display.clearDisplay();
            break;
        }

        if (reset_cnt >= 10)
        {
            break;
        }
        if (wakeup == 1)
        {
            display.setTextColor(WHITE, BLACK);
        }
        else
        {
            display.setTextColor(BLACK, BLACK);
        }
        display.setCursor(SCREEN_WIDTH - i * 6 * FONT_SIZE, y);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text1[j]);
        }
        display.setCursor(SCREEN_WIDTH - i * 6 * FONT_SIZE, y + 16);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text2[j]);
        }
        display.setCursor(SCREEN_WIDTH - i * 6 * FONT_SIZE, y + 32);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text3[j]);
        }
        display.display();
        vTaskDelay((200 - (speed * 2)) / portTICK_PERIOD_MS);
        display.setTextColor(BLACK, BLACK);
        display.setCursor(SCREEN_WIDTH - i * 6 * FONT_SIZE, y);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text1[j]);
        }
        display.setCursor(SCREEN_WIDTH - i * 6 * FONT_SIZE, y + 16);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text2[j]);
        }
        display.setCursor(SCREEN_WIDTH - i * 6 * FONT_SIZE, y + 32);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text3[j]);
        }
        display.display();
    }
}

void draw_header(byte x, byte y, String text)
{
    if (wakeup == 1)
    {
        display.setTextColor(WHITE, BLACK);
    }
    else
    {
        display.setTextColor(BLACK, BLACK);
    }
    display.setCursor(x, y);
    display.print(text);
    display.display();
}

void i2cScan(TwoWire &i2cBus)
{
    Serial.print("I2C device scanner");
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++)
    {
        Serial.println(address, HEX);
        // Send a transmission to this address
        i2cBus.beginTransmission(address);
        error = i2cBus.endTransmission();

        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");

            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("I2C scan complete\n");
}

void display_oled_loop(void *param)
{
    I2Cone.begin(SCL_1, SDA_1, 400000);
    I2Ctwo.begin(SDA_2, SCL_2, 400000);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c))
    {
        Serial.println("SSD1306 allocation failed");
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    display.clearDisplay();
    display.setTextSize(FONT_SIZE);
    display.setTextColor(WHITE);

    display.clearDisplay();
    // Display the bitmap image on the OLED screen
    display.drawBitmap(0, 0, logo_oled, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    // Show the buffer on the display
    display.display();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    display.clearDisplay();

    while (1)
    {
        if (millis() - timer_wakeup > SCREEN_WAKEUP_TIMEOUT_MS)
        {
            wakeup = 0;
        }
        if (last_message != message_info_index && message_info_index != MENU_GOING_RESET)
        {
            last_message = message_info_index;
            display.clearDisplay();
        }

        String saved_config = get_last_setup();

        switch (message_info_index)
        {
        case MENU_SCANNER:
            draw_header(0, 0, "SCANNER");
            running_text(16,
                         "READING:" + keyboardBuffer,
                         "LAST SEND:" + last_send_scanned,
                         "",
                         70);
            break;
        case MENU_UDP:
            if (EEPROM.read(0) == 1)
            {
                draw_header(0, 0, "UDP SERVER");
            }
            else
            {
                draw_header(0, 0, "TCP SERVER");
            }
            running_text(16,
                         "ADDR:" + get_value(saved_config, "udp_ip_address"),
                         "PORT:" + get_value(saved_config, "udp_port"),
                         "",
                         70);
            break;
        case MENU_WIFI_HOTSPOT:
            draw_header(0, 0, "WiFi(AP)");
            running_text(16,
                         "SSID:" + get_value(saved_config, "sn") + "-" + get_value(saved_config, "name"),
                         "PASS:" + get_value(saved_config, "wifi_manager_password"),
                         "",
                         70);
            break;
        case MENU_WIFI_CLIENT:
            draw_header(0, 0, "WiFi(CLI)");
            running_text(16,
                         "SSID:" + get_value(saved_config, "SSID_client"),
                         "PASS:" + get_value(saved_config, "password"),
                         "",
                         70);
            break;
        case MENU_DEVICE_IP:
            Serial.println(WiFi.localIP().toString());
            draw_header(0, 0, "IP ADDRESS");
            if (EEPROM.read(0) == 1)
            {
                running_text(16,
                             WiFi.localIP().toString(),
                             "MODE : UDP",
                             "",
                             70);
            }
            else
            {
                running_text(16,
                             WiFi.localIP().toString(),
                             "MODE : TCP",
                             "",
                             70);
            }
            break;
        case MENU_GOING_RESET:
            if (wakeup == 1)
            {
                display.setTextColor(WHITE, BLACK);
            }
            else
            {
                display.setTextColor(BLACK, BLACK);
            }
            display.setCursor(0, 0);
            display.print("RESET!!");
            display.setCursor(0, 16);
            display.print("HOLD...");
            display.setCursor(0, 32);

            display.setTextColor(BLACK, BLACK);
            display.setCursor(48, 48);
            display.print("    ");
            display.display();

            if (wakeup == 1)
            {
                display.setTextColor(WHITE, BLACK);
            }
            else
            {
                display.setTextColor(BLACK, BLACK);
            }
            display.setCursor(48, 48);
            display.print(String(reset_cnt) + "%");

            for (uint8_t i = 0; i < map(reset_cnt, 0, 100, 0, 128); i++)
            {
                if (i <= 35 || i > 88)
                {
                    display.setCursor(i, 48);
                    display.print("=");
                }
            }
            display.display();
            break;
        default:
            draw_header(0, 0, "SCANNER");
            running_text(16,
                         "READING:" + keyboardBuffer,
                         "LAST SEND:" + last_send_scanned,
                         "",
                         70);
            break;
        }
    }
}
#endif