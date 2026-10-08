#include "input.h"
#include "raymath.h"
#include "safe_area.h"
#include "touch_layout.h"

namespace {

constexpr float kStickRadius = 80.0f;
constexpr float kStickDeadzone = 0.15f;
constexpr float kButtonSize = 100.0f;
constexpr float kButtonMargin = 20.0f;

Rectangle ThrottleUpRect() {
    SafeArea sa = GetSafeArea();
    return {GetScreenWidth() - sa.right - kButtonSize - kButtonMargin,
            GetScreenHeight() - sa.bottom - 2.0f * kButtonSize - 2.0f * kButtonMargin, kButtonSize, kButtonSize};
}

Rectangle ThrottleDownRect() {
    SafeArea sa = GetSafeArea();
    return {GetScreenWidth() - sa.right - kButtonSize - kButtonMargin,
            GetScreenHeight() - sa.bottom - kButtonSize - kButtonMargin, kButtonSize, kButtonSize};
}

Rectangle RestartRect() {
    const float w = 220.0f, h = 60.0f, gap = 20.0f;
    return {GetScreenWidth() * 0.5f - gap * 0.5f - w, GetScreenHeight() - GetSafeArea().bottom - h - 20.0f, w, h};
}

Rectangle MenuRect() {
    Rectangle r = RestartRect();
    r.x += r.width + 20.0f;
    return r;
}

SettingsButtons SettingsRects() {
    SafeArea sa = GetSafeArea();
    return LayoutSettingsButtons((float)GetScreenWidth(), sa.right, sa.top);
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

FlightInput ReadFlightInput(InputState &state, bool endButtonsActive, bool settingsActive) {
    FlightInput in;
    in.pitch = KeyAxis(KEY_DOWN, KEY_UP);
    in.roll = KeyAxis(KEY_LEFT, KEY_RIGHT);
    in.yaw = KeyAxis(KEY_A, KEY_D);
    in.throttle = KeyAxis(KEY_W, KEY_S);
    in.restart = IsKeyPressed(KEY_R);
    in.exit = IsKeyPressed(KEY_Q);
    in.menu = IsKeyPressed(KEY_M);

    int count = GetTouchPointCount();
    if (count > 0) state.touchUsed = true;

    bool stickFound = false;
    bool restartTouched = false;
    bool menuTouched = false;
    bool gfxTouched = false;
    bool volDownTouched = false;
    bool volUpTouched = false;
    float throttle = 0.0f;
    float halfWidth = GetScreenWidth() * 0.5f;
    Rectangle up = ThrottleUpRect();
    Rectangle down = ThrottleDownRect();
    Rectangle restartRect = RestartRect();
    Rectangle menuRect = MenuRect();
    SettingsButtons settings = SettingsRects();

    for (int i = 0; i < count; i++) {
        Vector2 p = GetTouchPosition(i);
        if (CheckCollisionPointRec(p, up)) throttle += 1.0f;
        if (CheckCollisionPointRec(p, down)) throttle -= 1.0f;
        if (endButtonsActive && CheckCollisionPointRec(p, restartRect)) restartTouched = true;
        if (endButtonsActive && CheckCollisionPointRec(p, menuRect)) menuTouched = true;
        bool onSettings = false;
        if (settingsActive && state.touchUsed) {
            bool g = TouchBoxContains(settings.gfx, p.x, p.y);
            bool d = TouchBoxContains(settings.volDown, p.x, p.y);
            bool u = TouchBoxContains(settings.volUp, p.x, p.y);
            gfxTouched |= g;
            volDownTouched |= d;
            volUpTouched |= u;
            onSettings = g || d || u;
        }
        if (!stickFound && !onSettings && p.x < halfWidth) {
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
    if (menuTouched && !state.menuHeld) in.menu = true;
    state.menuHeld = menuTouched;

    if (gfxTouched && !state.gfxHeld) in.cycleGraphics = true;
    if (volDownTouched && !state.volDownHeld) in.volumeStep -= 1;
    if (volUpTouched && !state.volUpHeld) in.volumeStep += 1;
    state.gfxHeld = gfxTouched;
    state.volDownHeld = volDownTouched;
    state.volUpHeld = volUpTouched;

    return in;
}

void DrawTouchOverlay(const InputState &state, bool endButtonsShown, bool settingsShown, const char *gfxLabel) {
    if (!state.touchUsed) return;

    Color fill = {255, 255, 255, 60};
    Color line = {40, 40, 40, 140};

    if (state.stickActive) {
        DrawCircleV(state.stickOrigin, kStickRadius, fill);
        DrawCircleLinesV(state.stickOrigin, kStickRadius, line);
        Vector2 offset = Vector2ClampValue(Vector2Subtract(state.stickPos, state.stickOrigin), 0.0f, kStickRadius);
        DrawCircleV(Vector2Add(state.stickOrigin, offset), 32.0f, (Color){255, 255, 255, 140});
    } else {
        SafeArea sa = GetSafeArea();
        Vector2 hint = {sa.left + (GetScreenWidth() - sa.left - sa.right) * 0.18f,
                        GetScreenHeight() * 0.75f - sa.bottom * 0.5f};
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

    if (endButtonsShown) {
        Rectangle r = RestartRect();
        DrawRectangleRec(r, (Color){255, 255, 255, 140});
        DrawRectangleLinesEx(r, 2.0f, line);
        DrawText("RESTART", (int)(r.x + (r.width - MeasureText("RESTART", 30)) * 0.5f), (int)(r.y + 15), 30, BLACK);
        Rectangle m = MenuRect();
        DrawRectangleRec(m, (Color){255, 255, 255, 140});
        DrawRectangleLinesEx(m, 2.0f, line);
        DrawText("MENU", (int)(m.x + (m.width - MeasureText("MENU", 30)) * 0.5f), (int)(m.y + 15), 30, BLACK);
    }

    if (settingsShown) {
        SettingsButtons b = SettingsRects();
        Color btnFill = {255, 255, 255, 140};
        const TouchBox boxes[] = {b.gfx, b.volDown, b.volUp};
        const char *labels[] = {gfxLabel, "Vol -", "Vol +"};
        for (int i = 0; i < 3; i++) {
            Rectangle r = {boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h};
            DrawRectangleRec(r, btnFill);
            DrawRectangleLinesEx(r, 2.0f, line);
            DrawText(labels[i], (int)(r.x + (r.width - MeasureText(labels[i], 18)) * 0.5f), (int)(r.y + 7), 18, BLACK);
        }
    }
}
