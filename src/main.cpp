#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "audio.h"
#include "flight.h"
#include "input.h"
#include "level.h"
#include "menu.h"
#include "safe_area.h"
#include "settings.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/smoke.h"
#include "objects/sprites.h"
#include "objects/explosion.h"
#include "objects/streaks.h"
#include "objects/debris.h"
#include "objects/character.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifdef FLIGHT_DEBUG
#include <cstdlib>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#define FLIGHT_IOS_SDL_MAIN 1
// UIKit needs SDL2main's real main to start the app delegate, which then calls SDL_main.
#include <SDL_main.h>
#endif
#endif

namespace {
enum class Screen { Menu, Playing, Crashed, Finished };

#ifdef FLIGHT_DEBUG
// Debug-only crash harness: --crash-test dives into the ground from FLIGHT_SPEED, saves PNGs named
// "<FLIGHT_SHOT_PREFIX>-<seconds after impact>.png" and quits. FLIGHT_FUEL (0..1) and FLIGHT_PRESET (0..2) also apply.
struct CrashTest {
    bool active = false;
    float speed = 40.0f;
    float timer = 0.0f;
    int nextShot = 0;
    const char *prefix = "crash";
};
constexpr float kCrashTestShots[] = {0.1f, 0.3f, 1.0f, 3.0f};
#endif

struct Game {
    Screen screen = Screen::Menu;
    MenuState menu;
    EngineAudio engineAudio;
    Camera3D camera = {0};
    PlaneModel planeModel;
    PlaneAnim planeAnim;
    WorldState world;
    SmokeState smoke;
    SmokeState crashSmoke;
    ExplosionState explosion;
    StreakState streaks;
    DebrisState debris;
    PlaneState planeStart;
    PlaneState plane;
    PlaneParams planeParams = BiplaneParams();
    LevelState level;
    InputState inputState;
    GraphicsSettings gfx;
    bool quitRequested = false;
    float masterVolume = 1.0f;
    float volumeShownSeconds = 0.0f;
#ifdef FLIGHT_DEBUG
    CrashTest crashTest;
#endif
    int hardLandingsInARow = 0;
};

int FitFontSize(const char *text, int size, int maxWidth) {
    while (size > 12 && MeasureText(text, size) > maxWidth) size--;
    return size;
}

bool HasLandedSafely(const PlaneState &plane) {
    return plane.landing == LandingResult::Safe && !plane.airborne;
}

Vector3 FuelTankWorld(const PlaneModel &planeModel, const PlaneState &plane) {
    if (!planeModel.breakData.hasFuelTank) return plane.position;
    return Vector3Add(plane.position, PlaneToWorld(planeModel.breakData.fuelTank, plane));
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

    bool inRun = g.screen != Screen::Menu;
    FlightInput input = ReadFlightInput(g.inputState, level.crashed || g.screen == Screen::Finished, inRun);

    if (IsKeyPressed(KEY_F1) || input.cycleGraphics) {
        CycleGraphicsPreset(g.gfx);
        ApplyTerrainColors(world, g.gfx);
        SaveGraphicsSettings(g.gfx);
    }

    float volumeDelta = (IsKeyPressed(KEY_EQUAL) ? 0.1f : 0.0f) - (IsKeyPressed(KEY_MINUS) ? 0.1f : 0.0f) + 0.1f * input.volumeStep;
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_EQUAL) || input.volumeStep != 0) {
        g.masterVolume = SetMasterVolumeClamped(g.masterVolume + volumeDelta);
        g.volumeShownSeconds = 1.5f;
    }
    g.volumeShownSeconds = fmaxf(g.volumeShownSeconds - dt, 0.0f);

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
        ClearSmoke(g.smoke);
        ClearSmoke(g.crashSmoke);
        ClearExplosion(g.explosion);
        ClearStreaks(g.streaks);
        ClearRain(world.rain);
        ClearSnow(world.snow);
        ClearDebris(g.debris);
        g.planeAnim = PlaneAnim{};
        ResetLevelProgress(level);
        StopOneShotSounds(engineAudio);
        StopWindAudio(engineAudio);
    };

