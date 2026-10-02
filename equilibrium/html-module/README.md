# HTML module — control room terminal

A browser-based, PIN-locked terminal with a mouse-only signal-tuning puzzle. No keyboard needed for gameplay.

See the [main README](../README.md) for the full OOCSI data exchange documentation and overall puzzle flow.

## Running it

Just open `index.html` directly in a browser — no build step, no local server required. The OOCSI library is loaded from a CDN, so an internet connection is needed.

## Access code

The lock screen uses a mouse-only rotary dial (no keyboard). The code is set in `index.html`:

```js
const CORRECT_PIN = "167874";
```

Change this to whatever code your physical clues (periodic table, underlined word, etc.) lead players to.

## Puzzle flow

1. Players unlock the terminal with the 6-digit dial code.
2. A "signal tuning" puzzle appears: drag the slider into the randomly-placed clear zone and hold for ~1.5 seconds to auto-transmit.
3. This sends `unlock_next` to the ESP32 module over OOCSI.
4. Once the ESP32 module reveals and logs a step, the transmission log updates automatically.
5. Once a step is completed on the valve, a new tuning puzzle appears for the next one.
6. Once all 3 steps are done, the temperature display updates automatically (sent by the ESP32 module).

This module has no knowledge of the actual valve sequence or directions — it only sends `unlock_next` and displays whatever values the ESP32 module sends back. The sequence itself lives entirely in the ESP32 module's code.

## Notes

- All effects (CRT overlay, screen jolts, flickers) are pure CSS/JS, no external assets beyond the Google Fonts and the OOCSI library.
- If offline use is needed, the OOCSI library and fonts would need to be downloaded and referenced locally instead of via CDN.
