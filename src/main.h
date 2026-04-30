// This contains some function declarations so the game can call back to main, using main as a
// form of an graphics library abstraction layer.
// eg if Sokol is replaced by SDL, game shouldn't change.

#pragma once

typedef struct {
    bool isActual; // 0 means this is a 'null terminator'.
    bool isClick; // false=simply a mouse movement.
    int button; // 0=left.
    float x; // Position in pixels, invalid when mouse is locked.
    float y;
    float dx; // relative horizontal mouse movement since last frame, always valid
    float dy; // relative vertical mouse movement since last frame, always valid
} MouseEvent;

void lock_mouse(bool lock);
bool is_mouse_locked();
