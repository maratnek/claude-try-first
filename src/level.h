#pragma once
#include "level_def.h"
#include "raylib.h"
#include <vector>

struct Checkpoint {
    Vector3 position;
    float radius;
    bool passed = false;
};

struct LevelState {
    Vector3 startPosition = {0.0f, 0.0f, 0.0f};
    float targetDistance = Level1Def().gateDistance;
    float distanceFlown = 0.0f;
    bool gateCrossed = false;
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

// Updates elapsed time, distanceFlown, gateCrossed and checkpoint-passed flags from the
// plane's position. The clock freezes at the gate. Does nothing once crashed.
void UpdateLevel(LevelState &level, Vector3 planePosition, float dt);

// Number of checkpoint rings flown through so far.
int CountPassed(const LevelState &level);

// True when the run is over: the gate is crossed and, if landing is required, the plane has rolled out (on the ground, speed <= 8 m/s).
bool IsRunFinished(const LevelState &level, bool airborne, float speed);

// Stars for a finished run: 3 = clean + all checkpoints, 2 = clean or all checkpoints, 1 = finished.
int ComputeStars(const LevelState &level, bool damaged);

enum class Medal { None, Bronze, Silver, Gold };

// Time medal for a finished run from the gate time: gold <= 24 s, silver <= 30 s, bronze <= 40 s.
Medal ComputeMedal(float elapsed);

// Stores the elapsed time as the best when it beats the previous one; main.cpp seeds bestTime from saved progress and saves after a finish.
void RecordFinish(LevelState &level);

void DrawFinishGate(const LevelState &level);
void DrawCheckpoints(const LevelState &level);
void DrawLevelHUD(const LevelState &level, bool damaged);

// Dimmed overlay with the Restart/Exit prompts and the gate time when the gate was crossed; Exit is omitted on Web/iOS. The touch Restart button is drawn by DrawTouchOverlay.
void DrawCrashScreen(const LevelState &level, bool touchUsed);

// Dimmed overlay with stars, medal, time, delta vs best, a Finished/All rings/Clean checklist and the next medal target; layout scales with screen height. The touch Restart/Menu buttons are drawn by DrawTouchOverlay.
void DrawResultsScreen(const LevelState &level, bool damaged, bool touchUsed);

// Clears progress (distance, gateCrossed, crashed, elapsed, passed checkpoints) so the
// level can be retried. Keeps the loaded model and the session best time.
void ResetLevelProgress(LevelState &level);

void UnloadLevel(LevelState &level);
