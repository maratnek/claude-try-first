#include "level.h"
#include "raymath.h"
#include "settings.h"
#include <cmath>

namespace {
constexpr int kStarsFinished = 1;
constexpr int kStarsCleanOrAllCheckpoints = 2;
constexpr int kStarsPerfect = 3;

void DrawStar(Vector2 c, float r, Color color) {
    Vector2 pts[10];
    for (int i = 0; i < 10; i++) {
        float a = (-90.0f + 36.0f * i) * DEG2RAD;
        float rad = (i % 2 == 0) ? r : r * 0.45f;
        pts[i] = {c.x + cosf(a) * rad, c.y + sinf(a) * rad};
    }
    for (int i = 0; i < 10; i++) {
        DrawTriangle(c, pts[(i + 1) % 10], pts[i], color);
    }
}
}  // namespace

int CountPassed(const LevelState &level) {
    int n = 0;
    for (const Checkpoint &cp : level.checkpoints) {
        if (cp.passed) n++;
    }
    return n;
}

void InitLevel(LevelState &level) {
    Vector3 s = level.startPosition;
    level.checkpoints = {
        {(Vector3){s.x + 10.0f, s.y + 15.0f, s.z + 150.0f}, 6.0f},
        {(Vector3){s.x - 10.0f, s.y + 25.0f, s.z + 350.0f}, 6.0f},
        {(Vector3){s.x + 8.0f, s.y + 12.0f, s.z + 550.0f}, 6.0f},
        {(Vector3){s.x - 8.0f, s.y + 20.0f, s.z + 750.0f}, 6.0f},
    };

    Mesh ringMesh = GenMeshTorus(0.5f, 5.5f, 12, 24);
    level.checkpointModel = LoadModelFromMesh(ringMesh);
}

void UpdateLevel(LevelState &level, Vector3 planePosition, float dt) {
    if (level.crashed) return;

    if (!level.completed) level.elapsed += dt;

    float dx = planePosition.x - level.startPosition.x;
    float dz = planePosition.z - level.startPosition.z;
    level.distanceFlown = sqrtf(dx * dx + dz * dz);
    if (level.distanceFlown >= level.targetDistance) {
        level.completed = true;
    }

    for (Checkpoint &cp : level.checkpoints) {
        if (!cp.passed && Vector3Distance(planePosition, cp.position) < cp.radius) {
            cp.passed = true;
        }
    }
}

int ComputeStars(const LevelState &level, bool damaged) {
    bool allCheckpoints = CountPassed(level) == (int)level.checkpoints.size();
    if (!damaged && allCheckpoints) return kStarsPerfect;
    if (!damaged || allCheckpoints) return kStarsCleanOrAllCheckpoints;
    return kStarsFinished;
}

void RecordFinish(LevelState &level) {
    level.newBest = level.bestTime < 0.0f || level.elapsed < level.bestTime;
    if (level.newBest) level.bestTime = level.elapsed;
}

void DrawFinishGate(const LevelState &level) {
    Vector3 gatePos = {
        level.startPosition.x,
        level.startPosition.y,
        level.startPosition.z + level.targetDistance,
    };

    DrawCylinder((Vector3){gatePos.x - 4.0f, 0.0f, gatePos.z}, 0.3f, 0.3f, 8.0f, 10, GOLD);
    DrawCylinder((Vector3){gatePos.x + 4.0f, 0.0f, gatePos.z}, 0.3f, 0.3f, 8.0f, 10, GOLD);
    DrawCube((Vector3){gatePos.x, 8.0f, gatePos.z}, 8.6f, 0.4f, 0.4f, GOLD);
}

void DrawCheckpoints(const LevelState &level) {
    for (const Checkpoint &cp : level.checkpoints) {
        Color tint = cp.passed ? (Color){80, 200, 100, 160} : (Color){255, 165, 0, 255};
        // Torus defaults to lying flat (hole facing up); tip it 90 degrees
        // around X so the hole faces the flight direction like a hoop.
        DrawModelEx(level.checkpointModel, cp.position, (Vector3){1.0f, 0.0f, 0.0f}, 90.0f, (Vector3){1.0f, 1.0f, 1.0f}, tint);
    }
}

void DrawLevelHUD(const LevelState &level, bool damaged) {
    int passedCount = 0;
    for (const Checkpoint &cp : level.checkpoints) {
        if (cp.passed) passedCount++;
    }

    DrawText(TextFormat("Distance: %.0f / %.0f m   Checkpoints: %d/%d",
                         level.distanceFlown, level.targetDistance,
                         passedCount, (int)level.checkpoints.size()),
             10, 85, 20, DARKGRAY);

    DrawText(damaged ? "Plane: DAMAGED" : "Plane: OK", 10, 180, 20, damaged ? MAROON : DARKGREEN);
}

void DrawCrashScreen() {
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, 150});
    const char *title = "CRASHED";
    DrawText(title, (w - MeasureText(title, 60)) / 2, h / 2 - 150, 60, RED);
#ifdef SETTINGS_MOBILE_OR_WEB
    const char *hint = "R: Restart     M: Menu";
#else
    const char *hint = "R: Restart     M: Menu     Esc / Q: Exit";
#endif
    DrawText(hint, (w - MeasureText(hint, 28)) / 2, h / 2 + 170, 28, WHITE);
}

void DrawResultsScreen(const LevelState &level, bool damaged) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, 150});
    const char *title = "LEVEL COMPLETE";
    DrawText(title, (w - MeasureText(title, 50)) / 2, h / 2 - 255, 50, GOLD);

    int stars = ComputeStars(level, damaged);
    for (int i = 0; i < 3; i++) {
        DrawStar({w / 2.0f + (i - 1) * 60.0f, h / 2.0f - 185.0f}, 26.0f,
                 i < stars ? GOLD : (Color){120, 120, 120, 255});
    }

    const char *lines[3] = {
        TextFormat("Time: %.2f s", level.elapsed),
        TextFormat("Checkpoints: %d/%d", CountPassed(level), (int)level.checkpoints.size()),
        damaged ? "Plane: DAMAGED" : "Plane: CLEAN",
    };
    for (int i = 0; i < 3; i++) {
        DrawText(lines[i], (w - MeasureText(lines[i], 24)) / 2, h / 2 - 150 + i * 28, 24, WHITE);
    }
    const char *best = TextFormat(level.newBest ? "Best: %.2f s  NEW BEST!" : "Best: %.2f s", level.bestTime);
    DrawText(best, (w - MeasureText(best, 24)) / 2, h / 2 - 150 + 3 * 28, 24, level.newBest ? GOLD : WHITE);

    const char *hint = "R: Restart     M: Menu";
    DrawText(hint, (w - MeasureText(hint, 28)) / 2, h / 2 + 170, 28, WHITE);
}

void ResetLevelProgress(LevelState &level) {
    level.distanceFlown = 0.0f;
    level.completed = false;
    level.crashed = false;
    level.elapsed = 0.0f;
    level.newBest = false;
    for (Checkpoint &cp : level.checkpoints) {
        cp.passed = false;
    }
}

void UnloadLevel(LevelState &level) {
    UnloadModel(level.checkpointModel);
}
