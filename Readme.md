# BrickWarrior

Brickout clone I wrote in high school in the 90s 👴

[Download for macOS](https://github.com/chrishulbert/brickwarrior/releases)

Ported to run on modern systems using Sokol.

![Screenshot](https://github.com/chrishulbert/brickwarrior/raw/main/readme/screenshot.png)

## Building

I've deliberately tried to make this as easy to build as possible. I've kept things as simple as possible with a hand-rolled Makefile.

```bash
make macos-app
make linux
make win # WIP
```

![Screenshot 2](https://github.com/chrishulbert/brickwarrior/raw/main/readme/screenshot2.png)

## Structure

* main.c
    * This is to be game-agnostic.
    * No game-specific code should be here.
    * This is responsible for Sokol integration.
    * This creates a frame buffer and allows the game to draw on it.
    * This is the initial execution entry point, aka 'main'.
* game.c/h
    * This is where things start getting game-specific.
    * This is loosely modelled after pico-8: init, update, draw, deinit.
    * The game itself shouldn't be too coupled to Sokol besides the parameters it passes to game, as it might be migrated to SDL later.
* image.c/h
    * This is a wrapper for stb_image for loading PNGs.

![Screenshot 3](https://github.com/chrishulbert/brickwarrior/raw/main/readme/screenshot3.png)
