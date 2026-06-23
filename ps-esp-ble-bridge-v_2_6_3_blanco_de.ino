/* =========================================================================
   PowerStream HW51-0000 - WiFi/MQTT <-> BLE Bridge V2.6.3 - WR-0 - 09.06.26
   -------------------------------------------------------------------------
   Blanco Vorlage - Beispiel: 
   Ersetzt den TCP-MQTT-Decoder durch eine lokale BLE-Bruecke:
   liest die volle PS-Telemetrie und steuert Sollwert/Helligkeit/Supply-Mode
   per BLE - unabhaengig von der WLAN/Cloud-Verbindung des PowerStream.

   Changelog:
   V0.0.1 - V2.3.9  Entwicklung (RE:APP, Decode, ...), Pruefungen
   V2.4.0 1. produktive Basis: Sollwert setzen + perm/inv Readback.
   V2.5.1 Volle Telemetrie additiv (generischer Protobuf-Walker), notifyCB
          bleibt EXAKT V2.4-Decode (Single-Notify, kein CRC8/Laengen-Gate;
          die Reassembly-Variante aus 2.5.0 hatte die Auth zerstoert).
          + NTP (echte Uhrzeit im WebLog), dynamic_power raus, Doppel-Entities
          behoben (perm/inv = V2.4-uniq_ids).
   V2.5.2 0-Paritaet (fehlendes proto3-Feld = 0 statt "unbekannt") +
          Diagnose-Sektion (WLAN/BLE/System, entity_category diagnostic).
   V2.5.3 Power-Supply-Mode als Select (0=supply,1=storage, BLE cmd_id 0x82) +
          inverter_on_off als Text "ein"/"aus".
   V2.5.4 Brightness gerundet statt abgeschnitten (50%->50%); "AP Name"-
          Diagnose ergaenzt; einmalige Doppel-Topic-Bereinigung entfernt
          (Altlasten sind weg, lief nur noch als No-Op).
   V2.5.5 HA-Button "ESP Neustart" (Software-Reboot via ESP.restart()).
   V2.5.6 BLE Link bekommt avty_t -> bei ESP-Stromverlust "nicht verfuegbar"
          (wie alle anderen Entities) statt stale "Verbunden". Bei reinem
          BLE-Ausfall (ESP laeuft) zeigt er weiterhin korrekt "Getrennt".
   V2.5.7 bat_input_watts (Feld 29, /10, W) als "Batterie Leistung" ergaenzt -
          vorzeichenbehaftet (Fluss zur/von Batterie). Der int32-Cast im Walker
          liefert das Vorzeichen bereits korrekt.
   V2.5.8 BLE Link zeigt jetzt "Getrennt" (Zustand) statt "nicht verfuegbar":
          Last-Will liegt auf 'link' (nicht 'avail'), Link-Sensor ohne avty_t.
          PS-Sensoren haengen ihre Verfuegbarkeit an 'link' (keine Daten ->
          nicht verfuegbar, kein Phantom-Zaehlen). Diagnose/RSSI behalten 'avail'
          + expire_after=50s (gehen bei ESP-Tod nach ~50s auf nicht verfuegbar).
          WLAN/System-Diagnose bleibt bei reinem BLE-Ausfall live.
   V2.5.9 expire_after als berechnete Konstante DIAG_EXPIRE_S = 3*STATUS_MS/1000+5
          (auto-skaliert mit dem Status-Takt; kein hartkodiertes Literal mehr).
          Rein kosmetisch - Laufzeitverhalten identisch zu V2.5.8 (50 s bei STATUS_MS = 15000 ).
   V2.5.9 Final Release - mit 3 parallel laufenden PowerStreams an L1,L2,L3.... laeuft!
   V2.6.0 WRITE-ON-CHANGE: permanent_watts wird nur noch geschrieben, wenn sich der
          Sollwert aendert ODER der PS-Ist-Wert (Feld 48) vom Soll abweicht. Kein
          periodisches Rewrite mehr (RESEND_MS entfernt) -> schont den PS-Flash, da
          keine Cloud mehr gegenschreibt. ESP rundet Sollwerte kaufmaennisch auf 5 W
          (wie die HA-Automatik). Karenz (WRITE_GRACE_MS) verhindert Thrash, bis der
          naechste Heartbeat den neuen Ist-Wert zeigt. Re-Write bei Soll!=Ist bleibt
          (selbstheilend bei verschlucktem Write). g_lastWrittenWatts lebt nur im RAM.
   V2.6.1 Sensor "Battery Pack Type" (Feld 42, Inverter-Status: 2 = DP verbunden/
          identifiziert) ergaenzt. Drei Sensoren standardmaessig deaktiviert (existent,
          aber in HA ausgeblendet): Inverter Temperatur (immer 0), Installation Country,
          Installation Town - via enabled_by_default:false, SENSORS-Tabelle unangetastet.
   V2.6.2 WLAN-Notfall: Faellt das WLAN durchgehend >=15 s aus (und BLE ist da), setzt
          der ESP autonom per BLE einen Failsafe-Sollwert (FAILSAFE_WATTS=50 W),
          damit der Inverter ohne HA nicht unkontrolliert weiterspeist. Latched -
          greift, sobald BLE erreichbar ist; loest sich bei WLAN-Rueckkehr (HA
          uebernimmt). Zusaetzlich bei WIFI_MODE 1: WLAN >=30 s weg -> Rescan/Re-Lock
          auf den dann staerksten AP (kein manueller Reset mehr noetig). Brief-Blips
          setzen den Timer zurueck (kein twitchy Verhalten).
   V2.6.3 Mehrere WLANs als Prioritaetsliste (WIFI_CREDS, SSID+PW je Eintrag). Der ESP
          bevorzugt das oberste erreichbare Netz (z.B. dedizierte Box), faellt sonst auf
          das naechste (z.B. Mesh) - jeweils mit eigenem Passwort. Best-at-Boot, Failsafe
          und Re-Lock arbeiten ueber die Liste; Re-Lock (30 s) waehlt bei Ausfall neu
          (Box-Prio). Kein aktives Zurueckwechseln bei gesunder Verbindung (Restart-Button
          erzwingt via Best-at-Boot wieder die Box). 1 Eintrag = klassisches Einzelnetz.

   Lese-Skalierung 1:1 aus decoder.py (V1.5.6). ACHTUNG:
     Inverter input/100 & op/10  - UMGEKEHRT zu PV/LLC (input/10 & op/100).
     inv_output_cur & inv_dc_cur /1000.
   BLE-Schreibbefehle = nur verifizierten Werte (!! NICHT die MQTT-cmd_ids
   aus decoder.py verwenden !!): Sollwert 0x81, Brightness 0x87, Supply 0x82, je dsrc/ddst=0,0.

   !! Benoetigt:  NimBLE-Arduino 2.x , PubSubClient !!
   
   (c) Just-Zuul 05/2026 - free for privat use only - NOT FOR SALE
   ========================================================================= */

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <NimBLEDevice.h>
#include <MD5Builder.h>
#include "mbedtls/aes.h"
#include <stdarg.h>
#include <time.h>
#include "esp_system.h"                      // esp_reset_reason()

// =================== KONFIG  (ab hier ausfuellen) ===================
//  --- gemeinsam fuer alle PowerStream ---
// WLAN-Zugangsdaten als PRIORITAETSLISTE (oberster Eintrag = bevorzugt).
// 1 Eintrag = klassisch ein Netz. 2+ Eintraege = mehrere SSIDs mit eigenem PW
// (z.B. dedizierte Box oben + Mesh als Fallback). Gleiches IP-Netz vorausgesetzt (außer bei DHCP Nutzung).
struct WifiCred { const char* ssid; const char* pass; };
const WifiCred WIFI_CREDS[] = {
  { "SSID 1", "PASS 1" },                      // Prio 1 (bevorzugt, z.B. dedizierter Router oder AP)
//  { "SSID 2", "PASS 2" },                    // Prio 2 (Fallback, z.B. Mesh) - bei nur 1 Netz -> Zeile auskommentieren
//  { "SSID 3", "PASS 3" },                    // Prio 3 (mögliches weiteres Fallback) - bei nur 1 Netz -> Zeile auskommentieren  
};
const int NUM_WIFI_CREDS = sizeof(WIFI_CREDS)/sizeof(WIFI_CREDS[0]);

// WLAN-Roaming-Schutz :
//   0 = Auto   1 = Best-at-Boot (empfohlen)   2 = Hard-Lock (feste BSSID)
#define WIFI_MODE  1
static const char* AP_NAME    = "SAMPLE-AP";
static uint8_t     AP_BSSID[6] = { 0xA1, 0xB2, 0xC3, 0xAA, 0xBB, 0xCC };

// SAMPLE-AP = A1:B2:C3:AA:BB:CC 


// Optionale statische IP (pro Geraet anpassen!). false = DHCP.
static const bool USE_STATIC_IP = true;                // !!!! <----- !! BEACHTEN !! - !! WICHTIG !! ---- !!!!!
IPAddress local_IP(192, 168, 100, 100);                // !!!!! pro Geraet aendern
IPAddress gateway (192, 168, 100,   1);
IPAddress subnet  (255, 255, 255,   0);
IPAddress dns1    (192, 168, 100,   1);
IPAddress dns2    (192, 168, 100,   1);

