#pragma once
#include "raylib.h"

struct PlaneState {
    Vector3 position = {0.0f, 20.0f, 0.0f};
    float yaw = 0.0f;    // heading, degrees; increases turning right
    float pitch = 0.0f;  // degrees; positive = nose up / climbing
    float roll = 0.0f;   // degrees; positive = banking right
    float speed = 25.0f; // forward speed, m/s
};

// Reads keyboard input and advances the plane's orientation, speed and
// position by dt seconds. Controls: Up/Down = pitch, Left/Right = roll
// (which also turns the plane), A/D = rudder yaw, W/S = throttle.
void UpdatePlaneControls(PlaneState &plane, float dt);

// Unit vector the plane's nose currently points along, in world space.
Vector3 GetPlaneForward(const PlaneState &plane);
