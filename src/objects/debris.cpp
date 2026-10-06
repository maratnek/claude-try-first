#include "debris.h"
#include "raymath.h"
#include "rlgl.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {
constexpr float kGravity = 9.81f;
constexpr float kLinearDrag = 0.1f;
constexpr float kAngularDrag = 0.1f;
constexpr float kRestHeight = 0.05f;
constexpr float kBounce = 0.35f;
constexpr float kBounceMinSpeed = 2.0f;
constexpr float kContactFriction = 0.92f;
constexpr float kContactSpinFriction = 0.93f;
constexpr float kSettleSpeed = 1.2f;
constexpr float kSettleSpin = 2.0f;
constexpr float kSettleBlend = 0.3f;
constexpr float kForceSettleAge = 4.5f;
constexpr float kMaxSpin = 540.0f * DEG2RAD;
constexpr float kMaxSpinStep = 1.6f * kMaxSpin;
constexpr float kMaxDebrisSpeed = 30.0f;
constexpr float kImpulseScale = 0.2f;
constexpr float kMinExcess = 0.3f;
constexpr float kImpulseUp = 0.4f;
constexpr float kInertiaFactor = 0.4f;
constexpr float kPlaneMomentumKept = 0.8f;
constexpr float kFixedPartMass = 5.0f;
constexpr float kFixedKickSpeed = 3.5f;
constexpr float kFixedKickUp = 4.0f;
constexpr float kMaxStep = 1.0f / 60.0f;
constexpr float kNudgeGap = 0.02f;
constexpr Vector3 kBladeHalf = {0.065f, 0.86f, 0.02f};
constexpr unsigned kBreakSeed = 0x5eed1920u;
constexpr float kStrengthJitter = 0.15f;
constexpr float kGroundExposureBonus = 0.3f;
constexpr float kLowestJointBand = 0.25f;

constexpr PlanePart kSpawnOrder[] = {
    PART_PROPELLER, PART_WHEEL_RIGHT, PART_WHEEL_LEFT, PART_RUDDER,
    PART_ELEVATOR, PART_AILERON_RIGHT, PART_AILERON_LEFT,
};

constexpr bool SpawnOrderExcludesHead() {
    for (PlanePart part : kSpawnOrder)
        if (part == PART_PILOT_HEAD) return false;
    return true;
}
static_assert(sizeof(kSpawnOrder) / sizeof(kSpawnOrder[0]) == kMaxFixedDebrisParts,
              "kMaxFixedDebrisParts must match kSpawnOrder");
static_assert(SpawnOrderExcludesHead(), "pilot head must never be thrown as debris");

Vector3 PlaneToWorld(Vector3 v, const PlaneState &plane) {
    v = Vector3RotateByAxisAngle(v, {0.0f, 0.0f, 1.0f}, -plane.roll * DEG2RAD);
    v = Vector3RotateByAxisAngle(v, {1.0f, 0.0f, 0.0f}, -plane.pitch * DEG2RAD);
    return Vector3RotateByAxisAngle(v, {0.0f, 1.0f, 0.0f}, plane.yaw * DEG2RAD);
}

unsigned Hash(unsigned a, unsigned b) {
    a ^= b + 0x9e3779b9u + (a << 6) + (a >> 2);
    a *= 2654435761u;
    a ^= a >> 15;
    a *= 2246822519u;
    return a ^ (a >> 13);
}

float Jitter(unsigned seed, int group) {
    return ((Hash(seed, (unsigned)group) & 0xffffu) / 32767.5f - 1.0f) * kStrengthJitter;
}

unsigned ImpactSeed(const ImpactInfo &impact) {
    unsigned seed = kBreakSeed;
    seed = Hash(seed, (unsigned)(int)roundf(impact.point.x * 10.0f));
    seed = Hash(seed, (unsigned)(int)roundf(impact.point.y * 10.0f));
    seed = Hash(seed, (unsigned)(int)roundf(impact.point.z * 10.0f));
    return Hash(seed, (unsigned)(int)roundf(impact.speed * 10.0f));
}

Vector3 HullCentre(const BreakData &data) {
    const BreakGroup &hull = data.groups[data.hull];
    return Vector3Scale(Vector3Add(hull.boundsMin, hull.boundsMax), 0.5f);
}

int TopFailedAncestor(const BreakData &data, const bool *failed, int group) {
    int top = -1;
    for (int g = group; g != data.hull; g = data.groups[g].parent) {
        if (failed[g]) top = g;
    }
    return top;
}

