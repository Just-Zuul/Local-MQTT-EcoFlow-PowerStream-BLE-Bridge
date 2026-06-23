## SETUP PS - ESP - BRIDGE - Deutsche Projektbeschreibung 
> 🇬🇧 [English Version](SETUP_PS_HW51_BLE_BRIDGE_final_EN.md)


## ⚠️ Hinweis & Haftungsausschluss

Dieses Repository beschreibt ausschließlich, wie ich persönlich meine EcoFlow PowerStream
Microinverter mittels eines ESP-Mikrocontrollers direkt und lokal über BLE (Bluetooth Low
Energy) steuere und auslese.

**Es handelt sich weder um eine Anleitung noch um eine Aufforderung zum Nachbau.**

### Nutzung auf eigenes Risiko

Wer Inhalte dieses Repositories – ganz oder in Teilen – verwendet, umsetzt, nachbaut oder
anderweitig nutzt, tut dies **ausschließlich auf eigene Gefahr und eigenes Risiko**.

Von jeder Person, die diese Inhalte verwendet, wird vorausgesetzt, dass sie über das
notwendige Grundlagenwissen in den Bereichen Elektronik, Embedded Systems, Elektroinstallation
sowie den jeweils geltenden gesetzlichen Vorschriften verfügt.

### Haftungsausschluss

Der Autor übernimmt **keinerlei Haftung** für:

- Schäden an Personen, Tieren oder Sachen
- Schäden an Geräten, Anlagen oder der Elektroinstallation
- Datenverluste oder Fehlfunktionen
- Folgeschäden jeglicher Art
- Verstöße gegen Hersteller-Garantiebedingungen oder Zertifizierungen
- Verstöße gegen gesetzliche Vorschriften oder Normen

Dies gilt unabhängig davon, ob Schäden durch direkte Nutzung, Modifikation oder fehlerhafte
Umsetzung der hier beschriebenen Methoden entstehen.

### Keine Gewährleistung

Die hier bereitgestellten Informationen werden **ohne jegliche Gewährleistung** bereitgestellt –
weder ausdrücklich noch stillschweigend. Es wird keine Garantie für Richtigkeit,
Vollständigkeit, Aktualität oder Funktionsfähigkeit übernommen.

# Marken & Drittanbieter

EcoFlow und PowerStream sind eingetragene Marken der EcoFlow Technology Inc. Dieses Projekt
steht in keiner Verbindung zu EcoFlow und wird von EcoFlow weder unterstützt noch genehmigt.

## Inhaltsverzeichnis 

# PowerStream HW51 ↔ ESP32 BLE-Bridge — Einrichtungsabfolge

 1. Überblick — was die Bridge macht & warum (lokal per BLE, ohne Cloud/Decoder), was sie steuert
 2. Funktionsumfang (Kurz-Feature-Liste: Telemetrie, Steuerung, Failsafe, Multi-WLAN, WebLog …)
 3. Voraussetzungen: Hard- & Software
 4. Arduino-Bibliotheken (+ micro-ecc-Troubleshooting, NimBLE 2.x)
 5. Benötigte Daten
   • Daten ermitteln (How-To): PowerStream-BLE-MAC (nRF Connect), User ID (GnoX), AP-BSSID
 6. Konfiguration — der KONFIG-Block Schritt für Schritt
   • WIFI_CREDS (Prioritätsliste) · WIFI_MODE · AP_BSSID
   • Statische IP vs. DHCP (inkl. Cross-Range-Hinweis)
   • MQTT: Broker / Port / User / PW
   • Geräte-Identität: SN · BLE-MAC · USER_ID · DEVICE_ID
   • Failsafe-Sollwert: FAILSAFE_WATTS (50 W)
 7. Flashen (Board wählen, Port, Upload-Speed, CP210x-Treiber)
 8. Home Assistant — was automatisch erscheint (MQTT-Discovery): Sensoren · Controls · BLE Link · Diagnose · deaktivierte Sensoren · bp_type-Bedeutung
 9. Bedienung & Features — Sollwert · Brightness · Supply-Mode · Restart-Button · WebLog (Browser -> ESP-IP)
