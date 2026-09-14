#include "character.h"
#include "rlgl.h"

void DrawCharacterObject(Vector3 position, float headingDegrees, Color bodyColor) {
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(headingDegrees, 0.0f, 1.0f, 0.0f);

    DrawCylinder((Vector3){0.0f, 0.0f, 0.0f}, 0.3f, 0.35f, 1.2f, 12, bodyColor);
    DrawSphere((Vector3){0.0f, 1.35f, 0.0f}, 0.25f, bodyColor);

    rlPopMatrix();
}
