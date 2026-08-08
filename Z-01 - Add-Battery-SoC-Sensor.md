# Adding a Battery SoC sensor

The PowerStream already sends the battery **state of charge** in its heartbeat — the
firmware just doesn't publish it by default. Adding it is **one line**, nothing else.

## Steps

1. Open your firmware file `ps-esp-ble-bridge-v_2_6_3_blanco_*.ino` in a text editor
   (Notepad++, Arduino IDE, …).

2. Find the sensor table: search for **`Batterie Leistung`** (or `SENSORS[] = {`).

3. Add this line **inside** that list — easiest right after the `Batterie Leistung` line:

   ```cpp
   { "bat_soc", "Batterie SoC", 31, 1, "%", "battery" },
   ```

   **Before:**
   ```cpp
   { "bat_input_watts","Batterie Leistung",        29, 10,  "W",  "power"   },
   };
   ```

   **After:**
   ```cpp
   { "bat_input_watts","Batterie Leistung",        29, 10,  "W",  "power"   },
   { "bat_soc",        "Batterie SoC",             31,  1,  "%",  "battery" },
   };
   ```

4. **Save**, flash the ESP — done.

Home Assistant creates a new sensor **"Batterie SoC"** automatically (0–100 %).
The sensor count in the WebLog header goes up by one on its own.

## Notes

- The value is heartbeat **field 31**, which the firmware already reads — that's why one line is enough.
- The `1` is the divisor: the value is already whole-number percent, so no scaling is needed.
- The name `"Batterie SoC"` is free — rename it if you like; the entity id follows the name.
- Keep the comma at the end of the line, and keep the line **inside** the `{ … };` block.
- This is the BMS SoC (integer %). A finer "display" SoC with decimals exists, but it lives in a
  second heartbeat type the firmware doesn't decode — not worth the effort for a simple readout.
