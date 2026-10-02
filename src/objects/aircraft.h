#pragma once
#include "raylib.h"
#include <string>
#include <vector>

struct AircraftPart {
    std::string name;
    Model model{};
    Vector3 pivot = {0.0f, 0.0f, 0.0f};
    Quaternion rest = {0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 axis = {1.0f, 0.0f, 0.0f};
    float angleDeg = 0.0f;
};

struct AircraftModel {
    Model body{};
    std::vector<AircraftPart> parts;
    bool loaded = false;
};

// dir = an exported aircraft folder, e.g. assets/models/aircraft/war-904 (see tools/aircraft-gen).
void LoadAircraftModel(AircraftModel &aircraft, const char *dir);

// Rotates a movable part (propeller, rudder_pivot, elevator_pivot, aileron_left_pivot,
// aileron_right_pivot) about its hinge axis; positive = right-hand rotation about the local axis.
void SetAircraftPartAngle(AircraftModel &aircraft, const char *name, float degrees);

// Same orientation convention as DrawPlaneObject.
void DrawAircraftObject(const AircraftModel &aircraft, Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees);

void UnloadAircraftModel(AircraftModel &aircraft);
