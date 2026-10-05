#pragma once
#include <string>
#include <vector>

// 4x4 matrix in glTF/OpenGL layout: element (row, col) at m[row + 4*col], translation in m[12..14].
// raylib's Matrix struct declares its fields in a different memory order, so it is not used for node math.
struct Mat4 {
    float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
};

struct GlbNode {
    std::string name;
    int parent = -1;
    Mat4 local;
    int trianglePrimitives = 0;
};

// raylib's LoadModel drops node names and bakes node transforms into vertices, so the glTF node tree is read separately here.
// Fills nodes in file order; returns false if the file is not a readable GLB.
bool LoadGlbNodes(const char *path, std::vector<GlbNode> &nodes);
