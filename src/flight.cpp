#include "flight.h"
#include "raymath.h"
#include <cmath>

namespace {

constexpr float kPitchRate = 60.0f;    // deg/sec while held
constexpr float kRollRate = 90.0f;     // deg/sec while held
constexpr float kYawRate = 40.0f;      // deg/sec rudder while held
constexpr float kBankTurnRate = 0.6f;  // extra yaw deg/sec per degree of roll
constexpr float kMaxPitch = 60.0f;
constexpr float kMaxRoll = 75.0f;
constexpr float kLevelRate = 50.0f;    // deg/sec auto-level when no input
constexpr float kAccel = 15.0f;        // m/s^2 while throttling
constexpr float kMinSpeed = 0.0f;
constexpr float kMinAltitudeAboveGround = 1.0f;
constexpr float kWheelHeight = 0.3f;
constexpr float kLiftoffSpeed = 18.0f;
constexpr float kLiftoffPitch = 5.0f;
constexpr float kLandingMaxSinkRate = 6.0f;  // m/s
constexpr float kLandingMaxSpeed = 30.0f;    // m/s
constexpr float kLandingMaxPitch = kLiftoffPitch;  // deg, nose up or down; must not allow liftoff pitch or a landing re-lifts immediately
constexpr float kLandingMaxRoll = 15.0f;     // deg
constexpr float kMinAirTimeForLanding = 1.5f;  // s, so liftoff isn't read as a touchdown

float MoveToward(float current, float target, float maxDelta) {
    if (fabsf(target - current) <= maxDelta) return target;
    return current + (target > current ? maxDelta : -maxDelta);
}

}  // namespace

void UpdatePlaneControls(PlaneState &plane, const FlightInput &input, float dt, float groundHeight) {
    float pitchInput = input.pitch;
    float rollInput = input.roll;
    float yawInput = input.yaw;
    float throttleInput = input.throttle;

    if (pitchInput != 0.0f) {
        plane.pitch += pitchInput * kPitchRate * dt;
    } else {
        plane.pitch = MoveToward(plane.pitch, 0.0f, kLevelRate * dt);
    }
    plane.pitch = Clamp(plane.pitch, -kMaxPitch, kMaxPitch);

    if (rollInput != 0.0f) {
        plane.roll += rollInput * kRollRate * dt;
    } else {
        plane.roll = MoveToward(plane.roll, 0.0f, kLevelRate * dt);
    }
    plane.roll = Clamp(plane.roll, -kMaxRoll, kMaxRoll);

    // Rudder input plus banking both turn the plane, like a real aircraft.
    plane.yaw += yawInput * kYawRate * dt;
    plane.yaw += plane.roll * kBankTurnRate * dt;

    plane.speed += throttleInput * kAccel * dt;
    plane.speed = Clamp(plane.speed, kMinSpeed, kMaxSpeed);

    if (!plane.airborne) {
        // On the ground: wheels follow the terrain, nose direction only
        // steers left/right (yaw), pitch is cosmetic until liftoff.
        float yawRad = plane.yaw * DEG2RAD;
        Vector3 groundForward = {sinf(yawRad), 0.0f, cosf(yawRad)};
        plane.position = Vector3Add(plane.position, Vector3Scale(groundForward, plane.speed * dt));
        plane.position.y = groundHeight + kWheelHeight;

        if (plane.speed >= kLiftoffSpeed && plane.pitch > kLiftoffPitch) {
            plane.airborne = true;
            plane.airTime = 0.0f;
            plane.landing = LandingResult::None;
        }
    } else {
        Vector3 forward = GetPlaneForward(plane);
        plane.position = Vector3Add(plane.position, Vector3Scale(forward, plane.speed * dt));

        plane.airTime += dt;

        float minY = groundHeight + kMinAltitudeAboveGround;
        if (plane.position.y <= minY && plane.airTime > kMinAirTimeForLanding) {
            float sinkRate = -plane.speed * sinf(plane.pitch * DEG2RAD);
            bool gentle = sinkRate <= kLandingMaxSinkRate && plane.speed <= kLandingMaxSpeed &&
                          fabsf(plane.pitch) <= kLandingMaxPitch && fabsf(plane.roll) <= kLandingMaxRoll;
            if (gentle) {
                plane.landing = LandingResult::Safe;
                plane.airborne = false;
                plane.position.y = groundHeight + kWheelHeight;
            } else {
                plane.landing = LandingResult::Hard;
            }
        } else if (plane.position.y < minY) {
            plane.position.y = minY;
        }
    }
}

Vector3 GetPlaneForward(const PlaneState &plane) {
    float yawRad = plane.yaw * DEG2RAD;
    float pitchRad = plane.pitch * DEG2RAD;
    return (Vector3){
        sinf(yawRad) * cosf(pitchRad),
        sinf(pitchRad),
        cosf(yawRad) * cosf(pitchRad),
    };
}
