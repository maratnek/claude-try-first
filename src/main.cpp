#include "raylib.h"
#include "objects/plane.h"
#include "objects/world.h"
#include "objects/character.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Flight Game - Step 2: Plane, World, Characters");
    SetTargetFPS(60);

    Camera3D camera = {0};
    camera.position = (Vector3){0.0f, 8.0f, -15.0f};
    camera.target = (Vector3){0.0f, 2.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 planePosition = {0.0f, 2.0f, 0.0f};

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
        DrawWorldObject();
        DrawPlaneObject(planePosition, 0.0f);
        DrawCharacterObject((Vector3){-3.0f, 0.0f, 3.0f}, 20.0f, BLUE);
        DrawCharacterObject((Vector3){3.0f, 0.0f, 3.0f}, -20.0f, ORANGE);
        EndMode3D();

        DrawText("Step 2: plane + world + minimal characters", 10, 10, 20, DARKGRAY);
        DrawFPS(10, 40);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
