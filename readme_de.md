
> 🇬🇧 [English Version](readme.md)

# Lokales MQTT – EcoFlow PowerStream - BLE Bridge
### ESP32 · BLE · MQTT · Home Assistant · V2.6.3

Vollständig lokale, cloudfreie PowerStream-Steuerung via MQTT → WLAN → ESP32 → BLE.  
Keine wiederkehrenden Verbindungsabbrüche. Keine Cloud-Abhängigkeit. In ~15 Minuten einsatzbereit.

---

## ⚠️ Hinweis & Haftungsausschluss

Dieses Projekt beschreibt ausschließlich, wie ich persönlich meine EcoFlow PowerStream Microinverter direkt und lokal über BLE (Bluetooth Low Energy) mit einem ESP32-Mikrocontroller steuere und auslese.

**Es handelt sich weder um eine Anleitung noch um eine Aufforderung zum Nachbau.**

Wer Inhalte dieses Repositories – ganz oder in Teilen – verwendet, tut dies **ausschließlich auf eigene Gefahr und eigenes Risiko**. Von jeder Person, die diese Inhalte verwendet, wird vorausgesetzt, dass sie über das notwendige Grundlagenwissen in den Bereichen Elektronik, Embedded Systems, Elektroinstallation sowie den jeweils geltenden gesetzlichen Vorschriften verfügt.

Der Autor übernimmt **keinerlei Haftung** für Personen-, Sach- oder Geräteschäden, Datenverluste, Fehlfunktionen, Garantieverluste oder Folgeschäden jeglicher Art.

---

## Warum dieses Projekt entstanden ist

Nach immer wiederkehrenden Problemen über einen Zeitraum von fast zwei Jahren bin ich zu dem Schluss gekommen, dass weder die EcoFlow-Cloud-Verbindung noch die verfügbaren lokalen PowerStream-Integrationen als wirklich zuverlässig bezeichnet werden können.

Nach wochenlanger Analyse bin ich zu ~98% überzeugt, dass das Problem in der WLAN-Komponente der PowerStream WiFi-Firmware liegt — genauer gesagt im ESP32-basierten IoT-Modul, das in bestimmten WiFi-Firmware-Versionen eingesetzt wird.

Downgrades, lokale Steuerungen via TCP, endloser Mailverkehr mit dem EcoFlow-Support — der letztendlich nicht verstehen wollte oder durfte, wo das Problem wirklich liegt — alles gescheitert.

---

## Verhalten im Cloud-Betrieb

Im Betrieb über die EcoFlow-Cloud:

1. Der PowerStream wird in der App / Cloud / API als offline angegeben.
2. Das Öffnen der EcoFlow-App bewirkt eine sofortige Neuverbindung.
3. Das erklärt, warum der bekannte Workaround — die App automatisch alle paar Minuten zu öffnen — tatsächlich funktioniert.

---

## Verhalten mit lokalem Decoder

Im lokalen Betrieb über den Decoder:

- Es gibt keine cloud-seitige Wiederherstellung.
- Manchmal bleibt die Verbindung stundenlang stabil.
- Manchmal kommt es alle 2, 4 oder 8 Minuten zu Abbrüchen.
- Manche Unterbrechungen dauern nur Sekunden, andere Stunden.

Wegen dieser Unregelmäßigkeit ist eine Fehlersuche kaum möglich.

---

## Meine Lösung: BLE statt WLAN

Irgendwann hörte ich auf, die WLAN-Implementierung zum Laufen bringen zu wollen, und wechselte zu einem Bluetooth Low Energy (BLE) basierten Ansatz.

Das Ergebnis ist außergewöhnlich stabil, schnell und zuverlässig.

**Das Konzept:**

