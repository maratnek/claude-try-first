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
    float elapsed = 0.0f;
    float bestTime = -1.0f;
    bool newBest = false;
    std::vector<Checkpoint> checkpoints;
    Model checkpointModel{};
};

// Sets up checkpoints and loads the reusable checkpoint ring model. Call
// once at startup, after startPosition is known.
void InitLevel(LevelState &level);

// Updates elapsed time, distanceFlown and checkpoint-passed flags from the
// plane's position. Does nothing once crashed.
void UpdateLevel(LevelState &level, Vector3 planePosition, float dt);

// Stars for a finished run: 3 = clean + all checkpoints, 2 = clean or all checkpoints, 1 = finished.
int ComputeStars(const LevelState &level, bool damaged);

// Stores the elapsed time as the session best when it beats the previous one (not persisted).
void RecordFinish(LevelState &level);

void DrawFinishGate(const LevelState &level);
void DrawCheckpoints(const LevelState &level);
void DrawLevelHUD(const LevelState &level, bool damaged);

// Dimmed overlay with the Restart/Exit prompts; Exit is omitted on Web/iOS. The touch Restart button is drawn by DrawTouchOverlay.
void DrawCrashScreen();

// Dimmed overlay with time, checkpoints, damage, stars and best time. The touch Restart/Menu buttons are drawn by DrawTouchOverlay.
void DrawResultsScreen(const LevelState &level, bool damaged);

// Clears progress (distance, completed, crashed, elapsed, passed checkpoints) so the
// level can be retried. Keeps the loaded model and the session best time.
void ResetLevelProgress(LevelState &level);

void UnloadLevel(LevelState &level);
