#include "plane.h"
#include "rlgl.h"

void LoadPlaneModel(PlaneModel &planeModel, const char *path) {
    planeModel.model = LoadModel(path);
    planeModel.loaded = planeModel.model.meshCount > 0;
}

void DrawPlaneObject(const PlaneModel &planeModel, Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees) {
    if (!planeModel.loaded) return;

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(yawDegrees, 0.0f, 1.0f, 0.0f);
    // Pitch/roll signs are negated here to match PlaneState's flight
    // convention (positive pitch climbs, positive roll banks right) under
    // rlRotatef's right-hand rotation direction.
    rlRotatef(-pitchDegrees, 1.0f, 0.0f, 0.0f);
    rlRotatef(-rollDegrees, 0.0f, 0.0f, 1.0f);

    DrawModel(planeModel.model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

    rlPopMatrix();
}

void UnloadPlaneModel(PlaneModel &planeModel) {
    if (planeModel.loaded) {
        UnloadModel(planeModel.model);
    }
}
