#pragma once
#include "raylib.h"
#include "objects/glb_nodes.h"
#include "flight.h"
#include <vector>

enum PlanePart {
    PART_PROPELLER,
    PART_RUDDER,
    PART_ELEVATOR,
    PART_AILERON_RIGHT,
    PART_AILERON_LEFT,
    PART_WHEEL_RIGHT,
    PART_WHEEL_LEFT,
    PART_COUNT
};

struct PlanePartRig {
    bool found = false;
    bool spins = false;  // spins about its own local Z; otherwise hinges about `axis` from a neutral pose
    Vector3 axis = {1.0f, 0.0f, 0.0f};
    Mat4 restLocal;
    Mat4 parentWorld;
    Mat4 invRestWorld;
};

struct PlaneModel {
    Model model{};
    bool loaded = false;
    bool animated = false;
    PlanePartRig parts[PART_COUNT];
    std::vector<int> meshPart;  // per raylib mesh: PlanePart it moves with, or -1
    std::vector<bool> meshIsBlur;
};

// Control-surface deflections in [-1, 1], propeller/wheel angles in degrees.
struct PlaneAnim {
    float elevator = 0.0f;
    float aileron = 0.0f;
    float rudder = 0.0f;
    float rpm = 0.0f;  // 0..1
    float propAngle = 0.0f;
    float wheelRate = 0.0f;  // deg/s
    float wheelAngle = 0.0f;
};

// Loads the biplane 3D asset from disk. Call once at startup, after
// InitWindow(). If loading fails, DrawPlaneObject simply draws nothing.
// If the glTF node tree can't be matched to the loaded meshes, the plane
// is still drawn, just without moving parts.
void LoadPlaneModel(PlaneModel &planeModel, const char *path);

void UpdatePlaneAnimation(PlaneAnim &anim, const PlaneState &plane, const FlightInput &input, float dt);

// Draws the plane model at the given world position and orientation in
// degrees, matching PlaneState's convention (positive pitch climbs,
// positive roll banks right).
void DrawPlaneObject(const PlaneModel &planeModel, const PlaneAnim &anim, Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees);

void UnloadPlaneModel(PlaneModel &planeModel);
