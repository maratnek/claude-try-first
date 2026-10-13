#pragma once
#include "raylib.h"
#include "settings.h"

struct PlaneLook {
    Shader shader{};
    int lightLoc = -1;
    bool loaded = false;
};

void LoadPlaneLook(PlaneLook &look);
void UnloadPlaneLook(PlaneLook &look);

// Gives every material of the model the cel shader (or back the raylib default one); cheap enough to call each frame.
void ApplyPlaneLook(PlaneLook &look, Model &model, const GraphicsSettings &gfx);

// Sun direction in plane space for the current attitude, so the bands stay fixed in the world as the plane turns.
void UpdatePlaneLight(PlaneLook &look, float yawDeg, float pitchDeg, float rollDeg);
