#include "autoplay.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <atomic>
#include <cstdarg>

namespace {

// Minimums are about half the heading change measured on the scripted path (see "dyaw" in the results log),
// so tuning noise passes but a dead control does not. Set for yawRate 8 deg/s; a higher rate only raises the measured value.
constexpr float kStartPositionTolerance = 0.05f;
constexpr float kStartMaxSpeed = 0.5f;
constexpr float kMinRudderHeadingChange = 12.0f;
constexpr float kMinBankHeadingChange = 22.0f;

std::atomic<int> g_logErrors{0};

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
    {"wings_level", 2.0f, 0.5f, 0, 0, 0},
    {"bank_right", 3.0f, 0.5f, 0, -0.35f, 0},
};
static_assert(sizeof(kScript) / sizeof(kScript[0]) == kStepCount);

void CountingTraceLog(int level, const char *text, va_list args) {
    static const char *const names[] = {"ALL", "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL", "NONE"};
    char message[1024];
    vsnprintf(message, sizeof(message), text, args);
    FILE *out = level >= LOG_ERROR ? stderr : stdout;
    fprintf(out, "%s: %s\n", names[level], message);
    if (level >= LOG_ERROR) g_logErrors++;
}

AutoplaySnapshot Snapshot(const PlaneState &plane, bool crashed) {
    return {plane.position, plane.yaw, plane.speed, plane.airborne, crashed};
}

void Fail(std::vector<std::string> &out, const char *format, ...) {
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    out.push_back(message);
}

// Yaw increases toward the A key and the Left arrow (FlightInput yaw/roll +1), so left is positive and right negative.
void CheckHeadingChange(std::vector<std::string> &out, const AutoplaySnapshot *ends, int endCount, int step,
                        const char *label, float direction, float minimum) {
    if (step >= endCount || ends[step].crashed) return;
    float change = (ends[step].yaw - ends[step - 1].yaw) * direction;
    if (change < minimum) Fail(out, "%s turned %.2f deg, need at least %.2f deg toward the expected side", label, change, minimum);
}

void LogState(const AutoplayState &state, const char *event, const PlaneState &plane) {
    float dyaw = plane.yaw - (state.step > 0 ? state.ends[state.step - 1].yaw : plane.yaw);
    fprintf(state.log, "%-5s %-12s t=%6.2f pos=(%8.2f,%7.2f,%8.2f) yaw=%7.2f dyaw=%7.2f pitch=%6.2f roll=%6.2f speed=%5.1f alt=%6.2f airborne=%d\n",
            event, kScript[state.step].name, state.totalTime, plane.position.x, plane.position.y, plane.position.z,
            plane.yaw, dyaw, plane.pitch, plane.roll, plane.speed, plane.position.y, plane.airborne ? 1 : 0);
    fflush(state.log);
}

}  // namespace

std::vector<std::string> CheckAutoplayResults(const AutoplaySnapshot *ends, int endCount, Vector3 expectedStart, int logErrors) {
    std::vector<std::string> out;
    if (endCount > 0) {
        const AutoplaySnapshot &s = ends[kStepMenu];
        float off = Vector3Distance(s.position, expectedStart);
        if (off > kStartPositionTolerance || s.airborne || s.speed > kStartMaxSpeed)
            Fail(out, "bad start: %.2f m from level start, airborne=%d, speed=%.2f", off, s.airborne ? 1 : 0, s.speed);
    }
    for (int i = 0; i < endCount; i++) {
        if (ends[i].crashed) Fail(out, "crashed in step %s", kScript[i].name);
    }
    if (endCount > kStepRotate && !ends[kStepRotate].crashed && !ends[kStepRotate].airborne)
        Fail(out, "no liftoff by the end of rotate");
    if (endCount < kStepCount && (endCount == 0 || !ends[endCount - 1].crashed))
        Fail(out, "script ended early after %d of %d steps", endCount, kStepCount);
    CheckHeadingChange(out, ends, endCount, kStepRudderD, "rudder D", -1.0f, kMinRudderHeadingChange);
    CheckHeadingChange(out, ends, endCount, kStepRudderA, "rudder A", 1.0f, kMinRudderHeadingChange);
    CheckHeadingChange(out, ends, endCount, kStepBankLeft, "bank left", 1.0f, kMinBankHeadingChange);
    CheckHeadingChange(out, ends, endCount, kStepBankRight, "bank right", -1.0f, kMinBankHeadingChange);
    if (logErrors > 0) Fail(out, "game logged %d error(s)", logErrors);
    return out;
}

int AutoplayExitCode(const AutoplayState &state) {
    if (!state.active) return 0;
    return (state.finished && state.failures == 0) ? 0 : 1;
}

bool StartAutoplay(AutoplayState &state, const char *dir, const char *prefix) {
    state.active = true;
    SetTraceLogCallback(CountingTraceLog);
    state.dir = dir;
    state.prefix = prefix;
    state.log = fopen(TextFormat("%s/%s-results.txt", dir, prefix), "w");
    if (!state.log) {
        TraceLog(LOG_ERROR, "AUTOPLAY: cannot write results log in %s", dir);
        state.active = false;
        return false;
    }
    return true;
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
    state.ends[state.step] = Snapshot(plane, crashed);
    state.endCount = state.step + 1;
    const char *path = TextFormat("%s/%s-%02d-%s.png", state.dir, state.prefix, state.step, kScript[state.step].name);
    state.stepTime = 0.0f;
    if (crashed || ++state.step == kStepCount) {
        state.finished = true;
        std::vector<std::string> failures = CheckAutoplayResults(state.ends, state.endCount, state.expectedStart, g_logErrors.load());
        state.failures = (int)failures.size();
        for (const std::string &f : failures) {
            fprintf(state.log, "ASSERT FAIL: %s\n", f.c_str());
            TraceLog(LOG_ERROR, "ASSERT FAIL: %s", f.c_str());
        }
        fprintf(state.log, "RESULT: %s\n", failures.empty() ? "PASS" : "FAIL");
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