//  --- MQTT Broker ---                                // vervollständigen !! Achtung bei DHCP und Fallback WiFi Netzen !! muss erreichbar sein !!
const char* MQTT_HOST = "192.168.100.200";             // Mosquitto Host / HA-MQ  
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "mqtt-User";                   // leer "" falls keiner
const char* MQTT_PASS = "mqtt-PWD";                    // leer "" falls keiner

//  --- EcoFlow User ID ---
const char* EF_USER_ID = "0000000000000000000";        // EcoFlow UserID (fuer alle Geraete gleich)

// --- NTP / Zeit ---
static const char* NTP_SERVER_1 = "192.168.100.1";               // Router oder anderer Zeitgeber im lokalen LAN
static const char* NTP_SERVER_2 = "pool.ntp.org";                // Fallback - öffentlicher Zeitgeber
static const char* TZ_STRING    = "CET-1CEST,M3.5.0,M10.5.0/3";  // Zeitzone : CET/CEST inkl. Sommerzeit

//  --- pro Geraet individuell ---
const char* PS_SN     = "HW51ZOH4PS000000";    // Seriennummer des Inverters
const char* PS_MAC    = "77:66:ef:44:zz:ee";   // BLE-MAC des Inverters (kleinschreibung!!)
const char* DEVICE_ID = "WR0-PS0000";          // kurze ID -> Topics + MQTT-ClientID - Zusammensetzung wird: PowerStream-WR0-PS0000

//  -----------------------------------------------------------------------------------------------
//  --- Ab hier nur Änderungen, wenn die Auswirkung klar sind ---
//  --- Verhalten ---
const int   START_WATTS = 0;                   // Abweichender Startwert - Abgabe des Inverters bei ESP Start Sollwert bis MQTT etwas anderes sagt
const int   MIN_WATTS   = 0;                   // Sicherheitsgrenze MIN
const int   MAX_WATTS   = 800;                 // Beschränkung der maximalen Wattabgabe (EF/PS-Spezifikationen beachten)
const uint32_t CONTROL_CHECK_MS = 5000;        // Soll-Ist-Pruefintervall (nur lesen/vergleichen)
const int      CONTROL_TOL_W    = 5;           // Soll-Ist: Toleranz (W) - zugleich Schreib-Deadband
const uint32_t WRITE_GRACE_MS   = 6000;        // Mindestabstand zwischen Re-Asserts (Heartbeat-Latenz, kein Thrash)
const uint32_t STATUS_MS        = 15000;       // Link/RSSI-Status-Publish
const uint32_t TELEMETRY_MS     = 3000;        // permanent_watts/inv_output publishen
const uint32_t BLE_RETRY_MS     = 8000;        // Pause zwischen BLE-Reconnect-Versuchen

// --- WLAN-Notfall / Failsafe (V2.6.2) ---
const int      FAILSAFE_WATTS   = 50;          // sicherer Sollwert, wenn WLAN laenger weg ist (wird per BLE gesetzt)
const uint32_t FAILSAFE_WLAN_MS = 15000;       // WLAN so lange DURCHGEHEND weg -> Failsafe-Sollwert
const uint32_t WIFI_RELOCK_MS   = 30000;       // WLAN so lange weg (nur Mode 1) -> AP neu bewerten (Rescan/Re-Lock)

// Diagnose-expire_after (Sekunden!). Faustregel: >= 2x Status-Takt, sonst Flackern.
// Auto-skaliert mit STATUS_MS: 3 Status-Zyklen + 5 s Reserve. (STATUS_MS ist in ms -> /1000)
const uint32_t DIAG_EXPIRE_S    = 3 * STATUS_MS / 1000 + 5;   // = 50 s bei STATUS_MS=15000

// =================================================================
// --------------  ! Ab hier keine Änderungen !  -------------------
// --------  ! DO NOT CHANGE ANYTHING BEHIND THIS POINT  ! ---------
// =================================================================

static const char* FW_VERSION   = "2.6.3";

#define SVC_UUID    "00000001-0000-1000-8000-00805f9b34fb"
#define WRITE_UUID  "00000002-0000-1000-8000-00805f9b34fb"
#define NOTIFY_UUID "00000003-0000-1000-8000-00805f9b34fb"

String TOPIC_SET, TOPIC_SETSTATE, TOPIC_RSSI, TOPIC_LINK, TOPIC_AVAIL;
String TOPIC_BRIGHT, TOPIC_BRIGHTSTATE;
String TOPIC_SUPPLY, TOPIC_SUPPLYSTATE;
String TOPIC_RESTART;
String MQTT_CLIENTID;

uint8_t SESSION_KEY[16], IV_BASE[16];

WiFiClient    g_wifi;
PubSubClient  g_mqtt(g_wifi);
WebServer     g_web(80);

NimBLEClient*               g_client = nullptr;
NimBLERemoteCharacteristic* g_write  = nullptr;
NimBLERemoteCharacteristic* g_notify = nullptr;
volatile bool g_bleConnected = false;
volatile bool g_authOk       = false;
volatile bool g_authFailed   = false;

int      g_targetWatts  = START_WATTS;
int      g_targetBright = -1;     // -1 = noch kein Soll von HA
int      g_targetSupply = -1;     // -1 = noch kein Soll von HA (0=supply,1=storage)
int      g_curBrightPct = -1;     // Readback %
int      g_permWatts    = -1;     // permanent_watts (W)
int      g_invOutput    = -1;     // inverter_output (W)
bool     g_telemFresh   = false;
int      g_lastWrittenWatts = -1;    // zuletzt tatsaechlich gesendeter Sollwert (RAM, -1 = noch nichts)
uint32_t g_lastWriteMs      = 0;     // Zeitpunkt des letzten permanent_watts-Writes (Karenz)
uint32_t g_lastCtrlCheck = 0;
uint32_t g_lastStatus   = 0;
uint32_t g_lastTelem    = 0;
uint32_t g_lastBleTry   = 0;
bool     g_discoverySent = false;
int      g_discStep      = -1;

// Generischer Protobuf-Feldspeicher (Feldnummer 0..63 -> letzter Wert)
static long g_field[64];
static bool g_fieldSeen[64];

// WLAN-State (BSSID-Lock, nicht-blockierend)
uint8_t  g_lockedBssid[6]   = {0};
bool     g_bssidLockActive  = false;
bool     g_wifiConnecting   = false;
uint32_t g_wifiConnectStart = 0;
uint32_t g_wlanDownSince    = 0;     // Beginn des WLAN-Ausfalls (0 = WLAN da)
bool     g_failsafeActive   = false; // true = Sollwert wurde auf FAILSAFE_WATTS gezogen
int      g_credIdx          = 0;     // aktiver Eintrag in WIFI_CREDS (welches Netz/PW)
bool     g_ntpStarted       = false;

// Diagnose-Zaehler/-State
uint32_t g_mqttReconnects = 0;
uint32_t g_bleReconnects  = 0;
int      g_lastBleReason  = 0;

// ----------------- Zeit-Helfer -----------------
bool timeSynced(){ return time(nullptr) > 1700000000; }   // > 2023-11 => SNTP da
String nowClock(){
  time_t now = time(nullptr); struct tm t;
  if(timeSynced() && localtime_r(&now,&t)){
    char b[16]; snprintf(b,sizeof(b),"%02d:%02d:%02d",t.tm_hour,t.tm_min,t.tm_sec); return String(b);
  }
  return String("--:--:--");
}

// ----------------- WebLog: Ring-Buffer + zentrale Log-Funktion -----------------
#define LOG_LINES     60
#define LOG_LINE_LEN  140
char     g_log[LOG_LINES][LOG_LINE_LEN];
uint16_t g_logHead  = 0;
uint16_t g_logCount = 0;

void wsLog(const char* fmt, ...) {
  char buf[LOG_LINE_LEN];
  int pre;
  time_t now = time(nullptr); struct tm t;
  if (timeSynced() && localtime_r(&now,&t)) {
    pre = snprintf(buf, sizeof(buf), "[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    uint32_t s = millis()/1000;                       // Fallback bis NTP da ist
    pre = snprintf(buf, sizeof(buf), "[+%02u:%02u:%02u] ",
                   (unsigned)(s/3600), (unsigned)((s%3600)/60), (unsigned)(s%60));
  }
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf + pre, sizeof(buf) - pre, fmt, ap);
  va_end(ap);
  Serial.println(buf);
  strncpy(g_log[g_logHead], buf, LOG_LINE_LEN - 1);
  g_log[g_logHead][LOG_LINE_LEN - 1] = 0;
  g_logHead = (g_logHead + 1) % LOG_LINES;
  if (g_logCount < LOG_LINES) g_logCount++;
}

