#pragma once
#include <vector>

enum class MenuAction { None, Play, Exit };

struct MenuEntry {
    const char *label;
    MenuAction action;
};

struct MenuState {
    std::vector<MenuEntry> entries;
    int selected = 0;
    bool touchHeld = false;
};

void InitMenu(MenuState &menu);

// Call on every switch to the menu so a press still held from the previous screen is not counted as a tap.
void EnterMenu(MenuState &menu);

// Returns the activated entry's action (keyboard, mouse click or touch tap), or None.
MenuAction UpdateMenu(MenuState &menu);

void DrawMenu(const MenuState &menu);
