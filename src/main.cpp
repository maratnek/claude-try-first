#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "audio.h"
#include "flight.h"
#include "level.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/character.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Flight Game - Step 5: Engine Audio");
    SetTargetFPS(60);
    rlSetClipPlanes(0.1, 20000.0);

    InitAudioDevice();
    EngineAudio engineAudio;
    LoadEngineAudio(engineAudio);
    const float kMaxPlaneSpeedForAudio = 60.0f;  // must match flight.cpp's kMaxSpeed

    Camera3D camera = {0};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    PlaneModel planeModel;
    LoadPlaneModel(planeModel, "assets/models/biplane-1920.glb");

    WorldState world;
    GenerateWorld(world);

    PlaneState planeStart;
    planeStart.position = (Vector3){0.0f, GetGroundHeight(world, 0.0f, 0.0f) + 0.3f, 0.0f};
    PlaneState plane = planeStart;

    LevelState level;
    level.startPosition = planeStart.position;
    InitLevel(level);

    while (!WindowShouldClose()) {
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

    UnloadEngineAudio(engineAudio);
    CloseAudioDevice();
    UnloadPlaneModel(planeModel);
    UnloadLevel(level);
    UnloadWorld(world);
    CloseWindow();
    return 0;
}