// ===========================================================================
//  SENSOR-TABELLE  (PS-relevant; Skalierung 1:1 aus decoder.py V1.5.6)
//  divisor 1 = roh (Codes/Status). perm/inv nutzen V2.4-Keys (kein Duplikat).
// ===========================================================================
struct Sensor {
  const char* key; const char* name; uint8_t field; uint16_t divisor;
  const char* unit; const char* devClass;
};
static const Sensor SENSORS[] = {
  // --- Inverter (input/100, op/10 !) ---
  { "inv_volt_in",  "Inverter Eingangsspannung",  35, 100, "V",  "voltage"     },
  { "inv_volt_op",  "Inverter Betriebsspannung",  36, 10,  "V",  "voltage"     },
  { "inv_cur_out",  "Inverter Ausgangsstrom",     37, 1000,"A",  "current"     },
  { "inv",          "Inverter Ausgang",           38, 10,  "W",  "power"       },  // V2.4-Key
  { "inv_temp",     "Inverter Temperatur",        39, 10,  "°C", "temperature" },
  { "inv_freq",     "Inverter Frequenz",          40, 10,  "Hz", "frequency"   },
  { "inv_dc_cur",   "Inverter DC-Strom",          41, 1000,"A",  "current"     },
  // --- PV1 (input/10, op/100) ---
  { "pv1_volt_in",  "PV1 Eingangsspannung",       16, 10,  "V",  "voltage"     },
  { "pv1_volt_op",  "PV1 Betriebsspannung",       17, 100, "V",  "voltage"     },
  { "pv1_cur",      "PV1 Strom",                  18, 10,  "A",  "current"     },
  { "pv1_watts",    "PV1 Leistung",               19, 10,  "W",  "power"       },
  { "pv1_temp",     "PV1 Temperatur",             20, 10,  "°C", "temperature" },
  // --- PV2 (input/10, op/100) ---
  { "pv2_volt_in",  "PV2 Eingangsspannung",       21, 10,  "V",  "voltage"     },
  { "pv2_volt_op",  "PV2 Betriebsspannung",       22, 100, "V",  "voltage"     },
  { "pv2_cur",      "PV2 Strom",                  23, 10,  "A",  "current"     },
  { "pv2_watts",    "PV2 Leistung",               24, 10,  "W",  "power"       },
  { "pv2_temp",     "PV2 Temperatur",             25, 10,  "°C", "temperature" },
  // --- LLC (input/10, op/100) ---
  { "llc_volt_in",  "LLC Eingangsspannung",       32, 10,  "V",  "voltage"     },
  { "llc_volt_op",  "LLC Betriebsspannung",       33, 100, "V",  "voltage"     },
  { "llc_temp",     "LLC Temperatur",             34, 10,  "°C", "temperature" },
  // --- Leistung / sonstiges ---
  { "perm",         "Permanent Watts",            48, 10,  "W",  "power"       },  // V2.4-Key
  { "rated_power",  "Nennleistung",               58, 10,  "W",  "power"       },
  { "bat_input_watts","Batterie Leistung",        29, 10,  "W",  "power"       },  // Feld 29, vorzeichenbehaftet: +/- = Fluss zur/von Batterie (wie Decoder)
  { "hb_freq",      "Heartbeat Frequenz",         57, 1,   "s",  "duration"    },
  // --- Status / Roh-Codes (alle sichtbar) ---
  { "inv_onoff",    "Inverter On/Off",            53, 1,   "",   ""            },
  { "inv_status",   "Inverter Status",            15, 1,   "",   ""            },
  { "inv_relay",    "Inverter Relay Status",      43, 1,   "",   ""            },
  { "bp_type",      "Battery Pack Type",          42, 1,   "",   ""            },  // Inverter meldet Pack-Typ: 2 = DP verbunden/identifiziert
  { "pv1_status",   "PV1 Status",                 11, 1,   "",   ""            },
  { "pv1_relay",    "PV1 Relay Status",           44, 1,   "",   ""            },
  { "pv2_status",   "PV2 Status",                 12, 1,   "",   ""            },
  { "pv2_relay",    "PV2 Relay Status",           45, 1,   "",   ""            },
  { "llc_status",   "LLC Status",                 14, 1,   "",   ""            },
  { "supply_prio",  "Supply Priority",            50, 1,   "",   ""            },
  { "install_ctry", "Installation Country",       46, 1,   "",   ""            },
  { "install_town", "Installation Town",          47, 1,   "",   ""            },
  // --- Error/Warning Codes ---
  { "inv_err",      "Inverter Error Code",        1,  1,   "",   ""            },
  { "inv_warn",     "Inverter Warning Code",      3,  1,   "",   ""            },
  { "pv1_err",      "PV1 Error Code",             2,  1,   "",   ""            },
  { "pv1_warn",     "PV1 Warning Code",           4,  1,   "",   ""            },
  { "pv2_err",      "PV2 Error Code",             5,  1,   "",   ""            },
  { "pv2_warn",     "PV2 Warning Code",           6,  1,   "",   ""            },
  { "llc_err",      "LLC Error Code",             9,  1,   "",   ""            },
  { "llc_warn",     "LLC Warning Code",           10, 1,   "",   ""            },
  { "wl_err",       "Wireless Error Code",        54, 1,   "",   ""            },
  { "wl_warn",      "Wireless Warning Code",      55, 1,   "",   ""            },
};
static const int NUM_SENSORS = sizeof(SENSORS) / sizeof(SENSORS[0]);

int decimalsFor(uint16_t d){ return d==1000?3 : d==100?2 : d==10?1 : 0; }
bool isMeasurement(const char* dc){ return dc && dc[0] && strcmp(dc,"duration")!=0; }

// ----------------- Krypto -----------------
void md5raw(const uint8_t* d, size_t n, uint8_t out[16]) {
  MD5Builder b; b.begin(); b.add((uint8_t*)d, n); b.calculate(); b.getBytes(out);
}
void aesCbc(bool enc, const uint8_t* in, uint8_t* out, size_t len) {
  mbedtls_aes_context a; mbedtls_aes_init(&a);
  uint8_t iv[16]; memcpy(iv, IV_BASE, 16);
  if (enc) { mbedtls_aes_setkey_enc(&a, SESSION_KEY, 128);
             mbedtls_aes_crypt_cbc(&a, MBEDTLS_AES_ENCRYPT, len, iv, in, out); }
  else     { mbedtls_aes_setkey_dec(&a, SESSION_KEY, 128);
             mbedtls_aes_crypt_cbc(&a, MBEDTLS_AES_DECRYPT, len, iv, in, out); }
  mbedtls_aes_free(&a);
}
// ----------------- CRC -----------------
uint8_t crc8c(const uint8_t* d, size_t n){ uint8_t c=0;
  for(size_t i=0;i<n;i++){c^=d[i];for(int b=0;b<8;b++)c=(c&0x80)?(uint8_t)((c<<1)^0x07):(uint8_t)(c<<1);} return c; }
uint16_t crc16a(const uint8_t* d, size_t n){ uint16_t c=0;
  for(size_t i=0;i<n;i++){c^=d[i];for(int b=0;b<8;b++)c=(c&1)?(uint16_t)((c>>1)^0xA001):(uint16_t)(c>>1);} return c; }

// ----------------- Packet/Frame -----------------
size_t buildPacket(uint8_t* o,uint8_t src,uint8_t dst,uint8_t cs,uint8_t ci,
                   const uint8_t* pl,size_t pn,uint8_t ver,uint8_t dsrc,uint8_t ddst){
  size_t i=0; o[i++]=0xAA; o[i++]=ver; o[i++]=pn&0xFF; o[i++]=(pn>>8)&0xFF;
  o[i++]=crc8c(o,4); o[i++]=0x0D;
  o[i++]=0;o[i++]=0;o[i++]=0;o[i++]=0; o[i++]=0;o[i++]=0;
  o[i++]=src;o[i++]=dst;
  if(ver>=3){o[i++]=dsrc;o[i++]=ddst;}
  o[i++]=cs;o[i++]=ci;
  if(pn){memcpy(o+i,pl,pn);i+=pn;}
  uint16_t c=crc16a(o,i); o[i++]=c&0xFF;o[i++]=(c>>8)&0xFF; return i;
}
size_t encryptFrame(const uint8_t* pkt,size_t pn,uint8_t* out){
  memcpy(out,pkt,5); size_t bl=pn-5, pad=((bl+15)/16)*16;
  static uint8_t buf[256]; memcpy(buf,pkt+5,bl); if(pad>bl)memset(buf+bl,0,pad-bl);
  aesCbc(true,buf,out+5,pad); return 5+pad;
}
size_t varint(uint32_t n,uint8_t* o){size_t i=0;while(true){uint8_t b=n&0x7F;n>>=7;o[i++]=n?(b|0x80):b;if(!n)break;}return i;}

