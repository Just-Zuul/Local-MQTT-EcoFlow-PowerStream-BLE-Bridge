
## SETUP PS - ESP - BRIDGE (Français)

> 🇩🇪 [Deutsche Version](SETUP_PS_HW51_BLE_BRIDGE_final_DE.md)  
> 🇬🇧 [English Version](SETUP_PS_HW51_BLE_BRIDGE_final_EN.md)


## ⚠️ Avis & clause de non-responsabilité

Ce dépôt décrit uniquement comment je contrôle et lis personnellement mes propres micro-onduleurs EcoFlow PowerStream directement et localement via BLE (Bluetooth Low Energy), à l'aide d'un microcontrôleur ESP.

**Il ne s'agit ni d'un guide de construction, ni d'une invitation à le reproduire.**

### À utiliser à vos propres risques

Quiconque utilise, met en œuvre, reproduit ou exploite de quelque manière que ce soit le contenu de ce dépôt – en tout ou en partie – le fait **entièrement à ses propres risques et périls**.

Toute personne utilisant ce contenu est présumée posséder les connaissances de base nécessaires dans les domaines suivants :

électronique, systèmes embarqués, installations électriques, ainsi que la réglementation légale applicable.

### Clause de non-responsabilité


L'auteur **décline toute responsabilité** pour :

- Dommages corporels, matériels ou affectant les animaux
- Dommages aux appareils, installations ou à l'installation électrique
- Perte de données ou dysfonctionnements
- Dommages indirects de toute nature
- Non-respect des conditions de garantie ou des certifications du fabricant
- Non-respect des réglementations ou normes en vigueur

Ceci s'applique que les dommages résultent d'une utilisation directe, d'une modification ou d'une mise en œuvre défectueuse des méthodes décrites ici.

### Aucune garantie

Les informations fournies ici le sont **sans aucune garantie**, expresse ou implicite.

Aucune garantie n'est donnée quant à leur exactitude, leur exhaustivité, leur actualité ou leur fonctionnement.

### Marques déposées et tiers

EcoFlow et PowerStream sont des marques déposées d'EcoFlow Technology Inc. Ce projet n'est en aucun cas affilié à EcoFlow et n'est ni approuvé ni cautionné par EcoFlow.


# Table des matières 

## PowerStream HW51 ↔ ESP32 BLE Bridge — Setup Guide

1. Vue d'ensemble — Fonctionnement et utilité du pont (connexion locale via BLE, sans cloud ni décodeur), éléments contrôlés

2. Fonctionnalités (liste non exhaustive : télémétrie, contrôle, sécurité intégrée, multi-WiFi, journalisation Web…)

3. Prérequis : matériel et logiciel

4. Bibliothèques Arduino (+ dépannage micro-ecc, NimBLE 2.x)

5. Données requises

• Obtention des données : adresse MAC BLE PowerStream (nRF Connect), identifiant utilisateur (GnoX), BSSID du point d'accès

6. Configuration — bloc CONFIG étape par étape

• Identifiants WIFI (liste de priorité) · Mode WIFI · BSSID du point d'accès

• Adresse IP statique ou DHCP (avec remarque sur les adresses IP inter-plages)

• MQTT : courtier / port / utilisateur / mot de passe

• Identité du périphérique : numéro de série · adresse MAC BLE · identifiant utilisateur · identifiant du périphérique

• Seuil de sécurité : puissance de sécurité (50 W)

7. Programmation (Sélectionnez la carte, le port, la vitesse de chargement, le pilote CP210x)

8. Home Assistant — Éléments affichés automatiquement (découverte MQTT) : capteurs, commandes, liaison BLE, diagnostics, capteurs désactivés, signification de bp_type

9. Fonctionnement et fonctionnalités — consigne, luminosité, mode d'alimentation, bouton de redémarrage, WebLog (navigateur → ESP-IP)

10. Explication du comportement (le « pourquoi ») : écriture à la modification (compatible EEPROM), disponibilité (liaison BLE « Déconnectée »), basculement Wi-Fi (meilleure connexion au démarrage + réinitialisation), logique de sécurité, priorité multi-WiFi

11. Automatisation Home Assistant (exemple) : injection nulle, arrondi à 5 W, gestion delta

12. Dépannage : version NimBLE, micro-ECC, interruption du flash, connexion BLE, accessibilité Wi-Fi/broker (portée croisée)

