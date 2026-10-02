#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "audio.h"
#include "flight.h"
#include "input.h"
#include "level.h"
#include "menu.h"
#include "settings.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/character.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace {
enum class Screen { Menu, Playing, Crashed, Finished };

struct Game {
    Screen screen = Screen::Menu;
    MenuState menu;
    EngineAudio engineAudio;
    Camera3D camera = {0};
    PlaneModel planeModel;
    PlaneAnim planeAnim;
    WorldState world;
    PlaneState planeStart;
    PlaneState plane;
    PlaneParams planeParams = BiplaneParams();
    LevelState level;
    InputState inputState;
    GraphicsSettings gfx;
    bool quitRequested = false;
    float masterVolume = 1.0f;
    float volumeShownSeconds = 0.0f;
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

    if (IsKeyPressed(KEY_F1)) {
        CycleGraphicsPreset(g.gfx);
        SaveGraphicsSettings(g.gfx);
    }

    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_EQUAL)) {
        g.masterVolume = SetMasterVolumeClamped(g.masterVolume + (IsKeyPressed(KEY_EQUAL) ? 0.1f : -0.1f));
        g.volumeShownSeconds = 1.5f;
    }
    g.volumeShownSeconds = fmaxf(g.volumeShownSeconds - dt, 0.0f);

    FlightInput input = ReadFlightInput(g.inputState);
    MenuAction menuAction = MenuAction::None;
    if (g.screen == Screen::Menu) menuAction = UpdateMenu(g.menu);

    bool crashOrResultsButton = (g.screen == Screen::Crashed || g.screen == Screen::Finished) &&
                                (input.menu || input.restart);
#ifndef SETTINGS_MOBILE_OR_WEB
    if (level.crashed && input.exit) crashOrResultsButton = true;
#endif
    if (menuAction != MenuAction::None || crashOrResultsButton) PlayClickSound(engineAudio);

    auto resetRun = [&]() {
        plane = planeStart;
        g.planeAnim = PlaneAnim{};
        ResetLevelProgress(level);
        StopOneShotSounds(engineAudio);
        StopWindAudio(engineAudio);
    };

    if (g.screen == Screen::Menu) {
        if (menuAction == MenuAction::Play) {
            resetRun();
            g.screen = Screen::Playing;
        } else if (menuAction == MenuAction::Exit) {
            g.quitRequested = true;
        }
    } else if ((g.screen == Screen::Crashed || g.screen == Screen::Finished) && input.menu) {
        resetRun();
        EnterMenu(g.menu);
        g.screen = Screen::Menu;
    } else if (input.restart && (level.crashed || g.screen == Screen::Finished || HasLandedSafely(plane))) {
        resetRun();
        g.screen = Screen::Playing;
    } else if (g.screen == Screen::Playing) {
        float groundHeight = GetGroundHeight(world, plane.position.x, plane.position.z);
        float slopeYaw = plane.yaw * DEG2RAD;
        float sx = sinf(slopeYaw) * 2.0f, sz = cosf(slopeYaw) * 2.0f;
        float slopeDeg = atan2f(GetGroundHeight(world, plane.position.x + sx, plane.position.z + sz) -
                                    GetGroundHeight(world, plane.position.x - sx, plane.position.z - sz),
                                4.0f) * RAD2DEG;
        bool wasAirborne = plane.airborne;
        int passedBefore = CountPassed(level);
        UpdatePlaneControls(plane, g.planeParams, input, dt, groundHeight, slopeDeg);
        UpdateLevel(level, plane.position, dt);
        if (CountPassed(level) > passedBefore) PlayChimeSound(engineAudio);
        if (wasAirborne && !plane.airborne && plane.landing == LandingResult::Safe) PlayTouchdownSound(engineAudio);

        ObstacleHit hit = CheckObstacleHit(world, plane.position, 1.5f);
        if (hit == ObstacleHit::Soft) {
            if (!plane.damaged) PlayDamageSound(engineAudio);
            plane.damaged = true;
        }
        if (hit == ObstacleHit::Hard || plane.landing == LandingResult::Hard) {
            level.crashed = true;
            g.screen = Screen::Crashed;
            PlayCrashSound(engineAudio);
        } else if (level.completed) {
            RecordFinish(level);
            g.screen = Screen::Finished;
        }
    }
