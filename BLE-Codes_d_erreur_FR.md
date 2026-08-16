# codes d'erreur BLE — que signifie `reason=0x...` ?

Lorsqu'une connexion BLE est interrompue, le journal Web (`http://<ESP-IP>/`) affiche une ligne comme :

```
[BLE] déconnecté raison=0x213

```

Cette page explique ce que signifie ce numéro — **et ce que vous pouvez faire à ce sujet.**

----------

## L'essentiel en une phrase

Le code vous indique **qui** a mis fin à la connexion :

Code

Qui s'est déconnecté ?

**0x213**

**Le PowerStream** a expulsé l'ESP

**0x216**

**L'ESP** s'est déconnecté tout seul

**0x208**

**Nobody**  — La liaison radio a tout simplement été interrompue.

Le reste se trouve ci-dessous.

----------

## Les codes qui se produisent réellement en pratique

### 0x213 —Le périphérique distant s'est déconnecté.

_(Connexion interrompue par l'utilisateur distant)_

**Le PowerStream a déconnecté l'ESP.** L'ESP ne s'est pas arrêté.

Ce message apparaît généralement avec « [BLE] Authentification non confirmée ». Dans ce cas, le PowerStream **n'a pas accepté** l'authentification et a interrompu la connexion.

**Ce qu'il faut faire:**

1. **Vérifiez le numéro de série (S/N)** — caractère par caractère, exactement comme il est imprimé sur l'appareil. Erreur fréquente.
2. **Vérifiez l'identifiant utilisateur EcoFlow** — compte correct, copié sans erreur.
3. **Fermez l'application EcoFlow** — le Bluetooth Low Energy (BLE) ne permet qu'**un seul** appareil connecté à la fois.

----------

### 0x216 — L'ESP s'est déconnecté.

_(Connexion interrompue par l'hôte local)_

**L'ESP s'est déconnecté** car il n'a pas pu poursuivre. Il se déconnecte automatiquement dans les cas suivants :

- **l'authentification n'a pas été confirmée** (il attend 10 s, puis abandonne) → `auth not confirmed`
- **le service BLE attendu est introuvable** → `no service`
- **les caractéristiques sont manquantes** → `no chars`
- **l'abonnement a échoué** → `subscribe FAIL`
- 
**Que faire ?**

1. **Vérifiez le numéro de série (S/N)**. S'il est incorrect, l'ESP ne peut même pas déchiffrer la réponse du PowerStream, et l'authentification échouera.
2. **Vérifiez l'identifiant utilisateur EcoFlow.**
3. En cas d'erreur `no service` ou `no chars` : le périphérique cible est-il bien un PowerStream ? D'autres périphériques proposent des services différents.

> Y a-t-il un message `AUTH FAIL 0xNN` juste avant ? Si oui, le numéro de série est correct et ce code indique la véritable raison — voir la section **Authentification : `AUTH FAIL 0xNN`** ci-dessous.

----------

### 0x208 — Liaison radio perdue

_(Délai d'expiration de la supervision de la connexion)_

Personne n'a déconnecté volontairement ; la **connexion radio a simplement été interrompue**. L'ESP se reconnecte automatiquement ensuite.

**Que faire :**

- **Réduisez la distance**, évitez les obstacles (métal, béton).
- Vérifiez l'**alimentation** de l'ESP (alimentation ou câble défectueux → déconnexions).
- Ce phénomène est parfois **normal** et sans conséquence, tant que la connexion est rétablie.

----------

### 0x23E — La connexion n'a pas pu être établie.

_(Échec de la connexion)_

La connexion a échoué.

**Que faire :** vérifiez la distance et la réception ; le PS était peut-être connecté à un autre appareil (l’application, par exemple). L’ESP réessaie automatiquement.

----------

### 0x202 — Connexion inconnue

_(Identifiant de connexion inconnu)_

La tentative de connexion a été interrompue avant d'être établie. Généralement sans conséquence si une nouvelle tentative est effectuée.

----------

### 0x222 — Délai d'attente dépassé sur la liaison radio

_(Délai de réponse LMP/LL dépassé)_

L'appareil distant n'a pas répondu à temps. La cause est presque toujours **des interférences radio ou une distance trop importante**.

----------

### 0x21F — Erreur non spécifiée

_(Erreur non spécifiée)_

Code générique sans signification précise. Il provient généralement du système de stockage (par exemple, lors d'un redémarrage interne). S'il n'apparaît que ponctuellement et que la connexion est rétablie, ignorez-le.

----------

## Authentification : `AUTH FAIL 0xNN` — réponse de PowerStream

Le journal affiche souvent **deux espaces numériques différents** l'un en dessous de l'autre ; ne les confondez pas :

- `disconnected reason=0x2..` → **Couche BLE** (qui a déconnecté) — voir ci-dessus.

- `AUTH FAIL 0x..` → **Code d'authentification EcoFlow** : ce que PowerStream indique concernant la connexion.

**Point important :** dès qu'un `AUTH FAIL 0xNN` apparaît, votre **numéro de série (S/N)** est déjà correct ; la réponse du PowerStream peut être déchiffrée (clé = `MD5(S/N)`). Le code `NN` vous indique alors ce qui a _réellement_ échoué.

Code

Signification

Ce que cela signifie concrètement

`0x01`

NeedRefreshToken

Le jeton doit être actualisé.

`0x02`

DeviceInternalError

erreur interne de l'appareil

`0x03`

DeviceAlreadyBound

déjà lié (ailleurs)

**`0x04`**

NeedBindInstallFirst

**L'appareil n'est pas associé** → Dans l'application EcoFlow, **supprimez-le et rajoutez-le**

`0x05`

AppSendDataError

données de requête malformées

**`0x06`**

WrongKey

**Clé incorrecte → vérifiez l'identifiant utilisateur (et le numéro de série)**

`0x07`

MaximumDevicesError

nombre maximal de liaisons atteint

`0x00` n'est **pas** une erreur ; il s'agit du cas de réussite et il s'affiche sous la forme `>>> AUTH OK <<<`. Toute autre valeur non répertoriée ici = code inconnu.

**Et la différence cruciale avec `auth not confirmed` :**

- **SANS** un `AUTH FAIL 0xNN` précédent (délai d'attente de 10 secondes uniquement) → l'appareil **n'a pas répondu de manière exploitable** → **vérifiez le numéro de série, l'identifiant utilisateur et l'adresse MAC.**

- **AVEC** un `AUTH FAIL 0xNN` précédent → le numéro de série est correct, l'appareil **a refusé** la connexion → **consultez le code ci-dessus** (généralement `0x04` = liaison, `0x06` = clé).

----------

## Si votre code ne figure pas ici

Vous pouvez le décoder vous-même : c’est un simple calcul :

1. Les codes commencent par **0x2…** → il s’agit de la **couche HCI** (couche radio).
2. **Les deux derniers chiffres** indiquent la raison de l’erreur : `0x213` → `13`.
3. Consultez la liste officielle des codes d’erreur BLE (spécification BLE) : → [https://mynewt.apache.org/latest/network/ble_hs/ble_hs_return_codes.html](https://mynewt.apache.org/latest/network/ble_hs/ble_hs_return_codes.html)

Exemple : `0x213` = base `0x200` (HCI) + `0x13` = "Connexion interrompue par l'utilisateur distant" = le périphérique distant déconnecté.

----------

## Guide de référence rapide

Symptôme

Cause la plus probable

`auth not confirmed`  **sans** un `AUTH FAIL` précédent (délai d'attente uniquement)

**Vérifiez le numéro de série / l'identifiant utilisateur / l'adresse MAC**

`AUTH FAIL 0x04`

Appareil **non lié** → supprimer puis rajouter dans l'application (le numéro de série et l'identifiant suffisent)

`AUTH FAIL 0x06`

**Identifiant utilisateur incorrect** (le numéro de série est correct)

Toujours`no service`  /  `no chars`

L'appareil cible n'est **pas un PowerStream**.

`0x213` dès la connexion.

L'application **EcoFlow** maintient la connexion BLE.

`0x208` occasionnellement, la connexion est rétablie.

**Portée/liaison radio** — généralement sans conséquence.
Règle générale : si la connexion BLE est établie puis échoue à l’authentification, vérifiez d’abord le numéro de série et l’identifiant utilisateur, et non la carte ou l’adresse MAC (le fait qu’elle soit établie prouve que l’adresse MAC est correcte et qu’un appareil réel répond). Si un code d’erreur « AUTH FAIL 0xNN » apparaît dans le journal, il indique la véritable raison de l’échec : par exemple, « 0x04 » signifie que l’appareil n’est pas lié (le numéro de série et l’identifiant sont alors valides).