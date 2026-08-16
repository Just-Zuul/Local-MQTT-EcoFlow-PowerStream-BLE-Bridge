
> 🇩🇪 [Deutsche Version](readme_de.md)

# MQTT local – Pont BLE EcoFlow PowerStream ### ESP32 · BLE · MQTT · Home Assistant · V2.6.3

Contrôleur PowerStream entièrement local et sans cloud via MQTT → Wi-Fi → ESP32 → BLE.

Aucune déconnexion intempestive. Aucune dépendance au cloud. Prêt en 15 minutes environ.

---

## ⚠️ Avis et clause de non-responsabilité

Ce projet décrit uniquement comment je contrôle et lis personnellement mes propres micro-onduleurs EcoFlow PowerStream directement et localement via BLE (Bluetooth Low Energy), en utilisant un microcontrôleur ESP32.

**Ceci n'est ni un guide de construction ni une invitation à le reproduire.**

Toute personne utilisant le contenu de ce dépôt, en tout ou en partie, le fait **entièrement à ses propres risques et périls**. Il est présumé que toute personne utilisant ce contenu possède les connaissances de base nécessaires dans les domaines de l'électronique, des systèmes embarqués, des installations électriques et de la réglementation applicable.

L'auteur décline toute responsabilité en cas de dommages corporels, matériels ou matériels, de dommages aux appareils ou installations, de perte de données, de dysfonctionnements, de violations de garantie ou de tout dommage indirect.

---

## Pourquoi ce projet existe-t-il ?

Après avoir rencontré ce problème à maintes reprises pendant près de deux ans, j'en ai conclu que ni la connexion au cloud EcoFlow ni les intégrations PowerStream locales disponibles ne peuvent être considérées comme véritablement fiables.

Après des semaines d'analyse, je suis convaincu à environ 98 % que le problème provient du composant Wi-Fi du firmware PowerStream, et plus précisément du module IoT basé sur ESP32 utilisé dans certaines versions de ce firmware.