- ESP32-WROOM (~8 €) fungiert als BLE-Bridge für den PowerStream, wie eine eigene kleine Cloud, ganz lokal
- der ESP verbindet sich mit dem vorhandenen WLAN-Netzwerk
- Integriert sich direkt über MQTT in Home Assistant
- Kein Internet nötig, kein VLAN, kein Proxy, kein Container, kein Cloud-Dienst, kein spezielles Netzwerk-Setup nötig
- Firmware-Flash dauert ca. 10 Minuten
- Lokale Steuerung und Zugriff auf nahezu alle PowerStream-Werte
- Automatische Home Assistant Entity-Erstellung via MQTT Auto Discovery
- In wenigen Minuten bereit für Automationen

---

## Vorteile

✅ Vollständig lokaler Betrieb - auch ohne Internet (im Gegensatz zur App auf dem Telefon)  
✅ Keine Cloud-Abhängigkeit  
✅ Schnelle und zuverlässige Kommunikation  
✅ Home Assistant Auto Discovery  
✅ Keine wiederkehrenden Verbindungsabbrüche  
✅ Stromversorgung direkt über den USB-Port einer Delta-Batterie  

---

## Voraussetzungen

| Komponente | Hinweis |
|------------|---------|
| ESP32-WROOM | ~8–10 € |
| Setup-Zeit | ~15 Minuten |
| Home Assistant *¹ | mit MQTT-Broker |
| EcoFlow PowerStream HW51 | getestete Hardware |

*¹ Smart-Home-Systeme wie Home Assistant, ioBroker, openHAB — oder jeder andere MQTT-Broker — sind verwendbar. MQTT ist plattformunabhängig; die automatische Auto Discovery ist HA-spezifisch.

---

## Enthaltene Dateien

| Datei | Beschreibung |
|-------|--------------|
| [`SETUP_PS_HW51_BLE_BRIDGE_final_DE.md`](SETUP_PS_HW51_BLE_BRIDGE_final_DE.md) | 🇩🇪 Vollständige Setup-Anleitung (Deutsch) |
| [`SETUP_PS_HW51_BLE_BRIDGE_final_EN.md`](SETUP_PS_HW51_BLE_BRIDGE_final_EN.md) | 🇬🇧 Vollständige Setup-Anleitung (Englisch) |
| [`ps-esp-ble-bridge-v_2_6_3_blanco_de.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_de.ino) | 🇩🇪 ESP32-Firmware – Kommentare auf Deutsch |
| [`ps-esp-ble-bridge-v_2_6_3_blanco_en.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_en.ino) | 🇬🇧 ESP32-Firmware – Kommentare auf Englisch |
| [`HA-Automatisierung-Nulleinspeisung-Beispiel.md`](HA-Automatisierung-Nulleinspeisung-Beispiel.md) | 🇩🇪 Home Assistant Nulleinspeisung Automatisierungs-Beispiel |
| [`HA-Automation-ZeroFeedin-Example.md`](HA-Automation-ZeroFeedin-Example.md) | 🇬🇧 Home Assistant Zero Feed-in Automation Example |

**Hier starten:** Die Setup-Anleitung in deiner Sprache vollständig lesen, bevor du beginnst.

---

## Kurzanleitung

1. Setup-Anleitung (DE oder EN) vollständig lesen
2. BLE-MAC-Adresse des PowerStream ermitteln (nRF Connect App)
3. `.ino`-Firmware-Datei im Texteditor öffnen und eigene Werte eintragen
4. Mit der Arduino IDE auf den ESP32 flashen
5. HA-Automatisierungs-Beispiel importieren und Entitätsnamen anpassen

---

## Credits

Dieses Projekt wäre ohne die grundlegende Reverse-Engineering-Arbeit von **Roman Bashlovkin** nicht möglich gewesen:

> [rabits/ha-ef-ble](https://github.com/rabits/ha-ef-ble) — BLE-Protokoll-Dekodierung, Frame-Format, Protobuf-Definitionen und Feld-Mappings für EcoFlow-Geräte.

---

## Community

Fragen, Feedback und Ergebnisse → **[gerne in den Discussionen](https://github.com/Just-Zuul/Local-MQTT-EcoFlow-PowerStream-BLE-Bridge/discussions)**