10. Verhalten erklärt (das „Warum"): Write-on-Change (EEPROM-Schonung) · Availability (BLE Link „Getrennt") · WLAN-Failover (Best-at-Boot + Re-Lock) · Failsafe-Logik · Multi-WLAN-Priorität
11. HA-Automatisierung (Beispiel): Nulleinspeisung · 5-W-Rundung · Delta-Gating
12. Troubleshooting: NimBLE-Version · micro-ecc · Flash-Abbruch · BLE-Connect · WLAN/Broker-Erreichbarkeit (Cross-Range)
13. Hintergrund/Architektur (optional): lokal statt Cloud · EEPROM/EF-Support · dynamicWatts nicht BLE-schreibbar · Zwei-Kanal-Design
14. Credits & Quellen: rabits/ha-ef-ble · GnoX User-ID-Finder
15. Anmerkungen


### 1. Überblick — was die Bridge macht & warum

Die Bridge ist eine Firmware für einen kleinen ESP32-Mikrocontroller.
Der ESP verbindet sich **lokal über Bluetooth (BLE)** mit einem EcoFlow PowerStream und übernimmt zwei Aufgaben:

- **Auslesen:** Er liest laufend die Messwerte des PowerStream (Leistung, Batterie-Fluss, Status …).
- **Steuern:** Er setzt den **Einspeise-Sollwert** (und ein paar weitere Stellgrößen) direkt am Gerät.

Alles wird per **MQTT an Home Assistant** gemeldet, wo Sensoren und Bedien-Elemente automatisch
erscheinen.

**Warum lokal über BLE und nicht über die Cloud?** 
Mein System, mein Gerät, meine Daten, meine Kontrolle + mehr Zuverlässigkeit beim Steuern und alle Daten in Home Assistant.
Die Cloud-/MQTT-Verbindung des PowerStream bricht immer wieder ab — dann kommen Steuerbefehle verzögert oder gar nicht an. 
BLE ist eine **direkte, lokale Funkverbindung**: unabhängig von Internet, EcoFlow-Cloud und deren Servern.
Das Ergebnis ist eine schnelle, zuverlässige Regelung, die auch ohne Internet (und ohne EcoFlow) weiterläuft.

### 2. Funktionsumfang (Kurzüberblick)

- **Volle Telemetrie:** Leistung, Batterie-Fluss, Temperaturen, Statuswerte … als HA-Sensoren.
- **Steuerung:** Einspeise-Sollwert (W), LED-Helligkeit, Supply-Priorität, ESP-Neustart per Knopf.
- **Write-on-Change:** Der Sollwert wird nur geschrieben, wenn er sich ändert → schont das Gerät.
- **WLAN-Failsafe:** Fällt das WLAN länger aus, setzt der ESP autonom einen sicheren Wert (50 W).
- **WLAN-Failover + Multi-WLAN:** mehrere Netze als Prioritätsliste; automatischer AP-Wechsel bei Ausfall.
- **Eingebauter WebLog:** kleine Status-Webseite direkt auf dem ESP (Live-Status BLE/WLAN/MQTT + Log).
- **Auto-Integration in Home Assistant** über MQTT-Discovery — keine manuelle Entity-Anlage nötig.

### 3. Voraussetzungen, verwendete Hard- und Software
- PC (Windows/macOS/Linux)
- Arduino IDE — https://www.arduino.cc/en/software/
- ESP32-WROOM Modul — hier ein ESP32-WROOM Development Board mit CP2102-Chip, 38-Pin, USB-C (Bezug beim großen A oder dem Händler deines Vertrauens)
- Passendes USB-Kabel (hier USB-A auf USB-C)
- USB-Treiber: Für Boards mit **CP2102** ggf. den **CP210x VCP-Treiber von Silicon Labs** installieren — sonst erscheint am PC kein COM-Port.
- Dieses Repo

### 4. Arduino-Bibliotheken
Die Firmware benötigt zwei manuell zu installierende Bibliotheken.

**Manuell installieren (Arduino → Bibliotheksverwalter):**
- **PubSubClient** — von Nick O'Leary („knolleary") → MQTT-Client (`PubSubClient.h`)
- **NimBLE-Arduino** — von h2zero (Ryan Powell) → BLE-Stack (`NimBLEDevice.h`)
  - ⚠️ NimBLE-Arduino hat zwischen 1.x und 2.x eine spürbar geänderte API. **Es wird zwingend Version 2.x benötigt** (z. B. 2.5.0) — mit 1.x gibt es Compile-Fehler.

