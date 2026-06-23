## SETUP PS - ESP - BRIDGE (English)
> 🇩🇪 [Deutsche Version](SETUP_PS_HW51_BLE_BRIDGE_final_DE.md)  

## ⚠️ Notice & Disclaimer

This repository describes only how I personally control and read out my own EcoFlow PowerStream
microinverters directly and locally via BLE (Bluetooth Low Energy), using an ESP microcontroller.

**It is neither a build guide nor an invitation to replicate it.**

### Use at your own risk

Anyone who uses, implements, replicates or otherwise makes use of the contents of this repository –
in whole or in part – does so **entirely at their own risk and responsibility**.

Every person using this content is assumed to possess the necessary basic knowledge in the fields of
electronics, embedded systems, electrical installation, as well as the applicable legal regulations.

### Liability disclaimer

The author accepts **no liability whatsoever** for:

- Harm to persons, animals or property
- Damage to devices, installations or the electrical installation
- Data loss or malfunctions
- Consequential damage of any kind
- Violations of manufacturer warranty conditions or certifications
- Violations of legal regulations or standards

This applies regardless of whether damage arises from direct use, modification or faulty
implementation of the methods described here.

### No warranty

The information provided here is supplied **without any warranty** – neither express nor implied.
No guarantee is given for correctness, completeness, timeliness or functionality.

### Trademarks & third parties

EcoFlow and PowerStream are registered trademarks of EcoFlow Technology Inc. This project is in no way
affiliated with EcoFlow and is neither endorsed nor approved by EcoFlow.  


# Table of contents  

## PowerStream HW51 ↔ ESP32 BLE Bridge — Setup Guide

 1. Overview — what the bridge does & why (local via BLE, no cloud/decoder), what it controls
 2. Feature set (short list: telemetry, control, failsafe, multi-WiFi, WebLog …)
 3. Prerequisites: hardware & software
 4. Arduino libraries (+ micro-ecc troubleshooting, NimBLE 2.x)
 5. Required data
   • How to obtain the data: PowerStream BLE MAC (nRF Connect), User ID (GnoX), AP BSSID
 6. Configuration — the CONFIG block step by step
   • WIFI_CREDS (priority list) · WIFI_MODE · AP_BSSID
   • Static IP vs. DHCP (incl. cross-range note)
   • MQTT: broker / port / user / pw
   • Device identity: SN · BLE-MAC · USER_ID · DEVICE_ID
   • Failsafe setpoint: FAILSAFE_WATTS (50 W)
 7. Flashing (select board, port, upload speed, CP210x driver)
 8. Home Assistant — what appears automatically (MQTT discovery): sensors · controls · BLE Link · diagnostics · disabled sensors · bp_type meaning
 9. Operation & features — setpoint · brightness · supply mode · restart button · WebLog (browser -> ESP-IP)
10. Behaviour explained (the "why"): write-on-change (EEPROM-friendly) · availability (BLE Link "Disconnected") · WiFi failover (best-at-boot + re-lock) · failsafe logic · multi-WiFi priority
11. HA automation (example): zero feed-in · 5 W rounding · delta gating
12. Troubleshooting: NimBLE version · micro-ecc · flash abort · BLE connect · WiFi/broker reachability (cross-range)
13. Background/architecture (optional): local instead of cloud · EEPROM/EF support · dynamicWatts not BLE-writable · two-channel design
14. Credits & sources: rabits/ha-ef-ble · GnoX User ID Finder
15. Notes


### 1. Overview — what the bridge does & why

The bridge is firmware for a small ESP32 microcontroller.
The ESP connects **locally via Bluetooth (BLE)** to an EcoFlow PowerStream and handles two tasks:

- **Reading:** It continuously reads the PowerStream's measurements (power, battery flow, status …).
- **Control:** It sets the **feed-in setpoint** (and a few other control variables) directly on the device.

Everything is reported via **MQTT to Home Assistant**, where sensors and controls appear automatically.

**Why locally via BLE and not via the cloud?** 
My system, my device, my data, my control + more reliable control and all data in Home Assistant.
The PowerStream's cloud/MQTT connection keeps dropping — control commands then arrive late or not at all. 
BLE is a **direct, local radio link**: independent of the internet, EcoFlow cloud and their servers.
The result is fast, reliable control that keeps working even without internet (and without EcoFlow).

### 2. Feature set (short overview)

