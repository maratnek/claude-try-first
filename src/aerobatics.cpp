#include "aerobatics.h"
#include <cmath>

namespace {

constexpr float kFullTurnDeg = 340.0f;
constexpr float kHalfTurnDeg = 150.0f;
constexpr float kHalfTurnMaxDeg = 210.0f;
constexpr float kMaxHeadingDriftDeg = 45.0f;
constexpr float kMaxOffAxisRollDeg = 90.0f;
constexpr float kMaxOffAxisPitchDeg = 90.0f;
constexpr float kLevelToleranceDeg = 10.0f;
constexpr float kInProgressDeg = 45.0f;
constexpr float kMaxTrickSeconds = 20.0f;

float WrapDelta(float deg) {
    deg = std::fmod(deg, 360.0f);
    if (deg > 180.0f) deg -= 360.0f;
    if (deg < -180.0f) deg += 360.0f;
    return deg;
}

void ResetProgress(AerobaticsTracker &t) {
    t.hasPrev = false;
    t.pitchTravel = 0.0f;
    t.rollTravel = 0.0f;
    t.headingTravel = 0.0f;
    t.age = 0.0f;
    t.halfLoopDone = false;
    t.rollAtHalfLoop = 0.0f;
}

void Complete(AerobaticsTracker &t, Trick trick) {
    const AttitudeSample keep = t.prev;
    ResetProgress(t);
    t.hasPrev = true;
    t.prev = keep;
    t.completed = trick;
}

}  // namespace

AttitudeSample AttitudeFromPlane(const PlaneState &plane) {
    return {plane.pitch, plane.roll, plane.yaw};
}

void UpdateAerobaticsAttitude(AerobaticsTracker &t, const AttitudeSample &s, bool airborne, float dt) {
    if (!airborne) {
        ResetProgress(t);
        return;
    }
    if (!t.hasPrev) {
        t.hasPrev = true;
        t.prev = s;
        return;
    }

    t.pitchTravel += WrapDelta(s.pitchDeg - t.prev.pitchDeg);
    t.rollTravel += WrapDelta(s.rollDeg - t.prev.rollDeg);
    t.headingTravel += WrapDelta(s.headingDeg - t.prev.headingDeg);
    t.prev = s;
    t.age += dt;

    const bool level = std::fabs(WrapDelta(s.pitchDeg)) < kLevelToleranceDeg &&
                       std::fabs(WrapDelta(s.rollDeg)) < kLevelToleranceDeg;
    const bool inProgress = std::fabs(t.pitchTravel) > kInProgressDeg || std::fabs(t.rollTravel) > kInProgressDeg;
    if (t.age > kMaxTrickSeconds || (level && !inProgress)) {
        const AttitudeSample keep = s;
        ResetProgress(t);
        t.hasPrev = true;
        t.prev = keep;
        return;
    }

    const float pitch = std::fabs(t.pitchTravel);
    const float roll = std::fabs(t.rollTravel);
    const bool headingKept = std::fabs(t.headingTravel) < kMaxHeadingDriftDeg;

    if (pitch >= kFullTurnDeg && roll < kMaxOffAxisRollDeg && headingKept) {
        Complete(t, Trick::Loop);
        return;
    }
    // Why: any full roll counts; the path shape (a true barrel roll arcs around the flight line) is not checked.
    if (roll >= kFullTurnDeg && pitch < kMaxOffAxisPitchDeg && headingKept) {
        Complete(t, Trick::BarrelRoll);
        return;
    }

    if (pitch < kHalfTurnDeg) {
        t.halfLoopDone = false;
    } else if (!t.halfLoopDone && roll < kMaxOffAxisRollDeg) {
        t.halfLoopDone = true;
        t.rollAtHalfLoop = t.rollTravel;
    }
    if (t.halfLoopDone && headingKept && pitch <= kHalfTurnMaxDeg &&
        std::fabs(t.rollTravel - t.rollAtHalfLoop) >= kHalfTurnDeg) {
        Complete(t, Trick::Immelmann);
    }
}

void UpdateAerobatics(AerobaticsTracker &tracker, const PlaneState &plane, float dt) {
    UpdateAerobaticsAttitude(tracker, AttitudeFromPlane(plane), plane.airborne, dt);
}

Trick TakeCompletedTrick(AerobaticsTracker &tracker) {
    const Trick trick = tracker.completed;
    tracker.completed = Trick::None;
    return trick;
}
