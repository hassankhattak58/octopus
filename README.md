# Octopus

Octopus is an open-source ESP32-S3 laboratory keyboard device. It runs bounded DuckyScript-style text files over native USB HID or BLE HID after physical button confirmation. The firmware uses ESP-IDF v5.5.3. Connecting USB or pairing BLE never starts a script.

## Hardware and use

Use an ESP32-S3 board with native USB and a button from **GPIO 4 to GND** (internal pull-up enabled). The pin can be changed in `idf.py menuconfig`. On the two-socket board, **COM** is for flashing and serial logs; **USB** is the HID keyboard connection. The BLE pairing name is **Octopus Kbd**. Test only on a computer you own or may use.

Install [ESP-IDF v5.5.3](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32s3/get-started/index.html), then in its terminal run:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COM3 flash monitor
```

Replace `COM3` with your serial port. The build creates a LittleFS image from [scripts](scripts); `flash` installs both the app and scripts. The [Firmware workflow](.github/workflows/firmware.yml) runs parser tests and builds `octopus-merged.bin`. Flash that artifact at offset zero with `python -m esptool --chip esp32s3 --port COM3 write-flash 0x0 octopus-merged.bin`.

Serial output lists `.txt` files and the selected script. **Short press** cycles selection. **Hold at least 800 ms and release** to validate and execute. **Press during execution** to cancel immediately; the engine releases keys when it stops. USB takes priority when ready; otherwise a paired BLE host is used. No host means no run. The initial default is `notepad.txt` for USB and `hello.txt` for BLE until you select a file. One script runs at a time. The Notepad example is Windows-specific and creates an untitled document containing “Welcome, Octupus Here!”.

### Update scripts

Edit plain-text files in [scripts](scripts), then rebuild and flash. The Windows automation remains available: edit [payload.txt](payload.txt), then run `.\deploy-demo.ps1 -Port COM3`. It copies that file to `scripts/notepad.txt`, commits and pushes both files, waits for CI, downloads the merged image, checks the chip, and flashes it. `-ValidateOnly` checks size and NUL bytes; `-GenerateOnly` only copies the file. The firmware validates commands before execution. This public repository must not contain secrets.

## Supported DuckyScript-style commands

| Syntax | Action |
|---|---|
| `REM text` | Comment |
| `STRING text`, `STRINGLN text` | Type text; `STRINGLN` adds Enter |
| `DELAY ms` | Pause 0–10000 ms |
| `DEFAULT_DELAY ms`, `DEFAULTDELAY ms` | Pause after later text/key actions |
| `ENTER`, `RETURN`, `TAB`, `SPACE`, `BACKSPACE`, `ESC`, `ESCAPE`, `DELETE` | Single key |
| `UP`, `DOWN`, `LEFT`, `RIGHT` (optionally `ARROW` suffix) | Arrow key |
| `HOME`, `END`, `F1`–`F9` | Single key |
| `CTRL`, `CONTROL`, `SHIFT`, `ALT`, `GUI`, `WIN`, `WINDOWS` plus one key | Combination, e.g. `GUI r`, `CTRL ALT DELETE` |

Commands are case-insensitive. Text uses a US keyboard layout and printable US ASCII; unsupported characters and commands fail validation with a line number. Limits: 8192 bytes per file, 256 bytes per line, 128 actions, 10000 ms per delay, and 60 seconds estimated runtime. The engine checks cancellation at least every 10 ms during delays and between key reports. This is a **subset**, not complete Hak5 DuckyScript 3. Variables, loops, conditionals, `ATTACKMODE`, non-US layouts, and host-side features are unsupported. The board has no display; selection and errors appear in serial logs.

## Architecture and library choice

`ducky_script` parses and validates without ESP-IDF dependencies. `script_storage` mounts LittleFS and lists/selects files. `script_engine` executes through `keyboard_transport`, implemented by the existing USB and BLE paths. `button_controller` owns the physical control; `device_status` reports state. A future mobile app can select a file through a separately authenticated control service while retaining local confirmation.

| Option | ESP32-S3 transport / language | Memory and dependencies | License, maintenance, and fit |
|---|---|---|---|
| [Espressif esp_tinyusb](https://components.espressif.com/components/espressif/esp_tinyusb) + [esp_hid/NimBLE](https://github.com/espressif/esp-idf/tree/v5.5.3/examples/bluetooth/esp_hid_device) | Native USB + BLE; no script parser | Already present; NimBLE is the lighter BLE host | Espressif maintained; selected to preserve working paths |
| [Arduino ESP32 USB HID](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb.html) | Native USB; no parser | Adds Arduino core | Maintained, but requires framework migration |
| [Adafruit TinyUSB Arduino](https://github.com/adafruit/Adafruit_TinyUSB_Arduino) | Native USB; no parser | Adds Arduino and Adafruit wrapper | Active, redundant with current TinyUSB |
| [T-vK ESP32-BLE-Keyboard](https://github.com/T-vK/ESP32-BLE-Keyboard) | BLE only; no parser | Arduino BLE/NimBLE | Open source, but would replace working IDF BLE path |
| [s3-ducky](https://github.com/kanishka-sahoo/s3-ducky) | USB firmware; DuckyScript 1 and some 3 features per project | Whole firmware and writable drive | Open source, but not a small dual-transport component |

The comparison is qualitative; no cross-library RAM or flash benchmark was run. [Hak5’s payload repository](https://github.com/hak5/usbrubberducky-payloads) is a payload collection, not a drop-in ESP32-S3 interpreter. [joltwallet/littlefs](https://components.espressif.com/components/joltwallet/littlefs) provides script storage. See [NOTICE.md](NOTICE.md) for licenses and attribution. Parser tests run in CI before the ESP-IDF build; hardware behavior after these changes requires verification on the board.