13. Contexte/architecture (facultatif) : local au lieu du cloud, prise en charge EEPROM/EF, dynamicWatts non inscriptible par BLE, deux canaux Conception

14. Crédits et sources : rabits/ha-ef-ble · GnoX User ID Finder

15. Notes


### 1. Aperçu — Fonctionnement et utilité du pont


Le pont est un firmware pour un petit microcontrôleur ESP32.

L'ESP se connecte **localement via Bluetooth (BLE)** à un EcoFlow PowerStream et gère deux tâches :

- **Lecture :** Il lit en continu les mesures du PowerStream (puissance, débit de la batterie, état…).

- **Contrôle :** Il définit le **point de consigne d'injection** (et quelques autres variables de contrôle) directement sur l'appareil.

Toutes les données sont transmises via **MQTT** à Home Assistant, où les capteurs et les commandes apparaissent automatiquement.

**Pourquoi une connexion locale via BLE et non via le cloud ?**

Mon système, mon appareil, mes données, mon contrôle : un contrôle plus fiable et toutes les données dans Home Assistant.

La connexion cloud/MQTT du PowerStream est instable : les commandes arrivent en retard, voire pas du tout.

Le BLE est une **liaison radio directe et locale** : indépendante d'Internet, du cloud EcoFlow et de ses serveurs.

Le résultat : un contrôle rapide et fiable qui fonctionne même sans Internet (et sans EcoFlow).

### 2. Fonctionnalités (aperçu succinct)

- **Télémétrie complète :** alimentation, consommation de la batterie, températures, valeurs d'état… comme les capteurs de Home Assistant.

- **Contrôle :** consigne d'alimentation (W), luminosité des LED, priorité d'alimentation, redémarrage de l'ESP par bouton.

- **Mise à jour uniquement :** la consigne n'est enregistrée que lorsqu'elle change réellement → faible sollicitation de l'appareil.

- **Sécurité Wi-Fi :** en cas d'indisponibilité prolongée du Wi-Fi, l'ESP définit automatiquement une valeur de sécurité (50 W).

- **Basculement Wi-Fi + multi-Wi-Fi :** plusieurs réseaux avec liste de priorité ; basculement automatique du point d'accès en cas de coupure.

- **Journal Web intégré :** une petite page Web d'état directement sur l'ESP (état en temps réel BLE/Wi-Fi/MQTT + journal).

- **Intégration automatique à Home Assistant :** via la découverte MQTT — aucune configuration manuelle requise.

### 3. Prérequis, matériel et logiciels utilisés

- PC (Windows/macOS/Linux)
- IDE Arduino — https://www.arduino.cc/en/software/
- Module ESP32-WROOM — carte de développement ESP32-WROOM avec puce CP2102, 38 broches, USB-C (disponible chez Arduino ou chez le revendeur de votre choix)
- Câble USB adapté (USB-A vers USB-C)
- Pilote USB : pour les cartes avec **CP2102**, installez le **pilote VCP CP210x de Silicon Labs** si nécessaire ; sinon, aucun port COM n'apparaîtra sur votre PC.
- Ce dépôt

### 4. Bibliothèques Arduino
Le firmware nécessite deux bibliothèques qui doivent être installées manuellement.


**Installation manuelle (Arduino → Gestionnaire de bibliothèques) :**

- **PubSubClient** — par Nick O'Leary (« knolleary ») → Client MQTT (`PubSubClient.h`)

- **NimBLE-Arduino** — par h2zero (Ryan Powell) → Pile BLE (`NimBLEDevice.h`)

- ⚠️ L'API de NimBLE-Arduino a sensiblement changé entre les versions 1.x et 2.x. **La version 2.x est obligatoire** (par exemple, la version 2.5.0) — avec la version 1.x, des erreurs de compilation se produisent.

**Inclus dans le package de la carte ESP32 d'Espressif Systems** (aucune installation séparée requise : installez simplement le package via le *Gestionnaire de cartes*) :

- `WiFi.h`
- `WebServer.h`
- `MD5Builder.h`
- `mbedtls/aes.h` (mbedTLS, intégré à ESP-IDF/core)
- `esp_system.h` (`esp_reset_reason()`)