int SelectDetached(const BreakData &data, const PlaneState &plane, const ImpactInfo &impact, int cap, bool *detach, float *overload) {
    const int n = (int)data.groups.size();
    const float energy = 0.5f * impact.speed * impact.speed;
    Vector3 dir = Vector3Normalize(impact.velocity);
    Vector3 centre = HullCentre(data);
    unsigned seed = ImpactSeed(impact);

    Vector3 offset[kMaxBreakGroups];
    float lowest = FLT_MAX;
    for (int g = 0; g < n; g++) {
        offset[g] = PlaneToWorld(Vector3Subtract(data.groups[g].joint, centre), plane);
        if (g != data.hull && data.groups[g].meshCount > 0) lowest = fminf(lowest, offset[g].y);
    }

    bool failed[kMaxBreakGroups] = {};
    for (int g = 0; g < n; g++) {
        overload[g] = 0.0f;
        detach[g] = false;
        const BreakGroup &grp = data.groups[g];
        if (g == data.hull || grp.meshCount == 0) continue;
        float exposure = Clamp(0.35f + 0.65f * Vector3DotProduct(Vector3Normalize(offset[g]), dir), 0.2f, 1.0f);
        if (impact.ground && offset[g].y <= lowest + kLowestJointBand) exposure += kGroundExposureBonus;
        float strength = grp.strength * grp.jointScale * (1.0f + Jitter(seed, g));
        overload[g] = energy * exposure / strength;
        failed[g] = overload[g] >= 1.0f;
    }

    bool any = false;
    for (int g = 0; g < n; g++) any = any || failed[g];
    if (!any) {
        int best = -1;
        for (int g = 0; g < n; g++) {
            if (g != data.hull && data.groups[g].meshCount > 0 && (best < 0 || overload[g] > overload[best])) best = g;
        }
        if (best >= 0) failed[best] = true;
    }

    bool isRoot[kMaxBreakGroups] = {};
    for (int g = 0; g < n; g++) {
        isRoot[g] = failed[g] && TopFailedAncestor(data, failed, g) == g;
    }

    bool selected[kMaxBreakGroups] = {};
    int selectedCount = 0;
    for (const std::vector<int> &members : data.keep) {
        int best = -1;
        for (int g : members) {
            if (TopFailedAncestor(data, failed, g) >= 0 && (best < 0 || overload[g] > overload[best])) best = g;
        }
        if (best < 0 || selectedCount >= cap) continue;
        int root = TopFailedAncestor(data, failed, best);
        if (!selected[root]) {
            selected[root] = true;
            selectedCount++;
        }
    }
    while (selectedCount < cap) {
        int best = -1;
        for (int g = 0; g < n; g++) {
            if (isRoot[g] && !selected[g] && (best < 0 || overload[g] > overload[best])) best = g;
        }
        if (best < 0) break;
        selected[best] = true;
        selectedCount++;
    }

    int count = 0;
    for (int g = 0; g < n; g++) {
        int top = TopFailedAncestor(data, failed, g);
        detach[g] = top >= 0 && selected[top] && data.groups[g].meshCount > 0;
        if (detach[g]) count++;
    }
    return count;
}


float Signed(unsigned seed, unsigned salt) {
    return (Hash(seed, salt) & 0xffffu) / 32767.5f - 1.0f;
}

Quaternion PlanePose(const PlaneState &plane) {
    Quaternion yaw = QuaternionFromAxisAngle({0.0f, 1.0f, 0.0f}, plane.yaw * DEG2RAD);
    Quaternion pitch = QuaternionFromAxisAngle({1.0f, 0.0f, 0.0f}, -plane.pitch * DEG2RAD);
    Quaternion roll = QuaternionFromAxisAngle({0.0f, 0.0f, 1.0f}, -plane.roll * DEG2RAD);
    return QuaternionMultiply(yaw, QuaternionMultiply(pitch, roll));
}

Vector3 WorldPoint(const DebrisPiece &p, int i) {
    return Vector3Add(p.position, Vector3RotateByQuaternion(p.points[i], p.orientation));
}