#ifdef FLIGHT_DEBUG
    if (g.crashTest.active && g.screen == Screen::Menu) menuAction = MenuAction::Play;
#endif
    if (g.screen == Screen::Menu) {
        if (menuAction == MenuAction::Play) {
            resetRun();
            AdvanceWeather(world);
            g.screen = Screen::Playing;
#ifdef FLIGHT_DEBUG
            if (g.crashTest.active) {
                plane.position.y = GetGroundHeight(world, 0.0f, 0.0f) + 60.0f;
                plane.airborne = true;
                plane.speed = g.crashTest.speed;
                plane.pitch = -30.0f;
                plane.enginePower = 0.5f;
            }
#endif
        } else if (menuAction == MenuAction::Exit) {
            g.quitRequested = true;
        }
    } else if ((g.screen == Screen::Crashed || g.screen == Screen::Finished) && input.menu) {
        resetRun();
        EnterMenu(g.menu);
        g.screen = Screen::Menu;
    } else if (input.restart && (level.crashed || g.screen == Screen::Finished || HasLandedSafely(plane))) {
        resetRun();
        AdvanceWeather(world);
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
#ifdef FLIGHT_DEBUG
        if (g.crashTest.active) input.pitch = -0.01f;
#endif
        UpdatePlaneControls(plane, WithLandingAssist(g.planeParams, g.hardLandingsInARow), input, dt, groundHeight, slopeDeg);
        UpdateLevel(level, plane.position, dt);
        if (CountPassed(level) > passedBefore) PlayChimeSound(engineAudio);
        if (wasAirborne && !plane.airborne && plane.landing == LandingResult::Safe) {
            PlayTouchdownSound(engineAudio);
            g.hardLandingsInARow = 0;
        }

        ObstacleHit hit = CheckObstacleHit(world, plane.position, 1.5f);
        if (hit == ObstacleHit::Soft) {
            if (!plane.damaged) PlayDamageSound(engineAudio);
            plane.damaged = true;
        }
        if (hit == ObstacleHit::Hard || plane.landing == LandingResult::Hard) {
            if (plane.landing == LandingResult::Hard) g.hardLandingsInARow++;
            level.crashed = true;
            g.screen = Screen::Crashed;
            float s = FuelFraction(plane, g.planeParams);
            PlayCrashSound(engineAudio, s);
            Vector3 velocity = Vector3Add(Vector3Scale(GetPlaneForward(plane), plane.speed), {0.0f, -plane.fallSpeed, 0.0f});
            ImpactInfo impact = {plane.position, velocity, Vector3Length(velocity), plane.landing == LandingResult::Hard, 1.0f + 0.5f * s};
            Vector3 tank = FuelTankWorld(planeModel, plane);
            bool high = g.gfx.preset == GraphicsPreset::High;
            StartExplosion(g.explosion, tank, s, g.gfx.explosionFire ? (high ? 3 : 1) : 0, high);
            StartExplosionSmoke(g.crashSmoke, tank, s, g.gfx.explosionSmoke);
            SpawnDebris(g.debris, planeModel, plane, impact, g.gfx.debrisPieces);
        } else if (IsRunFinished(level, plane.airborne, plane.speed)) {
            RecordFinish(level);
            g.screen = Screen::Finished;
        }
    }
#ifndef SETTINGS_MOBILE_OR_WEB
    if (level.crashed && input.exit) g.quitRequested = true;
