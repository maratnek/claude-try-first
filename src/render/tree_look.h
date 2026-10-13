#pragma once
#include "raylib.h"
#include "objects/scatter.h"
#include "settings.h"

void LoadTreeLook();
void UnloadTreeLook();

void DrawTreeCards(const ScatterState &scatter, float density, Vector3 viewPosition, float drawDistance);

bool TreeCardsAvailable();