// The 26 extreme vertices along the cube directions give a tight convex stand-in for the mesh (a tumbling wing touches
// the ground where its geometry does, not where an upright bounding box says). A voxel-thinned sample of the remaining
// vertices keeps a long edge or strut from hiding between two extremes when the terrain steps under it.
void BuildSupportPoints(DebrisPiece &p, const PlaneModel &planeModel, int group, int part) {
    Vector3 dirs[26];
    int dirCount = 0;
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
            for (int z = -1; z <= 1; z++) {
                if (x == 0 && y == 0 && z == 0) continue;
                dirs[dirCount++] = Vector3Normalize({(float)x, (float)y, (float)z});
            }

    Vector3 best[26];
    float bestDot[26];
    for (int d = 0; d < dirCount; d++) bestDot[d] = -FLT_MAX;
    std::vector<Vector3> vertices;
    bool hasBlades = part == PART_PROPELLER;
    for (int m = 0; m < planeModel.model.meshCount; m++) {
        bool inPiece = group >= 0 ? planeModel.breakData.meshGroup[m] == group : planeModel.meshPart[m] == part;
        if (!inPiece || planeModel.meshIsBlur[m]) continue;
        if (planeModel.meshPart[m] == PART_PROPELLER) hasBlades = true;
        const Mesh &mesh = planeModel.model.meshes[m];
        for (int v = 0; v < mesh.vertexCount; v++) {
            Vector3 point = {mesh.vertices[v * 3], mesh.vertices[v * 3 + 1], mesh.vertices[v * 3 + 2]};
            vertices.push_back(Vector3Subtract(point, p.pivot));
            for (int d = 0; d < dirCount; d++) {
                float dot = Vector3DotProduct(point, dirs[d]);
                if (dot > bestDot[d]) {
                    bestDot[d] = dot;
                    best[d] = point;
                }
            }
        }
    }

    p.pointCount = 0;
    if (!vertices.empty()) {
        for (int d = 0; d < dirCount; d++) p.points[p.pointCount++] = Vector3Subtract(best[d], p.pivot);
        const float cells[] = {0.5f, 0.75f, 1.0f, 1.5f, 2.5f, 5.0f};
        std::vector<std::pair<uint64_t, int>> keyed(vertices.size());
        for (float cell : cells) {
            for (size_t v = 0; v < vertices.size(); v++) {
                auto bucket = [&](float value) { return (uint64_t)(int64_t)(floorf(value / cell) + 4096.0f) & 0x1fffffu; };
                keyed[v] = {bucket(vertices[v].x) << 42 | bucket(vertices[v].y) << 21 | bucket(vertices[v].z), (int)v};
            }
            std::sort(keyed.begin(), keyed.end());
            int unique = 0;
            for (size_t v = 0; v < keyed.size(); v++) unique += v == 0 || keyed[v].first != keyed[v - 1].first;
            if (unique > kMaxSurfacePoints) continue;
            for (size_t v = 0; v < keyed.size(); v++) {
                if (v == 0 || keyed[v].first != keyed[v - 1].first) p.points[p.pointCount++] = vertices[keyed[v].second];
            }
            break;
        }
    }
    if (hasBlades) {
        Vector3 centre = Vector3Subtract(planeModel.parts[PART_PROPELLER].pivot, p.pivot);
        for (int c = 0; c < 8; c++) {
            Vector3 corner = {(c & 1 ? 1.0f : -1.0f) * kBladeHalf.x, (c & 2 ? 1.0f : -1.0f) * kBladeHalf.y, (c & 4 ? 1.0f : -1.0f) * kBladeHalf.z};
            p.points[p.pointCount++] = Vector3Add(centre, corner);
        }
    }
    if (p.pointCount == 0) p.points[p.pointCount++] = {0.0f, 0.0f, 0.0f};

    p.radius = 0.0f;
    for (int i = 0; i < p.pointCount; i++) p.radius = fmaxf(p.radius, Vector3Length(p.points[i]));
}

void ClampSpin(DebrisPiece &p, float limit) {
    float length = Vector3Length(p.angularVel);
    if (length > limit) p.angularVel = Vector3Scale(p.angularVel, limit / length);
}

void InitPiece(DebrisPiece &p, const PlaneModel &planeModel, const PlaneState &plane, int group, int part, Vector3 pivot, float mass) {
    p.group = group;
    p.part = group >= 0 ? PART_COUNT : (PlanePart)part;
    p.pivot = pivot;
    p.mass = mass;
    p.position = Vector3Add(plane.position, PlaneToWorld(pivot, plane));
    p.orientation = PlanePose(plane);
    p.velocity = {0.0f, 0.0f, 0.0f};
    p.angularVel = {0.0f, 0.0f, 0.0f};
    p.age = 0.0f;
    p.settleTime = 0.0f;
    p.settling = false;
    p.settled = false;
    BuildSupportPoints(p, planeModel, group, part);
}