**Kommt mit dem _ESP32-Board-Paket von Espressif Systems_** (kein separater Install — nur das Board-Paket im *Boardverwalter* installieren):
- `WiFi.h`
- `WebServer.h`
- `MD5Builder.h`
- `mbedtls/aes.h` (mbedTLS, in der ESP-IDF/Core gebündelt)
- `esp_system.h` (`esp_reset_reason()`)

> **Troubleshooting micro-ecc:** Ein Async-WebServer wird **nicht** benötigt (der Code nutzt das synchrone Core-`WebServer.h`). Falls `micro-ecc` (z. B. aus einem anderen Projekt) installiert ist und es Linker-Konflikte mit NimBLE gibt, die Standalone-`micro-ecc` **entfernen oder umbenennen** — NimBLE 2.x bringt seine eigene Krypto mit.

### 5. Benötigte Daten zum Einrichten (minimum):

- WLAN-SSID
- WLAN-Passwort

- MAC-Adresse des **2,4-GHz**-Netzes des nächstgelegenen bzw. gewünschten WLAN-APs (optional *1)

- MAC-Adresse des PowerStream (z.B. per App **nRF Connect** auslesen)
- Seriennummer (SN) des PowerStream

- EcoFlow **User ID** — am einfachsten über den **„EcoFlow User ID Finder" von GnoX**: https://gnox.github.io/user_id (Ergänzung zur ha-ef-ble-Integration). Login mit EcoFlow-Zugangsdaten; die Anfragen laufen **direkt im Browser an EcoFlows offizielle API** (keine Weitergabe an Dritt-Server). EU-Konten: Region **`api-e.ecoflow.com`** oder „Auto" wählen.

- MQTT-Broker: Adresse, Port, Benutzername und Passwort (sofern mit Benutzer/Passwort gearbeitet wird)

- Gewünschte **statische IP** für den ESP im Netzwerk (ggf. mit Gateway/Subnetzmaske), falls statische Konfiguration genutzt wird *2

- **DEVICE_ID** — frei wählbarer Gerätename (z. B. `WR1-PS1234`); taucht in den MQTT-Topics und HA-Entitäten auf und benennt eindeutig die Entitäten (Hinweis: Die Entitäten werden dann zu: sensor.powerstream_<device_id>_entitätsname )
- *(optional)* **Zeitzone** (TZ-String; Default Mitteleuropa `CET-1CEST,M3.5.0,M10.5.0/3`)

> *1 Die ESP-Firmware bietet die Möglichkeit, die WLAN-Verbindung gezielt zu beeinflussen → siehe Abschnitt **WLAN** in Punkt 6.
> *2 Möglich sind DHCP oder feste IP (feste IP ist empfohlen! - Hinweise zu Fallback-WLAN beachten!) 



Wenn man alle Daten zusammen hat, Arduino soweit installiert ist, die wichtigsten Einstellungen anpassen:


<img width="613" height="812" alt="grafik" src="https://github.com/user-attachments/assets/55312555-a6ef-41ba-8555-044e76e51109" />



Anschließend das Arduinoscript: **ps-esp-ble-bridge-v_2_6_3_blanco.ino** mit einem Editor öffnen (z.B. Notepad++), um die Angaben anzupassen.

### 6. Konfiguration - Aufbau des Kopfes mit den anzupassenden Angaben:


ZEILE 87 = // =================== KONFIG  (ab hier ausfuellen) =================== = Beginn der Konfiguration  

