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
    bool exit = false;
    bool menu = false;
    bool cycleGraphics = false;
    int volumeStep = 0;
};

struct InputState {
    bool touchUsed = false;
    bool stickActive = false;
    Vector2 stickOrigin = {0.0f, 0.0f};
    Vector2 stickPos = {0.0f, 0.0f};
    bool restartHeld = false;
    bool menuHeld = false;
    bool gfxHeld = false;
    bool volDownHeld = false;
    bool volUpHeld = false;
};

// Combines keyboard, touch and mouse. The mouse only presses on-screen buttons; the stick and throttle are touch-only.
// Restart/Menu buttons only register when endButtonsActive (crashed or finished).
// Graphics/volume buttons register when settingsActive and either touch has been used or an end screen is up.
FlightInput ReadFlightInput(InputState &state, bool endButtonsActive, bool settingsActive);

// Draws the Restart/Menu buttons on end screens (always) and the settings buttons there; the stick and
// throttle buttons, and settings during flight, only once touch has been used.
void DrawTouchOverlay(const InputState &state, bool endButtonsShown, bool settingsShown, const char *gfxLabel);
