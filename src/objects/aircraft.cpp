#include "aircraft.h"
#include "raymath.h"
#include "rlgl.h"
#include <cstdio>
#include <sstream>

void LoadAircraftModel(AircraftModel &aircraft, const char *dirArg) {
    // dirArg may itself live in a TextFormat ring buffer that the TextFormat calls below would overwrite.
    const std::string dir = dirArg;
    aircraft.body = LoadModel((dir + "/body.glb").c_str());
    aircraft.loaded = aircraft.body.meshCount > 0;

    char *text = LoadFileText((dir + "/rig.txt").c_str());
    if (text == nullptr) return;
    std::istringstream lines(text);
    UnloadFileText(text);

    std::string line;
    while (std::getline(lines, line)) {
        if (line.empty() || line[0] == '#') continue;
        char name[64];
        AircraftPart part;
        int n = sscanf(line.c_str(), "%63s %f %f %f %f %f %f %f %f %f %f", name,
                       &part.pivot.x, &part.pivot.y, &part.pivot.z,
                       &part.rest.x, &part.rest.y, &part.rest.z, &part.rest.w,
                       &part.axis.x, &part.axis.y, &part.axis.z);
        if (n != 11) continue;
        part.name = name;
        part.model = LoadModel((dir + "/" + name + ".glb").c_str());
        aircraft.parts.push_back(part);
    }
}

void SetAircraftPartAngle(AircraftModel &aircraft, const char *name, float degrees) {
    for (AircraftPart &part : aircraft.parts) {
        if (part.name == name) part.angleDeg = degrees;
    }
}

void DrawAircraftObject(const AircraftModel &aircraft, Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees) {
    if (!aircraft.loaded) return;

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(yawDegrees, 0.0f, 1.0f, 0.0f);
    // Negated to match PlaneState's convention under rlRotatef, same as DrawPlaneObject.
    rlRotatef(-pitchDegrees, 1.0f, 0.0f, 0.0f);
    rlRotatef(-rollDegrees, 0.0f, 0.0f, 1.0f);

    DrawModel(aircraft.body, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

    for (const AircraftPart &part : aircraft.parts) {
        rlPushMatrix();
        rlTranslatef(part.pivot.x, part.pivot.y, part.pivot.z);
        Vector3 restAxis;
        float restAngle;
        QuaternionToAxisAngle(part.rest, &restAxis, &restAngle);
        if (restAngle != 0.0f) rlRotatef(restAngle * RAD2DEG, restAxis.x, restAxis.y, restAxis.z);
        rlRotatef(part.angleDeg, part.axis.x, part.axis.y, part.axis.z);
        DrawModel(part.model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
        rlPopMatrix();
    }

    rlPopMatrix();
}

void UnloadAircraftModel(AircraftModel &aircraft) {
    if (aircraft.loaded) UnloadModel(aircraft.body);
    for (AircraftPart &part : aircraft.parts) {
        if (part.model.meshCount > 0) UnloadModel(part.model);
    }
    aircraft.parts.clear();
    aircraft.loaded = false;
}