#endif

    UpdateSmoke(g.smoke, plane.position, GetPlaneForward(plane), plane.damaged && g.screen == Screen::Playing, g.gfx.smokePuffs, dt);
    UpdateExplosionSmoke(g.crashSmoke, dt);
    UpdateExplosion(g.explosion, dt);
    UpdateDebris(g.debris, world, dt);
    UpdatePlaneAnimation(g.planeAnim, plane, input, level.crashed, g.gfx.pilotHead, g.planeParams.maxSpeed, dt);
    if (g.screen == Screen::Playing) {
        UpdateEngineAudio(engineAudio, plane.enginePower, plane.speed / g.planeParams.maxSpeed);
        UpdateWindAudio(engineAudio, plane.speed / g.planeParams.maxSpeed, plane.airborne);
    } else {
        StopEngineAudio(engineAudio);
        StopWindAudio(engineAudio);
    }

    float yawRad = plane.yaw * DEG2RAD;
    Vector3 chaseOffset = {-12.0f * sinf(yawRad), 5.0f, -12.0f * cosf(yawRad)};
    camera.position = Vector3Add(plane.position, chaseOffset);
    camera.target = Vector3Add(plane.position, GetPlaneForward(plane));

    UpdateStreaks(g.streaks, camera.position, GetPlaneForward(plane), plane.speed / g.planeParams.maxSpeed,
                  g.screen == Screen::Playing && plane.airborne, g.gfx.speedStreaks);

    UpdateRain(world.rain, camera.position, ActiveRainDrops(world, g.gfx), dt);
    UpdateSnow(world.snow, camera.position, ActiveSnowFlakes(world, g.gfx), dt);

    BeginDrawing();
    ClearBackground(SKYBLUE);

    BeginMode3D(camera);
    DrawWorldObject(world, g.gfx, plane.position);
    DrawFinishGate(level);
    DrawCheckpoints(level);
    if (g.gfx.blobShadow) DrawBlobShadow(world, plane.position, plane.yaw);
    DrawPlaneObject(planeModel, g.planeAnim, plane.position, plane.yaw, plane.pitch, plane.roll, g.gfx.propBlur, DebrisDetachedParts(g.debris), DebrisDetachedGroups(g.debris), plane.damaged && g.gfx.damageVisuals);
    DrawDebris(g.debris, planeModel);
    if (g.gfx.smokePuffs > 0) DrawSmoke(g.smoke, camera);
    DrawExplosion(g.explosion, camera);
    DrawSmoke(g.crashSmoke, camera);
    if (g.gfx.speedStreaks > 0) DrawStreaks(g.streaks);
    if (g.gfx.characters) {
        DrawCharacterObject((Vector3){-3.0f, GetGroundHeight(world, -3.0f, 3.0f), 3.0f}, 20.0f, BLUE);
        DrawCharacterObject((Vector3){3.0f, GetGroundHeight(world, 3.0f, 3.0f), 3.0f}, -20.0f, ORANGE);
    }
    EndMode3D();
    DrawExplosionFlash(g.explosion);

    if (g.volumeShownSeconds > 0.0f) {
        const char *volumeText = TextFormat("Volume: %d%%", (int)roundf(g.masterVolume * 100.0f));
        DrawText(volumeText, GetScreenWidth() - (int)GetSafeArea().right - MeasureText(volumeText, 20) - 10,
                 85 + (int)GetSafeArea().top, 20, DARKGRAY);
    }

    if (g.screen == Screen::Menu) {
        DrawMenu(g.menu);
        EndDrawing();
        return;
    }

    const SafeArea sa = GetSafeArea();
    const int ox = 10 + (int)sa.left, oy = (int)sa.top;
#ifdef __EMSCRIPTEN__
    const int hudRight = 150;
#else
    const int hudRight = 10;