> **Dépannage de micro-ecc :** Un serveur web asynchrone n'est **pas** nécessaire (le code utilise le noyau synchrone `WebServer.h`). Si `micro-ecc` (par exemple, depuis un autre projet) est installé et qu'il y a des conflits de liens avec NimBLE, **supprimez ou renommez** le fichier `micro-ecc` autonome ; NimBLE 2.x utilise son propre système de chiffrement.

### 5. Données requises pour la configuration (minimum) :


- SSID du réseau Wi-Fi

- Mot de passe du réseau Wi-Fi

- Adresse MAC du réseau **2,4 GHz** du point d'accès Wi-Fi le plus proche ou souhaité (facultatif *1)

- Adresse MAC du PowerStream (par exemple, obtenue avec l'application **nRF Connect**)

- Numéro de série (SN) du PowerStream

- Identifiant utilisateur EcoFlow — le plus simple est d'utiliser l'outil **« EcoFlow User ID Finder » de GnoX** : https://gnox.github.io/user_id (complément à l'intégration ha-ef-ble). Connectez-vous avec vos identifiants EcoFlow ; les requêtes sont envoyées **directement du navigateur à l'API officielle d'EcoFlow** (sans redirection vers des serveurs tiers). Comptes européens : sélectionnez la région **`api-e.ecoflow.com`** ou « Auto ».

- Serveur MQTT : adresse, port, nom d’utilisateur et mot de passe (le cas échéant)

- Adresse IP statique souhaitée pour l’ESP sur le réseau (avec éventuellement passerelle et masque de sous-réseau), si une configuration statique est utilisée *2

- **ID_DEVICE** : nom d’appareil au choix (par exemple, `WR1-PS1234`). Apparaît dans les sujets MQTT et les entités HA, et nomme ces entités de manière unique (remarque : les entités deviennent alors : sensor.powerstream_<device_id>_nom_entité).

- *(optionnel)* **Fuseau horaire** (chaîne de caractères ; par défaut : Europe centrale `CET-1CEST,M3.5.0,M10.5.0/3`)

> *1 Le firmware ESP offre la possibilité de gérer spécifiquement la connexion Wi-Fi → voir la section **Wi-Fi** de la section 6.

> *2 DHCP ou adresse IP fixe sont possibles (l’adresse IP fixe est recommandée ! – voir les indications sur le Wi-Fi de secours !)

Une fois toutes les données rassemblées et Arduino installé, ajustez les paramètres les plus importants :



<img width="613" height="812" alt="grafik" src="https://github.com/user-attachments/assets/55312555-a6ef-41ba-8555-044e76e51109" />



Ouvrez ensuite le croquis Arduino : **ps-esp-ble-bridge-v_2_6_3_blanco_fr.ino** dans un éditeur (par exemple Notepad++) pour ajuster les entrées.

### 6. Configuration - structure de l'en-tête avec les entrées à ajuster :

LIGNE 87 = // =================== CONFIGURATION (à compléter) =================== = début de la configuration

LIGNE 94 = réseau Wi-Fi principal et mot de passe

LIGNES 95 à 96 = saisissez le SSID et le mot de passe Wi-Fi si vous souhaitez un réseau de secours... sinon, commentez la ligne avec // (pour que les numéros de ligne suivants restent corrects)
```
const WifiCred WIFI_CREDS[] = {
  { "SSID 1", "PASS 1" },                    // Priorité 1 (préférée, par exemple routeur dédié ou point d'accès)
  { "SSID 2", "PASS 2" },                    // Priorité 2 secours, par exemple réseau maillé) - si un seul réseau -> commenter la ligne
  { "SSID 3", "PASS 3" },                    // Priorité 3 (éventuellement un autre recours) - si un seul réseau -> commenter la ligne
};
```
Avec un seul réseau utilisé, cela devient :
```
const WifiCred WIFI_CREDS[] = {
  { "SSID-DEFINI 1", "PASS-DEFINI 1" },        // Priorité 1 (préférée, par exemple routeur dédié ou point d'accès)
// { "SSID 2", "PASS 2" }, // Priorité 2 (secours, par exemple réseau maillé) - si un seul réseau -> commenter la ligne
// { "SSID 3", "PASS 3" }, // Priorité 3 (éventuellement en cas de second recours) - si un seul réseau -> commenter la ligne
};
```

LINE 100 - 106:
```
L100 : // Protection contre l'itinérance Wi-Fi :
L101 : // 0 = Auto 1 = Meilleur au démarrage (recommandé) 2 = Verrouillage matériel (BSSID fixe)
L102 : #define WIFI_MODE 1
L103 : static const char* AP_NAME = "EXAMPLE-AP";
L104 : static uint8_t AP_BSSID[6] = { 0xA1, 0xB2, 0xC3, 0xAA, 0xBB, 0xCC };
L105 :
L106 : // EXAMPLE-AP = A1:B2:C3:AA:BB:CC -> vous pouvez indiquer ici le point d'accès utilisé et son adresse MAC.
```
L100 : // Protection contre l’itinérance Wi-Fi :

L102 = utiliser le mode 0 ou 1 si plusieurs réseaux Wi-Fi sont possibles ou actifs (lignes 95-96 remplies). Le mode 2 est réservé à un **seul** réseau avec un point d’accès fixe (Attention ! Nouveau point d’accès : reprogrammez l’ESP !).

L103 = le nom du point d’accès affiché en mode 2 (ou à titre de référence en mode 1 si vous connaissez le point d’accès le plus proche).

L104 = l’adresse MAC du point d’accès auquel se connecter en mode 2 (ou, en modes 0 et 1, simplement le nom du point d’accès le plus proche – affichée dans les données de diagnostic).


LIGNES 109 - 115 :
```
L109 : // Adresse IP statique optionnelle (à ajuster selon l'appareil !). false = DHCP.
L110 : static const bool USE_STATIC_IP = true; // !!!! <----- !! REMARQUE !! - !! IMPORTANT !! ---- !!!!!
L111 : IPAddress local_IP(192, 168, 100, 100); // !!!!! à modifier selon l'appareil
L112 : IPAddress gateway (192, 168, 100, 1);
L113 : IPAddress subnet (255, 255, 255, 0);
L114 : IPAddress dns1 (192, 168, 100, 1);
L115 : IPAddress dns2 (192, 168, 100, 1);
```
Ligne 110 = Utiliser une adresse IP fixe (vrai) ou DHCP (faux). Une adresse IP fixe est le comportement par défaut et recommandé, à condition que **tous les réseaux utilisés appartiennent à la même plage d'adresses IP** (la présence de plusieurs points d'accès/SSID sur le même réseau ne pose aucun problème dans ce cas).

