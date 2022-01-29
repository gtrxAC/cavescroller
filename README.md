# ![CaveScroller](branding/logo.png)
Move around in a scrolling cave and avoid falling spikes. Entry for the [raylib 5K gamejam](https://itch.io/jam/raylib-5k-gamejam).
* [itch.io page](https://gtrxac.itch.io/cavescroller)

## Screenshots
![](branding/screenshots.png)

## Gameplay
* Runs on Web (PC and Mobile)
* Avoid hitting falling spikes and the cave walls
* The cave gets harder to move around in as you progress
* 3 difficulty levels

## Controls
There are two control modes: keyboard mode and touch mode. Touch mode also works with the mouse, but keyboard is recommended on PC. You can change the control mode in Options.
* Press WASD or arrow keys to move around. Going below the map will kill you, and horizontal movement is restricted to the left third of the screen.
* In touch mode, tapping on the screen will move you to that position. In a way, this is easier than the keyboard controls, so the game is faster to make up for that.
* If the level is impossible (the cave is steeper than the player movement speed, shouldn't happen in most cases), you can press R to generate a new level. This will reset your score and lives.
* Press ESC to go to the main menu.
* Press B in game to open the debug screen. In touch mode, there is a link in Options.
* Press G to quickly toggle low/high graphics.
* Whatever you do, don't press T when in game.

## Building
Currently only web building from Linux is supported, but it shouldn't be too hard to port to other platforms that support raylib.
1. Install emsdk into `cavescroller/emsdk`. You can follow [this tutorial](https://emscripten.org/docs/getting_started/downloads.html).
2. Clone [raylib](https://github.com/raysan5/raylib) into `cavescroller/raylib`.
3. Run `./setup.sh` to build raylib.
4. Run `./build.sh` to compile the game.

## Credits
* Font: [IBM PC BIOS 8×8](https://int10h.org/oldschool-pc-fonts/fontlist/font?ibm_bios), (c) 2016-2020 VileR
* Everything else made by me using [GIMP](https://www.gimp.org/), [mtPaint](http://mtpaint.sourceforge.net) and [rFXGen](https://raylibtech.itch.io/rfxgen).