ZEILE 94 = Hauptnetzwerk WLAN und Passwort  
ZEILE 95 bis 96 = WLAN SSID und Passwörter hinterlegen, sofern Fallback gewollt ist... Sonst Zeile mit // auskommentiert, (damit die folgenden Zeilennummern weiterhin stimmen)  
```
const WifiCred WIFI_CREDS[] = {
  { "SSID 1", "PASS 1" },                    // Prio 1 (bevorzugt, z.B. dedizierter Router oder AP)
  { "SSID 2", "PASS 2" },                    // Prio 2 (Fallback, z.B. Mesh) - bei nur 1 Netz -> Zeile auskommentieren
  { "SSID 3", "PASS 3" },                    // Prio 3 (mögliches weiteres Fallback) - bei nur 1 Netz -> Zeile auskommentieren  
};
```
WIRD bei einem verwendeten Netz zu:
```
const WifiCred WIFI_CREDS[] = {
  { "GIVEN-SSID 1", "GIVEN-PASS 1" },        // Prio 1 (bevorzugt, z.B. dedizierter Router oder AP)
//  { "SSID 2", "PASS 2" },                  // Prio 2 (Fallback, z.B. Mesh) - bei nur 1 Netz -> Zeile auskommentieren
//  { "SSID 3", "PASS 3" },                  // Prio 3 (mögliches weiteres Fallback) - bei nur 1 Netz -> Zeile auskommentieren  
};

```
```
ZEILE 100 - 106:
Z100: // WLAN-Roaming-Schutz :
Z101: //   0 = Auto   1 = Best-at-Boot (empfohlen)   2 = Hard-Lock (feste BSSID)
Z102: #define WIFI_MODE  1
Z103: static const char* AP_NAME    = "SAMPLE-AP";
Z104: static uint8_t     AP_BSSID[6] = { 0xA1, 0xB2, 0xC3, 0xAA, 0xBB, 0xCC };
Z105: 
Z106: // SAMPLE-AP = A1:B2:C3:AA:BB:CC   -> hier kann man sich vermerken welchen AP man verwendet und welche MAC er hat

Z102 = den Modus 0 oder 1 verwenden, wenn mehr als ein Wifi-Netz möglich ist oder verwendet wird (Zeile 95 - 96 befüllt) Modus 2 nur bei **einem** Netzwerk und fest zugewiesenem AP (Achtung ! neuer AP = ESP neu flashen !)
Z103 = Der AP Name, der bei Modus 2 angezeigt wird (oder als Hilfsangabe bei 1 wenn man weiß, welcher der nächste AP ist)
Z104 = die MAC des zu bindenden AP bei Modus 2 (oder als Anzeige, welcher AP der nächste ist, wenn 0 oder 1 verwendet wird - wird in den Diagnosedaten angezeigt)
```
```
ZEILE 109 - 115:
Z109: // Optionale statische IP (pro Geraet anpassen!). false = DHCP.
Z110: static const bool USE_STATIC_IP = true;                // !!!! <----- !! BEACHTEN !! - !! WICHTIG !! ---- !!!!!
Z111: IPAddress local_IP(192, 168, 100, 100);                // !!!!! pro Geraet aendern
Z112: IPAddress gateway (192, 168, 100,   1);
Z113: IPAddress subnet  (255, 255, 255,   0);
Z114: IPAddress dns1    (192, 168, 100,   1);
Z115: IPAddress dns2    (192, 168, 100,   1);
```
ZEILE 110 = Feste IP (true) oder DHCP (false) nutzen. Feste IP ist Default und empfohlen, solange **alle genutzten Netze im selben IP-Bereich** liegen (auch mehrere APs/SSIDs im selben Netz sind damit kein Problem).  
Nur wenn Fallback-Netze in **unterschiedlichen IP-Bereichen/Subnetzen** liegen, MUSS DHCP (USE_STATIC_IP = false) verwendet werden.  

Minimum Konfiguration für WLAN / NETZWERK Einstellungen: EINE SSID+PASS, WIFI MODE 1 (default), Zeile 111 - 115 für  feste IP  

Hinweis: der eingebaute Webserver ist unter der IP des ESP zu erreichen. eine feste IP vereinfacht die Nutzung.  

 
ZEILE 117 - 121:  
- die vorhandenen MQTT Einstellungen vervollständigen / anpassen  
```
ZEILE 123 - 124:
Z123: //  --- EcoFlow User ID ---
Z124: const char* EF_USER_ID = "0000000000000000000";        // EcoFlow UserID (fuer alle Geraete gleich)
```
**Die User ID kann man recht einfach mit Hilfe des Tools unter :  https://gnox.github.io/user_id   ermitteln.**
```
ZEILE 126 - 129:
Z126: // --- NTP / Zeit ---
Z127: static const char* NTP_SERVER_1 = "192.168.100.1";               // Router oder anderer Zeitgeber im lokalen LAN
Z128: static const char* NTP_SERVER_2 = "pool.ntp.org";                // Fallback - öffentlicher Zeitgeber
Z129: static const char* TZ_STRING    = "CET-1CEST,M3.5.0,M10.5.0/3";  // Zeitzone : CET/CEST inkl. Sommerzeit
```
Anpassen der Zeitserver und der Zeitzone (voreingestellt Zentraleuropa)


