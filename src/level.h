#pragma once
#include "raylib.h"
#include <vector>

struct Checkpoint {
    Vector3 position;
    float radius;
    bool passed = false;
};

struct LevelState {
    Vector3 startPosition = {0.0f, 0.0f, 0.0f};
    float targetDistance = 1000.0f;
    float distanceFlown = 0.0f;
    bool completed = false;
    bool crashed = false;
    std::vector<Checkpoint> checkpoints;
    Model checkpointModel{};
};

// Sets up checkpoints and loads the reusable checkpoint ring model. Call
// once at startup, after startPosition is known.
void InitLevel(LevelState &level);

// Updates distanceFlown and checkpoint-passed flags from the plane's
// position. Does nothing once crashed.
void UpdateLevel(LevelState &level, Vector3 planePosition);

void DrawFinishGate(const LevelState &level);
void DrawCheckpoints(const LevelState &level);
void DrawLevelHUD(const LevelState &level, bool damaged);

// Dimmed overlay with the Restart/Exit prompts; Exit is omitted on Web/iOS. The touch Restart button is drawn by DrawTouchOverlay.
void DrawCrashScreen();

// Clears progress (distance, completed, crashed, passed checkpoints) so the
// level can be retried after a crash. Keeps the loaded model.
void ResetLevelProgress(LevelState &level);

void UnloadLevel(LevelState &level);
