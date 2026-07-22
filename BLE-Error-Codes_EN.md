# BLE error codes — what does `reason=0x...` mean?

When a BLE connection drops, the WebLog (`http://<ESP-IP>/`) shows a line like:

```
[BLE] disconnected reason=0x213
```

This page explains what the number means — **and what you can do about it.**

---

## The essentials in one sentence

The code tells you **who** ended the connection:

| Code | Who disconnected? |
|------|-------------------|
| **0x213** | **The PowerStream** kicked the ESP out |
| **0x216** | **The ESP** hung up by itself |
| **0x208** | **Nobody** — the radio link simply dropped |

Everything else is below.

---

## The codes that actually occur in practice

### 0x213 — The remote device disconnected
*(Remote User Terminated Connection)*

**The PowerStream threw the ESP out.** The ESP did not abort!

Usually seen together with `[BLE] auth not confirmed`. In that case the PS did **not accept** the authentication and cut the link.

**What to do:**
1. **Check the serial number (S/N)** — character by character, exactly as printed on the device. Most common mistake.
2. **Check the EcoFlow User ID** — correct account, copied cleanly.
3. **Close the EcoFlow app** — BLE allows only **one** connected device at a time.

---

### 0x216 — The ESP disconnected itself
*(Connection Terminated By Local Host)*

**The ESP hung up** — because it could not proceed. It disconnects on its own when:

- the **authentication was not confirmed** (it waits 10 s, then gives up) → `auth not confirmed`
- the expected **BLE service was not found** → `no service`
- the **characteristics are missing** → `no chars`
- **subscribing failed** → `subscribe FAIL`

**What to do:**
1. **Check the serial number (S/N)** — if it is wrong, the ESP cannot even decrypt the PS's reply, so authentication can never succeed.
2. **Check the EcoFlow User ID.**
3. On `no service` / `no chars`: is the target device really a **PowerStream**? Other devices expose different services.

> Is there an `AUTH FAIL 0xNN` right before it? Then the S/N is already correct, and that code tells the real reason — see the section **Authentication: `AUTH FAIL 0xNN`** below.

---

### 0x208 — Radio link lost
*(Connection Supervision Timeout)*

Nobody disconnected actively — the **radio contact simply went away**. The ESP reconnects automatically afterwards.

**What to do:**
- **Reduce the distance**, avoid obstacles (metal, concrete).
- Check the ESP's **power supply** (weak PSU/cable → dropouts).
- Occasionally this is **normal** and harmless, as long as the connection comes back.

---

### 0x23E — Connection could not be established
*(Connection Failed to be Established)*

The connection setup itself failed.

**What to do:** check distance/reception; the PS may have been connected elsewhere at that moment (the app!). The ESP retries automatically.

---

### 0x202 — Unknown connection
*(Unknown Connection Identifier)*

The connection attempt was aborted before it was up. Usually harmless if a normal retry follows.

---

### 0x222 — Timeout on the radio link
*(LMP/LL Response Timeout)*

The remote device did not answer in time. The cause is almost always **radio interference or too much distance**.

---

### 0x21F — Unspecified error
*(Unspecified Error)*

A catch-all code with no precise meaning. Usually comes from the PS (e.g. during an internal restart). If it appears only occasionally and the connection returns: ignore it.

---

## Authentication: `AUTH FAIL 0xNN` — the PowerStream's reply

The log often shows **two different number spaces** right below each other — don't mix them up:

- `disconnected reason=0x2..` → **BLE layer** (who disconnected) — see above.
- `AUTH FAIL 0x..` → **EcoFlow auth code**: what the PowerStream says about the login.

**Key point:** the moment an `AUTH FAIL 0xNN` appears at all, your **S/N is already correct** — the PS's reply could be decrypted (key = `MD5(S/N)`). The code `NN` then tells you what *actually* fails:

| Code | Meaning | What it means concretely |
|------|---------|--------------------------|
| `0x01` | NeedRefreshToken | token needs refreshing |
| `0x02` | DeviceInternalError | internal device error |
| `0x03` | DeviceAlreadyBound | already bound (elsewhere) |
| **`0x04`** | NeedBindInstallFirst | **device is not bound** → in the EcoFlow app, **remove and re-add it** |
| `0x05` | AppSendDataError | malformed request data |
| **`0x06`** | WrongKey | **wrong key → check the User ID (and S/N)** |
| `0x07` | MaximumDevicesError | maximum number of bindings reached |

`0x00` is **not** an error — that is the success case and shows up as `>>> AUTH OK <<<`. Any other value not listed here = unknown code.

**And the crucial difference with `auth not confirmed`:**

- **WITHOUT** a preceding `AUTH FAIL 0xNN` (just the 10-second timeout) → the PS **did not answer in a usable way** → **check S/N, User ID and MAC.**
- **WITH** an `AUTH FAIL 0xNN` before it → the S/N is fine, the PS **actively rejected** the login → **read the code above** (usually `0x04` = bind, `0x06` = key).

---

## If your code is not listed here

You can **decode it yourself** — it is simple arithmetic:

1. The codes start with **0x2..** → that is the **HCI layer** (radio layer).
2. **The last two digits** are the actual reason: `0x213` → `13`.
3. Look that value up in the official list (BLE specification, "BLE Error Codes"):
   → https://mynewt.apache.org/latest/network/ble_hs/ble_hs_return_codes.html

Example: `0x213` = base `0x200` (HCI) + `0x13` = "Remote User Terminated Connection" = the remote device disconnected.

---

## Quick reference

| Symptom | Most likely cause |
|---------|-------------------|
| `auth not confirmed` **without** a preceding `AUTH FAIL` (timeout only) | **check S/N / User ID / MAC** |
| `AUTH FAIL 0x04` | device **not bound** → remove + re-add in the app (S/N & ID are fine) |
| `AUTH FAIL 0x06` | **User ID wrong** (S/N is fine) |
| Always `no service` / `no chars` | Target device is **not a PowerStream** |
| `0x213` immediately on connecting | The **EcoFlow app** is holding the BLE connection |
| `0x208` occasionally, connection returns | **Radio range/link** — usually uncritical |

> Rule of thumb: if BLE gets as far as `connected` and then fails at authentication, check **S/N and User ID** first — not the board or the MAC (the fact that it connects at all proves the MAC is correct and a real device is answering). **If an `AUTH FAIL 0xNN` appears in the log, that code tells the real reason** — e.g. `0x04` = not bound (S/N & ID are already fine in that case).