size_t fAuthStatus(uint8_t* o){ uint8_t p[64]; size_t n=buildPacket(p,0x21,0x35,0x35,0x89,nullptr,0,3,1,1); return encryptFrame(p,n,o); }
size_t fAutoAuth(uint8_t* o){
  uint8_t md5[16]; String s=String(EF_USER_ID)+String(PS_SN);
  md5raw((const uint8_t*)s.c_str(),s.length(),md5);
  uint8_t hp[32]; const char* H="0123456789ABCDEF";
  for(int i=0;i<16;i++){hp[i*2]=H[md5[i]>>4];hp[i*2+1]=H[md5[i]&0xF];}
  uint8_t p[80]; size_t n=buildPacket(p,0x21,0x35,0x35,0x86,hp,32,2,1,1); return encryptFrame(p,n,o);
}
size_t fSetLoad(uint8_t* o,int w){            // Sollwert: cmd_id 0x81, dsrc/ddst=0,0 (verifiziert)
  uint8_t pl[8]; pl[0]=0x08; size_t pn=1+varint((uint32_t)(w*10),pl+1);
  uint8_t p[64]; size_t n=buildPacket(p,0x21,0x35,0x14,0x81,pl,pn,0x13,0,0); return encryptFrame(p,n,o);
}
size_t fSetBright(uint8_t* o,int bits){       // Brightness: cmd_id 0x87 (BLE!), Bits 0..1023
  uint8_t pl[8]; pl[0]=0x08; size_t pn=1+varint((uint32_t)bits,pl+1);
  uint8_t p[64]; size_t n=buildPacket(p,0x21,0x35,0x14,0x87,pl,pn,0x13,0,0); return encryptFrame(p,n,o);
}
size_t fSetSupply(uint8_t* o,int val){        // Supply-Mode: cmd_id 0x82 (BLE!), 0=supply 1=storage
  uint8_t pl[8]; pl[0]=0x08; size_t pn=1+varint((uint32_t)val,pl+1);
  uint8_t p[64]; size_t n=buildPacket(p,0x21,0x35,0x14,0x82,pl,pn,0x13,0,0); return encryptFrame(p,n,o);
}
String toHex(const uint8_t* d,size_t n){String s;const char* H="0123456789abcdef";for(size_t i=0;i<n;i++){s+=H[d[i]>>4];s+=H[d[i]&0xF];}return s;}

// ----------------- Krypto-Selbsttest -----------------
bool selfTest(){
  Serial.println(F("--- Krypto-Selbsttest ---"));
  bool ok=true;
  if(crc8c((const uint8_t*)"123456789",9)!=0xF4){Serial.println("CRC8 FAIL");ok=false;}
  if(crc16a((const uint8_t*)"123456789",9)!=0xBB3D){Serial.println("CRC16 FAIL");ok=false;}
  uint8_t m[16]; md5raw((const uint8_t*)"abc",3,m);
  if(toHex(m,16)!="900150983cd24fb0d6963f7d28e17f72"){Serial.println("MD5 FAIL");ok=false;}
  uint8_t k[16],iv[16]; md5raw((const uint8_t*)"abc",3,k); md5raw((const uint8_t*)"cba",3,iv);
  uint8_t savedK[16],savedIV[16]; memcpy(savedK,SESSION_KEY,16);memcpy(savedIV,IV_BASE,16);
  memcpy(SESSION_KEY,k,16);memcpy(IV_BASE,iv,16);
  uint8_t pt[16];memset(pt,'A',16);uint8_t ct[16];aesCbc(true,pt,ct,16);
  if(toHex(ct,16)!="f27b86b1dc9a9538743be06751cc3bc2"){Serial.println("AES FAIL");ok=false;}
  memcpy(SESSION_KEY,savedK,16);memcpy(IV_BASE,savedIV,16);
  Serial.println(ok?F("--- Selbsttest BESTANDEN ---"):F("--- Selbsttest FEHLGESCHLAGEN ---"));
  return ok;
}

// ----------------- Protobuf: generischer Walker -----------------
uint64_t pbVarint(const uint8_t* d, size_t len, size_t& i){
  uint64_t v=0; int sh=0;
  while(i<len){ uint8_t b=d[i++]; v|=(uint64_t)(b&0x7F)<<sh; if(!(b&0x80))break; sh+=7; if(sh>63)break; }
  return v;
}
// Speichert ALLE Varint-Felder nach Feldnummer; andere Wire-Types werden korrekt
// uebersprungen. Werte als int32 interpretiert (Vorzeichen z.B. fuer Temperaturen).
void pbExtractAll(const uint8_t* d, size_t len){
  size_t i=0;
  while(i<len){
    uint64_t tag=pbVarint(d,len,i); if(tag==0&&i>=len)break;
    uint32_t fn=tag>>3, wt=tag&7;
    if(wt==0){ uint64_t v=pbVarint(d,len,i);
      if(fn<64){ g_field[fn]=(long)(int32_t)v; g_fieldSeen[fn]=true; } }
    else if(wt==1){ i+=8; }
    else if(wt==2){ uint64_t l=pbVarint(d,len,i); i+=(size_t)l; }
    else if(wt==5){ i+=4; }
    else break;
  }
}

// ----------------- BLE -----------------
// notifyCB = EXAKT V2.4-Logik (Single-Notify, kein CRC8/frameLen-Gate),
// nur die Heartbeat-Auswertung nutzt jetzt den generischen Walker.
void notifyCB(NimBLERemoteCharacteristic* c,uint8_t* data,size_t len,bool isN){
  if(len<5||data[0]!=0xAA)return;
  uint8_t ver=data[1];
  uint16_t payLen=data[2]|(data[3]<<8);
  size_t bl=len-5, al=bl-(bl%16); if(al==0)return;
  static uint8_t dec[600]; if(al>sizeof(dec)) al=sizeof(dec);
  aesCbc(false,data+5,dec,al); if(al<13)return;
  uint8_t src=dec[7]; uint8_t cs,ci; size_t po;
  if(ver>=3){cs=dec[11];ci=dec[12];po=13;} else {cs=dec[9];ci=dec[10];po=11;}

  // Auth-Antwort (src=0x35, cmd_set=0x35, cmd_id=0x86)
  if(src==0x35&&cs==0x35&&ci==0x86){
    if(dec[po]==0x00){wsLog("[BLE] >>> AUTH OK <<<");g_authOk=true;}
    else{wsLog("[BLE] >>> AUTH FAIL 0x%02X <<<",dec[po]);g_authFailed=true;}
    return;
  }

  // inverter_heartbeat (cmd_set 0x14, cmd_id 0x01) -> volle Telemetrie
  if(src==0x35&&cs==0x14&&ci==0x01){
    size_t avail=(al>po)?(al-po):0;
    size_t n=(payLen<avail)?payLen:avail;
    if(n<4)return;
    static uint8_t pl[600]; if(n>sizeof(pl)) n=sizeof(pl);
    memcpy(pl,dec+po,n);
    uint8_t x=dec[1]; if(x){ for(size_t k=0;k<n;k++) pl[k]^=x; }   // de-XOR seq[0]
    pbExtractAll(pl,n);
    if(g_fieldSeen[48]) g_permWatts=(int)(g_field[48]/10);
    if(g_fieldSeen[38]) g_invOutput=(int)(g_field[38]/10);
    if(g_fieldSeen[56]) g_curBrightPct=(int)((g_field[56]/1023.0f)*100.0f + 0.5f);
    g_telemFresh=true;
    return;
  }
}
class ClientCB:public NimBLEClientCallbacks{
  void onConnect(NimBLEClient* c)override{ wsLog("[BLE] connected"); }
  void onDisconnect(NimBLEClient* c,int reason)override{
    wsLog("[BLE] disconnected reason=0x%X", reason);
    g_lastBleReason=reason; g_bleConnected=false; g_authOk=false;
  }
};
bool sendFrame(const uint8_t* f,size_t n){
  if(!g_write||!g_bleConnected)return false;
  return g_write->writeValue(f,n,false);
}
void bleSetWatts(int w){
  uint8_t f[64]; size_t n=fSetLoad(f,w);
  if(sendFrame(f,n)) wsLog("[BLE] set_load_power(%dW) gesendet",w);
}
// Kaufmaennische Rundung auf 5 W (wie die HA-Automatik) - fuer exakte Soll/Ist-Vergleiche.
int roundTo5(int w){ if(w<0) w=0; return ((w + 2) / 5) * 5; }
// Schreibt permanent_watts NUR wenn noetig: bei geaendertem Soll ODER wenn Ist != Soll.
// Im Ruhezustand (Ist == Soll) null Writes -> schont den PS-Flash. Re-Assert gedrosselt (Karenz).
void regulate(){
  if(!(g_bleConnected && g_authOk) || g_permWatts < 0) return;   // ohne BLE/Auth/Ist nichts tun
  int  soll     = g_targetWatts;                                 // bereits auf 5 W gerundet
  bool changed  = (soll != g_lastWrittenWatts);                  // neuer Sollwert?
  int  diff     = g_permWatts - soll; if(diff<0) diff = -diff;
  bool mismatch = (diff >= CONTROL_TOL_W);                        // Ist weicht ab?
  if(!changed && !mismatch) return;                              // alles gut -> kein Write
  if(!changed && (millis()-g_lastWriteMs < WRITE_GRACE_MS)) return; // Re-Assert nur gedrosselt
  wsLog("[Ctrl] Soll=%dW Ist=%dW %s -> schreibe", soll, g_permWatts, changed?"(neu)":"(Mismatch)");
  bleSetWatts(soll);
  g_lastWrittenWatts = soll;
  g_lastWriteMs      = millis();
}
void bleSetBright(int pct){
  int bits=(int)((pct/100.0f)*1023.0f + 0.5f); if(bits<0)bits=0; if(bits>1023)bits=1023;
  uint8_t f[64]; size_t n=fSetBright(f,bits);
  if(sendFrame(f,n)) wsLog("[BLE] set_brightness(%d%% -> %d bits) gesendet",pct,bits);
}
void bleSetSupply(int val){
  uint8_t f[64]; size_t n=fSetSupply(f,val);
  if(sendFrame(f,n)) wsLog("[BLE] set_supply_priority(%d) gesendet",val);
}
bool bleConnectAndAuth(){
  wsLog("[BLE] verbinde zu %s ...", PS_MAC);
  if(!g_client) { g_client=NimBLEDevice::createClient(); g_client->setClientCallbacks(new ClientCB(),false); }
  if(!g_client->connect(NimBLEAddress(PS_MAC,BLE_ADDR_PUBLIC))){ wsLog("[BLE] connect FAIL"); return false; }
  NimBLERemoteService* svc=g_client->getService(SVC_UUID);
  if(!svc){ wsLog("[BLE] no service"); g_client->disconnect(); return false; }
  g_write=svc->getCharacteristic(WRITE_UUID); g_notify=svc->getCharacteristic(NOTIFY_UUID);
  if(!g_write||!g_notify){ wsLog("[BLE] no chars"); g_client->disconnect(); return false; }
  if(!g_notify->subscribe(true,notifyCB)){ wsLog("[BLE] subscribe FAIL"); g_client->disconnect(); return false; }
  g_bleConnected=true; g_authOk=false; g_authFailed=false;
  delay(300);
  uint8_t f[256]; size_t n;
  n=fAuthStatus(f); g_write->writeValue(f,n,false); delay(400);
  n=fAutoAuth(f);   g_write->writeValue(f,n,false);
  uint32_t t0=millis(); while(!g_authOk&&!g_authFailed&&millis()-t0<10000) delay(20);
  if(!g_authOk){ wsLog("[BLE] Auth nicht bestaetigt"); g_client->disconnect(); return false; }
  g_bleReconnects++;
  if(g_mqtt.connected()) publishStatus();   // link sofort online -> PS-Sensoren ohne 15s-Lag verfuegbar
  // Sollwert NICHT blind schreiben - regulate() schreibt nur bei Ist!=Soll (write-on-change)
  if(g_targetBright>=0) bleSetBright(g_targetBright);
  if(g_targetSupply>=0) bleSetSupply(g_targetSupply);
  return true;
}

