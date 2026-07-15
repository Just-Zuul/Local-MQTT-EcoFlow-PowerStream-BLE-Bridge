# BLE-Fehlercodes — was bedeutet `reason=0x...` ?

Im WebLog (`http://<ESP-IP>/`) erscheint bei einer BLE-Trennung eine Zeile wie:

```
[BLE] disconnected reason=0x213
```

Diese Seite erklärt, was die Zahl bedeutet — **und was du tun kannst.**

---

## Das Wichtigste in einem Satz

Die erste Ziffer sagt, **wer** die Verbindung beendet hat:

| Code | Wer hat getrennt? |
|------|-------------------|
| **0x213** | **Der PowerStream** hat den ESP rausgeworfen |
| **0x216** | **Der ESP** hat selbst aufgelegt |
| **0x208** | **Niemand** — die Funkverbindung ist abgerissen |

Alles Weitere steht unten.

---

## Die Codes, die in der Praxis vorkommen

### 0x213 — Die Gegenstelle hat getrennt
*(Remote User Terminated Connection)*

**Der PowerStream hat den ESP rausgeworfen.** Nicht der ESP hat abgebrochen!

Meist zusammen mit `[BLE] Auth nicht bestaetigt`. Dann hat der PS die Anmeldung **nicht akzeptiert** und die Verbindung gekappt.

**Was tun:**
1. **Seriennummer (S/N) prüfen** — zeichengenau, wie auf dem Gerät. Häufigster Fehler.
2. **EcoFlow User ID prüfen** — richtiger Account, sauber kopiert.
3. **EcoFlow-App schließen** — BLE erlaubt immer nur **ein** verbundenes Gerät.

---

### 0x216 — Der ESP hat selbst getrennt
*(Connection Terminated By Local Host)*

**Der ESP hat aufgelegt** — weil er nicht weiterkam. Er trennt selbst, wenn:

- die **Anmeldung nicht bestätigt** wurde (er wartet 10 s, dann gibt er auf) → `Auth nicht bestaetigt`
- der erwartete **BLE-Dienst nicht gefunden** wurde → `no service`
- die **Merkmale fehlen** → `no chars`
- das **Abonnieren fehlschlug** → `subscribe FAIL`

**Was tun:**
1. **Seriennummer (S/N) prüfen** — ist sie falsch, kann der ESP die Antwort des PS gar nicht entschlüsseln. Die Anmeldung kann dann nie klappen.
2. **EcoFlow User ID prüfen.**
3. Bei `no service` / `no chars`: Ist das Zielgerät wirklich ein **PowerStream**? Andere Geräte haben andere Dienste.

---

### 0x208 — Funkverbindung abgerissen
*(Connection Supervision Timeout)*

Keiner hat aktiv getrennt — der **Funkkontakt ist einfach weg**. Der ESP verbindet sich danach automatisch neu.

**Was tun:**
- **Abstand verringern**, Hindernisse (Metall, Beton) vermeiden.
- **Stromversorgung** des ESP prüfen (schwaches Netzteil/Kabel → Aussetzer).
- Gelegentlich ist das **normal** und harmlos, solange die Verbindung zurückkommt.

---

### 0x23E — Verbindung kam nicht zustande
*(Connection Failed to be Established)*

Der Verbindungsaufbau selbst ist gescheitert.

**Was tun:** Abstand/Empfang prüfen; der PS war evtl. gerade anderweitig verbunden (App!). Der ESP versucht es automatisch erneut.

---

### 0x202 — Unbekannte Verbindung
*(Unknown Connection Identifier)*

Der Verbindungsversuch wurde abgebrochen, bevor er stand. Meist harmlos, wenn danach ein normaler Neuversuch folgt.

---

### 0x222 — Zeitüberschreitung auf der Funkstrecke
*(LMP/LL Response Timeout)*

Die Gegenstelle hat nicht rechtzeitig geantwortet. Ursache ist fast immer **Funkstörung oder zu große Entfernung**.

---

### 0x21F — Unspezifischer Fehler
*(Unspecified Error)*

Sammelcode ohne genaue Aussage. Kommt meist vom PS (z. B. bei einem internen Neustart). Wenn er nur vereinzelt auftritt und die Verbindung zurückkommt: ignorieren.

---

## Wenn dein Code hier nicht steht

Du kannst ihn **selbst dekodieren** — es ist eine simple Rechnung:

1. Die Codes beginnen mit **0x2..** → das ist die **HCI-Ebene** (Funk-Ebene).
2. **Die letzten zwei Stellen** sind der eigentliche Grund: `0x213` → `13`.
3. Diesen Wert in der offiziellen Liste nachschlagen (BLE-Spezifikation, „BLE Error Codes"):
   → https://mynewt.apache.org/latest/network/ble_hs/ble_hs_return_codes.html

Beispiel: `0x213` = Basis `0x200` (HCI) + `0x13` = „Remote User Terminated Connection" = die Gegenstelle hat getrennt.

---

## Kurz-Merker

| Symptom | Wahrscheinlichste Ursache |
|---------|---------------------------|
| `0x213` **oder** `0x216` **zusammen mit** `Auth nicht bestaetigt` | **S/N oder EcoFlow User ID falsch** |
| Immer `no service` / `no chars` | Zielgerät ist **kein PowerStream** |
| `0x213` sofort beim Verbinden | **EcoFlow-App** hält die BLE-Verbindung besetzt |
| `0x208` gelegentlich, Verbindung kommt zurück | **Funkstrecke/Reichweite** — meist unkritisch |

> Faustregel: Läuft BLE bis zum `connected` und scheitert dann an der Anmeldung, liegt es **fast immer an S/N oder User ID** — nicht am Board und nicht an der MAC. Denn dass überhaupt verbunden wird, beweist: die MAC stimmt und ein echtes Gerät antwortet.
