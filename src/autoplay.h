#pragma once
#include "flight.h"
#include "input.h"
#include <cstdio>

struct AutoplayState {
    bool active = false;
    bool finished = false;
    int step = 0;
    float stepTime = 0.0f;
    float totalTime = 0.0f;
    const char *dir = ".";
    const char *prefix = "autoplay";
    FILE *log = nullptr;
};

constexpr float kAutoplayDt = 1.0f / 60.0f;

void StartAutoplay(AutoplayState &state, const char *dir, const char *prefix);
bool AutoplayWantsPlay(const AutoplayState &state);
void AutoplayInput(const AutoplayState &state, FlightInput &input);
const char *AutoplayAdvance(AutoplayState &state, const PlaneState &plane, bool crashed);
void SaveScreenshot(const char *path);