// ----------------- WLAN -----------------
void formatBssid(const uint8_t* b, char* out, size_t n){
  snprintf(out,n,"%02X:%02X:%02X:%02X:%02X:%02X",b[0],b[1],b[2],b[3],b[4],b[5]);
}
// Scan ueber ALLE konfigurierten Netze. Liefert den Index des hoechstprioren Netzes,
// das ueberhaupt erreichbar ist (Prioritaet = Reihenfolge in WIFI_CREDS) + dessen
// staerkste BSSID. -1 = keines da. ("Box bevorzugen": Box-SSID erreichbar -> Box, sonst Mesh.)
int pickBestCred(uint8_t* outB, int* outRssi){
  wsLog("[WiFi] Scan ueber %d konfigurierte Netz(e)...", NUM_WIFI_CREDS);
  WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
  int n=WiFi.scanNetworks(false,false);
  if(n<=0){ wsLog("[WiFi] keine Netze"); return -1; }
  int found=-1;
  for(int c=0; c<NUM_WIFI_CREDS && found<0; c++){
    int best=-999,idx=-1;
    for(int i=0;i<n;i++) if(WiFi.SSID(i)==String(WIFI_CREDS[c].ssid)&&WiFi.RSSI(i)>best){best=WiFi.RSSI(i);idx=i;}
    if(idx>=0){
      memcpy(outB,WiFi.BSSID(idx),6); *outRssi=best; found=c;
      char m[18]; formatBssid(outB,m,sizeof(m));
      wsLog("[WiFi] '%s' (Prio %d) bester AP %s @ %d dBm", WIFI_CREDS[c].ssid, c+1, m, best);
    }
  }
  if(found<0) wsLog("[WiFi] keine der konfigurierten SSIDs gefunden");
  WiFi.scanDelete(); return found;
}
void wifiModeInit(){
  g_bssidLockActive=false; memset(g_lockedBssid,0,6); g_credIdx=0;
  if(WIFI_MODE==1){
    uint8_t b[6]; int r; int c=pickBestCred(b,&r);
    if(c>=0){ g_credIdx=c; memcpy(g_lockedBssid,b,6); g_bssidLockActive=true; }
    else wsLog("[WiFi] Mode1 Scan fehlgeschlagen -> Auto (Prio 1)");
  } else if(WIFI_MODE==2){
    g_credIdx=0; memcpy(g_lockedBssid,AP_BSSID,6); g_bssidLockActive=true;
    char m[18]; formatBssid(AP_BSSID,m,sizeof(m));
    wsLog("[WiFi] Mode2 Hard-Lock auf %s (%s, Prio-1-Zugangsdaten)", m, AP_NAME);
  } else {
    uint8_t b[6]; int r; int c=pickBestCred(b,&r);   // Auto: Netz waehlen (Box-Prio), aber kein BSSID-Lock
    if(c>=0) g_credIdx=c;
    wsLog("[WiFi] Mode0 Auto (Netz: %s)", WIFI_CREDS[g_credIdx].ssid);
  }
}
void wifiEnsure(){
  if(WiFi.status()==WL_CONNECTED){
    g_wlanDownSince=0;
    if(g_failsafeActive){ g_failsafeActive=false; wsLog("[Failsafe] WLAN zurueck -> HA uebernimmt wieder"); }
    if(g_wifiConnecting){
      g_wifiConnecting=false;
      char m[18]; uint8_t* cb=WiFi.BSSID(); if(cb)formatBssid(cb,m,sizeof(m)); else strcpy(m,"?");
      wsLog("[WiFi] OK  IP %s  AP %s%s",
            WiFi.localIP().toString().c_str(), m, g_bssidLockActive?" (LOCKED)":"");
    }
    if(!g_ntpStarted){
      configTzTime(TZ_STRING, NTP_SERVER_1, NTP_SERVER_2);
      g_ntpStarted=true;
      wsLog("[NTP] Sync angefordert (%s, TZ %s)", NTP_SERVER_1, TZ_STRING);
    }
    return;
  }
  // ---- WLAN ist NICHT verbunden: Ausfall verfolgen ----
  if(g_wlanDownSince==0) g_wlanDownSince=millis();
  uint32_t down = millis()-g_wlanDownSince;
  // Failsafe: WLAN durchgehend >= FAILSAFE_WLAN_MS weg -> Sollwert auf FAILSAFE_WATTS (latched, greift sobald BLE da)
  if(!g_failsafeActive && down>=FAILSAFE_WLAN_MS){
    g_failsafeActive=true; g_targetWatts=FAILSAFE_WATTS;
    wsLog("[Failsafe] WLAN >%us weg -> Sollwert %dW", (unsigned)(FAILSAFE_WLAN_MS/1000), FAILSAFE_WATTS);
    regulate();   // sofort anwenden, falls BLE da
  }
  // Selbstheilung: WLAN >= WIFI_RELOCK_MS weg -> Netz/AP neu bewerten (Rescan, Box-Prio). Nicht im Hard-Lock.
  if(WIFI_MODE!=2 && down>=WIFI_RELOCK_MS){
    wsLog("[WiFi] WLAN >%us weg -> Netz/AP neu bewerten (Rescan)", (unsigned)(WIFI_RELOCK_MS/1000));
    if(g_wifiConnecting){ WiFi.disconnect(); g_wifiConnecting=false; }
    wifiModeInit();              // neuer Scan -> neues Netz/Lock (Box-Prio, sonst Mesh)
    g_wlanDownSince=millis();    // Timer zuruecksetzen (kein Dauer-Rescan)
  }
  if(g_wifiConnecting){
    if(millis()-g_wifiConnectStart>20000){ wsLog("[WiFi] connect timeout"); WiFi.disconnect(); g_wifiConnecting=false; }
    return;
  }
  WiFi.mode(WIFI_STA); WiFi.setHostname(MQTT_CLIENTID.c_str()); WiFi.setSleep(false);
  if(USE_STATIC_IP) WiFi.config(local_IP,gateway,subnet,dns1,dns2);
  const WifiCred& cr = WIFI_CREDS[g_credIdx];
  if(g_bssidLockActive){
    WiFi.begin(cr.ssid,cr.pass,0,g_lockedBssid);
    char m[18]; formatBssid(g_lockedBssid,m,sizeof(m));
    wsLog("[WiFi] connect '%s' (Lock %s)...", cr.ssid, m);
  } else { WiFi.begin(cr.ssid,cr.pass); wsLog("[WiFi] connect '%s' (Auto)...", cr.ssid); }
  g_wifiConnecting=true; g_wifiConnectStart=millis();
}

