#include <string.h>

#include "game.h"
#include "image.h"

void game_draw(uint32_t *framebuffer) {
    Image* back = image_load("assets/img/back.png");
    memcpy(framebuffer, back->data, SCREENWIDTH * SCREENHEIGHT * 4);
    image_free(back);
}
