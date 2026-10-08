#pragma once
#include "flight.h"
#include "input.h"
#include <cstdio>
#include <string>
#include <vector>

enum AutoplayStepId {
    kStepMenu,
    kStepGroundRoll,
    kStepRotate,
    kStepCruise,
    kStepRudderD,
    kStepRudderA,
    kStepBankLeft,
    kStepWingsLevel,
    kStepBankRight,
    kStepCount,
};

struct AutoplaySnapshot {
    Vector3 position = {0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float speed = 0.0f;
    bool airborne = false;
    bool crashed = false;
};

struct AutoplayState {
    bool active = false;
    bool finished = false;
    int step = 0;
    float stepTime = 0.0f;
    float totalTime = 0.0f;
    const char *dir = ".";
    const char *prefix = "autoplay";
    FILE *log = nullptr;
    AutoplaySnapshot ends[kStepCount];
    int endCount = 0;
    Vector3 expectedStart = {0.0f, 0.0f, 0.0f};
    int failures = 0;
};

constexpr float kAutoplayDt = 1.0f / 60.0f;

bool StartAutoplay(AutoplayState &state, const char *dir, const char *prefix);
std::vector<std::string> CheckAutoplayResults(const AutoplaySnapshot *ends, int endCount, Vector3 expectedStart, int logErrors);
int AutoplayExitCode(const AutoplayState &state);
bool AutoplayWantsPlay(const AutoplayState &state);
void AutoplayInput(const AutoplayState &state, FlightInput &input);
const char *AutoplayAdvance(AutoplayState &state, const PlaneState &plane, bool crashed);
void SaveScreenshot(const char *path);
