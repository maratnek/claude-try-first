#include "break_data.h"
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

struct JointRecord {
    std::string child, parent;
    Vector3 position;
    float scale;
    int line;
};

bool ParseFloat(const std::string &s, float &out) {
    char *end = nullptr;
    out = strtof(s.c_str(), &end);
    return end != s.c_str() && *end == '\0';
}

std::string SidecarPath(const char *glbPath) {
    std::string path = glbPath;
    const std::string ext = ".glb";
    if (path.size() >= ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0) {
        path.resize(path.size() - ext.size());
    }
    return path + ".break.txt";
}

int FindGroup(const BreakData &data, const std::string &name) {
    for (size_t i = 0; i < data.groups.size(); i++) {
        if (data.groups[i].name == name) return (int)i;
    }
    return -1;
}

void MeasureGroups(BreakData &data, const Model &model) {
    std::vector<double> sum(data.groups.size() * 3, 0.0);
    std::vector<long> vertexCount(data.groups.size(), 0);
    for (BreakGroup &g : data.groups) {
        g.meshCount = 0;
        g.boundsMin = {FLT_MAX, FLT_MAX, FLT_MAX};
        g.boundsMax = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    }
    for (int i = 0; i < model.meshCount; i++) {
        int gi = data.meshGroup[i];
        BreakGroup &g = data.groups[gi];
        g.meshCount++;
        const Mesh &mesh = model.meshes[i];
        for (int v = 0; v < mesh.vertexCount; v++) {
            const float *p = &mesh.vertices[v * 3];
            for (int k = 0; k < 3; k++) sum[gi * 3 + k] += p[k];
            vertexCount[gi]++;
            g.boundsMin = {fminf(g.boundsMin.x, p[0]), fminf(g.boundsMin.y, p[1]), fminf(g.boundsMin.z, p[2])};
            g.boundsMax = {fmaxf(g.boundsMax.x, p[0]), fmaxf(g.boundsMax.y, p[1]), fmaxf(g.boundsMax.z, p[2])};
        }
    }
    for (size_t gi = 0; gi < data.groups.size(); gi++) {
        BreakGroup &g = data.groups[gi];
        if (vertexCount[gi] == 0) {
            g.boundsMin = g.boundsMax = g.joint;
            g.centreOfMass = g.joint;
            continue;
        }
        g.centreOfMass = {(float)(sum[gi * 3] / vertexCount[gi]), (float)(sum[gi * 3 + 1] / vertexCount[gi]), (float)(sum[gi * 3 + 2] / vertexCount[gi])};
    }
}

}  // namespace

