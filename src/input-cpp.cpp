#include "input.h"
#include <wili8jam/usb-host/fwUSBHost.h>

extern fwUSBHost obUSBHost;

extern "C" void spry_initInput() {
    // Register keyboard callback for input state tracking
    obUSBHost.m_obHID.getKeyboard().setKeyCallback(input_key_callback);

    // Register modifier key polling so Ctrl/Shift/Alt state is synced each frame
    input_set_modifier_poll([]() -> uint8_t {
        return obUSBHost.m_obHID.getKeyboard().getModifiers();
    });
}
