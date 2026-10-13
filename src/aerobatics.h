#pragma once
#include "flight.h"

enum class Trick { None, Loop, BarrelRoll, Immelmann };

// Unbounded, continuous Euler attitude in degrees: a loop keeps adding to pitchDeg (past 90, 180, ...)
// and a roll keeps adding to rollDeg. Sign conventions match PlaneState: pitch + = nose up, roll + = right.
// A half loop therefore leaves pitchDeg at 180 with the heading unchanged; the reversal is carried by the pitch travel.
struct AttitudeSample {
    float pitchDeg = 0.0f;
    float rollDeg = 0.0f;
    float headingDeg = 0.0f;
};

struct AerobaticsTracker {
    bool hasPrev = false;
    AttitudeSample prev;
    float pitchTravel = 0.0f;
    float rollTravel = 0.0f;
    float headingTravel = 0.0f;
    float age = 0.0f;
    bool halfLoopDone = false;
    float rollAtHalfLoop = 0.0f;
    Trick completed = Trick::None;
};

// PlaneState pitch is clamped to +-maxPitch (60) and roll to +-maxRoll (75) by the flight model, so
// the travel needed for any trick is unreachable from a real PlaneState today.
AttitudeSample AttitudeFromPlane(const PlaneState &plane);

void UpdateAerobaticsAttitude(AerobaticsTracker &tracker, const AttitudeSample &sample, bool airborne, float dt);

void UpdateAerobatics(AerobaticsTracker &tracker, const PlaneState &plane, float dt);

Trick TakeCompletedTrick(AerobaticsTracker &tracker);
