# Using Qwiic / STEMMA QT for I2C devices

> **Qwiic** (SparkFun) and **STEMMA QT** (Adafruit) are the same physical
> connector: a **JST SH 1.0 mm, 4-pin** plug carrying **GND, 3.3 V, SDA, SCL**.
> They are the de-facto standard for reliable, pluggable hobby I2C.

## Why use it here
- **Locking connectors** that don't work loose (the cure for breadboard flakiness).
- Standard pinout and **3.3 V** levels — matches the ESP32-S3 directly.
- Pre-crimped cables are cheap, so you never need to crimp the tiny 1.0 mm pins yourself.

## Pinout and wire colors
The cable wires follow a standard color code:

| Wire colour | Signal |
|---|---|
| Black  | GND   |
| Red    | 3.3 V |
| Blue   | SDA   |
| Yellow | SCL   |

The connector is keyed, but it can still be forced — always match black-to-black.

## Connecting to a breakout that only has pin headers

Three practical options, easiest first:

1. **Pre-crimped JST-SH pigtail** — a JST connector on one end and four tinned
   leads on the other. Solder the four leads to the breakout's header pins.
   Recommended: no crimping, and you only work with familiar 0.1" pins.
2. **Adapter board** — a tiny PCB with a JST-SH socket and four 0.1"
   through-holes (GND / 3.3 V / SDA / SCL). Solder four short wires from the
   breakout header to the adapter, then use a normal Qwiic cable.
3. **Cable with pre-attached headers** — a Qwiic cable that ends in male or
   female 0.1" headers; plug straight onto the breakout.

## Soldering quick reference

**Tools:** temperature-controlled iron (60–80 W), 0.8 mm rosin-core solder,
flux, brass tip cleaner or damp sponge, "helping hands", flush cutters, and
heat-shrink tubing.

**Technique:**
1. Set the iron to about **320 °C (600 °F)** for leaded or ~350 °C for lead-free.
2. **Tin** the iron tip, then **tin** the pad and the tinned wire lead separately.
3. Hold the tinned lead against the pad and heat **both together** for 1–2 s.
4. Touch the solder to the joint (not the iron); let it flow, then remove.
5. The finished joint should look **shiny and concave**, like a small cone.

**Common faults:**
- *Cold joint* — dull, cracked, or balled-up solder. Reheat until it flows.
- *Bridge* — too much solder shorting two adjacent pins. Wick it off.
- *Burned insulation* — wire heated too long/too close; trim and re-strip.

Slide **heat-shrink** over each joint and shrink it for strain relief.

## Pin mapping for this project's boards

| Board | VCC | GND | SDA | SCL | Notes |
|---|---|---|---|---|---|
| ADS1115  | VDD | GND | SDA | SCL | ADDR→GND = 0x48; ADDR→VDD = 0x49 |
| DS3231   | VCC | GND | SDA | SCL | SQW/32K pin optional |
| MPU6050  | VCC | GND | SDA | SCL | AD0→VDD = 0x69; INT optional |
| PCF8574  | VCC | GND | SDA | SCL | A0–A2 set the I2C address |

The ESP32-S3 side: SDA = GPIO4, SCL = GPIO5 (see `main/include/common.h`).

## Bus topology and pull-ups
- Use a **4-port Qwiic hub** (star topology) or short daisy-chains.
- Keep total cable length **under ~1 m**.
- Most breakouts already have **10 kΩ** pull-ups on SDA/SCL. Two or three in
  parallel gives ~3–5 kΩ, which is fine. If you hang many devices on the bus,
  use one **4.7 kΩ** pair at the hub and leave the onboard ones.

## Verify it
Flash the firmware and watch the **I2C scanner** in the boot log — every
expected address (`0x48`, `0x49`, `0x57`, `0x68`, `0x69`, PCF8574) must appear
on every boot. Then gently wiggle each cable; nothing should drop off the scan.

## Tutorials
- Adafruit — *Guide to Excellent Soldering*: https://learn.adafruit.com/adafruit-guide-excellent-soldering
- Adafruit — *Introducing Adafruit STEMMA QT*: https://learn.adafruit.com/introducing-adafruit-stemma-qt
- SparkFun — *Qwiic Connect System*: https://learn.sparkfun.com/tutorials/qwiic-connect-system
- SparkFun — *How to Solder: Through-Hole Soldering*: https://learn.sparkfun.com/tutorials/how-to-solder-through-hole-soldering