// ----------------- MQTT -----------------
const char* wifiModeStr(){ return WIFI_MODE==2?"Hard-Lock":WIFI_MODE==1?"Best-at-Boot":"Auto"; }
const char* apNameStr(){ return WIFI_MODE==2 ? AP_NAME : (WIFI_MODE==1 ? "(auto: best-at-boot)" : "(auto)"); }
String bssidCfgStr(){
  if(!g_bssidLockActive) return String("-");
  char m[18]; formatBssid(g_lockedBssid,m,sizeof(m)); return String(m);
}
String bleMacEsp(){ return String(NimBLEDevice::getAddress().toString().c_str()); }
const char* resetReasonStr(){
  switch(esp_reset_reason()){
    case ESP_RST_POWERON:  return "power_on";
    case ESP_RST_SW:       return "sw";
    case ESP_RST_PANIC:    return "panic";
    case ESP_RST_INT_WDT:  return "int_wdt";
    case ESP_RST_TASK_WDT: return "task_wdt";
    case ESP_RST_WDT:      return "wdt";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_DEEPSLEEP:return "deepsleep";
    case ESP_RST_EXT:      return "ext";
    default:               return "unknown";
  }
}
// publisht Link + alle Diagnose-Werte (statisch + dynamisch). Aufruf: bei Connect + im Status-Intervall.
void publishStatus(){
  String d = String("ps/") + DEVICE_ID + "/diag/";
  char b[24];
  // Link + WLAN-RSSI (Bestands-Topics)
  g_mqtt.publish(TOPIC_LINK.c_str(), (g_bleConnected&&g_authOk)?"online":"offline", true);
  snprintf(b,sizeof(b),"%d",WiFi.RSSI());                      g_mqtt.publish(TOPIC_RSSI.c_str(), b, true);
  // dynamisch
  int br=(g_bleConnected&&g_client)?g_client->getRssi():0;
  snprintf(b,sizeof(b),"%d",br);                               g_mqtt.publish((d+"brssi").c_str(),b,true);
  snprintf(b,sizeof(b),"%u",(unsigned)ESP.getFreeHeap());      g_mqtt.publish((d+"heap").c_str(),b,true);
  snprintf(b,sizeof(b),"%u",(unsigned)g_bleReconnects);        g_mqtt.publish((d+"breconn").c_str(),b,true);
  snprintf(b,sizeof(b),"%u",(unsigned)g_mqttReconnects);       g_mqtt.publish((d+"mreconn").c_str(),b,true);
  snprintf(b,sizeof(b),"%u",(unsigned)(millis()/1000));        g_mqtt.publish((d+"uptime").c_str(),b,true);
  snprintf(b,sizeof(b),"0x%X",g_lastBleReason);                g_mqtt.publish((d+"bdisc").c_str(),b,true);
  // statisch (aendern sich selten; retained re-publish ist billig)
  g_mqtt.publish((d+"ip").c_str(),        WiFi.localIP().toString().c_str(), true);
  g_mqtt.publish((d+"ssid").c_str(),      WiFi.SSID().c_str(),               true);
  g_mqtt.publish((d+"wmac").c_str(),      WiFi.macAddress().c_str(),         true);
  g_mqtt.publish((d+"bssid").c_str(),     (WiFi.status()==WL_CONNECTED)?WiFi.BSSIDstr().c_str():"-", true);
  g_mqtt.publish((d+"bssid_cfg").c_str(), bssidCfgStr().c_str(),             true);
  g_mqtt.publish((d+"wmode").c_str(),     wifiModeStr(),                     true);
  g_mqtt.publish((d+"apname").c_str(),    apNameStr(),                       true);
  g_mqtt.publish((d+"block").c_str(),     g_bssidLockActive?"AN":"AUS",      true);
  g_mqtt.publish((d+"bmac_esp").c_str(),  bleMacEsp().c_str(),               true);
  g_mqtt.publish((d+"bmac_inv").c_str(),  PS_MAC,                            true);
  g_mqtt.publish((d+"reset").c_str(),     resetReasonStr(),                  true);
  g_mqtt.publish((d+"fw").c_str(),        FW_VERSION,                        true);
}
void publishSetState(){
  char b[8]; snprintf(b,sizeof(b),"%d",g_targetWatts); g_mqtt.publish(TOPIC_SETSTATE.c_str(),b,true);
}
void publishBrightState(){
  if(g_curBrightPct>=0){ char b[8]; snprintf(b,sizeof(b),"%d",g_curBrightPct); g_mqtt.publish(TOPIC_BRIGHTSTATE.c_str(),b,true); }
}
void publishTelemetry(){
  String base = String("ps/") + DEVICE_ID + "/";
  for(int i=0;i<NUM_SENSORS;i++){
    const Sensor& s = SENSORS[i];
    long raw = g_fieldSeen[s.field] ? g_field[s.field] : 0;   // proto3: fehlendes Feld = 0
    String t = base + s.key;
    if(strcmp(s.key,"inv_onoff")==0){                          // 1/0 -> Text (Wunsch)
      g_mqtt.publish(t.c_str(), raw?"ein":"aus", true);
      continue;
    }
    char val[24];
    if(s.divisor==1) snprintf(val,sizeof(val),"%ld", raw);
    else             snprintf(val,sizeof(val),"%.*f", decimalsFor(s.divisor), (float)raw/s.divisor);
    g_mqtt.publish(t.c_str(), val, true);
  }
  // Supply-Mode-Select State (Feld 50: 0=supply, 1=storage)
  { int sp = g_fieldSeen[50] ? (int)g_field[50] : 0;
    g_mqtt.publish(TOPIC_SUPPLYSTATE.c_str(),
                   sp==0 ? "Prioritize power supply" : "Prioritize power storage", true); }
  publishBrightState();
}

// ---- Discovery ----
String devJson(){
  String d = "\"dev\":{\"ids\":[\"ps_"; d += DEVICE_ID;
  d += "\"],\"name\":\"PowerStream "; d += DEVICE_ID;
  d += "\",\"mf\":\"Just-Zuul\",\"mdl\":\"ESP32 - PS-HW51 - BLE - Bridge\",\"sw\":\"";
  d += FW_VERSION; d += "\",\"hw\":\"EcoFlow PowerStream HW51\",\"sn\":\""; d += PS_SN; d += "\"}";
  return d;
}
// Diese Sensoren existieren, sind in HA aber standardmaessig deaktiviert (immer 0 / uninteressant).
bool sensorOffByDefault(const char* k){
  return !strcmp(k,"inv_temp") || !strcmp(k,"install_ctry") || !strcmp(k,"install_town");
}
void publishSensorDiscovery(int i){
  const Sensor& s = SENSORS[i];
  String st = String("ps/") + DEVICE_ID + "/" + s.key;
  char t[200];
  snprintf(t,sizeof(t),"homeassistant/sensor/ps_%s/%s/config", DEVICE_ID, s.key);
  String p = "{";
  p += "\"name\":\""; p += s.name; p += "\",";
  p += "\"uniq_id\":\"ps_"; p += DEVICE_ID; p += "_"; p += s.key; p += "\",";
  p += "\"stat_t\":\""; p += st; p += "\",";
  p += "\"avty_t\":\""; p += TOPIC_LINK; p += "\",";   // PS-Daten an link: keine Daten -> nicht verfuegbar
  if(s.unit[0]){
    p += "\"unit_of_meas\":\""; p += s.unit; p += "\",";
    if(s.devClass[0]){ p += "\"dev_cla\":\""; p += s.devClass; p += "\","; }
    if(isMeasurement(s.devClass)){ p += "\"stat_cla\":\"measurement\",\"force_update\":true,"; }
  }
  if(sensorOffByDefault(s.key)) p += "\"enabled_by_default\":false,";
  p += devJson(); p += "}";
  g_mqtt.publish(t, p.c_str(), true);
}
// Diagnose-Entities (alle entity_category=diagnostic). Wert kommt aus publishStatus().
struct Diag { const char* id; const char* name; const char* unit; const char* devClass; };
static const Diag DIAGS[] = {
  { "brssi",     "BLE RSSI",                 "dBm", "signal_strength" },
  { "heap",      "Freier Heap",              "B",   ""                },
  { "breconn",   "BLE Reconnects",           "",    ""                },
  { "mreconn",   "MQTT Reconnects",          "",    ""                },
  { "uptime",    "Uptime",                   "s",   "duration"        },
  { "bdisc",     "BLE Disconnect-Grund",     "",    ""                },
  { "ip",        "WLAN IP-Adresse",          "",    ""                },
  { "ssid",      "WLAN SSID",                "",    ""                },
  { "wmac",      "WLAN MAC (ESP)",           "",    ""                },
  { "bssid",     "AP BSSID (aktuell)",       "",    ""                },
  { "bssid_cfg", "AP BSSID (konfiguriert)",  "",    ""                },
  { "wmode",     "WLAN Modus",               "",    ""                },
  { "apname",    "AP Name",                  "",    ""                },
  { "block",     "BSSID Lock",               "",    ""                },
  { "bmac_esp",  "BLE MAC (ESP)",            "",    ""                },
  { "bmac_inv",  "BLE MAC (Inverter)",       "",    ""                },
  { "reset",     "Neustart-Grund",           "",    ""                },
  { "fw",        "Firmware",                 "",    ""                },
};
static const int NUM_DIAG = sizeof(DIAGS)/sizeof(DIAGS[0]);

