# ECG Monitor — Test Session Walkthrough

A repeatable, checklist-driven procedure for bringing up the ECG monitor and
verifying a recording session. Each gate has a **pass** criterion; the
automated checker ([`scripts/check_test_log.py`](../scripts/check_test_log.py))
reads the serial log and reports the same gates.

**Pin / address reference**

| Function | Pins / address |
|---|---|
| I²C bus | SDA = GPIO4, SCL = GPIO5 (10 kΩ pull-ups to 3.3 V) |
| SD card (SPI) | MOSI = 17, MISO = 15, CLK = 18, CS = 16 (10 kΩ pull-ups on MISO/MOSI/CS) |
| ADS1115 #1 | I²C `0x48` (ADDR → GND) |
| ADS1115 #2 | I²C `0x49` (ADDR → VDD) |
| DS3231 RTC (+ AT24C32) | I²C `0x68` (EEPROM `0x57`) |
| MPU6050 | I²C `0x69` (AD0 → VDD) |
| PCF8574 (optional) | I²C `0x20` |
| ADS1115 ALERT/RDY | #1 → GPIO21, #2 → GPIO7 (open-drain, internal pull-up enabled) |

---

## 1. Pre-flight (connections)

- [ ] **Power**: battery pack or USB applied; every module shares one common GND.
- [ ] **I²C**: all devices wired to SDA/SCL; 10 kΩ pull-ups on SDA and SCL.
- [ ] **SD SPI**: four signal wires + pull-ups in place; card seated, FAT32.
- [ ] **ADS1115**: both boards on the bus at 0x48 / 0x49.
- [ ] **ALERT/RDY**: chip #2 ALRT → GPIO7. (Chip #0 is currently in polling mode —
      `CHIP0_IGNORE_ALERT_POLL=1` — so GPIO21 may be left disconnected.)
- [ ] **AD8232**: real unit — RA/LA/LL electrodes → AD8232 #2 OUT → ADS1115 #2 AIN0;
      dummy unit — AD8232 #1 → ADS1115 #1.
- [ ] **MPU6050**: AD0 tied high, taped to the chest.

## 2. Power-on gate

Connect the USB-serial cable and open the monitor at **115200 baud**, then power
the board. Expected sequence, in order:

1. Chip banner (ESP32-S3, 2 cores, flash size).
2. **I²C scan** shows `48 49 57 68 69` (and `20` if the PCF8574 is fitted).
3. `ADS1115 #1 (0x48) found` and `ADS1115 #2 (0x49) found`.
4. `DS3231 found`.
5. `MPU6050 (0x69) found and initialized - motion gating enabled`.
6. `MOUNT OK. Card info: Name: …` (no `MOUNT FAILED`).
7. WiFi connects, `System time synchronized`, `DS3231 successfully updated`, WiFi shuts down.
8. `Setup_ADC_SD: called`, `Recording starts in 10 s…`.

**Gate passed when:** steps 2–6 all appear and there is no `MOUNT FAILED`.

## 3. Recording gate (120 s)

When `*** RECORDING STARTED ***` appears, remain still.

- [ ] No `chip N (GPIO X): timeout waiting for ALERT/RDY` lines.
- [ ] No `LEADS OFF` lines.
- [ ] `SD count` is climbing in the `Running...` heartbeat lines.
- [ ] Ends with `RECORDING COMPLETE: N records, 0 write errors` and
      `ISHNE file closed (N samples/lead)` with **N > 0**.

**Gate passed when:** `N records` > 0 and `write errors` = 0.

> **Note on clipping:** the firmware `LEADS OFF` warning only fires after ~70
> *consecutive* railed samples. Intermittent clipping (a channel pegged at
> ±32767 part of the time) may pass this gate while still corrupting the
> waveform. To check for that, run
> `python3 scripts/motion_artifact_blms.py`, which reports the railed-sample
> count per channel.

## 4. Data gate (on the SD card)

- [ ] `ecg_YYYYMMDD_HHMMSS.ecg` grows well past its 522-byte header.
- [ ] `ecg_trace.csv` gained rows.
- [ ] `motion.csv` gained rows.

## 5. Symptom → fix

| Symptom | Most likely cause |
|---|---|
| Device missing from I²C scan | Loose wire / power / missing pull-up on that device |
| `MOUNT FAILED` | SD card power loose, CS/CLK/MOSI wire, or card unseated |
| `chip … timeout waiting for ALERT/RDY` | ALRT wire for that chip loose (or GPIO21 left off for polling chip 0) |
| `LEADS OFF` / ECG pegged at ±32767 | AD8232 gain/range (now ±4.096 V) or electrode contact |
| `MPU6050 (0x69) not found` | MPU6050 wiring (VCC/GND/SDA/SCL/AD0) |
| Recording has 0 records | One ADS1115 not contributing (see timeout/ALRT above) |

## 6. Automated gate

```bash
python3 scripts/check_test_log.py docs/output-2026-11-02.txt
```

Prints a `[PASS]/[FAIL]` checklist for the I²C scan, SD mount, MPU6050, ALRT
timeouts, leads-off, and the final recording count. Exit code is 0 only when
everything passes.
