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
    uint16_t delta_size = (text_capacity - (text_len % text_capacity)) + text_capacity;
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

    for (uint16_t i = 0; i < text1.length() * 6 * FONT_SIZE; i++)
    {
        display.setTextColor(WHITE, BLACK);
        display.setCursor(SCREEN_WIDTH - i, y);
        for (uint16_t j = 0; j < i / (6 * FONT_SIZE); j++)
        {
            display.print(text1[j]);
        }
        display.setCursor(SCREEN_WIDTH - i, y + 16);
        for (uint16_t j = 0; j < i / (6 * FONT_SIZE); j++)
        {
            display.print(text2[j]);
        }
        display.setCursor(SCREEN_WIDTH - i, y + 32);
        for (uint16_t j = 0; j < i / (6 * FONT_SIZE); j++)
        {
            display.print(text3[j]);
        }
        display.display();
        vTaskDelay((200 - (speed * 2)) / portTICK_PERIOD_MS);
        display.setTextColor(BLACK, BLACK);
        display.setCursor(SCREEN_WIDTH - i, y);
        for (uint16_t j = 0; j < i / (6 * FONT_SIZE); j++)
        {
            display.print(text1[j]);
        }
        display.setCursor(SCREEN_WIDTH - i, y + 16);
        for (uint16_t j = 0; j < i / (6 * FONT_SIZE); j++)
        {
            display.print(text2[j]);
        }
        display.setCursor(SCREEN_WIDTH - i, y + 32);
        for (uint16_t j = 0; j < i / (6 * FONT_SIZE); j++)
        {
            display.print(text3[j]);
        }
        display.display();
    }
}

void draw_header(byte x, byte y, String text)
{
    Serial.println("headprepare");
    display.setCursor(x, y);
    display.print(text);
    display.display();
    Serial.println("headfinish");
}
void scroll_text(byte y, String text1, uint32_t speed)
{
    /*
    Cari lebar layar
    cari kapasitas text layar
    buat panjang ukuran text menjadi seukuran layar
    agar dapat discroll sampai habis dengan cara menambhakan dengan "spasi"
    sebanyak sisa kapasitas text pada layar
    */
    uint16_t text_len = text1.length();
    uint16_t text_capacity = SCREEN_WIDTH / (6 * FONT_SIZE);
    uint16_t delta_size = text_capacity - (text_capacity % text_len);
    for (uint16_t i = 0; i < delta_size; i++)
    {
        text1 += " ";
    }

    // scroll text
    for (uint16_t i = 0; i < text1.length(); i += 2)
    {
        display.setCursor(SCREEN_WIDTH - (0 + (i * 6 * FONT_SIZE)), y);
        display.setTextColor(WHITE, BLACK);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text1[j]);
        }
        display.display();
        vTaskDelay((200 - (speed * 2)) / portTICK_PERIOD_MS);
        display.setCursor(SCREEN_WIDTH - (0 + (i * 6 * FONT_SIZE)), y);
        display.setTextColor(BLACK, BLACK);
        for (uint16_t j = 0; j < i; j++)
        {
            display.print(text1[j]);
        }
        display.display();
    }
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
    I2Cone.begin(SCL_1, SDA_1);
    I2Ctwo.begin(SDA_2, SCL_2);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c))
    {
        Serial.println("SSD1306 allocation failed");
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
    display.clearDisplay();
    display.setTextSize(FONT_SIZE);
    display.setTextColor(WHITE);
    uint8_t last_message = 0;
    while (1)
    {
        if (last_message != message_info_index)
        {
            last_message = message_info_index;
            display.clearDisplay();
        }

        String saved_config = get_last_setup();
        draw_header(10, 0, "UDP:");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        running_text(16, "ADDR:" + get_value(saved_config, "udp_ip_address"), "PORT:" + get_value(saved_config, "udp_port"), "", 95);
        // switch (message_info_index)
        // {
        // case 0:
        //     draw_header(10, 0, "SCANNER:");
        //     running_text(0, keyboardBuffer, 60);
        //     break;
        // case 1:
        //     draw_header(10, 0, "UDP:");
        //     running_text(14, "ADDR:" + get_value(saved_config, "udp_ip_address") + "\nPORT:" + get_value(saved_config, "udp_port"), 60);
        //     break;
        // default:
        //     draw_header(10, 0, "SCANNER:");
        //     running_text(0, keyboardBuffer, 60);
        //     break;
        // }
    }
}
#endif