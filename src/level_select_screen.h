#pragma once
#include "level_select.h"

enum class LevelSelectAction { None, Open, Back };

struct LevelSelectState {
    std::vector<LevelCard> cards;
    int selected = 0;
    float shake = 0.0f;
    float scroll = 0.0f;
    bool touchHeld = false;
};

void InitLevelSelect(LevelSelectState &state);

// Call on every switch to the screen so a press still held from the previous screen is not counted as a tap.
void EnterLevelSelect(LevelSelectState &state, const ProgressData &progress);

// Open is returned only for an openable card; selecting a locked one just starts a shake.
LevelSelectAction UpdateLevelSelect(LevelSelectState &state, const ProgressData &progress, float dt);

const LevelCard &SelectedCard(const LevelSelectState &state);

void DrawLevelSelect(const LevelSelectState &state, const ProgressData &progress);
