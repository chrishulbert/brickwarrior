// The intention re separation of responsibilities is for the game-specific code to start here.

#pragma once

#include <stdint.h>

#define SCREENWIDTH (640)
#define SCREENHEIGHT (480)
#define TITLE ("BrickWarrior")

void game_draw(uint32_t *framebuffer);