Le DHCP (USE_STATIC_IP = faux) ne doit être utilisé que si les réseaux de secours appartiennent à des **plages d'adresses IP/sous-réseaux différents**.


Configuration minimale pour les paramètres Wi-Fi/réseau : un seul SSID et mot de passe, mode Wi-Fi 1 (par défaut), lignes 111 à 115 pour une adresse IP fixe.

Remarque : le serveur web intégré est accessible à l’adresse IP de l’ESP. Une adresse IP fixe facilite l’accès.


LIGNES 117 - 121 :

- Compléter/ajuster les paramètres MQTT existants

LIGNES 123 - 124 :

L123 : // --- Identifiant utilisateur EcoFlow ---

L124 : const char* EF_USER_ID = "0000000000000000000"; // Identifiant utilisateur EcoFlow (identique pour tous les appareils)

**L'identifiant utilisateur peut être obtenu facilement grâce à l'outil disponible à l'adresse : https://gnox.github.io/user_id**

LIGNES 126 - 129 :

L126 : // --- NTP / heure ---

L127 : static const char* NTP_SERVER_1 = "192.168.100.1"; // Routeur ou autre source de temps sur le réseau local
L128 : static const char* NTP_SERVER_2 = "pool.ntp.org"; // Serveur de temps public de secours
L129 : static const char* TZ_STRING = "CET-1CEST,M3.5.0,M10.5.0/3"; // Fuseau horaire : CET/CEST avec heure d'été

```
Réglage des serveurs de temps et du fuseau horaire (préréglé sur Europe centrale)

LIGNES 131 - 134 :

```
L131 : // --- informations unique du PowerStream ---
L132 : const char* PS_SN = "HW51ZOH4PS000000"; // Numéro de série de l'onduleur
L133 : const char* PS_MAC = "77:66:ef:44:zz:ee"; // Adresse MAC BLE de l'onduleur (en minuscules !!)