void LaunchPiece(DebrisPiece &p, const PlaneState &plane, Vector3 planeVelocity, Vector3 awayDir, float excessEnergy, Vector3 jointFromCentre, unsigned seed, unsigned salt) {
    awayDir = Vector3Normalize(Vector3Add(awayDir, {0.0f, kImpulseUp, 0.0f}));
    awayDir = Vector3Normalize(Vector3Add(awayDir, {Signed(seed, salt) * 0.25f, 0.0f, Signed(seed, salt + 1) * 0.25f}));
    Vector3 impulse = Vector3Scale(awayDir, kImpulseScale * excessEnergy);
    p.velocity = Vector3Add(Vector3Scale(planeVelocity, kPlaneMomentumKept), Vector3Scale(impulse, 1.0f / p.mass));
    float speed = Vector3Length(p.velocity);
    if (speed > kMaxDebrisSpeed) p.velocity = Vector3Scale(p.velocity, kMaxDebrisSpeed / speed);

    float inertia = p.mass * p.radius * p.radius * kInertiaFactor;
    Vector3 arm = PlaneToWorld(jointFromCentre, plane);
    p.angularVel = Vector3Scale(Vector3CrossProduct(arm, impulse), 1.0f / inertia);
    p.angularVel = Vector3Add(p.angularVel, {Signed(seed, salt + 2) * 3.0f, Signed(seed, salt + 3) * 3.0f, Signed(seed, salt + 4) * 3.0f});
    ClampSpin(p, kMaxSpin);
}

float LowestClearance(const DebrisPiece &p, const WorldState &world) {
    float lowest = FLT_MAX;
    for (int i = 0; i < p.pointCount; i++) {
        Vector3 w = WorldPoint(p, i);
        lowest = fminf(lowest, w.y - GetGroundHeight(world, w.x, w.z));
    }
    return lowest;
}

void RestOnGround(DebrisPiece &p, const WorldState &world) {
    p.position.y += kRestHeight - LowestClearance(p, world);
}

void PieceBounds(const DebrisPiece &p, Vector3 &lo, Vector3 &hi) {
    lo = {FLT_MAX, FLT_MAX, FLT_MAX};
    hi = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (int i = 0; i < p.pointCount; i++) {
        Vector3 w = WorldPoint(p, i);
        lo = Vector3Min(lo, w);
        hi = Vector3Max(hi, w);
    }
}

void NudgeApart(DebrisState &debris, int index, const WorldState &world) {
    DebrisPiece &p = debris.pieces[index];
    Vector3 lo, hi;
    PieceBounds(p, lo, hi);
    for (int j = 0; j < debris.count; j++) {
        if (j == index || !debris.pieces[j].settled) continue;
        Vector3 otherLo, otherHi;
        PieceBounds(debris.pieces[j], otherLo, otherHi);
        float ox = fminf(hi.x, otherHi.x) - fmaxf(lo.x, otherLo.x);
        float oy = fminf(hi.y, otherHi.y) - fmaxf(lo.y, otherLo.y);
        float oz = fminf(hi.z, otherHi.z) - fmaxf(lo.z, otherLo.z);
        if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f) continue;
        if (ox < oz) {
            float dir = (lo.x + hi.x) < (otherLo.x + otherHi.x) ? -1.0f : 1.0f;
            p.position.x += dir * (ox + kNudgeGap);
        } else {
            float dir = (lo.z + hi.z) < (otherLo.z + otherHi.z) ? -1.0f : 1.0f;
            p.position.z += dir * (oz + kNudgeGap);
        }
        RestOnGround(p, world);
        PieceBounds(p, lo, hi);
    }
}