```
ZEILE 131 - 134:
Z131: //  --- pro Geraet individuell ---
Z132: const char* PS_SN     = "HW51ZOH4PS000000";    // Seriennummer des Inverters
Z133: const char* PS_MAC    = "77:66:ef:44:zz:ee";   // BLE-MAC des Inverters (kleinschreibung!!)
Z134: const char* DEVICE_ID = "WR0-PS0000";          // kurze ID -> Topics + MQTT-ClientID - Zusammensetzung wird: PowerStream-WR0-PS0000
```
Die Seriennummer MUSS zwingend mit dem Inverter übereinstimmen!!  
Die BLE-MAC des PowerStream kann man einfach mit der App "nRF" ermitteln. Der Inverter zeigt sich beim Scan mit seiner Kennung "HW-1234". ZEILE 133 : **KLEINSCHREIBUNG !! beachten**  
Die Device ID wird für die Benennung der Entität verwendet. Nur eine "1" würde zu "PowerStream-1"   

```
ZEILE 149 - 152:
Z150: const int      FAILSAFE_WATTS   = 50;          // sicherer Sollwert, wenn WLAN laenger weg ist (wird per BLE gesetzt)
```
Hier lässt sich die Einspeiseleistung für die Noteinstellung bei WLAN-Verlust anpassen (Grundverbrauchswert oder Grundeinspeisewert).  


 ### 7. Flashen (Board wählen, Port, Upload-Speed, CP210x-Treiber)

Nach vollständiger Anpassung der Vorlage mit allen notwendigen Daten den Code in die Arduino-Oberfläche kopieren, das Board anschließen und flashen.

Voraussetzungen wurden schon in Punkt 3 behandelt.

Schritt für Schritt:

1. **Board wählen:** Werkzeuge → Board → ESP32 Arduino → **„ESP32 Dev Module"** (passt für gängige ESP32-WROOM-Boards).
2. **Port wählen:** Werkzeuge → Port → den COM-Port des ESP auswählen.
   - Kein Port sichtbar? → CP210x-Treiber fehlt (siehe Punkt 3) oder ein reines Ladekabel ohne Datenadern.