L134 : const char* DEVICE_ID = "WR0-PS0000" ; // ID court -> sujets + ID client MQTT - composé comme : PowerStream-WR0-PS0000
```
Le numéro de série DOIT correspondre exactement à celui de l'onduleur !!

L'adresse MAC BLE du PowerStream peut être facilement trouvée avec l'application « nRF ». Lors de la recherche, l'onduleur apparaît avec son identifiant « HW-1234 ». LIGNE 133 : **notez les MINUSCULES !!**

L'ID de l'appareil est utilisé pour nommer les entités. Un simple « 1 » deviendrait « PowerStream-1 ».

LIGNES 149 - 152 :

```
L150 : const int FAILSAFE_WATTS = 50 ; // Point de consigne de sécurité en cas de perte prolongée de Wi-Fi (réglage via BLE)

 Ici, vous pouvez ajuster la puissance d'injection pour le mode de secours en cas de perte de Wi-Fi (consommation de base ou valeur d'injection de base).


 ### 7. Mise à jour (sélectionnez la carte, le port, la vitesse de téléchargement, le pilote CP210x)

Après avoir entièrement ajusté le modèle avec toutes les données requises, copiez le code dans l'interface Arduino, connectez la carte et flashez-la.

Les prérequis ont déjà été abordés dans la section 3.


Procédure étape par étape :

