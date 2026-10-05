#pragma once
#include "raylib.h"
#include "input.h"

struct PlaneParams {
    float maxSpeed;
    float accel;         // m/s^2 at full throttle
    float dragLinear;    // 1/s
    float dragQuad;      // 1/m
    float stallSpeed;    // m/s, airborne below this the nose drops
    float liftoffSpeed;  // m/s
    float liftoffPitch;  // deg
    float controlSpeed;  // m/s at which control authority reaches 1
    float pitchRate;     // deg/sec at full authority
    float rollRate;
    float yawRate;
    float stallDropRate;  // deg/sec nose-drop at zero airspeed
    float stallAuthority; // authority multiplier while stalled
    float climbDecel;     // m/s^2 speed lost per unit sin(pitch), gained when diving
    float stallPitchFloor;  // deg, stall nose-drop stops this far below level
    float bankTurnRate;     // extra yaw deg/sec per degree of roll
    float maxPitch;
    float maxRoll;
    float levelRate;        // deg/sec auto-level when no input
    float wheelHeight;
    float minAltitudeAboveGround;
    float landingMaxSinkRate;   // m/s
    float landingMaxSpeed;      // m/s
    float landingMaxRoll;       // deg
    float landingMaxNoseDown;   // deg; nose-up limit is liftoffPitch so a landing can't re-lift
};

PlaneParams BiplaneParams();

enum class LandingResult { None, Safe, Hard };

struct PlaneState {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;    // heading, degrees; increases turning right
    float pitch = 0.0f;  // degrees; positive = nose up / climbing
    float roll = 0.0f;   // degrees; positive = banking right
    float speed = 0.0f;  // forward speed, m/s
    bool airborne = false;
    bool damaged = false;
    float airTime = 0.0f;  // seconds since liftoff
    LandingResult landing = LandingResult::None;
};

// Advances the plane's orientation, speed and position by dt seconds from
// the given input (roll also turns the plane). Control authority scales
// with airspeed squared, drag decays speed, and below params.stallSpeed in
// the air the nose drops until speed recovers. groundSlopeDeg is the terrain
// slope along the heading; grounded with no pitch input, pitch settles to it
// (clamped well under liftoffPitch so slope alone never lifts the plane off).
// While grounded, the plane rolls along the ground on its heading only and
// needs enough speed and nose-up pitch to lift off; groundHeight is the
// terrain height directly under the plane, used both for ground-rolling
// and as a soft floor once airborne. When the airborne plane reaches the
// ground, plane.landing becomes Safe (plane returns to ground-rolling) or
// Hard (caller should treat it as a crash); it resets on the next liftoff.
void UpdatePlaneControls(PlaneState &plane, const PlaneParams &params, const FlightInput &input, float dt, float groundHeight, float groundSlopeDeg);

// Unit vector the plane's nose currently points along, in world space.
Vector3 GetPlaneForward(const PlaneState &plane);
