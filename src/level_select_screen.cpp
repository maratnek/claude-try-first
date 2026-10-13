#include "level_select_screen.h"
#include "raylib.h"
#include "safe_area.h"
#include "settings.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kCardGap = 20.0f;
constexpr float kShakeSeconds = 0.35f;
constexpr float kScrollSpeed = 12.0f;
constexpr float kBackWidth = 110.0f;
constexpr float kBackHeight = 44.0f;
constexpr float kBackMargin = 10.0f;
constexpr unsigned char kLockedDarkening = 178;

struct CardPalette {
    Color sky;
    Color ground;
};

CardPalette PaletteFor(const std::string &id) {
    if (id == "tut_takeoff") return {{140, 200, 235, 255}, {110, 170, 90, 255}};
    if (id == "tut_turns") return {{150, 205, 240, 255}, {120, 175, 80, 255}};
    if (id == "level1") return {{130, 190, 235, 255}, {90, 160, 80, 255}};
    if (id == "mountains") return {{170, 180, 195, 255}, {110, 115, 120, 255}};
    return {{120, 200, 235, 255}, {60, 140, 200, 255}};
}

float CardWidth(const Rectangle &safe) {
    return std::clamp(safe.width * 0.55f, 150.0f, 240.0f);
}

float CardHeight(const Rectangle &safe, float width) {
    return std::min(width * 1.4f, safe.height * 0.55f);
}

Rectangle CardRect(int index, float scroll) {
    Rectangle safe = GetSafeRect();
    float w = CardWidth(safe), h = CardHeight(safe, w);
    float cx = safe.x + safe.width * 0.5f;
    return {cx - w * 0.5f + (index - scroll) * (w + kCardGap), safe.y + safe.height * 0.5f - h * 0.5f, w, h};
}

Rectangle BackRect() {
    Rectangle safe = GetSafeRect();
    return {safe.x + kBackMargin, safe.y + kBackMargin, kBackWidth, kBackHeight};
}

int FitFont(const char *text, int size, int maxWidth) {
    while (size > 10 && MeasureText(text, size) > maxWidth) size--;
    return size;
}

void DrawCentered(const char *text, float cx, float y, int size, Color color) {
    DrawText(text, (int)(cx - MeasureText(text, size) * 0.5f), (int)y, size, color);
}

float DrawWrapped(const std::string &text, float x, float y, float width, int size, Color color) {
    std::string line;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t space = text.find(' ', pos);
        std::string word = text.substr(pos, space == std::string::npos ? std::string::npos : space - pos);
        std::string tryLine = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureText(tryLine.c_str(), size) > width) {
            DrawText(line.c_str(), (int)x, (int)y, size, color);
            y += size + 3;
            line = word;
        } else {
            line = tryLine;
        }
        if (space == std::string::npos) break;
        pos = space + 1;
    }
    DrawText(line.c_str(), (int)x, (int)y, size, color);
    return y + size + 3;
}

void DrawPadlock(Vector2 c, float size) {
    Color color = {235, 235, 235, 255};
    float bodyW = size, bodyH = size * 0.75f;
    float shackleR = size * 0.32f;
    DrawRing({c.x, c.y - bodyH * 0.1f}, shackleR - 4.0f, shackleR, 180.0f, 360.0f, 24, color);
    DrawRectangleRounded({c.x - bodyW * 0.5f, c.y - bodyH * 0.1f, bodyW, bodyH}, 0.2f, 6, color);
    DrawCircleV({c.x, c.y + bodyH * 0.2f}, size * 0.09f, (Color){60, 60, 60, 255});
}

void DrawCard(const LevelCard &card, int number, Rectangle r, bool selected, const ProgressData &progress) {
    bool open = IsOpenable(card, progress);
    CardPalette palette = PaletteFor(card.id);
    float artH = r.height * 0.42f;
    DrawRectangleRec(r, (Color){245, 245, 240, 255});
    DrawRectangleRec({r.x, r.y, r.width, artH}, palette.sky);
    DrawRectangleRec({r.x, r.y + artH * 0.7f, r.width, artH * 0.3f}, palette.ground);
    DrawText(TextFormat("%d", number), (int)(r.x + 10), (int)(r.y + 6), 34, (Color){255, 255, 255, 230});

    float tx = r.x + 10, tw = r.width - 20;
    float y = r.y + artH + 8;
    const char *name = card.name.c_str();
    DrawText(name, (int)tx, (int)y, FitFont(name, 26, (int)tw), BLACK);
    y += 34;
    y = DrawWrapped(card.tagline, tx, y, tw, 16, (Color){60, 60, 60, 255});
    if (open) {
        float best = CardBestTime(card, progress);
        const char *line = best > 0.0f ? TextFormat("Best %.1f s", best) : "Not finished";
        DrawText(line, (int)tx, (int)(r.y + r.height - 28), 18, (Color){40, 90, 40, 255});
    } else {
        DrawRectangleRec(r, (Color){0, 0, 0, kLockedDarkening});
        DrawPadlock({r.x + r.width * 0.5f, r.y + artH * 0.45f}, r.width * 0.2f);
        DrawText(name, (int)tx, (int)(r.y + artH + 8), FitFont(name, 26, (int)tw), (Color){235, 235, 235, 255});
    }
    DrawRectangleLinesEx(r, selected ? 4.0f : 2.0f, selected ? (Color){255, 220, 60, 255} : (Color){40, 40, 40, 200});
}
}  // namespace