void LoadBreakData(BreakData &data, const char *glbPath, const std::vector<GlbNode> &nodes, const Model &model) {
    data = BreakData();
    std::string path = SidecarPath(glbPath);
    std::ifstream file(path);
    if (!file) {
        TraceLog(LOG_INFO, "BREAK: no sidecar at %s, using the fixed part list", path.c_str());
        return;
    }

    bool failed = false;
    auto fail = [&](int line, const char *fmt, const char *arg) {
        TraceLog(LOG_WARNING, "BREAK: %s:%d: %s %s", path.c_str(), line, fmt, arg);
        failed = true;
    };

    std::vector<std::vector<int>> groupNodes;
    std::vector<JointRecord> joints;
    std::vector<std::string> orderNames;
    std::vector<int> nodeGroup(nodes.size(), -1);

    std::string text;
    int lineNo = 0;
    while (std::getline(file, text)) {
        lineNo++;
        size_t hash = text.find('#');
        if (hash != std::string::npos) text.resize(hash);
        std::istringstream in(text);
        std::vector<std::string> tok;
        for (std::string t; in >> t;) tok.push_back(t);
        if (tok.empty()) continue;

        if (tok[0] == "group") {
            if (tok.size() < 5) { fail(lineNo, "group needs name mass strength and at least one node:", text.c_str()); continue; }
            BreakGroup g;
            g.name = tok[1];
            if (FindGroup(data, g.name) >= 0) { fail(lineNo, "duplicate group", g.name.c_str()); continue; }
            bool isHull = g.name == "hull";
            if (!ParseFloat(tok[2], g.mass) || g.mass <= 0.0f) fail(lineNo, "mass must be a positive number in group", g.name.c_str());
            if (isHull) {
                if (tok[3] != "-") fail(lineNo, "hull strength must be '-' in group", g.name.c_str());
            } else if (!ParseFloat(tok[3], g.strength) || g.strength <= 0.0f) {
                fail(lineNo, "strength must be a positive number in group", g.name.c_str());
            }
            int gi = (int)data.groups.size();
            std::vector<int> members;
            for (size_t t = 4; t < tok.size(); t++) {
                bool found = false;
                for (size_t n = 0; n < nodes.size(); n++) {
                    if (nodes[n].name != tok[t]) continue;
                    found = true;
                    if (nodeGroup[n] >= 0) fail(lineNo, "node listed in two groups:", tok[t].c_str());
                    nodeGroup[n] = gi;
                    members.push_back((int)n);
                }
                if (!found) fail(lineNo, "unknown node", tok[t].c_str());
            }
            if (isHull) data.hull = gi;
            data.groups.push_back(g);
            groupNodes.push_back(members);
        } else if (tok[0] == "joint") {
            JointRecord j;
            j.line = lineNo;
            if (tok.size() != 7 || !ParseFloat(tok[3], j.position.x) || !ParseFloat(tok[4], j.position.y) ||
                !ParseFloat(tok[5], j.position.z) || !ParseFloat(tok[6], j.scale) || j.scale <= 0.0f) {
                fail(lineNo, "joint needs child parent x y z strength_scale:", text.c_str());
                continue;
            }
            j.child = tok[1];
            j.parent = tok[2];
            joints.push_back(j);
        } else if (tok[0] == "order") {
            orderNames.assign(tok.begin() + 1, tok.end());
        } else if (tok[0] == "fuel_tank") {
            if (tok.size() != 5 || !ParseFloat(tok[1], data.fuelTank.x) || !ParseFloat(tok[2], data.fuelTank.y) ||
                !ParseFloat(tok[3], data.fuelTank.z) || !ParseFloat(tok[4], data.fuelRadius) || data.fuelRadius <= 0.0f) {
                fail(lineNo, "fuel_tank needs x y z radius:", text.c_str());
                continue;
            }
            data.hasFuelTank = true;
        } else {
            fail(lineNo, "unknown record", tok[0].c_str());
        }
    }

    if (FindGroup(data, "hull") < 0) fail(0, "no hull group", "");
    if (data.groups.size() > (size_t)kMaxBreakGroups) fail(0, "too many groups", "");
    if (failed) return;

    std::vector<bool> hasJoint(data.groups.size(), false);
    for (const JointRecord &j : joints) {
        int child = FindGroup(data, j.child), parent = FindGroup(data, j.parent);
        if (child < 0) { fail(j.line, "joint for unknown group", j.child.c_str()); continue; }
        if (parent < 0) { fail(j.line, "joint to unknown group", j.parent.c_str()); continue; }
        if (child == data.hull) { fail(j.line, "hull cannot have a joint:", j.child.c_str()); continue; }
        if (hasJoint[child]) { fail(j.line, "second joint for group", j.child.c_str()); continue; }
        hasJoint[child] = true;
        BreakGroup &g = data.groups[child];
        g.parent = parent;
        g.joint = j.position;
        g.jointScale = j.scale;
    }
    for (size_t gi = 0; gi < data.groups.size(); gi++) {
        if ((int)gi != data.hull && !hasJoint[gi]) fail(0, "group has no joint:", data.groups[gi].name.c_str());
    }
    if (failed) return;
    for (size_t gi = 0; gi < data.groups.size(); gi++) {
        int cursor = (int)gi;
        for (size_t steps = 0; cursor != data.hull && steps <= data.groups.size(); steps++) cursor = data.groups[cursor].parent;
        if (cursor != data.hull) fail(0, "joint cycle through group", data.groups[gi].name.c_str());
    }

    std::vector<bool> ordered(data.groups.size(), false);
    for (const std::string &name : orderNames) {
        int gi = FindGroup(data, name);
        if (gi < 0 || gi == data.hull) { fail(0, "order names unknown or hull group", name.c_str()); continue; }
        if (!ordered[gi]) data.order.push_back(gi);
        ordered[gi] = true;
    }
    if (failed) return;
    for (size_t gi = 0; gi < data.groups.size(); gi++) {
        if ((int)gi != data.hull && !ordered[gi]) data.order.push_back((int)gi);
    }

    int triangleMeshes = 0;
    for (const GlbNode &n : nodes) triangleMeshes += n.trianglePrimitives;
    if (triangleMeshes != model.meshCount) {
        TraceLog(LOG_WARNING, "BREAK: node tree does not match the loaded meshes, ignoring %s", path.c_str());
        return;
    }

    data.meshGroup.assign(model.meshCount, data.hull);
    int mesh = 0;
    for (size_t n = 0; n < nodes.size(); n++) {
        int group = data.hull;
        for (int p = (int)n; p >= 0; p = nodes[p].parent) {
            if (nodeGroup[p] >= 0) {
                group = nodeGroup[p];
                break;
            }
        }
        for (int k = 0; k < nodes[n].trianglePrimitives; k++, mesh++) data.meshGroup[mesh] = group;
    }

    MeasureGroups(data, model);
    for (const BreakGroup &g : data.groups) {
        if (g.meshCount == 0) TraceLog(LOG_WARNING, "BREAK: group %s has no meshes", g.name.c_str());
        TraceLog(LOG_INFO, "BREAK: group %-10s %2d meshes, %5.1f kg, strength %5.1f, com (%.2f, %.2f, %.2f), bounds y %.2f..%.2f",
                 g.name.c_str(), g.meshCount, g.mass, g.strength, g.centreOfMass.x, g.centreOfMass.y, g.centreOfMass.z, g.boundsMin.y, g.boundsMax.y);
    }
    data.valid = true;
}
