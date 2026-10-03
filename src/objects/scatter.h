#pragma once
#include "raylib.h"
#include <vector>

struct WorldState;

enum class ScatterKind { Tree, Rock };

struct ScatterProp {
    Vector3 position;
    float scale;
    float yaw;
    ScatterKind kind;
    unsigned char tint;
};

// Props are stored in random order, so any prefix is a uniform thinning; density picks the prefix length.
struct ScatterState {
    std::vector<ScatterProp> props;
};

void GenerateScatter(ScatterState &scatter, const WorldState &world);

void DrawScatter(const ScatterState &scatter, float density, Vector3 viewPosition, float drawDistance);
