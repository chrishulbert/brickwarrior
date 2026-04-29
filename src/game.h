// The intention re separation of responsibilities is for the game-specific code to start here.

#pragma once

#include <stdint.h>

#define SCREENWIDTH (640)
#define SCREENHEIGHT (480)
#define TITLE ("BrickWarrior")

// Pico8-inspired lifecycle functions:
void game_init();
void game_deinit();
void game_update(uint32_t* framebuffer, float duration, int* keys, int* chars); // It'd be more pure if update wasn't passed the frame admittedly...
void game_draw(uint32_t* framebuffer);
bool game_should_quit();
