# Kiiboom Cybrix 16 – QMK Firmware with Full VIA Support

[![QMK](https://img.shields.io/badge/QMK-v0.34+-blue.svg)](https://qmk.fm)
[![VIA](https://img.shields.io/badge/VIA-Protocol%20v13%20(v3)-brightgreen.svg)](https://usevia.app)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

This repository provides an open-source **QMK Firmware** implementation and a **VIA draft definition** for the **Kiiboom Cybrix 16** macropad (16 mechanical keys + 3 rotary encoders + per-key RGB matrix backlighting).

If you previously encountered errors like `NotAllowedError: Failed to open the device`, `Received invalid protocol version from device`, or *"Kiiboom Cybrix 16 does not appear to be a VIA-compatible device"*, this project fixes those issues completely.

---

## ✨ Features

- **Full VIA Support (Protocol v13 / VIA v3)**: Dynamic key remapping, layer switching, and macro support stored directly in EEPROM.
- **3 Rotary Encoders (`e0`, `e1`, `e2`)**: Independent clockwise and counterclockwise actions configurable dynamically in VIA (`ENCODER_MAP_ENABLE = yes`).
- **Push-Button Knobs**: Fully mapped in the switch matrix (Row 4).
- **RGB Matrix Backlighting**: Per-key animations, hue, saturation, speed, and brightness controllable via VIA.
- **Modern QMK Compatibility**: Tested and verified on modern QMK versions (0.34+) using the standardized `keyboard.json` schema and ChibiOS GPIO driver API.
- **Precompiled Binaries Included**: Flash right away without installing the QMK build toolchain.

---

## 📁 Repository Structure

```text
cybrix/
├── README.md                           # This guide
├── LICENSE                             # GNU General Public License v2.0
├── cybrix16_via.json                   # VIA definition file for usevia.app
├── firmware/
│   ├── kiiboom_cybrix16_via.bin        # Precompiled VIA firmware (ready to flash)
│   ├── kiiboom_cybrix16_via.hex        # Intel HEX format of VIA firmware
│   ├── kiiboom_cybrix16_default.bin    # Stock default keymap binary
│   └── kiiboom_cybrix16_default.hex    # Intel HEX format of default keymap
├── keyboards/
│   └── kiiboom/
│       └── cybrix16/                   # QMK source files
│           ├── config.h
│           ├── cybrix16.c
│           ├── keyboard.json
│           ├── rules.mk
│           └── keymaps/
│               ├── default/
│               │   ├── keymap.c
│               │   └── rules.mk
│               └── via/
│                   ├── keymap.c
│                   └── rules.mk
└── udev/
    └── 50-qmk.rules                    # Linux udev rules for WebHID & bootloader access
```

---

## ⚡ Quick Start: Flash & Setup in 2 Minutes

You don't need to compile from source. You can directly flash the precompiled binary in `firmware/`.

### Step 1: Put the Cybrix 16 into Bootloader Mode

1. Unplug the USB cable from the macropad.
2. **Hold down the top-left key (ESC / Row 0, Column 0)**.
3. Plug the USB cable back in while still holding the key.
4. Release the key after 1-2 seconds.
5. The device will enumerate as an STM32duino bootloader:
   - **VID: `1eaf` | PID: `0003`** (`Leaflabs Maple DFU interface`)

*(Alternative: Press the physical reset button on the back of the PCB if your case allows access).*

### Step 2: Flash the Firmware

#### Linux / macOS (using `dfu-util`)

Ensure `dfu-util` is installed (`sudo apt install dfu-util` on Debian/Ubuntu, `sudo pacman -S dfu-util` on Arch/CachyOS, or `brew install dfu-util` on macOS).

Run:

```bash
dfu-util -d 1EAF:0003 -a 2 -R -D firmware/kiiboom_cybrix16_via.bin
```

> **Note on DFU output:**
> If you see `Download done.` followed by `dfu-util: unable to read DFU status after completion (LIBUSB_ERROR_PIPE)`, **this is completely normal!** The STM32duino bootloader immediately restarts the MCU into application mode upon finishing the write, which severs the USB DFU pipe.

#### Windows (using QMK Toolbox)

1. Download and open [QMK Toolbox](https://github.com/qmk/qmk_toolbox/releases).
2. Open `firmware/kiiboom_cybrix16_via.bin` in QMK Toolbox.
3. Put the macropad into bootloader mode (QMK Toolbox will show `Maple 003 device connected`).
4. Click **Flash**.

---

## 🌐 Configuring in VIA (usevia.app)

Since the Kiiboom Cybrix 16 is not yet part of VIA's public cloud repository, you load the definition locally:

1. Open **Chrome, Chromium, Edge, or Brave** and navigate to:
   👉 **[https://usevia.app](https://usevia.app)**
2. Click **Authorize device** and select **Kiiboom Cybrix 16** (`VID: 0x5343, PID: 0x0080`).
3. Click the **Settings (gear icon ⚙️)** in the top bar.
4. Enable **"Show Design tab"**.
5. Switch to the newly visible **Design tab** (paintbrush icon 🖌️).
6. *(Important)* If the toggle **"Use V2 definitions (deprecated)"** is shown, make sure it is enabled.
7. Click **"Load Draft Definition"** and select the [`cybrix16_via.json`](cybrix16_via.json) file from this repository.
8. Switch back to the **Configure** tab (keyboard icon ⌨️).

You will now see:
- All 16 switch keys in a 4×4 grid.
- The 3 rotary encoders (`e0`, `e1`, `e2`) with independent CW / CCW rotation actions.
- The push clicks for each knob.
- Dedicated lighting controls for the RGB Matrix.

---

## 🐧 Linux Users: Fixing `NotAllowedError: Failed to open the device`

On Linux, Chromium/Chrome requires permission to access raw HID devices (`/dev/hidraw*`). If you see `NotAllowedError`, copy the provided udev rules:

```bash
sudo cp udev/50-qmk.rules /etc/udev/rules.d/50-qmk.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Then unplug and reconnect the Cybrix 16.

---

## 🛠️ Building from Source with QMK

If you want to customize the code or build the firmware yourself:

### 1. Install & Set Up QMK

Follow the official [QMK Setup Guide](https://docs.qmk.fm/#/newbs_getting_started):

```bash
python3 -m pip install --user qmk
qmk setup
```

### 2. Copy the Keyboard Files

Copy the `keyboards/kiiboom/cybrix16` folder into your local `qmk_firmware` directory:

```bash
cp -r keyboards/kiiboom/cybrix16 /path/to/qmk_firmware/keyboards/kiiboom/
```

### 3. Compile

```bash
cd /path/to/qmk_firmware
qmk compile -kb kiiboom/cybrix16 -km via
```

### 4. Flash

With the keyboard in bootloader mode:

```bash
qmk flash -kb kiiboom/cybrix16 -km via
```

---

## 📋 Hardware Specifications & Pinout

| Component | Pin / Configuration |
|---|---|
| **MCU** | STM32F103 (ARM Cortex-M3) |
| **Bootloader** | STM32duino (`1eaf:0003`, Alt 2 @ `0x08002000`) |
| **USB VID / PID** | `0x5343` / `0x0080` |
| **Matrix Rows (5)** | `A0`, `A1`, `A3`, `A2`, `A4` |
| **Matrix Columns (4)** | `B0`, `B10`, `B2`, `B1` |
| **Diode Direction** | `ROW2COL` |
| **Rotary Encoder 1 (`e0`)** | Pad A: `A6`, Pad B: `C14`, Switch: `Row 4, Col 2` |
| **Rotary Encoder 2 (`e1`)** | Pad A: `C13`, Pad B: `A5`, Switch: `Row 4, Col 1` |
| **Rotary Encoder 3 (`e2`)** | Pad A: `C15`, Pad B: `A7`, Switch: `Row 4, Col 3` |
| **RGB Matrix Data Pin** | `B11` (WS2812 driver) |

---

## ❓ Troubleshooting

| Issue | Cause | Solution |
|---|---|---|
| `NotAllowedError: Failed to open the device` | Missing Linux udev permissions | Install `udev/50-qmk.rules` as described above and replug the USB cable. |
| `Received invalid protocol version from device` | Macropad is running factory firmware without QMK/VIA | Flash `firmware/kiiboom_cybrix16_via.bin`. |
| Cybrix 16 not recognized in VIA automatically | The keyboard definition is not in VIA's cloud DB | Enable the Design tab in VIA Settings and load `cybrix16_via.json`. |
| `dfu-util: unable to read DFU status` | Normal STM32duino reboot behavior | This is harmless and indicates a successful flash. Reconnect in VIA. |
| Compile error `ENCODER_A_PINS redefined` | Legacy `config.h` encoder definitions | Encoders are now configured in `keyboard.json` under `"encoder"`. Use the files in this repo. |

---

## 📄 License

This firmware and layout are licensed under the **GNU General Public License v2.0 or later** ([GPL-2.0-or-later](LICENSE)), in accordance with the QMK Firmware project.
