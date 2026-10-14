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
    const LevelDef *def = &Level1Def();
    int stageIndex = 0;
    Vector3 startPosition = {0.0f, 0.0f, 0.0f};
    float targetDistance = Level1Def().gateDistance;
    float distanceFlown = 0.0f;
    bool gateCrossed = false;
    bool crashed = false;
    bool hasFlown = false;
    float elapsed = 0.0f;
    float bestTime = -1.0f;
    bool newBest = false;
    std::vector<Checkpoint> checkpoints;
    Vector3 lastPlanePosition = {0.0f, 0.0f, 0.0f};
    bool hasLastPlanePosition = false;
    Model checkpointModel{};
};

// Rebuilds targetDistance and the checkpoint rings from level.def at level.startPosition; no GPU work, safe to call on every level switch.
void BuildCheckpoints(LevelState &level);

// BuildCheckpoints plus loading the reusable checkpoint ring model. Call once at startup, after startPosition is known.
void InitLevel(LevelState &level);

// Updates elapsed time, distanceFlown, gateCrossed and checkpoint-passed flags from the
// plane's position; a ring counts only when the plane crosses its plane through the hole, and hasFlown latches once
// the plane has been airborne. The clock freezes at the gate. Does nothing once crashed.
void UpdateLevel(LevelState &level, Vector3 planePosition, bool airborne, float dt);

// Advances stageIndex through the level's stages: Takeoff when airborne, Rings when the plane's z (relative to the start) reaches endZ, Gate when the gate is crossed.
// The last stage never advances: a landing stage ends with IsRunFinished, which needs the speed that UpdateLevel does not get.
void UpdateStage(LevelState &level, Vector3 planePosition, bool airborne);

// Number of checkpoint rings flown through so far.
int CountPassed(const LevelState &level);

// True when the run is over: the gate is crossed and, if landing is required, the plane has flown at some point and has rolled out (on the ground, speed <= 8 m/s).
bool IsRunFinished(const LevelState &level, bool airborne, float speed);

// Stars for a finished run: 3 = clean + all checkpoints, 2 = clean or all checkpoints, 1 = finished.
int ComputeStars(const LevelState &level, bool damaged);

enum class Medal { None, Bronze, Silver, Gold };

// Time medal for a finished run from the gate time against the def's gold/silver/bronze limits.
Medal ComputeMedal(const LevelDef &def, float elapsed);

// Stores the elapsed time as the best when it beats the previous one; main.cpp seeds bestTime from saved progress and saves after a finish.
void RecordFinish(LevelState &level);

void DrawFinishGate(const LevelState &level);
void DrawCheckpoints(const LevelState &level);
void DrawLevelHUD(const LevelState &level, bool damaged);

// Dimmed overlay with the gate time when the gate was crossed and the Exit key hint (desktop only). The Restart/Menu buttons (mouse and touch) are drawn by DrawTouchOverlay.
void DrawCrashScreen(const LevelState &level, bool touchUsed);

// Dimmed overlay with stars, medal, time, delta vs best, a Finished/All rings/Clean checklist and the next medal target; layout scales with screen height. The Restart/Menu/Next buttons (mouse and touch) are drawn by DrawTouchOverlay.
void DrawResultsScreen(const LevelState &level, bool damaged);

// Clears progress (distance, stageIndex, gateCrossed, crashed, hasFlown, elapsed, passed checkpoints) so the
// level can be retried. Keeps the loaded model and the session best time.
void ResetLevelProgress(LevelState &level);

void UnloadLevel(LevelState &level);
