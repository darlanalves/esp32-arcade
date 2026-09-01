# ESP32 Arcade

An ESP32-WROOM-32 arcade built around Bluetooth gamepads, a 240x280 ST7789V2 SPI display, and a small buzzer. It currently contains four games: Pong, Arkanoid, Snake, and Asteroids.

The project uses [Bluepad32](https://bluepad32.readthedocs.io/) for wireless controller support and Adafruit GFX/ST7789 libraries for rendering. The menu assigns connected controllers to player slots and sets the controller LED color to match that player's on-screen color.

## Hardware

The display is wired in SPI mode. Keep every device on a shared ground and use 3.3V logic and power for the display.

| Display pin            | ESP32-WROOM-32 pin |
| ---------------------- | -----------------: |
| `GND`                  |              `GND` |
| `VCC`                  |              `3V3` |
| `SCK` / `SCL` / `CLK`  |          `GPIO 18` |
| `MOSI` / `SDA` / `DIN` |          `GPIO 23` |
| `CS`                   |          `GPIO 32` |
| `DC` / `A0`            |          `GPIO 25` |
| `RST` / `RES`          |          `GPIO 27` |
| `BL` / `BLK` / `LED`   |          `GPIO 33` |

The buzzer positive lead is on `GPIO 26`; its negative lead goes to `GND`.

`GPIO 0`, `2`, `4`, `5`, `12`, and `15` are ESP32 boot-strapping pins. Do not move the TFT control signals onto them: attached display circuitry can interfere with flashing or booting.

## Build And Upload

Install the ESP32 Bluepad32 Arduino core plus these libraries:

- Bluepad32
- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library

Build the sketch from this folder:

```sh
arduino-cli compile -b esp32-bluepad32:esp32:esp32
```

Upload to the board, replacing the serial port when necessary:

```sh
arduino-cli upload -b esp32-bluepad32:esp32:esp32 -p /dev/cu.usbserial-0001
```

## Controls

In the menu, press D-pad up/down to change the highlighted game. Navigation is edge-triggered: release the D-pad before another press changes the selection. Press any face button (`A`, `B`, `X`, or `Y`) to start the highlighted game. Sticks do not start games.

The Home/system button returns to the menu from every game.

Player colors repeat by controller slot: P1 green, P2 blue, P3 cyan, P4 yellow, P5 red, P6 magenta. Pong and Arkanoid use P1/P2; Snake and Asteroids use P1.

## Games

### Pong

Two players move vertical paddles on the left and right sides. Use D-pad up/down or the left stick's vertical axis. The ball accelerates slightly after paddle hits; a point is scored when it crosses the opponent's edge. Either player can use L1/R1 to slow down or speed up the ball between 0.3x and 2.0x.

### Arkanoid

Two players control horizontal paddles in their half of the bottom edge with the left stick's horizontal axis. Bounce the ball into the brick field. Clearing all bricks advances to the next level, where cyan bricks become more common. Brick colors identify their type: red is normal, yellow triggers the reward sound, and cyan is the harder level-scaled type. Either player can use L1/R1 to adjust ball speed.

### Snake

Control the green snake with D-pad or the left stick. Eat red food to grow and earn points; hitting a wall or the snake's body ends the game. L1/R1 changes movement speed between 0.3x and 2.0x. Snake is incrementally rendered: only changed cells and HUD values are written to the display, which keeps it smooth on SPI.

### Asteroids

Aim the ship with the left stick. `A` or `X` accelerates, while `B` or `Y` fires. L2/R2 changes game speed. Asteroids wrap around the playfield edges; shooting a large asteroid splits it into two medium asteroids, then medium asteroids split into small ones. Destroy all asteroids to start the next wave.

Asteroids is rendered as small completed strips before each strip is transferred to the TFT. This avoids visible blank frames while fitting within the ESP32-WROOM-32's limited internal RAM.

## Implementation Notes

`esp32-arcade.ino` owns hardware initialization, controller connection callbacks, menu state, the shared display object, sounds, player colors, and game dispatch. Each game lives in its own `.ino` file and keeps its state inside a namespace (`Pong`, `Arkanoid`, `Snake`, or `Asteroids`) to avoid symbol collisions.

Games follow a small shared contract:

```cpp
void Game_init(ControllerPtr myControllers[]);
bool Game_update(ControllerPtr myControllers[]);
```

`Game_init()` prepares state and draws the initial frame. `Game_update()` processes input and draws one frame. Return `true` to continue, or `false` to leave the game and return to the menu. Pass controllers as a parameter rather than declaring another global controller array.

The 240x280 display reserves its bottom 18 pixels for game HUDs. `drawGameBorder()` draws the shared 5-pixel playfield border. Use `ST77XX_*` colors with Adafruit ST7789; `ST7789_*` and `ST7735_*` color names are not valid here.

## Contributing

When adding a game:

1. Create a new `*_game.ino` module and place state inside a unique namespace.
2. Implement the `Game_init()`/`Game_update()` contract above, including Home-button exit behavior.
3. Add forward declarations, a menu label, and dispatch branches in `esp32-arcade.ino`.
4. Preserve the playfield/HUD boundary and use `getPaddleColor()` for player-owned objects.
5. Avoid `tft.fillScreen()` inside a per-frame update. Prefer incremental redraws, or render small composited tiles for overlapping moving objects.
6. Compile the whole sketch with `arduino-cli compile -b esp32-bluepad32:esp32:esp32` before flashing.

For bug fixes, start with the owning game module and keep shared behavior in the launcher only when every game needs it. In particular, check RAM use when adding buffers: a full 240x262 16-bit framebuffer does not fit alongside Bluepad32, while Asteroids' 32-row render tile does.