1. **Sélection de la carte :** Outils → Carte → ESP32 Arduino → **« Module de développement ESP32 »** (compatible avec les cartes ESP32-WROOM courantes).
2. **Sélection du port :** Outils → Port → choisissez le port COM de l’ESP.
- Aucun port visible ? → Pilote CP210x manquant (voir section 3) ou câble de charge uniquement sans lignes de données.
3. **Vitesse de téléversement :** la valeur par défaut (115 200 bpm) convient. Si le téléversement s’interrompt, choisissez une valeur inférieure (par exemple, 921 600 bpm → 115 200 bpm).
4. **Téléversement :** cliquez sur la flèche (« Téléverser »). Le code est compilé et écrit sur l’ESP.
- Si le téléversement reste bloqué sur « Connexion… » : maintenez le bouton **BOOT** de l’ESP enfoncé jusqu’à ce que le téléversement démarre (certaines cartes nécessitent cette étape).
5. **Terminé :** En cas de succès, Arduino affiche « Téléversement terminé » ou « Réinitialisation matérielle via la broche RTS ». L’ESP redémarre et établit les connexions (Wi-Fi → MQTT → BLE).
**Vérification :** Vous pouvez observer le démarrage dans le moniteur série (115 200 bauds) ou ultérieurement dans le journal Web (http://<ESP-IP>/) : connexion Wi-Fi, connexion MQTT, puis connexion BLE au PowerStream.

### 8. Home Assistant — ce qui apparaît automatiquement (découverte MQTT)


Prérequis : dans Home Assistant, l’**intégration MQTT** est configurée (même broker que dans le programme) et **la découverte est activée** (par défaut). Après le flashage, l’ESP s’annonce automatiquement ; **aucune configuration manuelle d’entité** n’est nécessaire.

- **Appareil :** apparaît sous la forme **`PowerStream-<ID_DEVICE>`** (par exemple, `PowerStream-WR1-PS1234`).
- **Nommage des entités :** `sensor.powerstream_<id_de_l’appareil>_<nom>` (idem pour les boutons numériques, sélecteurs et boutons).
- **Contenu :**
- **Environ 46 capteurs :** — données de télémétrie du PowerStream (valeurs de puissance, flux de la batterie, températures, états…).
- **Commandes :** — consigne (Numéro), luminosité de la LED (Numéro), mode d’alimentation (Sélecteur), redémarrage (Bouton).
- **« Liaison BLE » :** — état de la connexion au PowerStream (voir section 10, Disponibilité).
- **~18 entités de diagnostic** — RSSI, adresse IP, durée de fonctionnement, point d'accès connecté, motif de réinitialisation, etc.
- **Capteurs désactivés par défaut :** température de l'onduleur (toujours 0), pays/ville d'installation —
Ces capteurs existent mais sont masqués ; activez-les dans Home Assistant si nécessaire.
- **`bp_type` :** état de l'onduleur — valeur **`2`** = batterie (Delta) connectée/détectée.

### 9. Fonctionnement et fonctionnalités

- **Consigne (W) :** Puissance d'injection. Définie via l'entité Nombre (manuellement ou par automatisation, section 11). Modifiée uniquement lors d'un changement réel (section 10).
- **Luminosité de la LED :** Luminosité de la LED de l'appareil (0–1023).
- **Mode d'alimentation :** Priorité d'alimentation (secteur ou batterie).
- **Bouton de redémarrage :** Redémarre l'ESP. Utile, par exemple, pour réinitialiser la sélection du point d'accès au démarrage (section 10).
- **Journal Web :** Ouvrez `http://<ESP-IP>/` dans votre navigateur. Affiche l'état en temps réel (BLE/WiFi/MQTT sous forme de cartes) et les dernières lignes du journal. Accessible même **sans** Home Assistant. Une adresse IP fixe facilite l'accès.

### 10. Explication du comportement (le « pourquoi »)

- **Écriture à la modification :** la consigne n'est enregistrée que lorsqu'elle change (ou si la valeur actuelle diffère de la valeur cible). En veille, cela signifie :
**aucune écriture** → faible impact sur la mémoire interne (EEPROM) du PowerStream.
- **Disponibilité (Liaison BLE) :** « Liaison BLE » affiche l'état **réel** : « Connecté »/« Déconnecté ». En cas de
défaillance de l'ESP, l'état est « Déconnecté » par défaut. Les capteurs PS deviennent **indisponibles »** lorsqu'aucune donnée BLE n'est reçue ;
ainsi, aucune valeur fantôme figée ne subsiste dans Home Assistant.
- **Basculement Wi-Fi (meilleure connexion au démarrage + reconnexion) :** au démarrage, l'ESP sélectionne le point d'accès accessible le plus puissant et s'y connecte. Si ce point d'accès est indisponible pendant une durée prolongée (≥ 30 s), il **effectue une nouvelle recherche automatique** ; aucune réinitialisation manuelle
n'est plus nécessaire.
- **Sécurité Wi-Fi :** si la connexion Wi-Fi est interrompue pendant au moins 15 secondes (et que le BLE est présent), l’ESP **définit automatiquement une consigne de sécurité (50 W)** afin que l’onduleur ne continue pas à fournir une alimentation incontrôlée à la dernière valeur sans Home Assistant. Lorsque la connexion Wi-Fi est rétablie, HA reprend le contrôle.
- **Priorité multi-Wi-Fi :** plusieurs réseaux peuvent être utilisés (section 6). L’ESP privilégie le **réseau accessible en premier** réseau de la liste (par exemple, un boîtier dédié) et utilise ensuite le suivant (par exemple, un réseau maillé) — chacun avec son propre mot de passe
- 
### 11. Automatisation HA (exemple)

Le pont fournit uniquement la **variable de contrôle** (la consigne). La logique de contrôle proprement dite — par exemple, une
**injection nulle** (régulation de la consommation du réseau à environ 0) — relève d'une **automatisation Home Assistant**.

Un **exemple générique et adaptable** (injection nulle avec arrondi à 5 W et une zone morte delta) est disponible dans le
fichier séparé :

> **`HA-Automation-ZeroFeedin-Example.md`**

Il contient le fichier YAML prêt à l'emploi ainsi qu'une explication de la logique — les noms des entités et le symbole du capteur de grille doivent être adaptés à votre propre système.

### 12. Dépannage (erreurs connues et solutions simples)

- **Pas de port COM sur Arduino :** Pilote VCP CP210x manquant (Section 3) — ou câble de charge uniquement sans lignes de données. Essayez un autre câble/port.
- **Erreur de compilation lors d'un rappel BLE (par exemple, `onDisconnect`) :** Version NimBLE incorrecte. Utilisez **NimBLE-Arduino 2.x**, et non 1.x (Section 4).
- **Erreur de l'éditeur de liens / symboles dupliqués (micro-ecc) :** Un fichier `micro-ecc` autonome provenant d'un autre projet entre en conflit. Supprimez-le ou renommez-le — NimBLE 2.x utilise son propre système de chiffrement.
- **Le téléversement ne démarre pas / s'interrompt :** Pendant la « Connexion… », maintenez le bouton **BOOT** enfoncé ; réduisez la vitesse de téléversement ; utilisez la bonne carte (« ESP32 Dev Module ») ; un câble USB plus court ou différent.
- **L'ESP n'apparaît pas dans Home Assistant :** L'intégration MQTT est-elle activée ? L'adresse, le port et l'identifiant du broker dans le programme sont-ils corrects ? La découverte MQTT est-elle activée ? Le journal Web (http://<ESP-IP>/) affiche l'état du protocole MQTT.
- **Impossible de se connecter en BLE :** L'adresse MAC du PowerStream est correcte et en **minuscules**, le numéro de série correspond exactement à celui de l'appareil, le PowerStream est allumé et à portée. Identifiant dans l'analyse nRF : `HW-…`.
- **Wi-Fi hors service / pas de basculement :** pour un basculement sur des **sous-réseaux distincts**, assurez-vous d'utiliser le **DHCP** (section 6). Remarque : le serveur MQTT doit être accessible depuis le réseau de secours ; sinon, l'ESP est connecté en Wi-Fi, mais Home Assistant est inaccessible.
- **Généralement :** le journal Web est le premier endroit à consulter ; il indique en temps réel le blocage (Wi-Fi, MQTT ou BLE). De plus, la plupart des « erreurs » se résolvent en relisant la section correspondante. ;-)

