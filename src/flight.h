#pragma once
#include "raylib.h"

struct PlaneState {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;    // heading, degrees; increases turning right
    float pitch = 0.0f;  // degrees; positive = nose up / climbing
    float roll = 0.0f;   // degrees; positive = banking right
    float speed = 0.0f;  // forward speed, m/s
    bool airborne = false;
};

// Reads keyboard input and advances the plane's orientation, speed and
// position by dt seconds. Controls: Up/Down = pitch, Left/Right = roll
// (which also turns the plane), A/D = rudder yaw, W/S = throttle.
// While grounded, the plane rolls along the ground on its heading only and
// needs enough speed and nose-up pitch to lift off; groundHeight is the
// terrain height directly under the plane, used both for ground-rolling
// and as a soft floor once airborne.
void UpdatePlaneControls(PlaneState &plane, float dt, float groundHeight);

// Unit vector the plane's nose currently points along, in world space.
Vector3 GetPlaneForward(const PlaneState &plane);