- **Full telemetry:** power, battery flow, temperatures, status values … as HA sensors.
- **Control:** feed-in setpoint (W), LED brightness, supply priority, ESP restart via button.
- **Write-on-change:** the setpoint is only written when it actually changes → gentle on the device.
- **WiFi failsafe:** if WiFi is gone for a while, the ESP autonomously sets a safe value (50 W).
- **WiFi failover + multi-WiFi:** several networks as a priority list; automatic AP switch on outage.
- **Built-in WebLog:** a small status web page right on the ESP (live status BLE/WiFi/MQTT + log).
- **Auto-integration into Home Assistant** via MQTT discovery — no manual entity setup needed.

### 3. Prerequisites, hardware and software used
- PC (Windows/macOS/Linux)
- Arduino IDE — https://www.arduino.cc/en/software/
- ESP32-WROOM module — here an ESP32-WROOM development board with CP2102 chip, 38-pin, USB-C (available from the big A or the retailer of your choice)
- A suitable USB cable (here USB-A to USB-C)
- USB driver: For boards with a **CP2102**, install the **CP210x VCP driver from Silicon Labs** if needed — otherwise no COM port appears on the PC.
- This repo

### 4. Arduino libraries
The firmware needs two libraries that must be installed manually.

**Install manually (Arduino → Library Manager):**
- **PubSubClient** — by Nick O'Leary ("knolleary") → MQTT client (`PubSubClient.h`)
- **NimBLE-Arduino** — by h2zero (Ryan Powell) → BLE stack (`NimBLEDevice.h`)
  - ⚠️ NimBLE-Arduino has a noticeably changed API between 1.x and 2.x. **Version 2.x is mandatory** (e.g. 2.5.0) — with 1.x you get compile errors.

**Comes with the _ESP32 board package by Espressif Systems_** (no separate install — just install the board package via the *Boards Manager*):
- `WiFi.h`
- `WebServer.h`
- `MD5Builder.h`
- `mbedtls/aes.h` (mbedTLS, bundled in the ESP-IDF/core)
- `esp_system.h` (`esp_reset_reason()`)

> **Troubleshooting micro-ecc:** An async web server is **not** needed (the code uses the synchronous core `WebServer.h`). If `micro-ecc` (e.g. from another project) is installed and there are linker conflicts with NimBLE, **remove or rename** the standalone `micro-ecc` — NimBLE 2.x brings its own crypto.

### 5. Data required for setup (minimum):

- WiFi SSID
- WiFi password

- MAC address of the **2.4 GHz** network of the nearest or desired WiFi AP (optional *1)

- MAC address of the PowerStream (e.g. read out with the **nRF Connect** app)
- Serial number (SN) of the PowerStream

- EcoFlow **User ID** — easiest via the **"EcoFlow User ID Finder" by GnoX**: https://gnox.github.io/user_id (a supplement to the ha-ef-ble integration). Log in with your EcoFlow credentials; the requests go **directly from the browser to EcoFlow's official API** (no forwarding to third-party servers). EU accounts: select region **`api-e.ecoflow.com`** or "Auto".

- MQTT broker: address, port, username and password (if user/password is used)

- Desired **static IP** for the ESP on the network (optionally with gateway/subnet mask), if static configuration is used *2

- **DEVICE_ID** — a freely chosen device name (e.g. `WR1-PS1234`); appears in the MQTT topics and HA entities and names the entities uniquely (note: the entities then become: sensor.powerstream_<device_id>_entityname )
- *(optional)* **Timezone** (TZ string; default Central Europe `CET-1CEST,M3.5.0,M10.5.0/3`)

> *1 The ESP firmware offers the option to influence the WiFi connection specifically → see the **WiFi** section in Section 6.
> *2 Either DHCP or a fixed IP is possible (fixed IP is recommended! - note the hints on fallback WiFi!) 



Once you have all the data together and Arduino is installed, adjust the most important settings:



<img width="613" height="812" alt="grafik" src="https://github.com/user-attachments/assets/55312555-a6ef-41ba-8555-044e76e51109" />



Then open the Arduino sketch: **ps-esp-ble-bridge-v_2_6_3_blanco_en.ino** in an editor (e.g. Notepad++) to adjust the entries.

### 6. Configuration - structure of the header with the entries to adjust:


LINE 87 = // =================== CONFIG  (fill in from here) =================== = start of the configuration

