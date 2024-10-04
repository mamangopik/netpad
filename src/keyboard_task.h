#ifndef keyboard_task_h
#define keyboard_task_h
#include "global.h"

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

void keyboardHandler(void *param)
{
    usbHost.begin();
    usbHost.setHIDLocal(HID_LOCAL_US);
    while (1)
    {
        usbHost.task();
        esp_task_wdt_reset();
    }
}

#endif