#endif
    const int hudWidth = GetScreenWidth() - (int)sa.right - hudRight - ox;
    const char *controlsText = g.inputState.touchUsed ? "Left stick: pitch/roll  +/-: engine power"
                                                      : "Arrows = pitch/roll, A/D = rudder, W/S = engine power";
    DrawText(controlsText, ox, 10 + oy, FitFontSize(controlsText, 20, hudWidth), DARKGRAY);
    const char *statusText = TextFormat("Speed: %.1f m/s   Altitude: %.1f m   Power: %d%%   %s",
                         plane.speed, plane.position.y, (int)roundf(plane.enginePower * 100.0f), plane.airborne ? "AIRBORNE" : "ON GROUND - raise power (W), pull up to take off; brake: power 0, then S");
    DrawText(statusText, ox, 35 + oy, FitFontSize(statusText, 20, hudWidth), DARKGRAY);
    DrawLevelHUD(level, plane.damaged);
    if (plane.landing == LandingResult::Hard) {
        DrawText("Hard landing!", ox, 150 + oy, 30, MAROON);
    } else if (HasLandedSafely(plane)) {
        DrawText("Landed safely - take off again", ox, 150 + oy, 30, DARKGREEN);
    }
    if (g.screen == Screen::Playing && level.gateCrossed && g.inputState.touchUsed) {
        DrawText("Power to zero, then hold - to brake after landing", ox, 215 + oy, 24, DARKGRAY);
    }
    DrawFPS(ox, 60 + oy);
    DrawText(TextFormat("Graphics: %s (F1)", GraphicsPresetName(g.gfx)), ox + 100, 60 + oy, 20, DARKGRAY);
    if (level.crashed) DrawCrashScreen(level, g.inputState.touchUsed);
    if (g.screen == Screen::Finished) DrawResultsScreen(level, plane.damaged);
    DrawTouchOverlay(g.inputState, level.crashed || g.screen == Screen::Finished, true, TextFormat("Gfx: %s", GraphicsPresetName(g.gfx)));
#ifdef FLIGHT_DEBUG
    if (g.crashTest.active && g.screen == Screen::Crashed) {
        g.crashTest.timer += dt;
        if (g.crashTest.nextShot < (int)(sizeof(kCrashTestShots) / sizeof(kCrashTestShots[0])) &&
            g.crashTest.timer >= kCrashTestShots[g.crashTest.nextShot]) {
            rlDrawRenderBatchActive();
            Image shot = LoadImageFromScreen();
            ExportImage(shot, TextFormat("%s-%.1f.png", g.crashTest.prefix, kCrashTestShots[g.crashTest.nextShot]));
            UnloadImage(shot);
            if (++g.crashTest.nextShot == (int)(sizeof(kCrashTestShots) / sizeof(kCrashTestShots[0]))) g.quitRequested = true;
        }
    }
#endif
    EndDrawing();
}

void UpdateFrameCallback(void *arg) {
    UpdateFrame(*static_cast<Game *>(arg));
}
}  // namespace

#ifdef FLIGHT_DEBUG
int main(int argc, char **argv) {
#elif defined(FLIGHT_IOS_SDL_MAIN)
int main(int, char **) {
#else
int main() {
#endif
    const int screenWidth = 1280;
    const int screenHeight = 720;

#ifdef __EMSCRIPTEN__
    // raylib only tracks the browser window size (canvas fills the page) when resizable
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(screenWidth, screenHeight, "Flight Game");
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
#ifdef FLIGHT_DEBUG
    if (const char *preset = getenv("FLIGHT_PRESET")) ApplyGraphicsPreset(game.gfx, static_cast<GraphicsPreset>(atoi(preset) % 3));
#endif
    GenerateWorld(game.world, game.gfx);

    const LevelDef &levelDef = Level1Def();
    game.planeStart.position = (Vector3){levelDef.startX, GetGroundHeight(game.world, levelDef.startX, levelDef.startZ) + levelDef.startHeightAboveGround, levelDef.startZ};
    game.planeStart.fuel = game.planeParams.fuelCapacity;
#ifdef FLIGHT_DEBUG
    if (const char *fuel = getenv("FLIGHT_FUEL")) game.planeStart.fuel = game.planeParams.fuelCapacity * (float)atof(fuel);
    for (int i = 1; i < argc; i++) {
        if (TextIsEqual(argv[i], "--crash-test")) game.crashTest.active = true;
    }
    if (const char *speed = getenv("FLIGHT_SPEED")) game.crashTest.speed = (float)atof(speed);
    if (const char *prefix = getenv("FLIGHT_SHOT_PREFIX")) game.crashTest.prefix = prefix;
#endif
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
    UnloadSprites();
    CloseWindow();
#endif
    return 0;
}