Liste des codes d'erreur pour la connexion BLE : [BLE-Codes_d_erreur_FR.md](BLE-Codes_d_erreur_FR.md)

### 13. Contexte / architecture (facultatif)

- **Local plutôt que cloud :** la connexion cloud/MQTT du PowerStream est sujette à des interruptions périodiques → le contrôle de cette connexion est donc peu fiable.
Le BLE est local, direct et indépendant d'Internet.
- **Compatible avec l'EEPROM :** la consigne (« permanent_watts ») est stockée dans l'EEPROM du PowerStream. Une écriture constante l'userait prématurément → d'où le principe de **l'écriture sur changement** (écriture uniquement lors d'un changement réel).
- **dynamicWatts :** le canal de contrôle « rapide » « dynamicWatts » est **lisible**, mais **non inscriptible via BLE**
(il est alimenté côté cloud/prise connectée). Le contrôle s'effectue donc via « permanent_watts ».
- **Système à deux canaux :** une valeur de base stable via BLE — le canal cloud est volontairement laissé inutilisé, afin que la configuration reste totalement **sans cloud**.

### 14. Crédits et sources

- **rabits/ha-ef-ble** (Roman Bashlovkin) — Rétro-ingénierie du protocole BLE : format de trame, définitions protobuf, correspondances de champs. Base centrale de ce projet.
- **GnoX — « EcoFlow User ID Finder »** (https://gnox.github.io/user_id) — Un moyen simple d'obtenir l'identifiant utilisateur, en complément de l'intégration ha-ef-ble.
- **NimBLE-Arduino** (h2zero / Ryan Powell) — Pile BLE.
- **PubSubClient** (Nick O'Leary) — Client MQTT.

EcoFlow et PowerStream sont des marques déposées d'EcoFlow Technology Inc. ; ce projet est indépendant et n'est en aucun cas affilié à EcoFlow.

### 15. Notes


En principe, l'ESP peut gérer sa connexion BLE simultanément à une solution locale via Wi-Fi (TCP - MQTT) ou même en parallèle de la connexion cloud standard-

Cependant, une connexion BLE simultanée entre le téléphone (smartphone ou tablette avec l'application EF) et l'ESP est impossible, car une connexion BLE est exclusive.

L'état du projet est celui de juin 2026, après 4 semaines de tests avec 3 PowerStreams.

Firmware principal PS : V1.0.1.228 (Firmware Wi-Fi jusqu'à : 1.x.4.158). Il est incertain qu'EcoFlow restreigne, ferme ou bloque les méthodes utilisées ici à l'avenir, mais cela reste envisageable compte tenu du développement rapide de leur infrastructure exclusivement cloud.

Après plus de 3 ans de problèmes constants de contrôle et de connexion cloud instable, je bénéficie désormais de plusieurs semaines de contrôle stable et silencieux de l'appareil.

À tous ceux qui souhaitent utiliser les informations rassemblées ici : amusez-vous bien et bonne chance ! Mais veuillez lire l’avertissement en haut de la page au préalable 😉

Remarque : Je n’ai pu tester qu’avec un kit PowerStream - DeltaPro (Gen1). Si vous possédez une autre batterie Delta, vérifiez que les valeurs de la batterie PS s’affichent correctement.

ASTUCE : Alimenter l’ESP à partir de la batterie permet d’économiser l’alimentation et d’alimenter l’ESP précisément lorsque l’onduleur fonctionne.
