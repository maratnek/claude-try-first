#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "audio.h"
#include "flight.h"
#include "level.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/character.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace {
const float kMaxPlaneSpeedForAudio = 60.0f;  // must match flight.cpp's kMaxSpeed

struct Game {
    EngineAudio engineAudio;
    Camera3D camera = {0};
    PlaneModel planeModel;
    WorldState world;
    PlaneState planeStart;
    PlaneState plane;
    LevelState level;
};

// Emscripten preloads assets into the virtual FS root; there is no meaningful application directory.
const char *AssetPath(const char *relative) {
#ifdef __EMSCRIPTEN__
    return TextFormat("assets/%s", relative);
#else
    return TextFormat("%sassets/%s", GetApplicationDirectory(), relative);
#endif
}

void UpdateFrame(Game &g) {
    PlaneState &plane = g.plane;
    LevelState &level = g.level;
    WorldState &world = g.world;
    Camera3D &camera = g.camera;
    EngineAudio &engineAudio = g.engineAudio;
    PlaneModel &planeModel = g.planeModel;
    const PlaneState &planeStart = g.planeStart;

    float dt = GetFrameTime();

    if (level.crashed) {
        if (IsKeyPressed(KEY_R)) {
            plane = planeStart;
            ResetLevelProgress(level);
        }
    } else {
        float groundHeight = GetGroundHeight(world, plane.position.x, plane.position.z);
        UpdatePlaneControls(plane, dt, groundHeight);
        UpdateLevel(level, plane.position);

        if (CheckObstacleHit(world, plane.position, 1.5f)) {
            level.crashed = true;
        }
    }

    UpdateEngineAudio(engineAudio, plane.speed / kMaxPlaneSpeedForAudio);

    float yawRad = plane.yaw * DEG2RAD;
    Vector3 chaseOffset = {-12.0f * sinf(yawRad), 5.0f, -12.0f * cosf(yawRad)};
    camera.position = Vector3Add(plane.position, chaseOffset);
    camera.target = Vector3Add(plane.position, GetPlaneForward(plane));

    BeginDrawing();
    ClearBackground(SKYBLUE);

    BeginMode3D(camera);
    DrawWorldObject(world);
    DrawFinishGate(level);
    DrawCheckpoints(level);
    DrawPlaneObject(planeModel, plane.position, plane.yaw, plane.pitch, plane.roll);
    DrawCharacterObject((Vector3){-3.0f, GetGroundHeight(world, -3.0f, 3.0f), 3.0f}, 20.0f, BLUE);
    DrawCharacterObject((Vector3){3.0f, GetGroundHeight(world, 3.0f, 3.0f), 3.0f}, -20.0f, ORANGE);
    EndMode3D();

    DrawText("Arrows = pitch/roll, A/D = rudder, W/S = throttle", 10, 10, 20, DARKGRAY);
    DrawText(TextFormat("Speed: %.1f m/s   Altitude: %.1f m   %s",
                         plane.speed, plane.position.y, plane.airborne ? "AIRBORNE" : "ON GROUND - throttle up, pull up to take off"),
             10, 35, 20, DARKGRAY);
    DrawLevelHUD(level);
    DrawFPS(10, 60);
    EndDrawing();
}

void UpdateFrameCallback(void *arg) {
    UpdateFrame(*static_cast<Game *>(arg));
}
}  // namespace

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Flight Game - Step 5: Engine Audio");
    rlSetClipPlanes(0.1, 20000.0);

    InitAudioDevice();
    static Game game;
    LoadEngineAudio(game.engineAudio);

    game.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    game.camera.fovy = 60.0f;
    game.camera.projection = CAMERA_PERSPECTIVE;

    LoadPlaneModel(game.planeModel, AssetPath("models/biplane-1920.glb"));

    GenerateWorld(game.world);

    game.planeStart.position = (Vector3){0.0f, GetGroundHeight(game.world, 0.0f, 0.0f) + 0.3f, 0.0f};
    game.plane = game.planeStart;

    game.level.startPosition = game.planeStart.position;
    InitLevel(game.level);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(UpdateFrameCallback, &game, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateFrame(game);
    }

    UnloadEngineAudio(game.engineAudio);
    CloseAudioDevice();
    UnloadPlaneModel(game.planeModel);
    UnloadLevel(game.level);
    UnloadWorld(game.world);
    CloseWindow();
#endif
    return 0;
}
