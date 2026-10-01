#pragma once
#include "raylib.h"

// Pitch/roll/yaw/throttle are in [-1, 1]. Signs match the original key
// mapping: pitch +1 = nose up (Down key / drag down), roll +1 = Left key /
// drag left, yaw +1 = A key, throttle +1 = W key / up button.
struct FlightInput {
    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;
    float throttle = 0.0f;
    bool restart = false;
};

struct InputState {
    bool touchUsed = false;
    bool stickActive = false;
    Vector2 stickOrigin = {0.0f, 0.0f};
    Vector2 stickPos = {0.0f, 0.0f};
    bool restartHeld = false;
};

// Combines keyboard and touch (raylib also maps the mouse to touch point 0).
FlightInput ReadFlightInput(InputState &state);

// Draws the stick and buttons; does nothing until touch has been used.
void DrawTouchOverlay(const InputState &state, bool crashed);