LINE 94 = primary WiFi network and password
LINE 95 to 96 = enter WiFi SSID and passwords if fallback is wanted... otherwise comment out the line with // (so the following line numbers stay correct)
```
const WifiCred WIFI_CREDS[] = {
  { "SSID 1", "PASS 1" },                    // Prio 1 (preferred, e.g. dedicated router or AP)
  { "SSID 2", "PASS 2" },                    // Prio 2 (fallback, e.g. mesh) - if only 1 network -> comment out the line
  { "SSID 3", "PASS 3" },                    // Prio 3 (possible further fallback) - if only 1 network -> comment out the line  
};
```
WITH a single network in use this becomes:
```
const WifiCred WIFI_CREDS[] = {
  { "GIVEN-SSID 1", "GIVEN-PASS 1" },        // Prio 1 (preferred, e.g. dedicated router or AP)
//  { "SSID 2", "PASS 2" },                  // Prio 2 (fallback, e.g. mesh) - if only 1 network -> comment out the line
//  { "SSID 3", "PASS 3" },                  // Prio 3 (possible further fallback) - if only 1 network -> comment out the line  
};
```


LINE 100 - 106:
```
L100: // WiFi roaming protection :
L101: //   0 = Auto   1 = Best-at-Boot (recommended)   2 = Hard-Lock (fixed BSSID)
L102: #define WIFI_MODE  1
L103: static const char* AP_NAME    = "SAMPLE-AP";
L104: static uint8_t     AP_BSSID[6] = { 0xA1, 0xB2, 0xC3, 0xAA, 0xBB, 0xCC };
L105: 
L106: // SAMPLE-AP = A1:B2:C3:AA:BB:CC   -> here you can note which AP you use and which MAC it has
```
L102 = use mode 0 or 1 if more than one WiFi network is possible or in use (lines 95-96 filled). Mode 2 only for a **single** network with a fixed assigned AP (Caution! new AP = re-flash the ESP!)
L103 = the AP name shown in mode 2 (or as a reference note in mode 1 if you know which AP is the nearest)
L104 = the MAC of the AP to bind to in mode 2 (or, in mode 0 or 1, just shown as which AP is the nearest - displayed in the diagnostic data)


LINE 109 - 115:
```
L109: // Optional static IP (adjust per device!). false = DHCP.
L110: static const bool USE_STATIC_IP = true;                // !!!! <----- !! NOTE !! - !! IMPORTANT !! ---- !!!!!
L111: IPAddress local_IP(192, 168, 100, 100);                // !!!!! change per device
L112: IPAddress gateway (192, 168, 100,   1);
L113: IPAddress subnet  (255, 255, 255,   0);
L114: IPAddress dns1    (192, 168, 100,   1);
L115: IPAddress dns2    (192, 168, 100,   1);
```
LINE 110 = use a fixed IP (true) or DHCP (false). A fixed IP is the default and recommended, as long as **all networks used are in the same IP range** (multiple APs/SSIDs in the same network are no problem this way).  
Only if fallback networks are in **different IP ranges/subnets** must DHCP (USE_STATIC_IP = false) be used.  

Minimum configuration for WiFi / NETWORK settings: ONE SSID+PASS, WIFI MODE 1 (default), lines 111-115 for a fixed IP  

Note: the built-in web server is reachable at the ESP's IP. A fixed IP makes access easier.  


LINE 117 - 121:
- complete / adjust the existing MQTT settings  

LINE 123 - 124:
```
L123: //  --- EcoFlow User ID ---
L124: const char* EF_USER_ID = "0000000000000000000";        // EcoFlow User ID (same for all devices)
```
**The User ID can be obtained quite easily with the tool at:  https://gnox.github.io/user_id**  

LINE 126 - 129:
```
L126: // --- NTP / time ---
L127: static const char* NTP_SERVER_1 = "192.168.100.1";               // router or other time source on the local LAN
L128: static const char* NTP_SERVER_2 = "pool.ntp.org";                // fallback - public time source
L129: static const char* TZ_STRING    = "CET-1CEST,M3.5.0,M10.5.0/3";  // timezone: CET/CEST incl. daylight saving
```
Adjust the time servers and the timezone (preset to Central Europe)  


LINE 131 - 134:
```
L131: //  --- individual per device ---
L132: const char* PS_SN     = "HW51ZOH4PS000000";    // serial number of the inverter
L133: const char* PS_MAC    = "77:66:ef:44:zz:ee";   // BLE MAC of the inverter (lowercase!!)
L134: const char* DEVICE_ID = "WR0-PS0000";          // short ID -> topics + MQTT client ID - composed as: PowerStream-WR0-PS0000
```
The serial number MUST match the inverter exactly!!  
The PowerStream's BLE MAC can easily be found with the "nRF" app. During the scan the inverter shows up with its identifier "HW-1234". LINE 133: **note the LOWERCASE!!**  
The device ID is used to name the entities. Just a "1" would become "PowerStream-1"   