void publishDiagDiscovery(int i){
  const Diag& s = DIAGS[i];
  String st = String("ps/") + DEVICE_ID + "/diag/" + s.id;
  char t[200];
  snprintf(t,sizeof(t),"homeassistant/sensor/ps_%s/diag_%s/config", DEVICE_ID, s.id);
  String p = "{";
  p += "\"name\":\""; p += s.name; p += "\",";
  p += "\"uniq_id\":\"ps_"; p += DEVICE_ID; p += "_diag_"; p += s.id; p += "\",";
  p += "\"stat_t\":\""; p += st; p += "\",";
  p += "\"avty_t\":\""; p += TOPIC_AVAIL; p += "\",";
  p += "\"entity_category\":\"diagnostic\",\"expire_after\":" + String(DIAG_EXPIRE_S) + ",";   // Will liegt auf link -> Tod via expire
  if(s.unit[0]){
    p += "\"unit_of_meas\":\""; p += s.unit; p += "\",";
    if(s.devClass[0]){ p += "\"dev_cla\":\""; p += s.devClass; p += "\","; }
    if(strcmp(s.unit,"dBm")==0 || strcmp(s.unit,"B")==0){ p += "\"stat_cla\":\"measurement\","; }
  }
  p += devJson(); p += "}";
  g_mqtt.publish(t, p.c_str(), true);
}
void publishFixedDiscovery(){
  String dev = devJson();
  char t[200]; String p;

  snprintf(t,sizeof(t),"homeassistant/number/ps_%s/set/config",DEVICE_ID);
  p  = "{\"name\":\"Einspeisung Sollwert\",\"uniq_id\":\"ps_"; p+=DEVICE_ID; p+="_set\",";
  p += "\"cmd_t\":\""+TOPIC_SET+"\",\"stat_t\":\""+TOPIC_SETSTATE+"\",\"avty_t\":\""+TOPIC_AVAIL+"\",";
  p += "\"min\":"+String(MIN_WATTS)+",\"max\":"+String(MAX_WATTS)+",\"step\":5,\"mode\":\"box\",";
  p += "\"unit_of_meas\":\"W\",\"icon\":\"mdi:transmission-tower\","+dev+"}";
  g_mqtt.publish(t,p.c_str(),true);

  snprintf(t,sizeof(t),"homeassistant/number/ps_%s/bright/config",DEVICE_ID);
  p  = "{\"name\":\"Inverter Helligkeit\",\"uniq_id\":\"ps_"; p+=DEVICE_ID; p+="_bright\",";
  p += "\"cmd_t\":\""+TOPIC_BRIGHT+"\",\"stat_t\":\""+TOPIC_BRIGHTSTATE+"\",\"avty_t\":\""+TOPIC_AVAIL+"\",";
  p += "\"min\":0,\"max\":100,\"step\":1,\"mode\":\"slider\",";
  p += "\"unit_of_meas\":\"%\",\"icon\":\"mdi:brightness-6\","+dev+"}";
  g_mqtt.publish(t,p.c_str(),true);

  snprintf(t,sizeof(t),"homeassistant/select/ps_%s/supply_mode/config",DEVICE_ID);
  p  = "{\"name\":\"Power Supply Mode\",\"uniq_id\":\"ps_"; p+=DEVICE_ID; p+="_supply_mode\",";
  p += "\"cmd_t\":\""+TOPIC_SUPPLY+"\",\"stat_t\":\""+TOPIC_SUPPLYSTATE+"\",\"avty_t\":\""+TOPIC_AVAIL+"\",";
  p += "\"options\":[\"Prioritize power supply\",\"Prioritize power storage\"],";
  p += "\"icon\":\"mdi:transmission-tower-import\","+dev+"}";
  g_mqtt.publish(t,p.c_str(),true);

  snprintf(t,sizeof(t),"homeassistant/button/ps_%s/restart/config",DEVICE_ID);
  p  = "{\"name\":\"ESP Neustart\",\"uniq_id\":\"ps_"; p+=DEVICE_ID; p+="_restart\",";
  p += "\"cmd_t\":\""+TOPIC_RESTART+"\",\"avty_t\":\""+TOPIC_AVAIL+"\",";
  p += "\"dev_cla\":\"restart\",\"entity_category\":\"config\","+dev+"}";
  g_mqtt.publish(t,p.c_str(),true);

  snprintf(t,sizeof(t),"homeassistant/binary_sensor/ps_%s/link/config",DEVICE_ID);
  p  = "{\"name\":\"BLE Link\",\"uniq_id\":\"ps_"; p+=DEVICE_ID; p+="_link\",";
  p += "\"stat_t\":\""+TOPIC_LINK+"\",\"pl_on\":\"online\",\"pl_off\":\"offline\",";
  p += "\"dev_cla\":\"connectivity\",\"entity_category\":\"diagnostic\","+dev+"}";
  g_mqtt.publish(t,p.c_str(),true);

  snprintf(t,sizeof(t),"homeassistant/sensor/ps_%s/rssi/config",DEVICE_ID);
  p  = "{\"name\":\"WLAN RSSI\",\"uniq_id\":\"ps_"; p+=DEVICE_ID; p+="_rssi\",";
  p += "\"stat_t\":\""+TOPIC_RSSI+"\",\"avty_t\":\""+TOPIC_AVAIL+"\",\"expire_after\":" + String(DIAG_EXPIRE_S) + ",\"unit_of_meas\":\"dBm\",";
  p += "\"dev_cla\":\"signal_strength\",\"stat_cla\":\"measurement\",\"entity_category\":\"diagnostic\","+dev+"}";
  g_mqtt.publish(t,p.c_str(),true);
}
void stepDiscovery(){
  const int BATCH = 6;
  for(int b=0; b<BATCH && g_discStep>=0; b++){
    if(g_discStep==0){ publishFixedDiscovery(); g_discStep=1; continue; }
    int idx = g_discStep-1;
    if(idx < NUM_SENSORS){ publishSensorDiscovery(idx); g_discStep++; continue; }
    int didx = idx - NUM_SENSORS;
    if(didx < NUM_DIAG){ publishDiagDiscovery(didx); g_discStep++; continue; }
    g_discoverySent = true; g_discStep = -1;
    wsLog("[MQTT] Discovery komplett (%d Sensoren + %d Diagnose + Controls)", NUM_SENSORS, NUM_DIAG);
  }
}

void mqttCallback(char* topic,byte* payload,unsigned int len){
  if(strcmp(topic,"homeassistant/status")==0){
    if(len>=6 && strncmp((char*)payload,"online",6)==0){ g_discoverySent=false; g_discStep=0; }
    return;
  }
  if(strcmp(topic,TOPIC_SET.c_str())==0){
    char v[16]; size_t n=len<15?len:15; memcpy(v,payload,n); v[n]=0;
    int w=atoi(v); if(w<MIN_WATTS)w=MIN_WATTS; if(w>MAX_WATTS)w=MAX_WATTS;
    w = roundTo5(w);                                  // wie die Automatik: kaufmaennisch auf 5 W
    wsLog("[MQTT] Sollwert empfangen: %dW", w);
    g_targetWatts=w; publishSetState();
    regulate();                                       // schreibt nur, wenn Ist!=Soll
    return;
  }
  if(strcmp(topic,TOPIC_BRIGHT.c_str())==0){
    char v[16]; size_t n=len<15?len:15; memcpy(v,payload,n); v[n]=0;
    int pct=atoi(v); if(pct<0)pct=0; if(pct>100)pct=100;
    wsLog("[MQTT] Helligkeit empfangen: %d%%", pct);
    g_targetBright=pct;
    g_mqtt.publish(TOPIC_BRIGHTSTATE.c_str(), v, true);
    if(g_authOk) bleSetBright(pct);
    return;
  }
  if(strcmp(topic,TOPIC_SUPPLY.c_str())==0){
    char v[40]; size_t n=len<39?len:39; memcpy(v,payload,n); v[n]=0;
    int val = (strcmp(v,"Prioritize power storage")==0) ? 1 : 0;   // sonst supply(0)
    wsLog("[MQTT] Supply-Mode empfangen: %s -> %d", v, val);
    g_targetSupply=val;
    g_mqtt.publish(TOPIC_SUPPLYSTATE.c_str(), v, true);            // Echo
    if(g_authOk) bleSetSupply(val);
    return;
  }
  if(strcmp(topic,TOPIC_RESTART.c_str())==0){
    wsLog("[CMD] ESP-Neustart angefordert -> reboot");
    g_mqtt.publish(TOPIC_LINK.c_str(),"offline",true);            // BLE Link -> Getrennt (Will feuert bei sauberem Disconnect nicht)
    g_mqtt.publish(TOPIC_AVAIL.c_str(),"offline",true);           // Diag/Controls -> nicht verfuegbar
    delay(300);                                                   // MQTT flushen lassen
    ESP.restart();
    return;                                                       // (nicht erreicht)
  }
}
void mqttEnsure(){
  if(g_mqtt.connected())return;
  if(WiFi.status()!=WL_CONNECTED)return;
  bool ok = (strlen(MQTT_USER)>0)
    ? g_mqtt.connect(MQTT_CLIENTID.c_str(),MQTT_USER,MQTT_PASS,TOPIC_LINK.c_str(),0,true,"offline")
    : g_mqtt.connect(MQTT_CLIENTID.c_str(),nullptr,nullptr,TOPIC_LINK.c_str(),0,true,"offline");
  if(ok){
    g_mqttReconnects++;
    wsLog("[MQTT] connected");
    g_mqtt.subscribe(TOPIC_SET.c_str());
    g_mqtt.subscribe(TOPIC_BRIGHT.c_str());
    g_mqtt.subscribe(TOPIC_SUPPLY.c_str());
    g_mqtt.subscribe(TOPIC_RESTART.c_str());
    g_mqtt.subscribe("homeassistant/status");
    g_mqtt.publish(TOPIC_AVAIL.c_str(),"online",true);
    g_discoverySent=false; g_discStep=0;
    publishSetState(); publishStatus();
  } else wsLog("[MQTT] connect FAIL rc=%d", g_mqtt.state());
}