// Chooses the local face that rests lowest (smallest centre-of-mass height) among those already pointing roughly down,
// so a wing lies flat and a wheel does not balance on its rim.
Quaternion RestOrientation(const DebrisPiece &p) {
    const Vector3 axes[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    int bestAxis = -1;
    float bestExtent = FLT_MAX;
    for (int a = 0; a < 6; a++) {
        if (Vector3RotateByQuaternion(axes[a], p.orientation).y > -0.5f) continue;
        float extent = -FLT_MAX;
        for (int i = 0; i < p.pointCount; i++) extent = fmaxf(extent, Vector3DotProduct(p.points[i], axes[a]));
        if (extent < bestExtent) {
            bestExtent = extent;
            bestAxis = a;
        }
    }
    if (bestAxis < 0) return p.orientation;
    Vector3 current = Vector3RotateByQuaternion(axes[bestAxis], p.orientation);
    Quaternion correction = QuaternionFromVector3ToVector3(current, {0.0f, -1.0f, 0.0f});
    return QuaternionNormalize(QuaternionMultiply(correction, p.orientation));
}

void BeginSettle(DebrisPiece &p) {
    p.velocity = {0.0f, 0.0f, 0.0f};
    p.angularVel = {0.0f, 0.0f, 0.0f};
    p.settling = true;
    p.settleTime = 0.0f;
    p.settleFrom = p.orientation;
    p.settleTo = RestOrientation(p);
}

bool StepPiece(DebrisPiece &p, const WorldState &world, float h) {
    p.age += h;
    if (p.settling) {
        p.settleTime += h;
        float t = fminf(p.settleTime / kSettleBlend, 1.0f);
        p.orientation = QuaternionSlerp(p.settleFrom, p.settleTo, t);
        RestOnGround(p, world);
        return t >= 1.0f;
    }

    p.velocity.y -= kGravity * h;
    p.velocity = Vector3Scale(p.velocity, 1.0f - kLinearDrag * h);
    p.angularVel = Vector3Scale(p.angularVel, 1.0f - kAngularDrag * h);
    p.position = Vector3Add(p.position, Vector3Scale(p.velocity, h));
    Quaternion spin = {p.angularVel.x, p.angularVel.y, p.angularVel.z, 0.0f};
    Quaternion delta = QuaternionMultiply(spin, p.orientation);
    p.orientation = QuaternionNormalize({p.orientation.x + 0.5f * h * delta.x, p.orientation.y + 0.5f * h * delta.y,
                                         p.orientation.z + 0.5f * h * delta.z, p.orientation.w + 0.5f * h * delta.w});

    int deepest = -1;
    float penetration = 0.0f;
    for (int i = 0; i < p.pointCount; i++) {
        Vector3 w = WorldPoint(p, i);
        float depth = GetGroundHeight(world, w.x, w.z) - w.y;
        if (depth > penetration) {
            penetration = depth;
            deepest = i;
        }
    }
    if (deepest < 0) return false;

    p.position.y += penetration;
    Vector3 arm = Vector3Subtract(WorldPoint(p, deepest), p.position);
    float contactSpeed = p.velocity.y + Vector3CrossProduct(p.angularVel, arm).y;
    if (contactSpeed < 0.0f) {
        float restitution = contactSpeed < -kBounceMinSpeed ? kBounce : 0.0f;
        float invMass = 1.0f / p.mass;
        float invInertia = 1.0f / (p.mass * p.radius * p.radius * kInertiaFactor);
        Vector3 lever = Vector3CrossProduct(arm, {0.0f, 1.0f, 0.0f});
        float j = -(1.0f + restitution) * contactSpeed / (invMass + invInertia * Vector3DotProduct(lever, lever));
        p.velocity.y += j * invMass;
        p.angularVel = Vector3Add(p.angularVel, Vector3Scale(Vector3CrossProduct(arm, {0.0f, j, 0.0f}), invInertia));
    }
    float steps = h / kMaxStep;
    float friction = powf(kContactFriction, steps);
    p.velocity.x *= friction;
    p.velocity.z *= friction;
    p.angularVel = Vector3Scale(p.angularVel, powf(kContactSpinFriction, steps));
    ClampSpin(p, kMaxSpinStep);

    if ((Vector3Length(p.velocity) < kSettleSpeed && Vector3Length(p.angularVel) < kSettleSpin) || p.age > kForceSettleAge) BeginSettle(p);
    return false;
}
}  // namespace

void ClearDebris(DebrisState &debris) {
    debris.count = 0;
}

float DebrisLowestClearance(const DebrisPiece &piece, const WorldState &world) {
    return LowestClearance(piece, world);
}

