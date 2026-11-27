# Third-party attribution

`main/esp_hid_device_main.c`, `main/esp_hid_gap.c`, `main/esp_hid_gap.h`, `main/CMakeLists.txt`, and `main/Kconfig.projbuild` were adapted from Espressif Systems' ESP-IDF `examples/bluetooth/esp_hid_device` at tag `v5.5.3`:

https://github.com/espressif/esp-idf/tree/v5.5.3/examples/bluetooth/esp_hid_device

The upstream example states `SPDX-License-Identifier: Unlicense OR CC0-1.0`. Octopus changes include the GPIO trigger, fixed test text, BLE name, keyboard report count, and project configuration. ESP-IDF remains an external toolchain and dependency; it is not included in this repository.

The USB HID implementation in `main/usb_hid.c` follows the APIs and descriptor pattern from Espressif's ESP-IDF `examples/peripherals/usb/device/tusb_hid` at tag `v5.5.3`:

https://github.com/espressif/esp-idf/tree/v5.5.3/examples/peripherals/usb/device/tusb_hid

The USB stack is provided by `espressif/esp_tinyusb` through the ESP-IDF Component Manager.