void InitLevelSelect(LevelSelectState &state) {
    state.cards = BuildLevelCards();
}

void EnterLevelSelect(LevelSelectState &state, const ProgressData &progress) {
    state.selected = DefaultSelection(state.cards, progress);
    state.scroll = (float)state.selected;
    state.shake = 0.0f;
    state.touchHeld = GetTouchPointCount() > 0 || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

const LevelCard &SelectedCard(const LevelSelectState &state) {
    return state.cards[state.selected];
}

LevelSelectAction UpdateLevelSelect(LevelSelectState &state, const ProgressData &progress, float dt) {
    int count = (int)state.cards.size();
    state.shake = std::max(state.shake - dt, 0.0f);
    state.scroll += ((float)state.selected - state.scroll) * std::min(1.0f, kScrollSpeed * dt);

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) return LevelSelectAction::Back;
    if (IsKeyPressed(KEY_LEFT)) state.selected = MoveSelection(state.selected, -1, count);
    if (IsKeyPressed(KEY_RIGHT)) state.selected = MoveSelection(state.selected, 1, count);

    bool activate = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    bool pressed = false;
    Vector2 p = {0.0f, 0.0f};
    if (GetTouchPointCount() > 0) {
        pressed = true;
        p = GetTouchPosition(0);
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        pressed = true;
        p = GetMousePosition();
    }
    bool tapped = pressed && !state.touchHeld;
    state.touchHeld = pressed;
    if (tapped) {
        if (CheckCollisionPointRec(p, BackRect())) return LevelSelectAction::Back;
        for (int i = 0; i < count; i++) {
            if (!CheckCollisionPointRec(p, CardRect(i, state.scroll))) continue;
            // A first tap on a neighbouring card only brings it to the centre; the next tap opens it.
            if (i == state.selected) activate = true;
            state.selected = i;
        }
    }

    if (!activate) return LevelSelectAction::None;
    if (IsOpenable(state.cards[state.selected], progress)) return LevelSelectAction::Open;
    state.shake = kShakeSeconds;
    return LevelSelectAction::None;
}

void DrawLevelSelect(const LevelSelectState &state, const ProgressData &progress) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 150});
    Rectangle safe = GetSafeRect();
    float cx = safe.x + safe.width * 0.5f;

    Rectangle back = BackRect();
    DrawRectangleRec(back, (Color){255, 255, 255, 140});
    DrawRectangleLinesEx(back, 2.0f, (Color){40, 40, 40, 200});
    DrawCentered("Back", back.x + back.width * 0.5f, back.y + 10, 24, BLACK);

    const char *title = "SELECT LEVEL";
    int titleSize = FitFont(title, 48, (int)(safe.width * 0.9f));
    DrawCentered(title, cx, safe.y + std::max(kBackHeight + 2 * kBackMargin, safe.height * 0.08f), titleSize, WHITE);

    float shakeOffset = sinf(state.shake * 70.0f) * 10.0f * (state.shake / kShakeSeconds);
    for (int i = 0; i < (int)state.cards.size(); i++) {
        Rectangle r = CardRect(i, state.scroll);
        if (r.x + r.width < 0 || r.x > GetScreenWidth()) continue;
        bool selected = i == state.selected;
        if (selected) r.x += shakeOffset;
        DrawCard(state.cards[i], i + 1, r, selected, progress);
    }

    const LevelCard &card = SelectedCard(state);
    std::string condition = UnlockConditionText(card, state.cards, progress);
    Rectangle sel = CardRect(state.selected, state.scroll);
    if (!condition.empty()) {
        int size = FitFont(condition.c_str(), 24, (int)(safe.width * 0.9f));
        DrawCentered(condition.c_str(), cx + shakeOffset, sel.y + sel.height + 16, size, (Color){255, 220, 120, 255});
    }

#ifdef SETTINGS_MOBILE_OR_WEB
    const char *hint = "Tap a card to select, tap again to play";
#else
    const char *hint = "Left/Right + Enter, or click     Esc: Back";
#endif
    int hintSize = FitFont(hint, 22, (int)(safe.width * 0.95f));
    DrawCentered(hint, cx, safe.y + safe.height - 40, hintSize, WHITE);
}
