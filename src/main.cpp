#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "audio.h"
#include "flight.h"
#include "input.h"
#include "level.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/character.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace {
struct Game {
    EngineAudio engineAudio;
    Camera3D camera = {0};
    PlaneModel planeModel;
    PlaneAnim planeAnim;
    WorldState world;
    PlaneState planeStart;
    PlaneState plane;
    LevelState level;
    InputState inputState;
};

bool HasLandedSafely(const PlaneState &plane) {
    return plane.landing == LandingResult::Safe && !plane.airborne;
}

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

    FlightInput input = ReadFlightInput(g.inputState);

    if (input.restart && (level.crashed || HasLandedSafely(plane))) {
        plane = planeStart;
        g.planeAnim = PlaneAnim{};
        ResetLevelProgress(level);
    } else if (!level.crashed) {
        float groundHeight = GetGroundHeight(world, plane.position.x, plane.position.z);
        UpdatePlaneControls(plane, input, dt, groundHeight);
        UpdateLevel(level, plane.position);

        if (CheckObstacleHit(world, plane.position, 1.5f) || plane.landing == LandingResult::Hard) {
            level.crashed = true;
        }
    }

    UpdatePlaneAnimation(g.planeAnim, plane, input, level.crashed, dt);
    UpdateEngineAudio(engineAudio, plane.speed / kMaxSpeed);

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
    DrawPlaneObject(planeModel, g.planeAnim, plane.position, plane.yaw, plane.pitch, plane.roll);
    DrawCharacterObject((Vector3){-3.0f, GetGroundHeight(world, -3.0f, 3.0f), 3.0f}, 20.0f, BLUE);
    DrawCharacterObject((Vector3){3.0f, GetGroundHeight(world, 3.0f, 3.0f), 3.0f}, -20.0f, ORANGE);
    EndMode3D();

    DrawText(g.inputState.touchUsed ? "Left stick: pitch/roll  +/-: throttle"
                                    : "Arrows = pitch/roll, A/D = rudder, W/S = throttle",
             10, 10, 20, DARKGRAY);
    DrawText(TextFormat("Speed: %.1f m/s   Altitude: %.1f m   %s",
                         plane.speed, plane.position.y, plane.airborne ? "AIRBORNE" : "ON GROUND - throttle up, pull up to take off"),
             10, 35, 20, DARKGRAY);
    DrawLevelHUD(level);
    if (plane.landing == LandingResult::Hard) {
        DrawText("Hard landing!", 10, 150, 30, MAROON);
    } else if (HasLandedSafely(plane)) {
        DrawText("Landed safely - take off again", 10, 150, 30, DARKGREEN);
    }
    DrawFPS(10, 60);
    DrawTouchOverlay(g.inputState, level.crashed);
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
