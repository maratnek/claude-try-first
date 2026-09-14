#include "plane.h"
#include "rlgl.h"

void DrawPlaneObject(Vector3 position, float headingDegrees) {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(headingDegrees, 0.0f, 1.0f, 0.0f);

    // Fuselage
    DrawCube((Vector3){0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 4.0f, RED);
    DrawCubeWires((Vector3){0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 4.0f, MAROON);

    // Cockpit
    DrawCube((Vector3){0.0f, 0.55f, 0.8f}, 0.6f, 0.5f, 0.8f, SKYBLUE);

    // Main wings
    DrawCube((Vector3){0.0f, 0.0f, 0.2f}, 6.0f, 0.15f, 1.2f, LIGHTGRAY);

    // Horizontal tail stabilizer
    DrawCube((Vector3){0.0f, 0.0f, -1.8f}, 2.2f, 0.12f, 0.6f, LIGHTGRAY);

    // Vertical tail fin
    DrawCube((Vector3){0.0f, 0.7f, -1.8f}, 0.12f, 1.2f, 0.7f, GRAY);

    rlPopMatrix();
}
