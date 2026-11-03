# SeeMore

Repo for all the code for SeeMore

# This is for the Packages! 

# Arduino Packages — Senior Design Toolchain

Downloaded **Nov 2, 2026**. This README covers the Arduino IDE and the Heltec board-support packages pulled for the Senior Design project, what each one gives you, and how to install them.

## What was downloaded

| File | What it is | Notes |
|---|---|---|
| `arduino-ide_2.3.10_Windows_64bit.exe` (and `(1)` copy) | Arduino IDE 2.3.10 installer for 64-bit Windows (~158 MB) | Same installer twice; the `(1)` copy can be deleted |
| `package_heltec_esp32_index.json` | Boards Manager index for **Heltec ESP32** boards (WiFi Kit / WiFi LoRa 32 / Wireless Stick, etc.) | Core v3.3.8 |
| `package_CubeCell_index.json` (and `(1)` copy) | Boards Manager index for **Heltec CubeCell** (ASR650x LoRa) boards | Core versions 0.0.1 → 1.7.1; the two copies are byte-identical |

The `.json` files are **not** the board cores themselves. They are index files that tell the Arduino IDE's Boards Manager where to download each core and its toolchain.

---

## 1. Arduino IDE 2.3.10

1. Run `arduino-ide_2.3.10_Windows_64bit.exe` and accept the default install options.
2. Allow the USB driver prompts during install. The ESP32 and CubeCell boards both show up as USB-serial devices.

---

## 2. Heltec ESP32 package (`package_heltec_esp32_index.json`)

- **Package:** `Heltec-esp32`, maintained by Heltec Automation
- **Source:** https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
- **Platform:** *Heltec ESP32 Series Arduino Develop Environment*, architecture `esp32`, **v3.3.8** (~21 MB)

### Supported boards
- WiFi Kit 32, WiFi Kit 32 (V3)
- WiFi LoRa 32, (V2), (V3), (V4), WiFi LoRa 32 (V4 R8)
- Wireless Stick, Wireless Stick (V3)
- Wireless Stick Lite / Wireless Shell, Wireless Stick Lite (V3), Wireless Shell (V3), Wireless Mini Shell
- Wireless Paper, Wireless Tracker, Wireless Tracker (V2), Wireless Bridge
- Capsule Sensor (V3), E-Ink Driver
- Vision Master E213, E290, T190

### Toolchain it installs automatically
| Tool | Version |
|---|---|
| esp32-arduino-libs (ESP-IDF) | idf-release_v5.5-73550728-v2 |
| xtensa-esp-elf-gcc / riscv32-esp-elf-gcc | esp-14.2.0_20260121 |
| xtensa-esp-elf-gdb / riscv32-esp-elf-gdb | 16.3_20250913 |
| openocd-esp32 | v0.12.0-esp32-20251215 |
| esptool_py | 5.2.0 |
| mkspiffs / mklittlefs | 0.2.3 / 4.0.2-db0513a |
| dfu-util (from the `arduino` package) | 0.11.0-arduino5 |

Because this core is built on ESP-IDF 5.5, it uses the **ESP32 Arduino 3.x API**. Older 2.x examples may need small changes, for example to the LEDC/PWM and timer calls.

---

## 3. Heltec CubeCell package (`package_CubeCell_index.json`)

- **Package:** `CubeCell`, maintained by Heltec Automation (support@heltec.cn)
- **Source:** https://github.com/HelTecAutomation/CubeCell-Arduino (formerly ASR650x-Arduino)
- **Platform:** *CubeCell Development Framework*, architecture `CubeCell`
- **Versions listed:** 0.0.1 – 1.7.1. **Install 1.7.1**, the newest.

### Supported boards (v1.2.0 and later)
- CubeCell-Board, CubeCell-Board Plus
- CubeCell-Module, CubeCell-Module Plus
- CubeCell-Capsule (listed as "Cupsule" in the index)
- CubeCell-GPS
- CubeCell-1/2AA Node

Versions before 1.2.0 only support Board, Capsule and Module.

### Toolchain it installs automatically
| Tool | Version |
|---|---|
| gcc-arm-none-eabi | 8-2019-q3 |
| CubeCellelftool | 0.0.1 |
| CubeCellflash | 0.0.1 |

CubeCell boards are ARM Cortex-M0+ (ASR6501/6502) with an onboard SX1262 LoRa radio, built for low-power LoRa/LoRaWAN nodes. Heltec's ESP32 LoRa boards are the better fit when you need WiFi or BLE alongside LoRa.

---

## Installing the board packages

**Option A — Use the downloaded files (offline index)**
1. In Arduino IDE, go to **File → Preferences → Additional boards manager URLs**.
2. Add one line per file:
   ```
   file:///C:/Users/PCHER/Downloads/package_heltec_esp32_index.json
   file:///C:/Users/PCHER/Downloads/package_CubeCell_index.json
   ```
3. Open **Tools → Board → Boards Manager**, then search for and install:
   - **Heltec ESP32 Series Dev-boards** (v3.3.8)
   - **CubeCell Development Framework** (v1.7.1)

The index files still download the cores and toolchains from GitHub and Heltec's servers, so you need internet access for step 3.

**Option B — Use Heltec's hosted URLs (stays up to date)**
Paste these into the same Preferences field instead:
```
https://resource.heltec.cn/download/package_heltec_esp32_index.json
https://github.com/HelTecAutomation/CubeCell-Arduino/releases/download/V1.5.0/package_CubeCell_index.json
```
Option B picks up new releases automatically. The local files in Option A are a snapshot from today.

## Selecting your board
After installing, go to **Tools → Board → Heltec ESP32 Series Dev-boards** (or **CubeCell**) and pick your exact board and revision, such as *Heltec WiFi LoRa 32(V3)*. Then choose the COM port under **Tools → Port**. For LoRa boards, set the LoRa region (for example, US915) in the **Tools** menu before uploading.

## Housekeeping
- These duplicates are safe to delete: `arduino-ide_2.3.10_Windows_64bit (1).exe`, `package_CubeCell_index (1).json`, and the older `arduino-ide_2.3.10_Windows_64bit.exe` from Oct 12, which is the same version.
- If you use Option A, keep `package_heltec_esp32_index.json` and `package_CubeCell_index.json` where they are, because the IDE reads them from that path.
