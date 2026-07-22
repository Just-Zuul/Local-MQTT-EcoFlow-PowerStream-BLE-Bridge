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

Alles Weitere nachfolgend.

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

> Steht direkt davor ein `AUTH FAIL 0xNN`? Dann ist die S/N bereits korrekt, und dieser Code sagt den wahren Grund — siehe Abschnitt **Anmeldung: `AUTH FAIL 0xNN`** weiter unten.

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

## Anmeldung: `AUTH FAIL 0xNN` — die Antwort des PowerStream

Im Log stehen oft **zwei verschiedene Zahlenräume** direkt untereinander — nicht verwechseln:

- `disconnected reason=0x2..` → **BLE-Ebene** (wer hat getrennt) — siehe oben.
- `AUTH FAIL 0x..` → **EcoFlow-Anmeldecode**: was der PowerStream zur Anmeldung sagt.

**Merksatz:** Sobald überhaupt ein `AUTH FAIL 0xNN` im Log steht, ist deine **S/N bereits korrekt** — die Antwort des PS konnte ja entschlüsselt werden (Schlüssel = `MD5(S/N)`). Der Code `NN` sagt dann, was *wirklich* klemmt:

| Code | Bedeutung | Heißt konkret |
|------|-----------|---------------|
| `0x01` | NeedRefreshToken | Token muss erneuert werden |
| `0x02` | DeviceInternalError | interner Gerätefehler |
| `0x03` | DeviceAlreadyBound | schon (woanders) gebunden |
| **`0x04`** | NeedBindInstallFirst | **Gerät ist nicht gebunden** → in der EcoFlow-App **entfernen und neu hinzufügen** |
| `0x05` | AppSendDataError | fehlerhafte Anfrage-Daten |
| **`0x06`** | WrongKey | **falscher Schlüssel → User ID (und S/N) prüfen** |
| `0x07` | MaximumDevicesError | maximale Anzahl Bindungen erreicht |

`0x00` ist **kein** Fehler — das ist der Erfolgsfall und erscheint als `>>> AUTH OK <<<`. Ein anderer, hier nicht gelisteter Wert = unbekannter Code.

**Und der entscheidende Unterschied bei `Auth nicht bestaetigt`:**

- **OHNE** vorheriges `AUTH FAIL 0xNN` (nur der 10-Sekunden-Timeout) → der PS hat **gar nicht verwertbar geantwortet** → **S/N, User ID und MAC prüfen.**
- **MIT** `AUTH FAIL 0xNN` davor → S/N stimmt, der PS hat **aktiv abgelehnt** → **den Code oben lesen** (meist `0x04` = Bind, `0x06` = Key).

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
| `Auth nicht bestaetigt` **ohne** vorheriges `AUTH FAIL` (nur Timeout) | **S/N / User ID / MAC prüfen** |
| `AUTH FAIL 0x04` | Gerät **nicht gebunden** → in der App entfernen + neu hinzufügen (S/N & ID sind ok) |
| `AUTH FAIL 0x06` | **User ID falsch** (S/N ist ok) |
| Immer `no service` / `no chars` | Zielgerät ist **kein PowerStream** |
| `0x213` sofort beim Verbinden | **EcoFlow-App** hält die BLE-Verbindung besetzt |
| `0x208` gelegentlich, Verbindung kommt zurück | **Funkstrecke/Reichweite** — meist unkritisch |

> Faustregel: Läuft BLE bis zum `connected` und scheitert dann an der Anmeldung, prüfe zuerst **S/N und User ID** — nicht Board oder MAC (dass überhaupt verbunden wird, beweist ja: MAC stimmt, echtes Gerät antwortet). **Steht dabei ein `AUTH FAIL 0xNN` im Log, sagt dieser Code den wahren Grund** — z. B. `0x04` = nicht gebunden (S/N & ID sind dann bereits ok).
