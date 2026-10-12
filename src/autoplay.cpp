#include "autoplay.h"
#include "raylib.h"
#include "rlgl.h"

namespace {

struct AutoplayStep {
    const char *name;
    float seconds;
    float throttle, pitch, roll, yaw;
};

// Signs follow FlightInput: pitch +1 nose up, roll +1 left, yaw +1 = A key (left).
constexpr AutoplayStep kScript[] = {
    {"menu", 0.5f, 0, 0, 0, 0},
    {"ground_roll", 3.0f, 1, 0, 0, 0},
    {"rotate", 2.0f, 1, 0.3f, 0, 0},
    {"cruise", 3.0f, 0.5f, 0, 0, 0},
    {"rudder_d", 3.0f, 0.5f, 0, 0, -1},
    {"rudder_a", 3.0f, 0.5f, 0, 0, 1},
    {"bank_left", 3.0f, 0.5f, 0, 0.35f, 0},
    {"bank_right", 3.0f, 0.5f, 0, -0.35f, 0},
};
constexpr int kStepCount = sizeof(kScript) / sizeof(kScript[0]);

void LogState(const AutoplayState &state, const char *event, const PlaneState &plane) {
    fprintf(state.log, "%-5s %-12s t=%6.2f pos=(%8.2f,%7.2f,%8.2f) yaw=%7.2f pitch=%6.2f roll=%6.2f speed=%5.1f alt=%6.2f airborne=%d\n",
            event, kScript[state.step].name, state.totalTime, plane.position.x, plane.position.y, plane.position.z,
            plane.yaw, plane.pitch, plane.roll, plane.speed, plane.position.y, plane.airborne ? 1 : 0);
    fflush(state.log);
}

}  // namespace

void StartAutoplay(AutoplayState &state, const char *dir, const char *prefix) {
    state.active = true;
    state.dir = dir;
    state.prefix = prefix;
    state.log = fopen(TextFormat("%s/%s-results.txt", dir, prefix), "w");
    if (!state.log) {
        TraceLog(LOG_ERROR, "AUTOPLAY: cannot write results log in %s", dir);
        state.active = false;
    }
}

bool AutoplayWantsPlay(const AutoplayState &state) {
    return state.active && !state.finished && state.step > 0;
}

void AutoplayInput(const AutoplayState &state, FlightInput &input) {
    if (!state.active || state.finished) return;
    const AutoplayStep &s = kScript[state.step];
    input.throttle = s.throttle;
    input.pitch = s.pitch;
    input.roll = s.roll;
    input.yaw = s.yaw;
}

const char *AutoplayAdvance(AutoplayState &state, const PlaneState &plane, bool crashed) {
    if (!state.active || state.finished) return nullptr;
    state.stepTime += kAutoplayDt;
    state.totalTime += kAutoplayDt;
    if (!crashed && state.stepTime < kScript[state.step].seconds) return nullptr;

    LogState(state, crashed ? "CRASH" : "end", plane);
    const char *path = TextFormat("%s/%s-%02d-%s.png", state.dir, state.prefix, state.step, kScript[state.step].name);
    state.stepTime = 0.0f;
    if (crashed || ++state.step == kStepCount) {
        state.finished = true;
        if (state.log) fclose(state.log);
        state.log = nullptr;
    }
    return path;
}

void SaveScreenshot(const char *path) {
    // Text and 2D draws are still batched until EndDrawing; flush so the HUD is in the capture.
    rlDrawRenderBatchActive();
    Image shot = LoadImageFromScreen();
    ExportImage(shot, path);
    UnloadImage(shot);
}
