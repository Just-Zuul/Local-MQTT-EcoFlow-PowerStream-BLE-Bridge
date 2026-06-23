# HA-Automatisierung — Nulleinspeisung (generisches Beispiel)

Dieses Beispiel regelt den **Einspeise-Sollwert** der PowerStream-Bridge so, dass der **Netzbezug
gegen einen kleinen, gewollten Restwert** geht (Nulleinspeisung mit Sicherheitsreserve). Es ist
**generisch** — du musst die **Entitätsnamen (EDIT ENTITÄT)** an dein System anpassen und das
**Vorzeichen deines Netz-Sensors** prüfen.

## Voraussetzungen / Annahmen
- Ein **Netz-Leistungssensor** (Smartmeter / Shelly EM / o. ä.), z. B. `sensor.netz_leistung`
  mit der Konvention **`+` = Bezug aus dem Netz, `−` = Einspeisung ins Netz**.
- Die **Sollwert-Number** der Bridge, z. B. `number.powerstream_wr1_sollwert` (Name = deine `DEVICE_ID`).
- Leistungsgrenzen wie im Sketch: hier **0 … 800 W** (= `MIN_WATTS … MAX_WATTS`, ggf. anpassen).

## Die Logik in Kurzform
- **Grund-Netzbezug (Reserve):** Eine echte 0 ist instabil und kippt schnell in ungewollte Einspeisung.
  Ein kleiner **gewollter Restbezug** (`reserve`, z. B. 30 W) hält dich sicher auf der Bezugsseite.
  `reserve = 0` ergibt echte Nulleinspeisung.
- **Neuer Zielwert** = aktueller Sollwert + Netzbezug **− reserve** (Bezug → mehr einspeisen,
  Einspeisung → weniger; die Reserve bleibt als Restbezug stehen).
  - Beispiel: Netzbezug 200 W, `reserve` 30 → es werden nur 170 W ausgeglichen, **30 W Restbezug bleiben**.
- **Auf 5 W runden:** passt zum 5-W-Raster des Geräts und vermeidet Zappeln.
- **Totband (Delta-Gating):** nur schreiben, wenn sich der Zielwert um **≥ 15 W** ändert → spart unnötige
  Schreibvorgänge bei stabiler Last (greift Hand in Hand mit dem Write-on-Change der Firmware).
- **Takt:** alle 10 s (reicht völlig). Zusätzlich **schwellenbasierte** Trigger (`numeric_state`),
  die nur beim **Über-/Unterschreiten** einer Schwelle feuern (>100 / <20 / <2 W) — **nicht** bei
  jedem Messwert. Ein einfacher `state`-Trigger auf den Netz-Sensor würde dagegen im Sekundentakt
  (oder öfter) feuern und ist deshalb hier bewusst vermieden.

## YAML (anpassen!)
```yaml
alias: PowerStream Nulleinspeisung
description: Regelt den Einspeise-Sollwert auf einen kleinen Rest-Netzbezug (generisches Beispiel)
trigger:
  # Grundtakt - reicht voellig zum Nachregeln
  - platform: time_pattern
    seconds: "/10"                       # alle 10 Sekunden
  # Zusaetzlich SOFORT bei markanten Schwellen. numeric_state feuert nur beim
  # KREUZEN der Schwelle (einmalig), nicht bei jedem Messwert -> kein Dauerfeuer:
  - platform: numeric_state
    entity_id: sensor.netz_leistung      # EDIT ENTITÄT
    above: 100                           # spuerbarer Bezug -> schnell mehr einspeisen
  - platform: numeric_state
    entity_id: sensor.netz_leistung      # EDIT ENTITÄT
    below: 20                            # fast ausgeglichen -> nachjustieren
  - platform: numeric_state
    entity_id: sensor.netz_leistung      # EDIT ENTITÄT
    below: 2                             # quasi 0 / Einspeise-Gefahr -> zuegig zurueck
condition: []
action:
  - variables:
      reserve: 30                                                       # gewollter Rest-Netzbezug (W) - 0 = echte Nulleinspeisung
      grid: "{{ states('sensor.netz_leistung') | float(0) }}"            # + = Bezug, - = Einspeisung EDIT ENTITÄT
      cur:  "{{ states('number.powerstream_wr1_sollwert') | float(0) }}" # aktueller Sollwert EDIT ENTITÄT
      raw:  "{{ cur + grid - reserve }}"                                 # roher neuer Zielwert (Restbezug bleibt stehen)
      step: "{{ (raw / 5) | round(0) * 5 }}"                             # auf 5 W runden
      target: "{{ [[ step, 0 ] | max, 800 ] | min }}"                    # auf 0..800 W begrenzen
      delta: "{{ (target - cur) | abs }}"
  - condition: template
    value_template: "{{ delta >= 15 }}"                                  # Totband: nur bei spuerbarer Aenderung
  - service: number.set_value
    target:
      entity_id: number.powerstream_wr1_sollwert
    data:
      value: "{{ target }}"
mode: single
```

## Anpassen
- `reserve` (z. B. `30`) → **gewollter Rest-Netzbezug in Watt.** `0` = echte Nulleinspeisung;
  höher = größerer Sicherheitsabstand gegen ungewollte Einspeisung.
- `sensor.netz_leistung` → dein Netz-Leistungssensor. **Vorzeichen prüfen:** Liefert dein Sensor `+` bei
  *Einspeisung* statt bei *Bezug*, dann `raw: "{{ cur - grid - reserve }}"` verwenden.
- `number.powerstream_wr1_sollwert` → die Sollwert-Number deiner Bridge (deine `DEVICE_ID`).
- `0` / `800` → an deine `MIN_WATTS` / `MAX_WATTS` anpassen.
- `15` (Totband) und `/10` (Takt) → nach Geschmack; größeres Totband = ruhiger, kleineres = schneller.
- Schwellen `above: 100` / `below: 20` / `below: 2` → die Auslöse-Punkte für die schnelle Reaktion.
  Sinnvoll rund um deinen Reserve-Punkt wählen (bei `reserve` 30 liegt das Ziel bei +30 W Bezug).
  Optional je Schwelle ein `for: "00:00:03"` ergänzen, falls dir die Reaktion zu nervös ist.

> Hinweis: Dies ist ein **Startpunkt**, kein fertiger Regler für jede Anlage. Beobachte das Verhalten und
> passe Reserve/Totband/Takt an deine Last und deinen Sensor an.
