> 🇩🇪 [Deutsche Version](readme_de.md)

# Local MQTT – EcoFlow PowerStream BLE Bridge
### ESP32 · BLE · MQTT · Home Assistant · V2.6.3

A fully local, cloud-free PowerStream controller via MQTT → Wi-Fi → ESP32 → BLE.  
No recurring disconnects. No cloud dependency. Ready in ~15 minutes.

---

## ⚠️ Notice & Disclaimer

This project describes only how I personally control and read out my own EcoFlow PowerStream microinverters directly and locally via BLE (Bluetooth Low Energy), using an ESP32 microcontroller.

**It is neither a build guide nor an invitation to replicate it.**

Anyone using the contents of this repository — in whole or in part — does so **entirely at their own risk and responsibility**. Every person using this content is assumed to possess the necessary basic knowledge in the fields of electronics, embedded systems, electrical installation, and the applicable legal regulations.

The author accepts **no liability whatsoever** for harm to persons, animals or property, damage to devices or installations, data loss, malfunctions, warranty violations, or any consequential damage.

---

## Why this project exists

After repeatedly running into the same issue over almost two years, I concluded that neither the EcoFlow cloud connection nor the available local PowerStream integrations can be considered truly reliable.

After weeks of analysis, I am ~98% convinced that the problem lies in the Wi-Fi component of the PowerStream WiFi firmware — specifically in the ESP32-based IoT module used in certain WiFi firmware versions.

Downgrades, local control via TCP, endless email exchanges with EcoFlow support — who ultimately would not or could not acknowledge where the problem actually lies — everything failed.

---

## Cloud mode behavior

When operating through the EcoFlow cloud:

1. The PowerStream is shown as offline in the app / cloud / API.
2. Opening the EcoFlow app triggers an immediate reconnect.
3. This explains why the well-known workaround — automatically opening the app every few minutes — actually works.

---

## Local decoder behavior

When operating locally through the decoder:

- There is no cloud-triggered recovery.
- Sometimes the connection remains stable for hours.
- Sometimes disconnects occur every 2, 4, or 8 minutes.
- Some interruptions last only seconds; others may persist for hours.

Because the behavior is so inconsistent, troubleshooting becomes nearly impossible.

---

## My solution: BLE instead of Wi-Fi

At some point I stopped trying to make the Wi-Fi implementation work reliably and switched to a Bluetooth Low Energy (BLE) based approach.

The result is exceptionally stable, fast, and reliable.

**The concept:**

- ESP32-WROOM (~€8) acts as a BLE bridge for the PowerStream — like a small private cloud, entirely local
- The ESP connects to your existing Wi-Fi network
- Integrates directly with Home Assistant via MQTT
- No internet required, no VLANs, proxies, containers, cloud services, or custom network setup
- Firmware flashing takes about 10 minutes
- Local control and access to nearly all PowerStream values
- Automatic Home Assistant entity creation through MQTT Auto Discovery
- Ready for automation within minutes

---

## Benefits

✅ Fully local operation — even without internet (unlike the phone app)  
✅ No cloud dependency  
✅ Fast and reliable communication  
✅ Home Assistant Auto Discovery  
✅ No recurring disconnect issues  
✅ Powered directly from the USB port of a Delta battery  

---

## Requirements

| Component | Notes |
|-----------|-------|
| ESP32-WROOM | ~€8–10 |
| Setup time | ~15 minutes |
| Home Assistant *¹ | with MQTT broker |
| EcoFlow PowerStream HW51 | tested hardware |

*¹ Smart home platforms such as Home Assistant, ioBroker, openHAB — or any other MQTT broker — can be used. MQTT is platform-independent; automatic Auto Discovery is HA-specific.

---

## What's included

| File | Description |
|------|-------------|
| [`SETUP_PS_HW51_BLE_BRIDGE_final_DE.md`](SETUP_PS_HW51_BLE_BRIDGE_final_DE.md) | 🇩🇪 Complete setup guide (German) |
| [`SETUP_PS_HW51_BLE_BRIDGE_final_EN.md`](SETUP_PS_HW51_BLE_BRIDGE_final_EN.md) | 🇬🇧 Complete setup guide (English) |
| [`ps-esp-ble-bridge-v_2_6_3_blanco_de.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_de.ino) | 🇩🇪 ESP32 firmware — comments in German |
| [`ps-esp-ble-bridge-v_2_6_3_blanco_en.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_en.ino) | 🇬🇧 ESP32 firmware — comments in English |
| [`HA-Automatisierung-Nulleinspeisung-Beispiel.md`](HA-Automatisierung-Nulleinspeisung-Beispiel.md) | 🇩🇪 Home Assistant zero feed-in automation example |
| [`HA-Automation-ZeroFeedin-Example.md`](HA-Automation-ZeroFeedin-Example.md) | 🇬🇧 Home Assistant zero feed-in automation example |

**Start here:** Read the setup guide in your language completely before you begin.

---

## Quick Start

1. Read the setup guide (DE or EN) completely before starting
2. Identify your PowerStream BLE MAC address (nRF Connect app)
3. Open the `.ino` firmware file in a text editor and fill in your values
4. Flash to your ESP32 using Arduino IDE
5. Import the HA automation example and adapt entity names to your setup

---

## Credits

This project would not have been possible without the groundbreaking reverse-engineering work by **Roman Bashlovkin**:

> [rabits/ha-ef-ble](https://github.com/rabits/ha-ef-ble) — BLE protocol decoding, frame format, Protobuf definitions, and field mappings for EcoFlow devices.

---

## Community

Questions, feedback, and results → **[feel free to use the Discussions](https://github.com/Just-Zuul/Local-MQTT-EcoFlow-PowerStream-BLE-Bridge/discussions)**
