#include "raylib.h"
#include "raymath.h"
#include "flight.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/character.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Flight Game - Step 3: Keyboard Controls");
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    PlaneState plane;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        UpdatePlaneControls(plane, dt);

        float yawRad = plane.yaw * DEG2RAD;
        Vector3 chaseOffset = {-12.0f * sinf(yawRad), 5.0f, -12.0f * cosf(yawRad)};
        camera.position = Vector3Add(plane.position, chaseOffset);
        camera.target = Vector3Add(plane.position, GetPlaneForward(plane));

        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
        DrawWorldObject();
        DrawPlaneObject(plane.position, plane.yaw, plane.pitch, plane.roll);
        DrawCharacterObject((Vector3){-3.0f, 0.0f, 3.0f}, 20.0f, BLUE);
        DrawCharacterObject((Vector3){3.0f, 0.0f, 3.0f}, -20.0f, ORANGE);
        EndMode3D();

        DrawText("Step 3: Arrows = pitch/roll, A/D = rudder, W/S = throttle", 10, 10, 20, DARKGRAY);
        DrawText(TextFormat("Speed: %.1f m/s   Altitude: %.1f m", plane.speed, plane.position.y), 10, 35, 20, DARKGRAY);
        DrawFPS(10, 60);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
