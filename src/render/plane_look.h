#pragma once
#include "raylib.h"
#include "settings.h"

struct PlaneLook {
    Shader shader{};
    int lightLoc = -1;
    bool loaded = false;
};

void LoadPlaneLook(PlaneLook &look);
// Call before UnloadModel: the model would otherwise free the shared shader once per material.
void UnloadPlaneLook(PlaneLook &look, Model &model);

// Cheap enough to call each frame.
void ApplyPlaneLook(PlaneLook &look, Model &model, const GraphicsSettings &gfx);

void UpdatePlaneLight(PlaneLook &look, float yawDeg, float pitchDeg, float rollDeg);
