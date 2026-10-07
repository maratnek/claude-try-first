#include "menu.h"
#include "raylib.h"
#include "safe_area.h"
#include "settings.h"

namespace {
constexpr float kEntryWidth = 320.0f;
constexpr float kEntryHeight = 70.0f;
constexpr float kEntrySpacing = 20.0f;
constexpr int kEntrySlots = 2;
constexpr float kHintReserve = 90.0f;
constexpr int kTitleSize = 80;

Rectangle EntryRect(int index) {
    Rectangle safe = GetSafeRect();
    return {safe.x + (safe.width - kEntryWidth) * 0.5f,
            safe.y + safe.height - kHintReserve - kEntrySlots * (kEntryHeight + kEntrySpacing) + kEntrySpacing + index * (kEntryHeight + kEntrySpacing), kEntryWidth, kEntryHeight};
}
}  // namespace

void InitMenu(MenuState &menu) {
    menu.entries.clear();
    menu.entries.push_back({"Play", MenuAction::Play});
#ifndef SETTINGS_MOBILE_OR_WEB
    menu.entries.push_back({"Exit", MenuAction::Exit});
#endif
    menu.selected = 0;
}

void EnterMenu(MenuState &menu) {
    menu.selected = 0;
    menu.touchHeld = GetTouchPointCount() > 0 || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

MenuAction UpdateMenu(MenuState &menu) {
    int count = (int)menu.entries.size();
    if (IsKeyPressed(KEY_DOWN)) menu.selected = (menu.selected + 1) % count;
    if (IsKeyPressed(KEY_UP)) menu.selected = (menu.selected + count - 1) % count;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) return menu.entries[menu.selected].action;
#ifndef SETTINGS_MOBILE_OR_WEB
    if (IsKeyPressed(KEY_Q)) return MenuAction::Exit;
#endif

    bool pressed = false;
    Vector2 p = {0.0f, 0.0f};
    if (GetTouchPointCount() > 0) {
        pressed = true;
        p = GetTouchPosition(0);
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        pressed = true;
        p = GetMousePosition();
    }
    bool tapped = pressed && !menu.touchHeld;
    menu.touchHeld = pressed;
    if (tapped) {
        for (int i = 0; i < count; i++) {
            if (CheckCollisionPointRec(p, EntryRect(i))) {
                menu.selected = i;
                return menu.entries[i].action;
            }
        }
    }
    return MenuAction::None;
}

void DrawMenu(const MenuState &menu) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 120});
    Rectangle safe = GetSafeRect();
    int cx = (int)(safe.x + safe.width * 0.5f);
    const char *title = "FLIGHT GAME";
    int titleWidth = MeasureText(title, kTitleSize);
    int titleSize = kTitleSize;
    if (titleWidth > safe.width * 0.9f) titleSize = (int)(kTitleSize * safe.width * 0.9f / titleWidth);
    DrawText(title, cx - MeasureText(title, titleSize) / 2, (int)(safe.y + safe.height * 0.08f), titleSize, WHITE);
    for (int i = 0; i < (int)menu.entries.size(); i++) {
        Rectangle r = EntryRect(i);
        bool selected = i == menu.selected;
        DrawRectangleRec(r, selected ? (Color){255, 255, 255, 220} : (Color){255, 255, 255, 110});
        DrawRectangleLinesEx(r, 2.0f, (Color){40, 40, 40, 200});
        const char *label = menu.entries[i].label;
        DrawText(label, (int)(r.x + (r.width - MeasureText(label, 36)) * 0.5f), (int)(r.y + 17), 36, BLACK);
    }
#ifdef SETTINGS_MOBILE_OR_WEB
    const char *hint = "Tap Play";
#else
    const char *hint = "Up/Down + Enter/Space, or click     Q: Exit";
#endif
    DrawText(hint, cx - MeasureText(hint, 24) / 2, (int)(safe.y + safe.height) - 60, 24, WHITE);
}
