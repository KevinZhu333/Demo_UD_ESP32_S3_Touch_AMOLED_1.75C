# Demo UD ESP32-S3 Touch AMOLED 1.75C

Firmware project for the ESP32-S3 Touch AMOLED 1.75C board.

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
