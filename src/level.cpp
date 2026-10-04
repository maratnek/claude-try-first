#include "level.h"
#include "raymath.h"
#include "safe_area.h"
#include "settings.h"
#include <cmath>

namespace {
constexpr float kMedalGold = 24.0f;
constexpr float kMedalSilver = 30.0f;
constexpr float kMedalBronze = 40.0f;

constexpr bool kRequireLandingAfterGate = true;
constexpr float kRolloutSpeed = 8.0f;
constexpr float kLandingZoneLength = 300.0f;
constexpr float kLandingZoneWidth = 40.0f;

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

void DrawCentered(const char *text, int cx, int y, int size, Color color) {
    DrawText(text, cx - MeasureText(text, size) / 2, y, size, color);
}

const char *MedalName(Medal medal) {
    switch (medal) {
        case Medal::Gold: return "Gold";
        case Medal::Silver: return "Silver";
        case Medal::Bronze: return "Bronze";
        default: return "No medal";
    }
}

Color MedalColor(Medal medal) {
    switch (medal) {
        case Medal::Gold: return GOLD;
        case Medal::Silver: return (Color){200, 205, 215, 255};
        case Medal::Bronze: return (Color){205, 127, 50, 255};
        default: return (Color){120, 120, 120, 255};
    }
}

const char *NextTargetLine(Medal medal, int stars) {
    switch (medal) {
        case Medal::Gold: return stars == 3 ? "Gold - perfect run!" : "Gold - perfect run needs 3 stars";
        case Medal::Silver: return TextFormat("Silver! Gold at %.1f s", kMedalGold);
        case Medal::Bronze: return TextFormat("Bronze! Silver at %.1f s", kMedalSilver);
        default: return TextFormat("Bronze at %.1f s", kMedalBronze);
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

    if (!level.gateCrossed) level.elapsed += dt;

    float dx = planePosition.x - level.startPosition.x;
    float dz = planePosition.z - level.startPosition.z;
    level.distanceFlown = sqrtf(dx * dx + dz * dz);
    if (level.distanceFlown >= level.targetDistance) {
        level.gateCrossed = true;
    }

    for (Checkpoint &cp : level.checkpoints) {
        if (!cp.passed && Vector3Distance(planePosition, cp.position) < cp.radius) {
            cp.passed = true;
        }
    }
}

bool IsRunFinished(const LevelState &level, bool airborne, float speed) {
    if (!level.gateCrossed) return false;
    if (!kRequireLandingAfterGate) return true;
    return !airborne && speed <= kRolloutSpeed;
}

int ComputeStars(const LevelState &level, bool damaged) {
    bool allCheckpoints = CountPassed(level) == (int)level.checkpoints.size();
    if (!damaged && allCheckpoints) return kStarsPerfect;
    if (!damaged || allCheckpoints) return kStarsCleanOrAllCheckpoints;
    return kStarsFinished;
}

Medal ComputeMedal(float elapsed) {
    if (elapsed <= kMedalGold) return Medal::Gold;
    if (elapsed <= kMedalSilver) return Medal::Silver;
    if (elapsed <= kMedalBronze) return Medal::Bronze;
    return Medal::None;
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

    if (kRequireLandingAfterGate) {
        DrawCube((Vector3){gatePos.x, 0.05f, gatePos.z + kLandingZoneLength / 2.0f},
                 kLandingZoneWidth, 0.1f, kLandingZoneLength, (Color){80, 200, 100, 255});
    }
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
    SafeArea sa = GetSafeArea();

    DrawText(TextFormat("Distance: %.0f / %.0f m   Checkpoints: %d/%d",
                         level.distanceFlown, level.targetDistance,
                         passedCount, (int)level.checkpoints.size()),
             10 + (int)sa.left, 85 + (int)sa.top, 20, DARKGRAY);

    DrawText(damaged ? "Plane: DAMAGED" : "Plane: OK", 10 + (int)sa.left, 180 + (int)sa.top, 20, damaged ? MAROON : DARKGREEN);

    if (level.gateCrossed && kRequireLandingAfterGate) {
        const char *banner = "GATE! Land to finish";
        DrawText(banner, (GetScreenWidth() - MeasureText(banner, 40)) / 2, 110 + (int)sa.top, 40, GOLD);
    }
}

void DrawCrashScreen(const LevelState &level, bool touchUsed) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, 150});
    const char *title = "CRASHED";
    DrawText(title, (w - MeasureText(title, 60)) / 2, h / 2 - 150, 60, RED);
    if (level.gateCrossed) {
        const char *gateLine = TextFormat("Gate reached in %.2f s", level.elapsed);
        DrawText(gateLine, (w - MeasureText(gateLine, 28)) / 2, h / 2 - 70, 28, WHITE);
    }
