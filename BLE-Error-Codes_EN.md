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
| `0x213` **or** `0x216` **together with** `auth not confirmed` | **S/N or EcoFlow User ID wrong** |
| Always `no service` / `no chars` | Target device is **not a PowerStream** |
| `0x213` immediately on connecting | The **EcoFlow app** is holding the BLE connection |
| `0x208` occasionally, connection returns | **Radio range/link** — usually uncritical |

> Rule of thumb: if BLE gets as far as `connected` and then fails at authentication, it is **almost always the S/N or the User ID** — not the board and not the MAC. The fact that it connects at all proves the MAC is correct and a real device is answering.
