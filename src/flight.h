#pragma once
#include "raylib.h"
#include "input.h"

constexpr float kMaxSpeed = 60.0f;  // m/s

enum class LandingResult { None, Safe, Hard };

struct PlaneState {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;    // heading, degrees; increases turning right
    float pitch = 0.0f;  // degrees; positive = nose up / climbing
    float roll = 0.0f;   // degrees; positive = banking right
    float speed = 0.0f;  // forward speed, m/s
    bool airborne = false;
    float airTime = 0.0f;  // seconds since liftoff
    LandingResult landing = LandingResult::None;
};

// Advances the plane's orientation, speed and position by dt seconds from
// the given input (roll also turns the plane).
// While grounded, the plane rolls along the ground on its heading only and
// needs enough speed and nose-up pitch to lift off; groundHeight is the
// terrain height directly under the plane, used both for ground-rolling
// and as a soft floor once airborne. When the airborne plane reaches the
// ground, plane.landing becomes Safe (plane returns to ground-rolling) or
// Hard (caller should treat it as a crash); it resets on the next liftoff.
void UpdatePlaneControls(PlaneState &plane, const FlightInput &input, float dt, float groundHeight);

// Unit vector the plane's nose currently points along, in world space.
Vector3 GetPlaneForward(const PlaneState &plane);
