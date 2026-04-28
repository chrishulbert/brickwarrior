// The intention re separation of responsibilities is for the game-specific code to start here.

#pragma once

#include <stdint.h>

#define SCREENWIDTH (640)
#define SCREENHEIGHT (480)
#define TITLE ("BrickWarrior")

// Pico8-inspired lifecycle functions:
void game_init();
void game_deinit();
void game_update(float duration);
void game_draw(uint32_t *framebuffer);
