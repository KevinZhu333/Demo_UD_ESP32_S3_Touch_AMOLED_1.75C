# Demo UD ESP32-S3 Touch AMOLED 1.75C

Firmware workspace for the ESP32-S3 Touch AMOLED 1.75C board.

The Brookesia reproduction plan in
`docs_brookesia/03-reproduction-plan.md` is implemented in
`factory_demo/`. That project is an editable ESP-IDF Brookesia firmware based
on the public Waveshare example, not a byte-for-byte rebuild of the vendor
factory image.

The vendor factory binary should remain untouched and available as the restore
image:

```text
/Users/starspulse/Downloads/ESP32-S3-Touch-AMOLED-1.75C-main/Firmware/ESP32-S3-Touch-AMOLED-1.75C-FactoryOnly-260114.bin
```

## Brookesia Reproduction Project

Use `factory_demo/` for the source-built Brookesia launcher:

```bash
cd factory_demo
idf.py set-target esp32s3
idf.py build
```

`factory_demo/` keeps the public template's launcher ownership model:

- `main/main.cpp` is the only firmware entry point.
- `components/brookesia_core/` is the local Brookesia core copy.
- `components/brookesia_app_squareline_demo/` remains as the public demo app.
- `components/brookesia_app_factory_map/` is a custom launcher app registered
  through Brookesia's plugin registry.
- `components/brookesia_app_recorder/` is a custom recorder launcher app that
  captures microphone audio to `/storage/recording.wav`.
- `factory-partitions.csv` stores the parsed vendor reference map, while
  `partitions.csv` is the active source-build table.

Only flash this project after the restore procedure for the vendor `.bin` is
known.

## Risk Controls

This project is a maintainable source rebuild, not the original Waveshare
factory source. Keep these boundaries in place:

- Do not edit or replace the vendor factory `.bin`; keep it as the restore
  image.
- Do not copy the parsed factory partition map into `partitions.csv` without a
  successful baseline build and an explicit layout review.
- Keep custom apps as Brookesia components under `factory_demo/components/`.
  Do not add a second `app_main()`.
- Keep app-owned hardware work off the LVGL path. The recorder stops active
  capture in `back()` and `close()` before returning to the launcher.
- Treat `docs_brookesia/` as local investigation notes; it is ignored by Git.

## Flashing

Reference: https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/startproject.html

### 1. Open the Project

Open this as an existing ESP-IDF project in Visual Studio Code.

The easiest path is to download the GitHub repository as a ZIP file, extract it, and open the extracted folder.

In Visual Studio Code:

- Go to `View > Command Palette`.
- Run `ESP-IDF: Import ESP-IDF Project`.
- Choose the project folder.

### 2. Select the Serial Port

In Visual Studio Code:

- Go to `View > Command Palette`.
- Run `ESP-IDF: Select Port to Use`.
- Choose the serial port connected to the device.

A typical port may look like:

```text
/dev/tty.usbmodem1101 expressif
```

### 3. Set the ESP-IDF Target

In Visual Studio Code:

- Go to `View > Command Palette`.
- Run `ESP-IDF: Set Espressif Device Target`.
- Select the ESP32-S3 target.

A connected ESP32-S3 target may look like:

```text
ESP32-S3 chip (via builtin USB-JTAG)
ESP32-S3 debugging via builtin USB-JTAG
Status: CONNECTED
Location: usb://1-1
```

### 4. Configure the Project

In Visual Studio Code:

- Go to `View > Command Palette`.
- Run `ESP-IDF: SDK Configuration Editor`.
- Adjust any project settings as needed.
- Save the configuration.

### 5. Build

In Visual Studio Code:

- Go to `View > Command Palette`.
- Run `ESP-IDF: Build your Project`.
- Confirm the build completes cleanly.

### 6. Flash

In Visual Studio Code:

- Go to `View > Command Palette`.
- Run `ESP-IDF: Select Port to Use` and confirm the device serial port.
- Run `ESP-IDF: Flash your Project`.
- Choose UART, JTAG, or DFU flashing when prompted.
