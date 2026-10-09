#pragma once
#include "raylib.h"
#include "input.h"

struct PlaneParams {
    float maxSpeed;
    float accel;         // m/s^2 thrust at full engine power
    float throttleRate;  // engine power change per second while W/S is held
    float levelPower;    // engine power that holds level flight; below it the plane trims into a glide
    float glideRatio;    // unpowered glide trim, distance per unit height
    float gravity;       // m/s^2
    float liftDamping;   // 1/s, how fast lift cancels a fall at full lift
    float groundDrag;    // m/s^2 rolling resistance
    float brakeDecel;    // m/s^2 wheel brake, grounded with engine power at zero
    float dragLinear;    // 1/s
    float dragQuad;      // 1/m
    float stallSpeed;    // m/s, airborne below this the nose drops
    float liftoffSpeed;  // m/s
    float liftoffPitch;  // deg
    float controlSpeed;  // m/s at which control authority reaches 1
    float pitchRate;     // deg/sec at full authority
    float rollRate;
    float yawRate;          // deg/sec rudder-only yaw at full authority
    float taxiTurnRadius;   // m, tightest ground turn; taxi yaw rate = speed / radius so it never spins in place
    float taxiMaxTurnRate;  // deg/sec cap on taxi steering
    float stallDropRate;  // deg/sec nose-drop at zero airspeed
    float stallAuthority; // authority multiplier while stalled
    float stallPitchFloor;  // deg, stall nose-drop stops this far below level
    float maxTurnRate;      // deg/sec cap on the bank-driven turn rate (g * tan(bank) / v)
    float maxPitch;
    float maxRoll;
    float levelRate;        // deg/sec auto-level when no input
    float wheelHeight;
    float minAltitudeAboveGround;
    float landingMaxSinkRate;   // m/s
    float landingMaxSpeed;      // m/s
    float landingMaxRoll;       // deg
    float landingMaxNoseDown;   // deg; nose-up limit is liftoffPitch so a landing can't re-lift
    float fuelCapacity;         // units, about 1 l each
};

PlaneParams BiplaneParams();

enum class LandingResult { None, Safe, Hard };

struct PlaneState {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;    // heading, degrees; increases turning right
    float pitch = 0.0f;  // degrees; positive = nose up / climbing
    float roll = 0.0f;   // degrees; positive = banking right
    float speed = 0.0f;  // forward speed, m/s
    float enginePower = 0.0f;  // 0..1, persists until changed
    float fallSpeed = 0.0f;    // m/s downward from lost lift, on top of motion along the nose
    bool airborne = false;
    bool damaged = false;
    float fuel = 0.0f;  // units; nothing burns it yet, so it only sets the crash explosion size
    float airTime = 0.0f;  // seconds since liftoff
    LandingResult landing = LandingResult::None;
};

// Advances the plane's orientation, speed and position by dt seconds from
// the given input (airborne, bank angle turns the plane at g*tan(bank)/v and
// costs vertical lift by cos(bank); rudder yaws the plane flat, without bank; grounded,
// steering rate is proportional to ground speed). input.throttle changes
// plane.enginePower, which stays put; thrust = power * params.accel and
// never brakes in the air. Control authority scales with airspeed squared,
// drag decays speed, gravity pulls speed along the climb angle, and below
// params.stallSpeed in the air lift fades so the plane falls and the nose
// drops. groundSlopeDeg is the terrain slope along the heading; grounded
// with no pitch input, pitch settles to it (clamped well under liftoffPitch
// so slope alone never lifts the plane off). While grounded, the plane rolls
// along the ground on its heading only and needs enough speed and nose-up
// pitch to lift off; with power at zero, holding throttle-down brakes the
// wheels. groundHeight is the terrain height directly under the plane, used
// both for ground-rolling and as a soft floor once airborne. When the
// airborne plane reaches the ground, plane.landing becomes Safe (plane
// returns to ground-rolling) or Hard (caller should treat it as a crash); it
// resets on the next liftoff.
void UpdatePlaneControls(PlaneState &plane, const PlaneParams &params, const FlightInput &input, float dt, float groundHeight, float groundSlopeDeg);

float FuelFraction(const PlaneState &plane, const PlaneParams &params);

// m/s the plane is descending (positive = down), from lost lift plus the nose's vertical component.
float SinkRate(const PlaneState &plane);

enum class GroundWarning { None, Info, Caution, Danger };

// How loudly to warn about the ground: only below warnAltitude and while sinking;
// Caution/Danger by seconds to impact, or Caution when sinking faster than a safe landing allows.
GroundWarning GetGroundWarning(float heightAboveGround, float sinkRate, const PlaneParams &params);

// Unit vector the plane's nose currently points along, in world space.
Vector3 GetPlaneForward(const PlaneState &plane);
