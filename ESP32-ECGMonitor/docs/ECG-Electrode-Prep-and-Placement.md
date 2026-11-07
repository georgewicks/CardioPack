# ECG Electrode Prep, Placement & Holter-Style Recording

A practical summary of how clinicians prepare skin and place electrodes for
clean ECG/holter recordings, tailored to the AD8232 front-end used in this
project. Most of our signal-quality problems (respiration baseline wander,
lead-off railing, intermittent QRS) trace back to electrode–skin impedance,
so this is the highest-leverage fix on the hardware side.

---

## 1. Why prep matters

The ECG signal at the electrode is only **~1 mV**, riding on a body that is
also a big antenna for mains hum and a resistor that changes with skin
condition. The electrode–skin junction is the dominant impedance in the loop:

- **Dry / oily / hairy skin → high impedance →** baseline wander, 60 Hz noise,
  small QRS, lead-off detection.
- **Well-prepped skin → low, stable impedance →** larger, cleaner QRS.

The AD8232's right-leg drive can cancel *common-mode* interference, but only
if all three electrodes make good contact. A poor RL contact shows up as a
huge slow baseline swing — exactly what we saw on the Nov 5 recording.

---

## 2. Skin prep (the clinical standard)

Do these in order, every session:

1. **Choose the site.** Away from bone, large muscles, skin folds, hair,
   irritated or scarred skin, and areas rubbed by clothing. For the AD8232:
   RA/LA on the upper chest below the clavicles; RL on the right lower
   abdomen or right leg.

2. **Clip hair — do not shave.** Clipping avoids the micro-abrasions that
   cause inflammation and impedance drift. (Shaving is only done per local
   protocol when absolutely necessary.)

3. **Clean.** Wipe the site with an alcohol prep pad to remove oil, lotion,
   and dead surface debris. **Let it air-dry completely** — a wet site lifts
   the electrode.

4. **Abrade lightly.** Rub the site a few times with a dry gauze or a
   dedicated "skin prep" abrasive pad. This removes the dead outer layer
   (stratum corneum) and is the single biggest impedance reducer. You want
   the skin slightly pink, not raw.

5. **Apply a fresh electrode.** Check the package expiry; dried-out gel is
   the #1 cause of intermittent contact. Press the center (gel) firmly for a
   few seconds, then smooth the adhesive ring outward.

6. **Strain-relief the wires.** Loop and tape each lead wire a few cm from
   the electrode so body movement pulls on the tape, not the patch.

---

## 3. Electrode placement (3-electrode, AD8232)

> ⚠️ The AD8232 breakout is **not** a clinical 3-lead monitor. Its three pads
> are **RA (IN+), LA (IN−), and RL (right-leg *drive*)** — two signal inputs
> plus one driven output. Do **not** wire the third pad as "LL" (left leg).

| Pad | Purpose | Placement (this project) |
|-----|---------|--------------------------|
| **RA** | signal (+) | upper right chest, below the clavicle |
| **LA** | signal (−) | upper left chest, below the clavicle |
| **RL** | right-leg **drive** | right leg or right lower abdomen |

The leads-off LED on the AD8232 board should be **steady/off**. If it blinks,
one of the three leads is loose.

---

## 4. Holter-style (longer) recordings

For multi-minute to 24-hour captures, the same prep applies, plus:

- **Stabilize the recorder and cable bundle** so it doesn't swing or tug.
- **Keep the subject still** during capture; a motion log (we already record
  `motion.csv`) marks intervals to exclude.
- **Recheck adhesion** periodically; gel dries out over hours.
- **Avoid** electric blankets, high-current appliances near the leads, and
  phones in the chest pocket (EMI sources).
- Replace electrodes if impedance rises (seen as growing baseline wander).

---

## 5. Symptom → cause → fix

| Symptom (in our data) | Likely cause | Fix |
|---|---|---|
| Slow baseline wander (~0.5–0.8 Hz), large swing | high impedance / poor contact; respiration | re-prep skin, press gel, check RL |
| Rail-to-rail transient, then flat | lead popped off | re-secure electrode, strain-relief wires |
| Output pegged at rail | lead-off (AD8232 drives to rail) | re-attach lead, watch leads-off LED |
| 60 Hz "fuzz" | impedance mismatch between electrodes | re-prep all three sites equally |
| Muscle noise / spikes | electrode over muscle, tension | move site, relax subject |
| Small QRS amplitude | high impedance, wrong placement | abrade more, reposition |

---

## 6. Quick checklist per session

- [ ] Skin cleaned (alcohol) and **dry**
- [ ] Lightly abraded until pink
- [ ] Fresh, unexpired electrodes
- [ ] RA right chest, LA left chest, RL right leg/abdomen
- [ ] Gel pressed firmly, adhesive smooth
- [ ] Lead wires taped for strain relief
- [ ] AD8232 leads-off LED steady
- [ ] Subject still; motion log reviewed after capture
