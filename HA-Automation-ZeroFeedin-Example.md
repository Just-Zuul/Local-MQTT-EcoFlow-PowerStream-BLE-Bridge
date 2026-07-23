# HA Automation — Zero Feed-in (generic example)

This example regulates the **feed-in setpoint** of the PowerStream bridge so that **grid draw settles at a
small, intentional residual value** (zero feed-in with a safety reserve). It is **generic** — you must
adapt the **entity names (EDIT ENTITY)** to your system and check the **sign of your grid sensor**.

## Prerequisites / assumptions
- A **grid power sensor** (smart meter / Shelly EM / etc.), e.g. `sensor.grid_power`
  with the convention **`+` = draw from the grid, `−` = feed into the grid**.
- The bridge's **setpoint Number**, e.g. `number.powerstream_wr1_setpoint` (name = your `DEVICE_ID`).
- Power limits as in the sketch: here **0 … 800 W** (= `MIN_WATTS … MAX_WATTS`, adjust if needed).
- before switching over to ESP control, **"feed-in control" must be enabled (ON)** in the EcoFlow app — otherwise, especially if zero feed-in is intended, the PowerStream will still export everything once the battery is full and the automation has no effect!

## The logic in brief
- **Base grid draw (reserve):** a true 0 is unstable and quickly tips into unwanted feed-in.
  A small **intentional residual draw** (`reserve`, e.g. 30 W) keeps you safely on the draw side.
  `reserve = 0` gives true zero feed-in.
- **New target value** = current setpoint + grid draw **− reserve** (draw → feed in more,
  feed-in → less; the reserve remains as residual draw).
  - Example: grid draw 200 W, `reserve` 30 → only 170 W are compensated, **30 W residual draw remain**.
- **Round to 5 W:** matches the device's 5 W grid and avoids jitter.
- **Dead band (delta gating):** only write when the target changes by **≥ 15 W** → saves unnecessary
  writes under a stable load (works hand in hand with the firmware's write-on-change).
- **Cadence:** every 10 s (more than enough). Plus **threshold-based** triggers (`numeric_state`)
  that fire only when **crossing** a threshold (>100 / <20 / <2 W) — **not** on every reading.
  A plain `state` trigger on the grid sensor, by contrast, would fire every second (or more often)
  and is therefore deliberately avoided here.

## YAML (adapt!)
```yaml
alias: PowerStream Zero Feed-in
description: Regulates the feed-in setpoint to a small residual grid draw (generic example)
trigger:
  # Base cadence - plenty for regulating
  - platform: time_pattern
    seconds: "/10"                       # every 10 seconds
  # Additionally react IMMEDIATELY at notable thresholds. numeric_state fires only when
  # CROSSING the threshold (once), not on every reading -> no constant firing:
  - platform: numeric_state
    entity_id: sensor.grid_power         # EDIT ENTITY
    above: 100                           # noticeable draw -> quickly feed in more
  - platform: numeric_state
    entity_id: sensor.grid_power         # EDIT ENTITY
    below: 20                            # almost balanced -> fine-tune
  - platform: numeric_state
    entity_id: sensor.grid_power         # EDIT ENTITY
    below: 2                             # near 0 / feed-in risk -> pull back promptly
condition: []
action:
  - variables:
      reserve: 30                                                        # intentional residual grid draw (W) - 0 = true zero feed-in
      grid: "{{ states('sensor.grid_power') | float(0) }}"               # + = draw, - = feed-in EDIT ENTITY
      cur:  "{{ states('number.powerstream_wr1_setpoint') | float(0) }}" # current setpoint EDIT ENTITY
      raw:  "{{ cur + grid - reserve }}"                                 # raw new target value (residual draw remains)
      step: "{{ (raw / 5) | round(0) * 5 }}"                             # round to 5 W
      target: "{{ [[ step, 0 ] | max, 800 ] | min }}"                    # clamp to 0..800 W
      delta: "{{ (target - cur) | abs }}"
  - condition: template
    value_template: "{{ delta >= 15 }}"                                  # dead band: only on a noticeable change
  - service: number.set_value
    target:
      entity_id: number.powerstream_wr1_setpoint
    data:
      value: "{{ target }}"
mode: single
```

## Adapt
- `reserve` (e.g. `30`) → **intentional residual grid draw in watts.** `0` = true zero feed-in;
  higher = larger safety margin against unwanted feed-in.
- `sensor.grid_power` → your grid power sensor. **Check the sign:** if your sensor gives `+` for
  *feed-in* instead of *draw*, use `raw: "{{ cur - grid - reserve }}"`.
- `number.powerstream_wr1_setpoint` → your bridge's setpoint Number (your `DEVICE_ID`).
- `0` / `800` → adjust to your `MIN_WATTS` / `MAX_WATTS`.
- `15` (dead band) and `/10` (cadence) → to taste; larger dead band = calmer, smaller = faster.
- Thresholds `above: 100` / `below: 20` / `below: 2` → the trigger points for the fast reaction.
  Choose them around your reserve point (with `reserve` 30 the target is +30 W draw).
  Optionally add a `for: "00:00:03"` per threshold if the reaction feels too nervous.

> Note: this is a **starting point**, not a finished controller for every installation. Watch the
> behaviour and adjust reserve/dead band/cadence to your load and your sensor.
