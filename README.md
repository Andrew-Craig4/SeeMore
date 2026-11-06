# Camera node and base station: JSON control over LoRa

Two sketches:

- `BaseStation/` runs on any Heltec board. Type a JSON command in the Serial Monitor (115200 baud, line ending "Newline"). The base station sends it over LoRa and prints every reply.
- `CameraNode/` runs on the camera unit (Heltec WiFi LoRa 32 V4 R8). It carries out each command and replies with JSON. It also sends a `done` message when a move finishes.

Radio settings match the original Tx/Rx test sketches: 915 MHz, SF7, 125 kHz, CR 4/5.

**Libraries** (Library Manager): ArduinoJson (v7) and AccelStepper. The cameras use the small driver in `arducam.h` instead of the Arducam library. That library is hard-wired to the global `SPI` object, and Heltec's LoRa driver already uses that object for the radio.

**Wiring:** see `pins.h` (Test 1, direct drive).

## Commands

| Command | Example | What it does |
|---|---|---|
| `ping` | `{"id":1,"cmd":"ping"}` | Link check |
| `status` | `{"id":2,"cmd":"status"}` | Position, busy flag, limit switches, cameras, speed |
| `move` | `{"id":3,"cmd":"move","pan":30,"tilt":-10}` | Go to absolute output angles in degrees. Either axis can be left out. |
| `jog` | `{"id":4,"cmd":"jog","pan":-5}` | Move relative to the current position |
| `stop` | `{"id":5,"cmd":"stop"}` | Decelerate to a stop |
| `zero` | `{"id":6,"cmd":"zero"}` | Set the current position as pan 0, tilt 0 |
| `speed` | `{"id":7,"cmd":"speed","v":600}` | Max speed in half-steps/s (100–1000) |
| `release` | `{"id":8,"cmd":"release"}` | Turn the coils off. Moves already do this when they finish. |
| `snap` | `{"id":9,"cmd":"snap","cam":"tele","res":"VGA"}` | Take a photo and report its size. `cam`: `tele` or `wide`. `res`: QVGA, VGA, SVGA, HD, UXGA, FHD, QXGA, 5MP. |

`id` is optional. Replies echo it back. Example reply and move-finished message:

```
{"id":3,"ok":true,"pan":0,"tilt":0,"busy":true}
{"evt":"done","id":3,"pan":30,"tilt":-10,"busy":false,"lim":"open"}
```

Soft limits are pan ±170° and tilt ±60°. Angles are tracked by step counting from wherever the head was when the board powered up. Use `zero` once the head is pointing straight ahead.

The photo is not sent over LoRa yet. `snap` only confirms that a JPEG was captured and reports its size.
