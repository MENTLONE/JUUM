# JUUM Hardware Controller — Rev 01

This is the first USB-serial controller prototype for JUUM. The ESP32-S3 reads physical controls; the JUUM app continues to generate audio.

## Repository files
- `hardware/esp32s3/JUUM_Controller.ino` — Arduino firmware.
- `index.html` — browser/Web Serial bridge and connection controls.
- `electron/main.cjs` — serial-port selection support for the Windows standalone app.

## Parts list
- 1 × ESP32-S3 development board with native USB CDC and exposed GPIO 1–18 and GPIO 40.
- 8 × 10 kΩ linear (B10K) potentiometers.
- 4 × incremental rotary encoders, each with A/B outputs and a push switch.
- 16 × normally-open momentary push buttons for sequencer steps.
- 4 × normally-open momentary push buttons for Play, Stop, Randomize Notes, and Panic.
- 2 × MCP23017 16-bit I²C GPIO expanders (DIP or breakout boards).
- 16 × WS2812B/NeoPixel LEDs, or one 16-pixel strip/ring that can be arranged behind the step buttons.
- 1 × 74AHCT125 (or equivalent 5 V logic buffer) recommended for reliable 3.3 V ESP32-to-5 V LED data signaling.
- 1 × regulated 5 V, 1 A supply for the LEDs (recommended; do not feed LED-strip current through the ESP32 board).
- 8 × 100 nF ceramic capacitors for optional potentiometer ADC smoothing (one from each wiper to GND).
- 2 × 4.7 kΩ I²C pull-up resistors only if the breakout boards do not already provide pull-ups.
- 1 × breadboard/perfboard, hookup wire, headers, USB data cable, enclosure, knobs and button caps.
- Optional: 1000 µF electrolytic capacitor across LED +5 V/GND near the LEDs; 330 Ω resistor in series with LED data.

Estimated prototype cost: roughly $55–$130 depending on board, switches, LEDs and enclosure.

## Pin-by-pin wiring

### 8 potentiometers
Each potentiometer has three terminals: one outer terminal to **3V3**, the other outer terminal to **GND**, and the middle wiper to its ADC pin. If the knob turns backwards, swap the two outer terminals.

| Control | ESP32-S3 pin |
|---|---:|
| 1 Main Volume | GPIO 1 / ADC |
| 2 Wave Character | GPIO 2 / ADC |
| 3 Cutoff | GPIO 3 / ADC |
| 4 Resonance | GPIO 4 / ADC |
| 5 Sub Volume | GPIO 5 / ADC |
| 6 Reverb | GPIO 6 / ADC |
| 7 Delay | GPIO 7 / ADC |
| 8 Phase Bloom | GPIO 8 / ADC |

Use 3.3 V only on potentiometer circuits. Never feed 5 V into ESP32 GPIO/ADC pins.

### Four rotary encoders
For each encoder, connect common (C/GND) to GND and A/B to the assigned GPIOs. Use the encoder's push-switch contacts separately and connect them to the MCP23017 inputs listed below. The firmware enables pull-ups on A/B.

| Encoder | A | B | Push switch to MCP23017 |
|---|---:|---:|---|
| Chord | GPIO 9 | GPIO 10 | 0x21 GPA0 (pin 0) |
| Tempo | GPIO 11 | GPIO 12 | 0x21 GPA1 (pin 1) |
| Preset | GPIO 13 | GPIO 14 | 0x21 GPA2 (pin 2) |
| Master volume | GPIO 15 | GPIO 16 | 0x21 GPA3 (pin 3) |

### I²C bus and button expanders
Both MCP23017 boards share the same I²C bus. Connect both boards' VDD to 3V3, VSS to GND, SDA to GPIO 17, SCL to GPIO 18. Tie RESET high to 3V3 (preferably through 10 kΩ if the board does not already do so). Connect A0/A1/A2 to select addresses:
- Step expander: A0/A1/A2 = GND/GND/GND → 0x20.
- Function expander: A0/A1/A2 = 3V3/GND/GND → 0x21.
Connect all button contacts between the assigned MCP23017 input and GND; firmware uses internal pull-ups.

| Expander address | GPIO pins | Connect to |
|---|---|---|
| 0x20 | GPA0–GPA7, GPB0–GPB7 (pins 0–15) | Sequencer steps 1–16, in order |
| 0x21 | GPA0–GPA3 (pins 0–3) | Encoder push switches: chord, tempo, preset, master |
| 0x21 | GPA4 (pin 4) | Play |
| 0x21 | GPA5 (pin 5) | Stop |
| 0x21 | GPA6 (pin 6) | Randomize Notes |
| 0x21 | GPA7 (pin 7) | Panic / all-notes-off |

### Step LEDs
For 16 WS2812B pixels:
- LED strip +5 V → a regulated 5 V supply (USB 5 V may work for a small strip at low brightness, but a separate 5 V supply is recommended).
- LED GND → supply GND and ESP32 GND (common ground is mandatory).
- LED DIN → GPIO 40 through a 330 Ω series resistor.
- Optional 1000 µF capacitor across +5 V and GND at the strip input.
- Do not power a large LED strip from the ESP32 3V3 pin. Keep brightness low during prototype testing.

## Arduino IDE setup
1. Install the Espressif **esp32** board package.
2. Install libraries from Library Manager: **Adafruit MCP23017 Arduino Library**, **FastLED**, and **ArduinoJson**.
3. Select an ESP32-S3 board that exposes the listed GPIOs.
4. Enable **USB CDC On Boot**. Use the native USB connector that is wired to the ESP32-S3 USB peripheral, not a charge-only cable.
5. Upload `JUUM_Controller.ino`. Open Serial Monitor at 115200 only for diagnostics; close it before connecting JUUM, because only one app can own the serial port at a time.

## Control map
- Pots: Main Volume, Wave Character, Cutoff, Resonance, Sub Volume, Reverb, Delay, Phase Bloom.
- Encoder 1: cycle chord buttons.
- Encoder 2: change sequencer tempo by 1 BPM per detent.
- Encoder 3: select previous/next preset.
- Encoder 4: adjust master volume.
- Encoder push buttons: toggle chord sequence mode, load selected preset, toggle arpeggiator, Panic.
- Function buttons: Play, Stop, Randomize Notes, Panic.
- Step buttons toggle the matching 16-step sequencer step.

## USB protocol
Each message is a single JSON object followed by a newline:
- ESP32 → JUUM: `{"type":"pot","id":0,"value":2048}`
- ESP32 → JUUM: `{"type":"step","id":3}`
- ESP32 → JUUM: `{"type":"encoder","id":1,"delta":-1}`
- ESP32 → JUUM: `{"type":"encoderPress","id":0}`
- ESP32 → JUUM: `{"type":"button","id":0}`
- JUUM → ESP32: `{"type":"leds","steps":[false,true,...],"playhead":4}`

## First power-up checklist
1. Build and test the button/encoder wiring with USB disconnected from JUUM.
2. Check I²C addresses 0x20 and 0x21 if the firmware reports an expander error.
3. Verify all potentiometer wipers stay between 0 and 3.3 V.
4. Close Arduino Serial Monitor, open JUUM in Chrome/Edge, click **CONNECT ESP32-S3**, and select the board.
5. Turn a knob slowly; confirm the corresponding JUUM parameter changes. Test one step button and transport before assembling the panel.
6. For the Windows standalone app, rebuild after the repository's Electron serial-permission change.
