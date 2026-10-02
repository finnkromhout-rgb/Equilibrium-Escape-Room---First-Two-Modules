# ESP32 module — boiler room

The physical build: a valve with a hidden phone inside, an LED strip that acts as a progress bar, and an OLED screen that reveals each step's pressure value.

See the [main README](../README.md) for the full OOCSI data exchange documentation and overall puzzle flow.

## Hardware

- ESP32-C6 dev board
- SK6812 RGBW LED strip (this build uses 30 LEDs)
- 1.3" **SH1106**-driver I2C OLED display, 128x64 (not SSD1306 — a similar-looking but incompatible chip)
- A phone running the "Mr. Kip" OOCSI Thing, mounted behind the valve so it's hidden from players
- Breadboard, jumper wires
- A physical pressure gauge prop mounted near the valve, used by players to decode which pressure value maps to which direction

## Wiring

| Component | Pin |
|---|---|
| LED strip data | GPIO5 |
| LED strip GND / 5V | GND / 5V |
| OLED SDA | GPIO6 |
| OLED SCL | GPIO7 |
| OLED VCC | 3.3V |
| OLED GND | GND |

If GPIO6/7 aren't free on your specific board, use any other free GPIO pair and update `OLED_SDA`/`OLED_SCL` in the sketch.

## Libraries (Arduino Library Manager)

- `OOCSI` (ESP32 client)
- `Adafruit NeoPixel`
- `Adafruit GFX Library`
- `Adafruit SH110X` — **not** Adafruit SSD1306, that library produces a scrambled display on this OLED's SH1106 driver chip

## Configuration

Edit these constants at the top of `equilibrium_boiler.ino` before uploading:

- `ssid` / `password` — your WiFi network
- `SEQUENCE_PRESSURE` / `SEQUENCE_DIR` — the puzzle's 3-step sequence (currently East=1.0 bar → South=2.0 bar → West=3.0 bar)
- `D1_EAST`, `D1_WEST`, `D1_MAX_ABS`, `D1_TOL` — direction thresholds, **must be calibrated** against your actual mounted phone's sensor readings (see below)
- `HOLD_DURATION` — how long the valve must be held in the correct position (default 3000ms)
- `LED_START_DELAY` — pause before the LED progress bar starts filling, so it doesn't feel instant (default 500ms)

Reordering `SEQUENCE_PRESSURE`/`SEQUENCE_DIR` does **not** require any changes in the HTML module — the terminal just displays whatever pressure/step values it's sent, it has no sequence logic of its own.

## Calibration

`mr-kip_d1` is a single tilt axis ranging roughly from -2.0 to 2.0, wrapping at the ends (so a value can flip between e.g. +1.99 and -2.0 at the same physical position). East and West sit clearly apart on this axis, but South sits right at the wrap point, which is why it's detected differently (checking closeness to the axis's maximum magnitude, not a fixed number) rather than with a simple subtraction like East/West use.

To calibrate:
1. Mount the phone on the actual valve mechanism (readings change once physically mounted vs. held by hand).
2. Upload the sketch and open the Serial Monitor.
3. Turn the valve to each position used in your sequence and note the printed `mr-kip_d1` values.
4. Update `D1_EAST` / `D1_WEST` / `D1_TOL` to match.

North is intentionally excluded from puzzle logic since it isn't used in the sequence — no need to distinguish it from South.

## Why the OOCSI name is randomized

The OOCSI server only allows one active connection per registered name. Re-uploading code restarts the ESP32, which tries to reconnect immediately, often before the server has timed out the previous connection, causing a rejection (`name already registered`). Appending a random number at boot avoids this entirely, with no need to manually rename before every upload.