void SpawnDebris(DebrisState &debris, const PlaneModel &planeModel, const PlaneState &plane, const ImpactInfo &impact, int pieceCount) {
    debris.count = 0;
    if (!planeModel.animated) return;
    if (pieceCount < 1) pieceCount = 1;
    const BreakData &breakData = planeModel.breakData;
    Vector3 planeVelocity = impact.velocity;
    unsigned seed = ImpactSeed(impact);

    if (breakData.valid) {
        bool detach[kMaxBreakGroups];
        float overload[kMaxBreakGroups];
        SelectDetached(breakData, plane, impact, pieceCount, detach, overload);
        std::string log;
        for (size_t group = 0; group < breakData.groups.size(); group++) {
            if ((int)group == breakData.hull) continue;
            char entry[64];
            snprintf(entry, sizeof(entry), " %s%s=%.2f", detach[group] ? "+" : "", breakData.groups[group].name.c_str(), overload[group]);
            log += entry;
        }
        TraceLog(LOG_INFO, "BREAK: impact %.1f m/s %s, cap %d, overload (+ = detached):%s", impact.speed, impact.ground ? "ground" : "obstacle", pieceCount, log.c_str());
        Vector3 hullCentre = HullCentre(breakData);
        for (size_t group = 0; group < breakData.groups.size(); group++) {
            if (!detach[group]) continue;
            const BreakGroup &g = breakData.groups[group];
            DebrisPiece &p = debris.pieces[debris.count++];
            InitPiece(p, planeModel, plane, (int)group, -1, g.centreOfMass, g.mass);
            float strength = g.strength * g.jointScale;
            float excess = fmaxf(overload[group] - 1.0f, kMinExcess) * strength;
            Vector3 away = PlaneToWorld(Vector3Subtract(g.joint, hullCentre), plane);
            if (Vector3Length(away) < 0.01f) away = {0.0f, 1.0f, 0.0f};
            LaunchPiece(p, plane, planeVelocity, Vector3Normalize(away), excess, Vector3Subtract(g.joint, g.centreOfMass), seed, (unsigned)group * 8u);
        }
        return;
    }

    if (pieceCount > kMaxFixedDebrisParts) pieceCount = kMaxFixedDebrisParts;
    for (int i = 0; i < pieceCount; i++) {
        PlanePart part = kSpawnOrder[i];
        DebrisPiece &p = debris.pieces[debris.count++];
        InitPiece(p, planeModel, plane, -1, part, planeModel.parts[part].pivot, kFixedPartMass);
        unsigned salt = (unsigned)i * 8u;
        Vector3 kick = {Signed(seed, salt) * kFixedKickSpeed, kFixedKickUp * (0.4f + 0.6f * fabsf(Signed(seed, salt + 1))), Signed(seed, salt + 2) * kFixedKickSpeed};
        p.velocity = Vector3Add(Vector3Scale(planeVelocity, kPlaneMomentumKept), kick);
        p.angularVel = {Signed(seed, salt + 3) * kMaxSpin, Signed(seed, salt + 4) * kMaxSpin, Signed(seed, salt + 5) * kMaxSpin};
    }
}

void UpdateDebris(DebrisState &debris, const WorldState &world, float dt) {
    int steps = (int)ceilf(dt / kMaxStep);
    if (steps < 1) return;
    float h = dt / steps;
    for (int i = 0; i < debris.count; i++) {
        DebrisPiece &p = debris.pieces[i];
        for (int s = 0; s < steps && !p.settled; s++) {
            if (!StepPiece(p, world, h)) continue;
            NudgeApart(debris, i, world);
            p.settled = true;
        }
    }
}

unsigned DebrisDetachedParts(const DebrisState &debris) {
    unsigned mask = 0;
    for (int i = 0; i < debris.count; i++) {
        if (debris.pieces[i].group < 0) mask |= 1u << debris.pieces[i].part;
    }
    return mask;
}

unsigned DebrisDetachedGroups(const DebrisState &debris) {
    unsigned mask = 0;
    for (int i = 0; i < debris.count; i++) {
        if (debris.pieces[i].group >= 0) mask |= 1u << debris.pieces[i].group;
    }
    return mask;
}

void DrawDebris(const DebrisState &debris, const PlaneModel &planeModel) {
    for (int i = 0; i < debris.count; i++) {
        const DebrisPiece &p = debris.pieces[i];
        rlPushMatrix();
        rlTranslatef(p.position.x, p.position.y, p.position.z);
        rlMultMatrixf(MatrixToFloat(QuaternionToMatrix(p.orientation)));
        rlTranslatef(-p.pivot.x, -p.pivot.y, -p.pivot.z);
        if (p.group >= 0) DrawPlaneGroup(planeModel, p.group);
        else DrawPlanePart(planeModel, p.part);
        rlPopMatrix();
    }
}
