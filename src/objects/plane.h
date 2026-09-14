#pragma once
#include "raylib.h"

struct PlaneModel {
    Model model{};
    bool loaded = false;
};

// Loads the biplane 3D asset from disk. Call once at startup, after
// InitWindow(). If loading fails, DrawPlaneObject simply draws nothing.
void LoadPlaneModel(PlaneModel &planeModel, const char *path);

// Draws the plane model at the given world position and orientation in
// degrees, matching PlaneState's convention (positive pitch climbs,
// positive roll banks right).
void DrawPlaneObject(const PlaneModel &planeModel, Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees);

void UnloadPlaneModel(PlaneModel &planeModel);
