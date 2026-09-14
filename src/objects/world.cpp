#include "world.h"
#include "raylib.h"

void DrawWorldObject() {
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){400.0f, 400.0f}, (Color){80, 160, 90, 255});
    DrawGrid(80, 5.0f);

    // Marker pillars every 200m to give a sense of scale toward future levels.
    for (int i = 1; i <= 5; i++) {
        float z = i * 200.0f;
        DrawCylinder((Vector3){-15.0f, 0.0f, z}, 0.3f, 0.3f, 4.0f, 8, DARKGRAY);
        DrawCylinder((Vector3){15.0f, 0.0f, z}, 0.3f, 0.3f, 4.0f, 8, DARKGRAY);
    }
}
