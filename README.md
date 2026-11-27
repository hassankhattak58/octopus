# Octopus

Open source ESP32-S3 multipurpose device project. The first milestone is a Bluetooth Low Energy (BLE) HID keyboard that sends a short test message after a physical button press. A mobile app and other modules will be designed after the BLE keyboard works reliably on real hardware.

## Current state

- ESP-IDF firmware based on [Espressif's HID device example](https://github.com/espressif/esp-idf/tree/v5.5.3/examples/bluetooth/esp_hid_device), pinned to ESP-IDF **v5.5.3**.
- BLE keyboard profile using Espressif's `esp_hid` component and NimBLE. ESP32-S3 supports BLE, but not Bluetooth Classic.
- One press of a button wired between **GPIO 4 and GND** types `Octopus ready` followed by Enter into a paired computer's focused text field. The pin is configurable in `idf.py menuconfig` under **HID Example Configuration**.
- No mobile control, script interpreter, Wi-Fi capture, or USB HID yet.

## Hardware

An ESP32-S3 development board, USB data cable, and a momentary push button are enough for the first milestone. Connect one button terminal to GPIO 4 and the other to GND. Check your board's pinout first; if GPIO 4 is unavailable, choose another free input pin in menuconfig. The internal pull-up is enabled. Use a computer you own or have permission to test, and open a text editor before pressing the button.

## Build and test

1. Install [ESP-IDF v5.5.3 and its toolchain](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32s3/get-started/index.html). On Windows, use the ESP-IDF PowerShell or Command Prompt shortcut so `idf.py` is available.
2. Clone this repository and run:

   ```powershell
   idf.py set-target esp32s3
   idf.py build
   idf.py -p COMx flash monitor
   ```

   Replace `COMx` with your board's serial port. On Linux or macOS, use the corresponding `/dev/...` port.
3. Pair **Octopus Keyboard** from your test computer's Bluetooth settings. Put the cursor in a text editor. Press and release the wired button once. Expect `Octopus ready` and a newline. The serial monitor also logs the send event.

The keyboard mapping currently assumes a US English host layout. If you change the HID report map or device name, remove the old Bluetooth pairing and pair again so the host refreshes its cached descriptor. This first firmware has not been built or tested on physical hardware in this workspace; the toolchain and board are not present here.

## Plan

See [docs/roadmap.md](docs/roadmap.md) for the staged implementation and acceptance checks. The core design is a device service with separate HID transport, input/action engine, and later phone control. The phone app should select and confirm actions; the firmware should require an authenticated connection and local arming before sending any HID reports.

## Open source and attribution

Original Octopus material is MIT licensed; see [LICENSE](LICENSE). The files in `main/` derived from Espressif's ESP-IDF HID example retain their `SPDX-License-Identifier: Unlicense OR CC0-1.0` headers. See [NOTICE.md](NOTICE.md) for provenance. No third-party source is vendored beyond the adapted Espressif example.

Contributions are welcome through issues and pull requests. Please include your board model, ESP-IDF version, logs, and reproduction steps in bug reports. Test HID behavior only on devices you own or are authorized to use.