#ifndef SETTINGS_MOBILE_OR_WEB
    if (level.crashed && input.exit) g.quitRequested = true;
#endif

    UpdatePlaneAnimation(g.planeAnim, plane, input, level.crashed, g.planeParams.maxSpeed, dt);
    if (g.screen == Screen::Playing) {
        UpdateEngineAudio(engineAudio, plane.speed / g.planeParams.maxSpeed);
        UpdateWindAudio(engineAudio, plane.speed / g.planeParams.maxSpeed, plane.airborne);
    } else {
        StopEngineAudio(engineAudio);
        StopWindAudio(engineAudio);
    }

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
    if (g.gfx.blobShadow) DrawBlobShadow(world, plane.position, plane.yaw);
    DrawPlaneObject(planeModel, g.planeAnim, plane.position, plane.yaw, plane.pitch, plane.roll, g.gfx.propBlur);
    if (g.gfx.characters) {
        DrawCharacterObject((Vector3){-3.0f, GetGroundHeight(world, -3.0f, 3.0f), 3.0f}, 20.0f, BLUE);
        DrawCharacterObject((Vector3){3.0f, GetGroundHeight(world, 3.0f, 3.0f), 3.0f}, -20.0f, ORANGE);
    }
    EndMode3D();

    if (g.volumeShownSeconds > 0.0f) {
        const char *volumeText = TextFormat("Volume: %d%%", (int)roundf(g.masterVolume * 100.0f));
        DrawText(volumeText, GetScreenWidth() - MeasureText(volumeText, 20) - 10, 85, 20, DARKGRAY);
    }

    if (g.screen == Screen::Menu) {
        DrawMenu(g.menu);
        EndDrawing();
        return;
    }

    DrawText(g.inputState.touchUsed ? "Left stick: pitch/roll  +/-: throttle"
                                    : "Arrows = pitch/roll, A/D = rudder, W/S = throttle",
             10, 10, 20, DARKGRAY);
    DrawText(TextFormat("Speed: %.1f m/s   Altitude: %.1f m   %s",
                         plane.speed, plane.position.y, plane.airborne ? "AIRBORNE" : "ON GROUND - throttle up, pull up to take off"),
             10, 35, 20, DARKGRAY);
    DrawLevelHUD(level, plane.damaged);
    if (plane.landing == LandingResult::Hard) {
        DrawText("Hard landing!", 10, 150, 30, MAROON);
    } else if (HasLandedSafely(plane)) {
        DrawText("Landed safely - take off again", 10, 150, 30, DARKGREEN);
    }
    DrawFPS(10, 60);
    DrawText(TextFormat("Graphics: %s (F1)", GraphicsPresetName(g.gfx)), 110, 60, 20, DARKGRAY);
    if (level.crashed) DrawCrashScreen();
    if (g.screen == Screen::Finished) DrawResultsScreen(level, plane.damaged);
    DrawTouchOverlay(g.inputState, level.crashed || g.screen == Screen::Finished);
    EndDrawing();
}

void UpdateFrameCallback(void *arg) {
    UpdateFrame(*static_cast<Game *>(arg));
}
}  // namespace

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

#ifdef __EMSCRIPTEN__
    // raylib only tracks the browser window size (canvas fills the page) when resizable
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(screenWidth, screenHeight, "Flight Game - Step 5: Engine Audio");
    rlSetClipPlanes(0.1, 20000.0);

    InitAudioDevice();
    static Game game;
    LoadEngineAudio(game.engineAudio);

    game.camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    game.camera.fovy = 60.0f;
    game.camera.projection = CAMERA_PERSPECTIVE;

    LoadPlaneModel(game.planeModel, AssetPath("models/biplane-1920.glb"));

    InitMenu(game.menu);
    EnterMenu(game.menu);
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
    while (!WindowShouldClose() && !game.quitRequested) {
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
