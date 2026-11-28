#pragma once

#include <stdbool.h>
#include "keyboard_transport.h"

void octopus_usb_hid_init(void);
bool octopus_usb_hid_ready(void);
const octopus_keyboard_transport_t *octopus_usb_transport(void);
