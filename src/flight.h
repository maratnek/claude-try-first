#pragma once
#include "raylib.h"
#include "input.h"

struct PlaneState {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;    // heading, degrees; increases turning right
    float pitch = 0.0f;  // degrees; positive = nose up / climbing
    float roll = 0.0f;   // degrees; positive = banking right
    float speed = 0.0f;  // forward speed, m/s
    bool airborne = false;
};

// Advances the plane's orientation, speed and position by dt seconds from
// the given input (roll also turns the plane).
// While grounded, the plane rolls along the ground on its heading only and
// needs enough speed and nose-up pitch to lift off; groundHeight is the
// terrain height directly under the plane, used both for ground-rolling
// and as a soft floor once airborne.
void UpdatePlaneControls(PlaneState &plane, const FlightInput &input, float dt, float groundHeight);

// Unit vector the plane's nose currently points along, in world space.
Vector3 GetPlaneForward(const PlaneState &plane);
