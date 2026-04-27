# BrickWarrior

Brickout clone I wrote in the 90s

WIP porting it to run on modern systems.

## Separation of Responsibilities

* main.c
    * This is to be game-agnostic.
    * No game-specific code should be here.
    * This is responsible for Sokol integration.
    * This creates a frame buffer and allows the game to draw on it.
    * This is the initial execution entry point, aka 'main'.
* game.c/h
    * This is where things start getting game-specific.
    * This is loosely modelled after pico-8: init, update, draw, deinit.
* image.c/h
    * This is a wrapper for stb_image for loading PNGs.

## Building

```bash
make macos-app
```

I've kept things as simple as possible with a hand-rolled Makefile.