// ----------------- WebLog -----------------
String fmtUptime(){
  uint32_t s = millis() / 1000;
  uint32_t d = s / 86400, h = (s % 86400) / 3600, m = (s % 3600) / 60;
  char b[32];
  if (d > 0) snprintf(b, sizeof(b), "%ud %uh %um", (unsigned)d,(unsigned)h,(unsigned)m);
  else if (h > 0) snprintf(b, sizeof(b), "%uh %um", (unsigned)h,(unsigned)m);
  else snprintf(b, sizeof(b), "%um %us", (unsigned)m,(unsigned)(s%60));
  return String(b);
}
String htmlEscape(const char* s){
  String o; o.reserve(strlen(s)+8);
  for(; *s; s++){
    if(*s=='<') o += "&lt;"; else if(*s=='>') o += "&gt;";
    else if(*s=='&') o += "&amp;"; else o += *s;
  }
  return o;
}
String statCard(const char* label, const String& value, const char* cls=""){
  String s = "<div class='card'><div class='lbl'>";
  s += label; s += "</div><div class='val "; s += cls; s += "'>"; s += value; s += "</div></div>";
  return s;
}
void handleRoot(){
  String h; h.reserve(8192);
  h += F("<!DOCTYPE html><html lang='de'><head><meta charset='utf-8'>");
  h += "<title>PowerStream "; h += DEVICE_ID; h += "</title>";
  h += F("<meta http-equiv='refresh' content='5'>"
         "<meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<style>"
         "body{font-family:ui-monospace,Menlo,Consolas,monospace;background:#0e0e10;color:#e6e6e6;margin:0;padding:1em;}"
         "h2{margin:0 0 .8em 0;font-weight:600;}"
         ".sub{color:#888;font-size:.85em;margin-bottom:1em;}"
         ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:.6em;margin-bottom:1em;}"
         ".card{background:#1b1b1f;padding:.6em .9em;border-radius:.55em;border:1px solid #2a2a30;}"
         ".lbl{color:#888;font-size:.78em;margin-bottom:.15em;}"
         ".val{font-size:1.4em;font-weight:600;}"
         ".ok{color:#5ec27e;} .warn{color:#e0a955;} .err{color:#e26666;}"
         ".log{background:#000;border:1px solid #2a2a30;border-radius:.55em;padding:.7em;"
              "max-height:60vh;overflow-y:auto;white-space:pre-wrap;font-size:.85em;line-height:1.4;}"
         "</style></head><body>");

  h += "<h2>PowerStream "; h += DEVICE_ID; h += " &mdash; ESP32-BLE-Bridge V"; h += FW_VERSION; h += "</h2>";
  h += "<div class='sub'>SN "; h += PS_SN; h += " &middot; BLE "; h += PS_MAC;
  h += " &middot; "; h += String(NUM_SENSORS); h += " Sensoren &middot; Zeit "; h += nowClock(); h += "</div>";

  h += "<div class='grid'>";
  h += statCard("Soll", String(g_targetWatts)+" W");
  h += statCard("Permanent (Ist)", g_permWatts>=0 ? (String(g_permWatts)+" W") : String("&ndash;"));
  h += statCard("Inverter-Ausgang", g_invOutput>=0 ? (String(g_invOutput)+" W") : String("&ndash;"));
  h += statCard("Helligkeit", g_curBrightPct>=0 ? (String(g_curBrightPct)+" %") : String("&ndash;"));
  const char* bleTxt; const char* bleCls;
  if (g_bleConnected && g_authOk)      { bleTxt = "verbunden &amp; auth"; bleCls = "ok"; }
  else if (g_bleConnected)             { bleTxt = "verbunden, no-auth";    bleCls = "warn"; }
  else                                 { bleTxt = "getrennt";              bleCls = "err"; }
  h += statCard("BLE", bleTxt, bleCls);
  if (WiFi.status() == WL_CONNECTED) h += statCard("WLAN", String(WiFi.RSSI())+" dBm", "ok");
  else                               h += statCard("WLAN", "getrennt", "err");
  h += statCard("MQTT", g_mqtt.connected() ? "verbunden" : "getrennt", g_mqtt.connected() ? "ok" : "err");
  h += statCard("Uptime", fmtUptime());
  h += statCard("Heap", String(ESP.getFreeHeap())+" B");
  h += "</div>";

  h += "<div class='log'>";
  uint16_t start = (g_logHead - g_logCount + LOG_LINES) % LOG_LINES;
  for (uint16_t i = 0; i < g_logCount; i++) {
    uint16_t idx = (start + i) % LOG_LINES;
    h += htmlEscape(g_log[idx]); h += "\n";
  }
  h += "</div></body></html>";
  g_web.send(200, "text/html; charset=utf-8", h);
}
void handleNotFound(){ g_web.send(404, "text/plain", "Not found"); }

// ----------------- setup / loop -----------------
void setup(){
  Serial.begin(115200); delay(300);
  Serial.println(F("\n======================================"));
  Serial.println(F("  PowerStream Bridge V2.6.3"));
  Serial.printf ("  Geraet: %s  (SN %s)\n",DEVICE_ID,PS_SN);
  Serial.println(F("======================================"));

  md5raw((const uint8_t*)PS_SN,strlen(PS_SN),SESSION_KEY);
  size_t L=strlen(PS_SN); char rev[40];
  for(size_t i=0;i<L;i++)rev[i]=PS_SN[L-1-i]; rev[L]=0;
  md5raw((const uint8_t*)rev,L,IV_BASE);

  if(!selfTest()){Serial.println(F("STOP: Krypto-Selbsttest fehlgeschlagen."));return;}
  wsLog("Boot: PowerStream Bridge V%s (Device %s)", FW_VERSION, DEVICE_ID);

  String base   = String("ps/")+DEVICE_ID;
  TOPIC_SET         = base+"/set";
  TOPIC_SETSTATE    = base+"/set_state";
  TOPIC_BRIGHT      = base+"/bright/set";
  TOPIC_BRIGHTSTATE = base+"/bright_state";
  TOPIC_SUPPLY      = base+"/supply_mode/set";
  TOPIC_SUPPLYSTATE = base+"/supply_mode/state";
  TOPIC_RESTART     = base+"/restart";
  TOPIC_RSSI        = base+"/rssi";
  TOPIC_LINK        = base+"/link";
  TOPIC_AVAIL       = base+"/avail";
  MQTT_CLIENTID     = String("ps-bridge-")+DEVICE_ID;
  wsLog("Basis-Topic: %s", base.c_str());

  wifiModeInit();
  wifiEnsure();
  uint32_t tw=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-tw<12000){ wifiEnsure(); delay(200); }

  g_web.on("/", handleRoot);
  g_web.onNotFound(handleNotFound);
  g_web.begin();
  if (WiFi.status() == WL_CONNECTED) wsLog("[Web] http://%s/", WiFi.localIP().toString().c_str());
  else                               wsLog("[Web] gestartet, wartet auf WLAN");

  NimBLEDevice::init(""); NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  g_mqtt.setServer(MQTT_HOST,MQTT_PORT);
  g_mqtt.setBufferSize(1024);
  g_mqtt.setCallback(mqttCallback);
  mqttEnsure();
}

void loop(){
  wifiEnsure();
  if(!g_mqtt.connected()) mqttEnsure();
  g_mqtt.loop();
  g_web.handleClient();

  if(g_mqtt.connected() && !g_discoverySent) stepDiscovery();

  if(!g_bleConnected && millis()-g_lastBleTry>BLE_RETRY_MS){
    g_lastBleTry=millis();
    bleConnectAndAuth();
  }

  // WRITE-ON-CHANGE: nur schreiben wenn Soll sich aendert oder Ist!=Soll (kein periodisches Rewrite)
  if(millis()-g_lastCtrlCheck>CONTROL_CHECK_MS){
    g_lastCtrlCheck=millis();
    regulate();
  }
  if(g_mqtt.connected() && g_telemFresh && millis()-g_lastTelem>TELEMETRY_MS){
    g_lastTelem=millis(); g_telemFresh=false; publishTelemetry();
  }
  if(g_mqtt.connected() && millis()-g_lastStatus>STATUS_MS){
    g_lastStatus=millis(); publishStatus();
  }
  delay(20);
}