#ifdef SETTINGS_MOBILE_OR_WEB
    const char *hint = "R: Restart     M: Menu";
#else
    const char *hint = "R: Restart     M: Menu     Esc / Q: Exit";
#endif
    if (!touchUsed) DrawText(hint, (w - MeasureText(hint, 28)) / 2, h / 2 + 170, 28, WHITE);
}

void DrawResultsScreen(const LevelState &level, bool damaged, bool touchUsed) {
    int w = GetScreenWidth(), h = GetScreenHeight();
    DrawRectangle(0, 0, w, h, (Color){0, 0, 0, 150});

    constexpr float kLayoutHeight = 500.0f;
    float s = fminf(h / 600.0f, 1.0f);
    float top = (h - kLayoutHeight * s) / 2.0f;
    auto sz = [&](int base) { return (int)fmaxf(base * s, 10.0f); };
    auto y = [&](float base) { return (int)(top + base * s); };
    int cx = w / 2;

    DrawCentered("LEVEL COMPLETE", cx, y(0), sz(50), GOLD);

    int stars = ComputeStars(level, damaged);
    for (int i = 0; i < 3; i++) {
        DrawStar({cx + (i - 1) * 60.0f * s, (float)y(95)}, 26.0f * s,
                 i < stars ? GOLD : (Color){120, 120, 120, 255});
    }

    Medal medal = ComputeMedal(level.elapsed);
    float medalX = cx + 170.0f * s;
    DrawCircle((int)medalX, y(95), 22.0f * s, MedalColor(medal));
    DrawCentered(MedalName(medal), (int)medalX, y(125), sz(20), MedalColor(medal));

    DrawCentered(TextFormat("Time: %.2f s", level.elapsed), cx, y(150), sz(30), WHITE);
    if (level.newBest) {
        DrawCentered("NEW BEST", cx, y(188), sz(24), GOLD);
    } else {
        DrawCentered(TextFormat("+%.2f s vs best %.2f s", level.elapsed - level.bestTime, level.bestTime),
                     cx, y(188), sz(24), WHITE);
    }

    bool allRings = CountPassed(level) == (int)level.checkpoints.size();
    struct Goal {
        const char *text;
        bool met;
    } goals[3] = {
        {"Finished", true},
        {TextFormat("All rings (%d/%d)", CountPassed(level), (int)level.checkpoints.size()), allRings},
        {"Clean", !damaged},
    };
    Color unmet = (Color){255, 165, 0, 130};
    for (int i = 0; i < 3; i++) {
        const char *line = TextFormat("%s %s", goals[i].met ? "[x]" : "[ ]", goals[i].text);
        DrawCentered(line, cx, y(245 + i * 32), sz(26), goals[i].met ? WHITE : unmet);
    }

    DrawCentered(NextTargetLine(medal, stars), cx, y(365), sz(26), MedalColor(medal));

    if (!touchUsed) DrawCentered("R: Restart     M: Menu", cx, y(450), sz(28), WHITE);
}

void ResetLevelProgress(LevelState &level) {
    level.distanceFlown = 0.0f;
    level.gateCrossed = false;
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
