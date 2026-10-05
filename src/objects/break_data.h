#pragma once
#include "raylib.h"
#include "objects/glb_nodes.h"
#include <string>
#include <vector>

constexpr int kMaxBreakGroups = 16;

struct BreakGroup {
    std::string name;
    float mass = 0.0f;
    float strength = 0.0f;  // 0 for the hull, which never detaches
    int parent = -1;
    Vector3 joint = {0.0f, 0.0f, 0.0f};
    float jointScale = 1.0f;
    int meshCount = 0;
    Vector3 centreOfMass = {0.0f, 0.0f, 0.0f};
    Vector3 boundsMin = {0.0f, 0.0f, 0.0f};
    Vector3 boundsMax = {0.0f, 0.0f, 0.0f};
};

struct BreakData {
    bool valid = false;
    int hull = 0;
    std::vector<BreakGroup> groups;
    std::vector<int> meshGroup;  // per raylib mesh: index into groups
    std::vector<int> order;      // detach priority, weakest first
    std::vector<std::vector<int>> keep;  // classes of which one detached member must survive the piece cap
    bool hasFuelTank = false;
    Vector3 fuelTank = {0.0f, 0.0f, 0.0f};
    float fuelRadius = 0.0f;
};

// Reads the `.break.txt` sidecar next to the GLB. A missing or invalid file leaves `data.valid` false.
void LoadBreakData(BreakData &data, const char *glbPath, const std::vector<GlbNode> &nodes, const Model &model);