LINE 149 - 152:
```
L150: const int      FAILSAFE_WATTS   = 50;          // safe setpoint when WiFi is gone for longer (set via BLE)
```
Here you can adjust the feed-in power for the emergency setting on WiFi loss (base consumption or base feed-in value).  


 ### 7. Flashing (select board, port, upload speed, CP210x driver)

After fully adjusting the template with all the required data, copy the code into the Arduino interface, connect the board and flash.

Prerequisites were already covered in Section 3.

Step by step:

1. **Select board:** Tools → Board → ESP32 Arduino → **"ESP32 Dev Module"** (fits common ESP32-WROOM boards).
2. **Select port:** Tools → Port → choose the ESP's COM port.
   - No port visible? → CP210x driver missing (see Section 3) or a charge-only cable without data lines.
3. **Upload speed:** the default (115200) is fine. If the upload aborts, choose a lower value (e.g. 921600 → 115200).
4. **Upload:** click the arrow ("Upload"). The code is compiled and written to the ESP.
   - If it hangs at "Connecting……": hold down the **BOOT button** on the ESP until the upload starts (some boards need this).
5. **Done:** on success Arduino reports "Done uploading" or "Hard resetting via RTS pin". The ESP restarts and begins connecting (WiFi → MQTT → BLE).

**Check:** in the serial monitor (115200 baud) or later in the WebLog (http://<ESP-IP>/) you can watch the startup: WiFi connection, MQTT, then the BLE connect to the PowerStream.

### 8. Home Assistant — what appears automatically (MQTT discovery)

Prerequisite: in HA the **MQTT integration** is set up (same broker as in the sketch) and
**discovery is enabled** (default). After flashing, the ESP announces itself — **no manual entity
setup** is needed.

- **Device:** appears as **`PowerStream-<DEVICE_ID>`** (e.g. `PowerStream-WR1-PS1234`).
- **Entity naming:** `sensor.powerstream_<device_id>_<name>` (likewise for number/select/button).
- **What you get:**
  - **~46 sensors** — telemetry from the PowerStream (power values, battery flow, temperatures, status values …).
  - **Controls** — setpoint (Number), LED brightness (Number), supply mode (Select), restart (Button).
  - **"BLE Link"** — connection status to the PowerStream (see Section 10, Availability).
  - **~18 diagnostic entities** — RSSI, IP, uptime, connected AP, reset reason, etc.
- **Sensors disabled by default:** inverter temperature (always reads 0), Installation Country/Town —
  they exist but are hidden; enable them in HA if needed.
- **`bp_type`:** inverter status — value **`2`** = (Delta) battery connected/detected.

### 9. Operation & features

- **Setpoint (W):** the feed-in power. Set via the Number entity (manually or by automation, Section 11). Written only on a real change (Section 10).
- **LED brightness:** brightness of the device LED (0–1023).
- **Supply mode:** supply priority (e.g. power supply vs. battery).
- **Restart button:** restarts the ESP — handy e.g. to re-trigger the best-at-boot AP selection (Section 10).
- **WebLog:** open `http://<ESP-IP>/` in the browser. Shows live status (BLE/WiFi/MQTT as cards) and the latest log lines — reachable even **without** Home Assistant. A fixed IP makes access easier.

### 10. Behaviour explained (the "why")

- **Write-on-change:** the setpoint is only written when it changes (or actual ≠ target). At idle this means
  **zero writes** → gentle on the PowerStream's internal memory (EEPROM).
- **Availability (BLE Link):** "BLE Link" shows the **real** state — "Connected"/"Disconnected", and on
  ESP failure "Disconnected" via last-will. PS sensors become **"unavailable"** when no BLE data arrives —
  so no frozen phantom values remain in HA.
- **WiFi failover (best-at-boot + re-lock):** at startup the ESP picks the strongest reachable AP and
  "locks" onto it. If that AP is gone for longer (≥ 30 s), it **automatically rescans** — no manual reset
  needed anymore.
- **WiFi failsafe:** if WiFi is continuously gone for ≥ 15 s (and BLE is present), the ESP **autonomously
  sets a safe setpoint (50 W)** so the inverter does not keep feeding uncontrolled at the last value without
  Home Assistant. When WiFi returns, HA takes over again.
- **Multi-WiFi priority:** multiple networks possible (Section 6). The ESP prefers the **topmost reachable**
  network in the list (e.g. a dedicated box) and otherwise falls back to the next one (e.g. mesh) — each with
  its matching password.

### 11. HA automation (example)

The bridge only provides the **control variable** (the setpoint). The actual control logic — e.g. a
**zero feed-in** (regulating grid draw to ~0) — belongs in a **Home Assistant automation**.

A **generic, adaptable example** (zero feed-in with 5 W rounding and a delta dead band) is in the
separate file:

> **`HA-Automation-ZeroFeedin-Example.md`**

It contains the ready-made YAML plus an explanation of the logic — entity names and the sign of the grid
sensor must be adapted to your own system.

### 12. Troubleshooting (known errors & simple fixes)

- **No COM port in Arduino:** CP210x VCP driver missing (Section 3) — or a charge-only cable without data lines. Try a different cable/port.
- **Compile error at a BLE callback (e.g. `onDisconnect`):** wrong NimBLE version. Use **NimBLE-Arduino 2.x**, not 1.x (Section 4).
- **Linker error / duplicate symbols (micro-ecc):** a standalone `micro-ecc` from another project collides. Remove or rename it — NimBLE 2.x brings its own crypto.
- **Upload doesn't start / aborts:** at "Connecting……" hold the **BOOT button**; lower the upload speed; correct board ("ESP32 Dev Module"); a shorter/different USB cable.
- **ESP doesn't appear in HA:** MQTT integration active? Broker address/port/login in the sketch correct? MQTT discovery enabled? The WebLog (http://<ESP-IP>/) shows the MQTT status.
- **BLE won't connect:** PS MAC correct and written in **lowercase**, SN exactly as on the device, PowerStream powered on and within range. Identifier in the nRF scan: `HW-…`.
- **WiFi gone / no failover:** for fallback across **separate subnets** be sure to use **DHCP** (Section 6). Note: the MQTT broker must be reachable/routable from the fallback network, otherwise the ESP is on WiFi but HA is not reachable.
- **In general:** the WebLog is the first place to look — it shows live where it's stuck (WiFi, MQTT or BLE). And: most "errors" resolve by re-reading the relevant section here. ;-)

### 13. Background / architecture (optional)

- **Local instead of cloud:** the PowerStream's cloud/MQTT connection drops periodically → control over
  that connection is unreliable. BLE is local, direct and independent of the internet.
- **EEPROM-friendly:** the setpoint (`permanent_watts`) is stored in the PowerStream's EEPROM. Writing it
  constantly would wear it → hence **write-on-change** (write only on a real change).
- **dynamicWatts:** the "fast" control channel `dynamicWatts` is **readable**, but **not writable via BLE**
  (it is fed cloud-/smart-plug-side). Control therefore goes through `permanent_watts`.
- **Two-channel idea:** a stable base value via BLE — the cloud-mediated channel is deliberately left
  unused, so the setup stays fully **cloud-free**.

### 14. Credits & sources

- **rabits/ha-ef-ble** (Roman Bashlovkin) — reverse engineering of the BLE protocol: frame format,
  protobuf definitions, field mappings. The central foundation of this project.
- **GnoX — "EcoFlow User ID Finder"** (https://gnox.github.io/user_id) — an easy way to obtain the User ID,
  as a supplement to the ha-ef-ble integration.
- **NimBLE-Arduino** (h2zero / Ryan Powell) — BLE stack.
- **PubSubClient** (Nick O'Leary) — MQTT client.

EcoFlow and PowerStream are trademarks of EcoFlow Technology Inc.; this project is independent and in no
way affiliated with EcoFlow.

### 15. Notes

In principle the ESP can run its BLE connection at the same time as a local solution via WiFi (TCP - MQTT) or even alongside the standard cloud connection.
What does not work is a simultaneous BLE connection from the phone (smartphone or tablet with the EF app) and the ESP, because a BLE connection is exclusive.

The state of the whole project is June 2026, tested over 4 weeks with 3 PowerStreams. 
PS main firmware: V1.0.1.228 (WiFi FW up to: 1.x.4.158) - whether EcoFlow will restrict, close or block the methods used here in the future is uncertain, but conceivable given the aggressive cloud-only expansion of their structure.

After more than 3 years of constant trouble with the control and the unreliable cloud connection, I have now had weeks of stable, quiet device control.

To everyone who wants to use the knowledge gathered here: have fun and good luck, but please read the notice at the very top first 😉

Note: I could only test it with a PowerStream - DeltaPro (Gen1) bundle. Anyone with a different Delta battery must try out whether the PS battery values appear correctly.

TIP: powering the ESP from the battery saves the power supply and supplies the ESP with power exactly when the inverter is working, too.

