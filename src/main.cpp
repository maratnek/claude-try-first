#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "audio.h"
#include "flight.h"
#include "input.h"
#include "level.h"
#include "settings.h"
#include "objects/aircraft.h"
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
    AircraftModel aircraft;
    PlaneAnim planeAnim;
    WorldState world;
    PlaneState planeStart;
    PlaneState plane;
    PlaneParams planeParams = BiplaneParams();
    LevelState level;
    InputState inputState;
    GraphicsSettings gfx;
};

// Same deflection limits and signs as the biplane rig in plane.cpp.
void PoseAircraft(AircraftModel &aircraft, const PlaneAnim &anim) {
    SetAircraftPartAngle(aircraft, "propeller", anim.propAngle);
    SetAircraftPartAngle(aircraft, "elevator_pivot", anim.elevator * 25.0f);
    SetAircraftPartAngle(aircraft, "aileron_right_pivot", anim.aileron * 25.0f);
    SetAircraftPartAngle(aircraft, "aileron_left_pivot", -anim.aileron * 25.0f);
    SetAircraftPartAngle(aircraft, "rudder_pivot", -anim.rudder * 30.0f);
}

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
    const PlaneState &planeStart = g.planeStart;

    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_F1)) {
        CycleGraphicsPreset(g.gfx);
        SaveGraphicsSettings(g.gfx);
    }

    FlightInput input = ReadFlightInput(g.inputState);

    if (input.restart && (level.crashed || HasLandedSafely(plane))) {
        plane = planeStart;
        g.planeAnim = PlaneAnim{};
        ResetLevelProgress(level);
    } else if (!level.crashed) {
        float groundHeight = GetGroundHeight(world, plane.position.x, plane.position.z);
        float slopeYaw = plane.yaw * DEG2RAD;
        float sx = sinf(slopeYaw) * 2.0f, sz = cosf(slopeYaw) * 2.0f;
        float slopeDeg = atan2f(GetGroundHeight(world, plane.position.x + sx, plane.position.z + sz) -
                                    GetGroundHeight(world, plane.position.x - sx, plane.position.z - sz),
                                4.0f) * RAD2DEG;
        UpdatePlaneControls(plane, g.planeParams, input, dt, groundHeight, slopeDeg);
        UpdateLevel(level, plane.position);

        if (CheckObstacleHit(world, plane.position, 1.5f) || plane.landing == LandingResult::Hard) {
            level.crashed = true;
        }
    }

    UpdatePlaneAnimation(g.planeAnim, plane, input, level.crashed, g.planeParams.maxSpeed, dt);
    PoseAircraft(g.aircraft, g.planeAnim);
    UpdateEngineAudio(engineAudio, plane.speed / g.planeParams.maxSpeed);

    float yawRad = plane.yaw * DEG2RAD;
    Vector3 chaseOffset = {-12.0f * sinf(yawRad), 5.0f, -12.0f * cosf(yawRad)};
    camera.position = Vector3Add(plane.position, chaseOffset);
    camera.target = Vector3Add(plane.position, GetPlaneForward(plane));

    BeginDrawing();
    ClearBackground(SKYBLUE);

    BeginMode3D(camera);
    DrawWorldObject(world, g.gfx);
    DrawFinishGate(level);
    DrawCheckpoints(level);
    DrawAircraftObject(g.aircraft, plane.position, plane.yaw, plane.pitch, plane.roll);
    if (g.gfx.characters) {
        DrawCharacterObject((Vector3){-3.0f, GetGroundHeight(world, -3.0f, 3.0f), 3.0f}, 20.0f, BLUE);
        DrawCharacterObject((Vector3){3.0f, GetGroundHeight(world, 3.0f, 3.0f), 3.0f}, -20.0f, ORANGE);
    }
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
    DrawText(TextFormat("Graphics: %s (F1)", GraphicsPresetName(g.gfx)), 110, 60, 20, DARKGRAY);
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

    LoadAircraftModel(game.aircraft, AssetPath("models/aircraft/war-1"));

    InitGraphicsSettings(game.gfx);
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
    UnloadAircraftModel(game.aircraft);
    UnloadLevel(game.level);
    UnloadWorld(game.world);
    CloseWindow();
#endif
    return 0;
}
