# Roadmap

## Milestone 1 — BLE HID keyboard (current)

Use ESP-IDF v5.5.3, NimBLE, and Espressif `esp_hid`. Prove advertising, pairing, connection, key press and release reports, disconnect/reconnect, and the GPIO trigger on one known ESP32-S3 board. Record the exact board model and pinout before changing firmware. Acceptance: one button press types exactly one `Octopus ready` line in a text editor on a paired test computer; reconnect and repeat.

## Milestone 2 — local action engine

Define a small, versioned action format for text, key combinations, and delays. Keep input validation and limits in firmware: maximum action count, maximum duration, key release on cancellation, and a visible armed state. Keep transport code separate from the action engine so the same actions can later use USB HID. Add tests for parser bounds and cancellation. Acceptance: a saved benign demo runs once only after local arming and an explicit trigger.

## Milestone 3 — mobile control

Choose Android first for a working prototype, then assess iOS support. Add a separate BLE GATT control service alongside HID. Define commands for status, arm/disarm, select a saved demo, run, cancel, and result reporting. Require authenticated pairing/bonding, reject unauthenticated commands, and require a local button to arm the device. Keep the phone UI explicit about the target and upcoming action. Acceptance: phone can choose and trigger a saved demo while the paired test computer receives only the intended keys.

## Milestone 4 — reliability and release

Test multiple hosts and keyboard layouts, reconnects, power cycles, canceled sequences, and long presses. Document board variants, firmware flashing, app setup, licensing, and known limits. Add CI builds once the local ESP-IDF build is confirmed.

## Future modules

Treat Wi-Fi functions as separate, opt-in modules after the HID milestone. A nearby network scan can report SSID, channel, and signal strength; it does **not** reveal a Wi-Fi password or produce a password file. Any authorized wireless security research will need its own feasibility check, threat model, and hardware review before implementation. Keep it out of the first firmware and mobile protocol.

## Why this stack

ESP-IDF supplies the radio stack, `esp_hid`, board support, and build system. Its maintained [HID device example](https://github.com/espressif/esp-idf/tree/v5.5.3/examples/bluetooth/esp_hid_device) already supports ESP32-S3 and a BLE keyboard role. NimBLE keeps the firmware focused on BLE, which is the radio mode the ESP32-S3 offers. The later USB path can use Espressif's [USB HID support](https://docs.espressif.com/projects/esp-iot-solution/en/latest/usb/usb_overview/usb_device_solutions.html) without changing the action format.
