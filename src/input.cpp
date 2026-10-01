#include "input.h"
#include "raymath.h"

namespace {

constexpr float kStickRadius = 80.0f;
constexpr float kStickDeadzone = 0.15f;
constexpr float kButtonSize = 100.0f;
constexpr float kButtonMargin = 20.0f;

Rectangle ThrottleUpRect() {
    return {GetScreenWidth() - kButtonSize - kButtonMargin,
            GetScreenHeight() - 2.0f * kButtonSize - 2.0f * kButtonMargin, kButtonSize, kButtonSize};
}

Rectangle ThrottleDownRect() {
    return {GetScreenWidth() - kButtonSize - kButtonMargin,
            GetScreenHeight() - kButtonSize - kButtonMargin, kButtonSize, kButtonSize};
}

Rectangle RestartRect() {
    const float w = 260.0f, h = 80.0f;
    return {(GetScreenWidth() - w) * 0.5f, (GetScreenHeight() - h) * 0.5f, w, h};
}

float StickAxis(float delta) {
    float v = Clamp(delta / kStickRadius, -1.0f, 1.0f);
    float mag = fabsf(v);
    if (mag < kStickDeadzone) return 0.0f;
    return (v < 0.0f ? -1.0f : 1.0f) * (mag - kStickDeadzone) / (1.0f - kStickDeadzone);
}

float KeyAxis(int positive, int negative) {
    float v = 0.0f;
    if (IsKeyDown(positive)) v += 1.0f;
    if (IsKeyDown(negative)) v -= 1.0f;
    return v;
}

}  // namespace

FlightInput ReadFlightInput(InputState &state) {
    FlightInput in;
    in.pitch = KeyAxis(KEY_DOWN, KEY_UP);
    in.roll = KeyAxis(KEY_LEFT, KEY_RIGHT);
    in.yaw = KeyAxis(KEY_A, KEY_D);
    in.throttle = KeyAxis(KEY_W, KEY_S);
    in.restart = IsKeyPressed(KEY_R);

    int count = GetTouchPointCount();
    if (count > 0) state.touchUsed = true;

    bool stickFound = false;
    bool restartTouched = false;
    float throttle = 0.0f;
    float halfWidth = GetScreenWidth() * 0.5f;
    Rectangle up = ThrottleUpRect();
    Rectangle down = ThrottleDownRect();
    Rectangle restartRect = RestartRect();

    for (int i = 0; i < count; i++) {
        Vector2 p = GetTouchPosition(i);
        if (CheckCollisionPointRec(p, up)) throttle += 1.0f;
        if (CheckCollisionPointRec(p, down)) throttle -= 1.0f;
        if (CheckCollisionPointRec(p, restartRect)) restartTouched = true;
        if (!stickFound && p.x < halfWidth) {
            stickFound = true;
            if (!state.stickActive) state.stickOrigin = p;
            state.stickPos = p;
        }
    }
    state.stickActive = stickFound;

    if (stickFound) {
        // Screen y grows downward, so dragging down gives positive pitch (nose up) like the Down key.
        // Dragging left gives positive roll like the Left key.
        float touchPitch = StickAxis(state.stickPos.y - state.stickOrigin.y);
        float touchRoll = StickAxis(state.stickOrigin.x - state.stickPos.x);
        in.pitch = Clamp(in.pitch + touchPitch, -1.0f, 1.0f);
        in.roll = Clamp(in.roll + touchRoll, -1.0f, 1.0f);
    }
    in.throttle = Clamp(in.throttle + throttle, -1.0f, 1.0f);

    if (restartTouched && !state.restartHeld) in.restart = true;
    state.restartHeld = restartTouched;

    return in;
}

void DrawTouchOverlay(const InputState &state, bool crashed) {
    if (!state.touchUsed) return;

    Color fill = {255, 255, 255, 60};
    Color line = {40, 40, 40, 140};

    if (state.stickActive) {
        DrawCircleV(state.stickOrigin, kStickRadius, fill);
        DrawCircleLinesV(state.stickOrigin, kStickRadius, line);
        Vector2 offset = Vector2ClampValue(Vector2Subtract(state.stickPos, state.stickOrigin), 0.0f, kStickRadius);
        DrawCircleV(Vector2Add(state.stickOrigin, offset), 32.0f, (Color){255, 255, 255, 140});
    } else {
        Vector2 hint = {GetScreenWidth() * 0.18f, GetScreenHeight() * 0.75f};
        DrawCircleV(hint, kStickRadius, fill);
        DrawCircleLinesV(hint, kStickRadius, line);
    }

    Rectangle up = ThrottleUpRect();
    Rectangle down = ThrottleDownRect();
    DrawRectangleRec(up, fill);
    DrawRectangleLinesEx(up, 2.0f, line);
    DrawText("+", (int)(up.x + up.width * 0.5f - 8), (int)(up.y + up.height * 0.5f - 20), 40, line);
    DrawRectangleRec(down, fill);
    DrawRectangleLinesEx(down, 2.0f, line);
    DrawText("-", (int)(down.x + down.width * 0.5f - 8), (int)(down.y + down.height * 0.5f - 20), 40, line);

    if (crashed) {
        Rectangle r = RestartRect();
        DrawRectangleRec(r, (Color){255, 255, 255, 140});
        DrawRectangleLinesEx(r, 2.0f, line);
        DrawText("RESTART", (int)(r.x + 50), (int)(r.y + 25), 30, BLACK);
    }
}
