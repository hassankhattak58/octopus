# Octopus

Open source ESP32-S3 multipurpose device project. The first milestone is a button-triggered HID keyboard over Bluetooth Low Energy (BLE) and native USB. A mobile app and other modules will follow after the HID paths are reliable on real hardware.

## Current state

- ESP-IDF firmware based on [Espressif's HID device example](https://github.com/espressif/esp-idf/tree/v5.5.3/examples/bluetooth/esp_hid_device), pinned to ESP-IDF **v5.5.3**.
- BLE keyboard profile using Espressif's `esp_hid` component and NimBLE. ESP32-S3 supports BLE, but not Bluetooth Classic.
- One press of a button wired between **GPIO 4 and GND** uses the active HID transport. With native USB connected to a Windows PC, it opens an untitled Notepad window and types the final `STRING` from [payload.txt](payload.txt). Without native USB, it types `Octopus ready` and Enter over BLE to the paired computer. The pin is configurable in `idf.py menuconfig` under **HID Example Configuration**.
- No mobile control, script interpreter, or Wi-Fi capture yet.

## Hardware

An ESP32-S3 development board with a native USB socket, USB data cable, and a momentary push button are enough for both transports. Connect one button terminal to GPIO 4 and the other to GND. Check your board's pinout first; if GPIO 4 is unavailable, choose another free input pin in menuconfig. The internal pull-up is enabled. On boards with two sockets, use **COM** for flashing and serial logs, and **USB** for the native USB keyboard. Use a computer you own or have permission to test.

## Build and test

1. Install [ESP-IDF v5.5.3 and its toolchain](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32s3/get-started/index.html). On Windows, use the ESP-IDF PowerShell or Command Prompt shortcut so `idf.py` is available.
2. Clone this repository and run:

   ```powershell
   idf.py set-target esp32s3
   idf.py build
   idf.py -p COMx flash monitor
   ```

   Replace `COMx` with your board's serial port. On Linux or macOS, use the corresponding `/dev/...` port.
3. Pair **Octopus Kbd** from your test computer's Bluetooth settings. Put the cursor in a text editor. Press and release the wired button once. Expect `Octopus ready` and a newline. The serial monitor also logs the send event.
4. For the USB test on Windows, also connect the board's socket labeled **USB** to the computer with a data cable. Wait for Windows to recognize **Octopus USB Keyboard**. Press and release the button once. Expect a new untitled Notepad window containing the final `STRING` from `payload.txt`. USB takes priority while it is connected; unplug native USB to test BLE again.

If ESP-IDF is not installed locally, the [Firmware workflow](.github/workflows/firmware.yml) builds the project and publishes an `octopus-esp32s3-firmware` artifact containing `octopus-merged.bin`. Download it from a successful workflow run, then flash it at offset zero with `python -m esptool --chip esp32s3 --port COMx write-flash 0x0 octopus-merged.bin`.

### One-command Notepad demo update (Windows)

Edit [payload.txt](payload.txt) using this line-based format:

```text
REM Author: Ahmad
GUI r
DELAY 800
STRING notepad.exe
ENTER
STRING Hello Octupus
```

`REM` lines are comments. The five action lines must stay in this order: open the Windows Run dialog, wait 100–2000 ms, open `notepad.exe`, press Enter, and type one final line of text. The final `STRING` accepts up to 80 characters using letters, spaces, commas, and exclamation marks. The automater validates this exact Notepad sequence; it does not execute arbitrary commands. The file is committed to this **public** repository, so do not put secrets in it.

With the **COM** socket connected and `git`, `gh`, Python, and `esptool` installed, run from the repository root:

```powershell
.\deploy-demo.ps1 -Port COM3
```

The script validates the lines, generates the firmware header, commits and pushes the two demo files if they changed, waits for a successful GitHub Actions build, downloads the merged image, checks the chip on the selected port, and flashes it. It stops on any failure and refuses unrelated worktree changes. Use `-ValidateOnly` to check the file without building or flashing. Connect the board's **USB** socket afterward and press the button to test. Use a different `-Port` value if Windows assigned another COM number.

The keyboard mapping currently assumes a US English host layout. The Notepad sequence uses the Windows `Win+R` shortcut and is Windows-specific. If you change the HID report map or device name, remove the old Bluetooth pairing and pair again so the host refreshes its cached descriptor. This board has no display or keypad, so this prototype uses BLE Just Works bonding; it does not provide MITM protection. A phone control service will need a stronger authorization design before it can trigger actions.

## Plan

See [docs/roadmap.md](docs/roadmap.md) for the staged implementation and acceptance checks. The core design is a device service with separate HID transport, input/action engine, and later phone control. The phone app should select and confirm actions; the firmware should require an authenticated connection and local arming before sending any HID reports.

## Open source and attribution

Original Octopus material is MIT licensed; see [LICENSE](LICENSE). The files in `main/` derived from Espressif's ESP-IDF HID example retain their `SPDX-License-Identifier: Unlicense OR CC0-1.0` headers. See [NOTICE.md](NOTICE.md) for provenance. No third-party source is vendored beyond the adapted Espressif example.

Contributions are welcome through issues and pull requests. Please include your board model, ESP-IDF version, logs, and reproduction steps in bug reports. Test HID behavior only on devices you own or are authorized to use.
