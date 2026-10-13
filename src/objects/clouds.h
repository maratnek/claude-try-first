#pragma once
#include "raylib.h"
#include <vector>

struct CloudsDef;

struct CloudPuff {
    Vector3 offset;
    Vector3 radii;
};

struct Cloud {
    Vector3 position;
    float boundRadius;
    std::vector<CloudPuff> puffs;
};

// Clouds are stored in random order, so any prefix is a uniform thinning; count picks the prefix length.
struct CloudsState {
    std::vector<Cloud> clouds;
};

void GenerateClouds(CloudsState &clouds, const CloudsDef &def);

void DrawClouds(const CloudsState &clouds, int count, Vector3 viewPosition, float drawDistance);
