# Equilibrium

A two-room escape room module built around an arctic research station hit by a snowstorm. Two teams, split into separate rooms, must communicate and cooperate over a shared [OOCSI](https://oocsi.id.tue.nl/) network to restabilize the station before conditions become unbearable.

## Story

A group visiting an arctic research station gets caught in a sudden snowstorm, throwing the heating system out of balance. The group splits into two teams: one heads to the boiler room, now sweltering hot, while the other stays in the control room, freezing cold. Cut off from each other, both teams must work together through the station's systems to restore balance.

## Structure

- [`esp32-module/`](./esp32-module) — the physical boiler room build: a valve with a hidden phone sensor, an LED strip progress bar, and an OLED display.
- [`html-module/`](./html-module) — the browser-based control room terminal: a PIN-locked screen with a mouse-only signal-tuning puzzle.

Each module has its own README with hardware/setup details specific to that part. This document covers how the two communicate.

## How the puzzle works

1. The control room solves a signal-tuning puzzle, which transmits a request for the next step.
2. The ESP32's OLED reveals a pressure value for that step.
3. Using a physical pressure gauge prop near the valve, the boiler room team decodes which direction that pressure corresponds to, then turns the valve there and holds it.
4. An LED strip fills as a progress bar while holding.
5. This repeats for a 3-step sequence.
6. Once complete, the LED strip turns solid blue, the OLED confirms the system is stabilized, and the control room's temperature reading updates to show the room warming up.

## OOCSI network

Both modules connect to the same OOCSI server (`oocsi.id.tue.nl`) and communicate entirely over one shared channel: **`OOCSI-things/team-3`**.

Each device registers under its own unique name (`OOCSIName` in the ESP32 sketch), but all data exchange happens through messages sent on the shared channel above, not through direct device-to-device connections. Both modules subscribe to this channel and react only to the message keys relevant to them, ignoring everything else.

### Data exchange

| Key | Sent by | Received by | Type | Purpose |
|---|---|---|---|---|
| `mr-kip_d1` | Phone (Mr. Kip OOCSI Thing) | ESP32 module | float | Live rotation/tilt value from the phone mounted behind the valve. Used to detect which direction the valve is currently turned to. |
| `unlock_next` | HTML module | ESP32 module | int | Sent when the control room's tuning puzzle locks successfully. Value is the step number being unlocked (1, 2, 3). Tells the ESP32 to reveal that step's pressure value. |
| `revealed_step` | ESP32 module | HTML module | int | Echoed back after a step is revealed, so the terminal's transmission log stays in sync with what's shown on the OLED. |
| `revealed_pressure` | ESP32 module | HTML module | float | The pressure value (in bar) revealed for the current step, logged on the terminal. |
| `step_done` | ESP32 module | HTML module | int | Sent when the valve has been held correctly for the required duration. Value is the step number just completed. Triggers a new tuning puzzle round on the terminal. |
| `temp` | ESP32 module | HTML module | int | Sent once all steps are complete. Updates the control room's displayed temperature (e.g. from -20°C to -10°C), signaling the room has stabilized. |

### Why one shared channel

Using a single channel keeps the setup simple: every device just needs the same channel name, and OOCSI handles delivering messages to all subscribers. Each side only acts on the keys it cares about and ignores the rest (e.g. the ESP32 ignores its own `revealed_step`/`revealed_pressure` messages since nothing in its code listens for those keys).

### Testing the connection

Any OOCSI client subscribed to `OOCSI-things/team-3` (including the [OOCSI dashboard](https://oocsi.id.tue.nl/datatiles)) can be used to observe live traffic between the two modules while debugging, without needing both physical modules running at once.
