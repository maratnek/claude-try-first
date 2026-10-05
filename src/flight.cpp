#include "flight.h"
#include "raymath.h"
#include <cmath>

namespace {

constexpr float kMinAirTimeForLanding = 1.5f;  // s, so liftoff isn't read as a touchdown

float MoveToward(float current, float target, float maxDelta) {
    if (fabsf(target - current) <= maxDelta) return target;
    return current + (target > current ? maxDelta : -maxDelta);
}

float Authority(float speed, float controlSpeed) {
    float r = speed / controlSpeed;
    return Clamp(r * r, 0.0f, 1.0f);
}

}  // namespace

PlaneParams BiplaneParams() {
    PlaneParams p;
    p.maxSpeed = 60.0f;
    p.accel = 12.0f;
    p.throttleRate = 2.0f;
    p.levelPower = 0.2f;
    p.glideRatio = 9.0f;
    p.gravity = 9.81f;
    p.liftDamping = 3.0f;
    p.groundDrag = 0.8f;
    p.brakeDecel = 6.0f;
    p.dragLinear = 0.01f;
    p.dragQuad = 0.0031f;
    p.stallSpeed = 14.0f;
    p.liftoffSpeed = 18.0f;
    p.liftoffPitch = 5.0f;
    p.controlSpeed = 25.0f;
    p.pitchRate = 60.0f;
    p.rollRate = 90.0f;
    p.yawRate = 40.0f;
    p.stallDropRate = 70.0f;
    p.stallAuthority = 0.4f;
    p.stallPitchFloor = 20.0f;
    p.bankTurnRate = 0.6f;
    p.maxPitch = 60.0f;
    p.maxRoll = 75.0f;
    p.levelRate = 50.0f;
    p.wheelHeight = 0.3f;
    p.minAltitudeAboveGround = 1.0f;
    p.landingMaxSinkRate = 6.0f;
    p.landingMaxSpeed = 30.0f;
    p.landingMaxRoll = 15.0f;
    p.landingMaxNoseDown = 25.0f;
    return p;
}

void UpdatePlaneControls(PlaneState &plane, const PlaneParams &params, const FlightInput &input, float dt, float groundHeight, float groundSlopeDeg) {
    float pitchInput = input.pitch;
    float rollInput = input.roll;
    float yawInput = input.yaw;
    plane.enginePower = Clamp(plane.enginePower + input.throttle * params.throttleRate * dt, 0.0f, 1.0f);

    float authority = Authority(plane.speed, params.controlSpeed);
    bool stalled = plane.airborne && plane.speed < params.stallSpeed;
    if (stalled) authority *= params.stallAuthority;

    if (pitchInput != 0.0f) {
        plane.pitch += pitchInput * params.pitchRate * authority * dt;
    } else if (!stalled) {
        float restPitch = 0.0f;
        if (!plane.airborne) {
            float slopeLimit = params.liftoffPitch * 0.5f;
            restPitch = Clamp(groundSlopeDeg, -slopeLimit, slopeLimit);
        } else {
            float unpowered = 1.0f - Clamp(plane.enginePower / params.levelPower, 0.0f, 1.0f);
            restPitch = -atanf(1.0f / params.glideRatio) * RAD2DEG * unpowered;
        }
        plane.pitch = MoveToward(plane.pitch, restPitch, params.levelRate * dt);
    }
    if (stalled && plane.pitch > -params.stallPitchFloor) {
        plane.pitch -= params.stallDropRate * (1.0f - plane.speed / params.stallSpeed) * dt;
    }
    plane.pitch = Clamp(plane.pitch, -params.maxPitch, params.maxPitch);

    if (plane.airborne) {
        if (rollInput != 0.0f) {
            plane.roll += rollInput * params.rollRate * authority * dt;
        } else {
            plane.roll = MoveToward(plane.roll, 0.0f, params.levelRate * dt);
        }
    } else {
        plane.roll = MoveToward(plane.roll, 0.0f, params.levelRate * dt);
    }
    plane.roll = Clamp(plane.roll, -params.maxRoll, params.maxRoll);

    // Taxi steering is linear in ground speed and saturates early so it works at low speed.
    float yawAuthority = plane.airborne ? authority : Clamp(plane.speed / (params.controlSpeed * 0.4f), 0.0f, 1.0f);
    plane.yaw += yawInput * params.yawRate * yawAuthority * dt;
    plane.yaw += plane.roll * params.bankTurnRate * Authority(plane.speed, params.controlSpeed) * dt;

    plane.speed += plane.enginePower * params.accel * dt;
    plane.speed -= (params.dragLinear * plane.speed + params.dragQuad * plane.speed * plane.speed) * dt;
    if (plane.airborne) {
        plane.speed -= params.gravity * sinf(plane.pitch * DEG2RAD) * dt;
    } else {
        plane.speed -= params.groundDrag * dt;
        if (input.throttle < 0.0f && plane.enginePower <= 0.0f) plane.speed -= params.brakeDecel * dt;
    }
    plane.speed = Clamp(plane.speed, 0.0f, params.maxSpeed);

    if (!plane.airborne) {
        float yawRad = plane.yaw * DEG2RAD;
        Vector3 groundForward = {sinf(yawRad), 0.0f, cosf(yawRad)};
        plane.position = Vector3Add(plane.position, Vector3Scale(groundForward, plane.speed * dt));
        plane.position.y = groundHeight + params.wheelHeight;

        if (plane.speed >= params.liftoffSpeed && plane.pitch > params.liftoffPitch) {
            plane.airborne = true;
            plane.airTime = 0.0f;
            plane.fallSpeed = 0.0f;
            plane.landing = LandingResult::None;
        }
    } else {
        Vector3 forward = GetPlaneForward(plane);
        float lift = Authority(plane.speed, params.stallSpeed);
        plane.fallSpeed += (params.gravity * (1.0f - lift) - plane.fallSpeed * params.liftDamping * lift) * dt;
        plane.position = Vector3Add(plane.position, Vector3Scale(forward, plane.speed * dt));
        plane.position.y -= plane.fallSpeed * dt;

        plane.airTime += dt;

        float minY = groundHeight + params.minAltitudeAboveGround;
        if (plane.position.y <= minY && plane.airTime > kMinAirTimeForLanding) {
            float sinkRate = plane.fallSpeed - plane.speed * sinf(plane.pitch * DEG2RAD);
            bool gentle = sinkRate <= params.landingMaxSinkRate && plane.speed <= params.landingMaxSpeed &&
                          plane.pitch <= params.liftoffPitch && plane.pitch >= -params.landingMaxNoseDown && fabsf(plane.roll) <= params.landingMaxRoll;
            if (gentle) {
                plane.landing = LandingResult::Safe;
                plane.airborne = false;
                plane.fallSpeed = 0.0f;
                plane.position.y = groundHeight + params.wheelHeight;
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