3. **Upload-Speed:** Standard (115200) genügt. Bricht der Upload ab, einen niedrigeren Wert (z. B. 921600 → 115200) wählen.
4. **Hochladen:** auf den Pfeil („Hochladen") klicken. Der Code wird kompiliert und auf den ESP geschrieben.
   - Bleibt es bei „Connecting……" hängen: am ESP die **BOOT-Taste** gedrückt halten, bis der Upload startet (manche Boards brauchen das).
5. **Fertig:** Bei Erfolg meldet Arduino „Done uploading" bzw. „Hard resetting via RTS pin". Der ESP startet neu und beginnt sich zu verbinden (WLAN → MQTT → BLE).

**Kontrolle:** Im seriellen Monitor (115200 Baud) bzw. später im WebLog (http://<ESP-IP>/) sieht man den Startverlauf: WLAN-Verbindung, MQTT, dann den BLE-Connect zum PowerStream.

### 8. Home Assistant — was automatisch erscheint (MQTT-Discovery)

Voraussetzung: In HA ist die **MQTT-Integration** eingerichtet (gleicher Broker wie im Sketch) und
**Discovery aktiviert** (Standard). Nach dem Flashen meldet sich der ESP selbst an — es ist
**keine manuelle Entity-Anlage** nötig.

- **Gerät:** erscheint als **`PowerStream-<DEVICE_ID>`** (z. B. `PowerStream-WR1-PS1234`).
- **Entitäten-Benennung:** `sensor.powerstream_<device_id>_<name>` (analog für number/select/button).
- **Was kommt:**
  - **~46 Sensoren** — Telemetrie aus dem PowerStream (Leistungen, Batterie-Fluss, Temperaturen, Statuswerte …).
  - **Steuerungen** — Sollwert (Number), LED-Helligkeit (Number), Supply-Mode (Select), Neustart (Button).
  - **„BLE Link"** — Verbindungsstatus zum PowerStream (siehe Punkt 10, Availability).
  - **~18 Diagnose-Entitäten** — RSSI, IP, Uptime, verbundener AP, Reset-Grund usw.
- **Standardmäßig deaktivierte Sensoren:** Inverter-Temperatur (liefert immer 0), Installation Country/Town —
  sie sind vorhanden, aber ausgeblendet; bei Bedarf in HA aktivieren.
- **`bp_type`:** Inverter-Status — Wert **`2`** = (Delta-)Batterie verbunden/erkannt.

### 9. Bedienung & Features

- **Sollwert (W):** die Einspeiseleistung. Wird über die Number-Entity gesetzt (manuell oder per Automatisierung, Punkt 11). Schreibt nur bei echter Änderung (Punkt 10).
- **LED-Helligkeit:** Helligkeit der Geräte-LED (0–1023).
- **Supply-Mode:** Versorgungs-Priorität (z. B. Netzteil vs. Batterie).
- **Neustart-Knopf:** startet den ESP neu — praktisch, um z. B. die Best-at-Boot-AP-Wahl neu auszulösen (Punkt 10).
- **WebLog:** Aufruf über `http://<ESP-IP>/` im Browser. Zeigt Live-Status (BLE/WLAN/MQTT als Karten) und die letzten Log-Zeilen — auch **ohne** Home Assistant erreichbar. Eine feste IP erleichtert den Zugriff.

### 10. Verhalten erklärt (das „Warum")

- **Write-on-Change:** Der Sollwert wird nur geschrieben, wenn er sich ändert (oder Ist ≠ Soll). Im
  Ruhezustand also **null Schreibvorgänge** → das schont den internen Speicher (EEPROM) des PowerStream.
- **Availability (BLE Link):** „BLE Link" zeigt den **echten** Zustand — „Verbunden"/„Getrennt", bei
  ESP-Ausfall via Last-Will „Getrennt". PS-Sensoren werden **„nicht verfügbar"**, wenn keine BLE-Daten
  ankommen — so stehen keine eingefrorenen Phantomwerte in HA.
- **WLAN-Failover (Best-at-Boot + Re-Lock):** Beim Start wählt der ESP den stärksten erreichbaren AP und
  „lockt" darauf. Fällt dieser AP länger aus (≥ 30 s), sucht er **automatisch neu** (Rescan) — kein
  manueller Reset mehr nötig.
- **WLAN-Failsafe:** Ist das WLAN durchgehend ≥ 15 s weg (und BLE vorhanden), setzt der ESP **autonom
  einen sicheren Sollwert (50 W)**, damit der Inverter ohne Home Assistant nicht unkontrolliert mit dem
  letzten Wert weiterspeist. Kommt das WLAN zurück, übernimmt HA wieder.
- **Multi-WLAN-Priorität:** Mehrere Netze möglich (Punkt 6). Der ESP bevorzugt das **oberste erreichbare**
  Netz der Liste (z. B. eine dedizierte Box) und fällt sonst auf das nächste (z. B. Mesh) zurück — jeweils
  mit dem passenden Passwort.

### 11. HA-Automatisierung (Beispiel)

Die Bridge stellt nur die **Stellgröße** bereit (den Sollwert). Die eigentliche Regel-Logik — etwa eine
**Nulleinspeisung** (Netzbezug auf ~0 regeln) — gehört in eine **Home-Assistant-Automatisierung**.

Ein **generisches, anpassbares Beispiel** (Nulleinspeisung mit 5-W-Rundung und Delta-Totband) liegt in der
separaten Datei:

> **`HA-Automatisierung-Nulleinspeisung-Beispiel.md`**

Dort steht das fertige YAML samt Erklärung der Logik — Entitätsnamen und das Vorzeichen des Netz-Sensors
müssen an das eigene System angepasst werden.

### 12. Troubleshooting (bekannte Fehler & einfache Behebung)

- **Kein COM-Port in Arduino:** CP210x-VCP-Treiber fehlt (Punkt 3) — oder ein reines Ladekabel ohne Datenadern. Anderes Kabel/Port testen.
- **Compile-Fehler an einem BLE-Callback (z. B. `onDisconnect`):** falsche NimBLE-Version. **NimBLE-Arduino 2.x** verwenden, nicht 1.x (Punkt 4).
- **Linker-Fehler / doppelte Symbole (micro-ecc):** Standalone-`micro-ecc` aus einem anderen Projekt kollidiert. Entfernen oder umbenennen — NimBLE 2.x bringt eigene Krypto mit.
- **Upload startet nicht / bricht ab:** beim „Connecting……" die **BOOT-Taste** halten; Upload-Speed senken; richtiges Board („ESP32 Dev Module"); kürzeres/anderes USB-Kabel.
- **ESP erscheint nicht in HA:** MQTT-Integration aktiv? Broker-Adresse/Port/Login im Sketch korrekt? MQTT-Discovery aktiviert? WebLog (http://<ESP-IP>/) zeigt den MQTT-Status.
- **BLE verbindet nicht:** PS-MAC korrekt und **klein** geschrieben, SN exakt wie auf dem Gerät, PowerStream eingeschaltet und in Reichweite. Kennung beim nRF-Scan: `HW-…`.
- **WLAN weg / kein Failover:** bei Fallback über **getrennte Subnetze** unbedingt **DHCP** (Punkt 6). Achtung: der MQTT-Broker muss vom Fallback-Netz erreichbar/routbar sein, sonst ist der ESP zwar im WLAN, aber HA nicht erreichbar.
- **Allgemein:** Der WebLog ist die erste Anlaufstelle — er zeigt live, an welcher Stelle (WLAN, MQTT oder BLE) es klemmt. Und: die meisten „Fehler" lösen sich durch erneutes Lesen des passenden Punktes hier. ;-)

### 13. Hintergrund / Architektur (optional)

- **Lokal statt Cloud:** Die Cloud-/MQTT-Verbindung des PowerStream bricht periodisch ab → Steuerung über
  diese Verbindung ist unzuverlässig. BLE ist lokal, direkt und unabhängig vom Internet.
- **EEPROM-Schonung:** Der Sollwert (`permanent_watts`) landet im EEPROM des PowerStream. Permanentes
  Neuschreiben würde es belasten → deshalb **Write-on-Change** (nur bei echter Änderung schreiben).
- **dynamicWatts:** Der „schnelle" Regelkanal `dynamicWatts` ist **lesbar**, aber **nicht per BLE
  beschreibbar** (er wird cloud-/Smart-Plug-seitig gefüttert). Geregelt wird daher über `permanent_watts`.
- **Zwei-Kanal-Gedanke:** Stabiler Grundwert per BLE — der cloud-vermittelte Kanal bleibt bewusst ungenutzt,
  damit der Aufbau vollständig **cloud-frei** bleibt.

### 14. Credits & Quellen

- **rabits/ha-ef-ble** (Roman Bashlovkin) — Reverse-Engineering des BLE-Protokolls: Frame-Format,
  Protobuf-Definitionen, Feld-Mappings. Die zentrale Grundlage dieses Projekts.
- **GnoX — „EcoFlow User ID Finder"** (https://gnox.github.io/user_id) — einfache Ermittlung der User ID,
  als Ergänzung zur ha-ef-ble-Integration.
- **NimBLE-Arduino** (h2zero / Ryan Powell) — BLE-Stack.
- **PubSubClient** (Nick O'Leary) — MQTT-Client.

EcoFlow und PowerStream sind Marken der EcoFlow Technology Inc.; dieses Projekt ist unabhängig und steht in
keiner Verbindung zu EcoFlow.  

### 15. Anmerkungen

Grundsätzlich kann der ESP mit der BLE Verbindung zeitgleich mit einer lokalen Lösung via WLAN (TCP - MQTT) oder auch mit der Standard Cloud-Verbindung arbeiten.
Was nicht geht, ist die zeitgleiche BLE Verbindung mit dem Telefon (Mobiltelefon oder Tablet mit EF-App) und dem ESP, da eine BLE Verbindung einmalig ist.

Der Stand des gesamten Projektes ist Juni 2026, getestet über 4 Wochen mit 3 PowerStream. 
PS-Haupt-Firmware: V1.0.1.228 (WiFi-FW bis: 1.x.4.158) - ob EcoFlow in Zukunft die hier verwendeten Möglichkeiten beschränkt, schließt oder blockiert ist ungewiss, aber angesichts des aggressiven Cloud-Only-Strukturausbaus denkbar.

Nach über 3 Jahren ständigem Ärger mit der Steuerung und der unzuverlässigen Cloudverbindung habe ich nun seit Wochen eine stabile Ruhe in der Gerätesteuerung.

Alle, die dieses hier gesammelte Wissen nutzen möchten, viel Spaß und viel Erfolg, aber beachtet vorher die Einleitung ganz oben 😉

Hinweis: Ich konnte es nur mit einem PowerStream - DeltaPro (Gen1) Bundle testen. Wer eine andere Delta-Batterie hat, muss es ausprobieren, ob die PS-BAT-Werte korrekt erscheinen.

TIPP: den ESP an der Batterie zu betreiben spart das Netzteil und versorgt den ESP dann mit Strom, wenn auch der Inverter arbeitet.