Rétrogradations, contrôle local via TCP, échanges de courriels interminables avec le support EcoFlow (qui, au final, n'a pas voulu ou n'a pas pu reconnaître l'origine du problème) : rien n'y a fait.

---

## Comportement en mode Cloud

When operating through the EcoFlow cloud:

1. Le PowerStream apparaît comme hors ligne dans l'application, le cloud ou l'API.
2. L'ouverture de l'application EcoFlow provoque une reconnexion immédiate.
3. Ceci explique pourquoi la solution de contournement bien connue (ouvrir automatiquement l'application toutes les quelques minutes) fonctionne.

---

## Comportement du décodeur local


Lors d'une utilisation locale via le décodeur :

- Aucune reprise automatique depuis le cloud n'est possible.
- La connexion reste parfois stable pendant des heures.
- Des déconnexions surviennent parfois toutes les 2, 4 ou 8 minutes.
- Certaines interruptions ne durent que quelques secondes ; d'autres peuvent persister pendant des heures.

En raison de cette grande instabilité, le dépannage devient quasiment impossible.

---

## Ma solution : le BLE au lieu du Wi-Fi

À un moment donné, j'ai renoncé à essayer de rendre l'implémentation Wi-Fi fiable et je suis passé à une approche basée sur le Bluetooth Low Energy (BLE).

Le résultat est exceptionnellement stable, rapide et fiable.

**Le concept:**

- L'ESP32-WROOM (environ 8 €) sert de pont BLE pour le PowerStream : un petit cloud privé, entièrement local.
- L'ESP se connecte à votre réseau Wi-Fi existant.
- Intégration directe avec Home Assistant via MQTT.
- Aucune connexion Internet, VLAN, proxy, conteneur, service cloud ni configuration réseau personnalisée requis.
- Mise à jour du firmware en 10 minutes environ.
- Contrôle et accès locaux à la quasi-totalité des valeurs du PowerStream.
- Création automatique d'une entité Home Assistant via la découverte automatique MQTT.
- Automatisation en quelques minutes.

---

## Avantages

✅ Fonctionnement entièrement local, même sans Internet (contrairement à l'application mobile)
✅ Aucune dépendance au cloud
✅ Communication rapide et fiable
✅ Détection automatique de Home Assistant
✅ Aucune déconnexion intempestive
✅ Alimentation directe par le port USB d'une batterie Delta

---

## Prérequis

| Composant| Notes |
|-----------|-------|
| ESP32-WROOM | ~€8–10 |
| Temps d'installation | ~15 minutes |
| Home Assistant *¹ | avec broker MQTT |
| EcoFlow PowerStream HW51 | matériel testé |

*¹ Les plateformes domotiques telles que Home Assistant, ioBroker, openHAB (ou tout autre courtier MQTT) peuvent être utilisées. MQTT est indépendant de la plateforme ; la découverte automatique est spécifique à Home Assistant.

---

## Ce qui est inclus

| Fichier | Description |
|------|-------------|
| [`SETUP_PS_HW51_BLE_BRIDGE_final_FR.md`](SETUP_PS_HW51_BLE_BRIDGE_final_FR.md) | 🇫🇷 Guide d'installation complet (Français) |
| [`SETUP_PS_HW51_BLE_BRIDGE_final_DE.md`](SETUP_PS_HW51_BLE_BRIDGE_final_DE.md) | 🇩🇪 Guide d'installation complet (Allemand) |
| [`SETUP_PS_HW51_BLE_BRIDGE_final_EN.md`](SETUP_PS_HW51_BLE_BRIDGE_final_EN.md) | 🇬🇧 Guide d'installation complet (Anglais) |
| [`ps-esp-ble-bridge-v_2_6_3_blanco_fr.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_fr.ino) | 🇫🇷 ESP32 firmware — commentaires en français|
| [`ps-esp-ble-bridge-v_2_6_3_blanco_de.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_de.ino) | 🇩🇪 ESP32 firmware — commentaires en allemand|
| [`ps-esp-ble-bridge-v_2_6_3_blanco_en.ino`](ps-esp-ble-bridge-v_2_6_3_blanco_en.ino) | 🇬🇧 ESP32 firmware — commentaires en anglais |
| [`HA-Automatisierung-Nulleinspeisung-Beispiel.md`](HA-Automatisierung-Nulleinspeisung-Beispiel.md) | 🇩🇪 Exemple d'automatisation Home Assistant (injection zéro) |
| [`HA-Automation-ZeroFeedin-Example.md`](HA-Automation-ZeroFeedin-Example.md) | 🇬🇧 Exemple d'automatisation Home Assistant (injection zéro) |

**Démarrez ici:** Veuillez lire attentivement le guide d'installation dans votre langue avant de commencer.

---

## Démarrage rapide

1. Lisez attentivement le guide d'installation (FR, DE ou EN) avant de commencer.
2. Identifiez l'adresse MAC de votre PowerStream BLE (application nRF Connect).
3. Ouvrez le fichier firmware `.ino` dans un éditeur de texte et renseignez les valeurs.
4. Téléversez le firmware sur votre ESP32 à l'aide de l'IDE Arduino.
5. Importez l'exemple d'automatisation HA et adaptez les noms des entités à votre configuration.

---

## Crédits

Ce projet n’aurait pas été possible sans le travail de rétro-ingénierie novateur de **Roman Bashlovkin** :

> [rabits/ha-ef-ble](https://github.com/rabits/ha-ef-ble) — Décodage du protocole BLE, format de trame, définitions Protobuf et mappages de champs pour les appareils EcoFlow.

---

## Communauté

Questions, commentaires et résultats → **[N'hésitez pas à utiliser les discussions.](https://github.com/Just-Zuul/Local-MQTT-EcoFlow-PowerStream-BLE-Bridge/discussions)**