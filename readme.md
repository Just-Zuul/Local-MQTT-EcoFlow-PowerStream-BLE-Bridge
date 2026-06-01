# Why this project starts

After repeatedly running into the same issue over the course of weeks, months, and now almost two years, I've come to the conclusion that neither the EcoFlow cloud connection nor the currently available local PowerStream integrations can be considered truly reliable.

I'm spending several weeks investigating the root cause, I'm now about 98% convinced that the issue originates from the Wi-Fi component of the PowerStream firmware itself—or more specifically from the ESP32-based IoT module used in certain firmware versions.

---

# Firmware observations

I even asked EcoFlow support to downgrade the firmware on my PowerStreams. Unfortunately, the connection loss issue remained present on several older firmware versions as well.

Some users report stable operation with firmware **1.1.4.64**, while others mention that everything still works simply because they have not updated since early 2025.

> The second version number depends on the installed IoT module, which is why firmware branches such as **1.0.4.x** and **1.1.4.x** exist.

### Recommendation

⚠️ If you are still running an older firmware version, keep it.

Based on my experience, firmware newer than approximately **~1.x.4.70** is unlikely to work reliably in local mode.

---

# What I found

While investigating the problem, I managed to reverse-engineer the key exchange process and decrypt the communication between the PowerStream and the EcoFlow cloud.

However, my goal was never to completely reverse-engineer the cloud protocol. I simply wanted a reliable local solution.

Even with four PowerStreams using two different IoT module variants, I consistently observed the same behavior:

* The PowerStream closes its connection approximately every 6–8 minutes.
* During that process, it briefly disconnects from Wi-Fi.
* The behavior occurs across multiple firmware branches.
* Disconnect intervals are highly inconsistent.

---

# Cloud mode behavior

When operating through the EcoFlow cloud:

1. The PowerStream goes offline.
2. Opening the EcoFlow app causes it to reconnect.
3. This explains why the commonly used workaround of automatically opening the app every few minutes actually works.

---

# Local decoder behavior

When operating locally through the decoder:

* There is no cloud-triggered recovery.
* Sometimes the connection remains stable for hours.
* Sometimes disconnects occur every 2, 4, or 8 minutes.
* Some interruptions last only seconds.
* Others may persist for hours.

Because the behavior is so inconsistent, troubleshooting becomes nearly impossible.

---

# The solution: BLE instead of Wi-Fi

At some point I stopped trying to make the Wi-Fi implementation work reliably and switched to a Bluetooth Low Energy (BLE) based approach.

After some development work, the result turned out to be exceptionally stable, fast, and reliable.

The concept is simple:

* ESP32-WROOM (~€8) acts as a BLE bridge for the PowerStream.
* Connects to your existing Wi-Fi network.
* Integrates directly with Home Assistant via MQTT.
* No VLANs, proxies, containers, cloud services, or custom network setup required.
* Firmware flashing takes about 10 minutes.
* Provides local control and access to nearly all PowerStream values.
* Automatic Home Assistant entity creation through MQTT Auto Discovery.
* Ready for automation within minutes.

---

# Benefits

✅ Fully local operation

✅ No cloud dependency

✅ Fast and reliable communication

✅ Home Assistant Auto Discovery

✅ No recurring disconnect issues

✅ Powered directly from the USB port of a Delta battery

---

# Requirements

| Item        | Cost        |
| ----------- | ----------- |
| ESP32-WROOM | ~€8–10      |
| Setup time  | ~15 minutes |

Result: a fully local, reliable, and maintenance-free PowerStream integration.

---

# Interested?

If there is enough community interest, I will publish:

* Source code
* Wiring information
* Flashing instructions
* Complete setup guide

Please use the **Discussions** tab for questions, feedback, testing results, and community exchange.


[PowerStream Controller Discussion](https://github.com/Just-Zuul/Local-MQTT-EcoFlow-PowerStream-BLE-Bridge/discussions/1)
