#include "debris.h"
#include "raymath.h"
#include "rlgl.h"
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <string>

namespace {
constexpr float kGravity = 9.81f;
constexpr float kRestHeight = 0.15f;
constexpr float kBounceKeep = 0.4f;
constexpr float kGroundFriction = 0.6f;
constexpr float kSettleSpeed = 1.2f;
constexpr float kKickSpeed = 7.0f;
constexpr float kKickUp = 6.0f;
constexpr float kMaxSpinRate = 540.0f;
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

float Random(float range) {
    return GetRandomValue(-1000, 1000) / 1000.0f * range;
}

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

void InitDebrisMotion(DebrisPiece &p, const PlaneState &plane, Vector3 planeVelocity) {
    p.position = Vector3Add(plane.position, PlaneToWorld(p.pivot, plane));
    p.velocity = Vector3Add(planeVelocity, {Random(kKickSpeed), kKickUp * (0.4f + 0.6f * fabsf(Random(1.0f))), Random(kKickSpeed)});
    p.spinAngle = {0.0f, 0.0f, 0.0f};
    p.spinRate = {Random(kMaxSpinRate), Random(kMaxSpinRate), Random(kMaxSpinRate)};
    p.yaw = plane.yaw;
    p.pitch = plane.pitch;
    p.roll = plane.roll;
    p.settled = false;
}
}  // namespace

void ClearDebris(DebrisState &debris) {
    debris.count = 0;
}

void SpawnDebris(DebrisState &debris, const PlaneModel &planeModel, const PlaneState &plane, const ImpactInfo &impact, int pieceCount) {
    debris.count = 0;
    if (!planeModel.animated) return;
    if (pieceCount < 1) pieceCount = 1;
    const BreakData &breakData = planeModel.breakData;
    Vector3 planeVelocity = impact.velocity;

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
        for (size_t group = 0; group < breakData.groups.size(); group++) {
            if (!detach[group]) continue;
            const BreakGroup &g = breakData.groups[group];
            DebrisPiece &p = debris.pieces[debris.count++];
            p.group = (int)group;
            p.part = PART_COUNT;
            p.pivot = g.centreOfMass;
            // Upright rest pose only; tumbling groups can still hover or sink until debris physics learns bounds.
            p.restHeight = fmaxf(kRestHeight, g.centreOfMass.y - g.boundsMin.y);
            InitDebrisMotion(p, plane, planeVelocity);
        }
        return;
    }

    if (pieceCount > kMaxFixedDebrisParts) pieceCount = kMaxFixedDebrisParts;
    for (int i = 0; i < pieceCount; i++) {
        PlanePart part = kSpawnOrder[i];
        DebrisPiece &p = debris.pieces[debris.count++];
        p.group = -1;
        p.part = part;
        p.pivot = planeModel.parts[part].pivot;
        p.restHeight = kRestHeight;
        InitDebrisMotion(p, plane, planeVelocity);
    }
}

void UpdateDebris(DebrisState &debris, const WorldState &world, float dt) {
    for (int i = 0; i < debris.count; i++) {
        DebrisPiece &p = debris.pieces[i];
        if (p.settled) continue;

        p.velocity.y -= kGravity * dt;
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.spinAngle = Vector3Add(p.spinAngle, Vector3Scale(p.spinRate, dt));

        float floor = GetGroundHeight(world, p.position.x, p.position.z) + p.restHeight;
        if (p.position.y > floor) continue;
        p.position.y = floor;
        if (p.velocity.y < 0.0f) p.velocity.y = -p.velocity.y * kBounceKeep;
        p.velocity.x *= kGroundFriction;
        p.velocity.z *= kGroundFriction;
        p.spinRate = Vector3Scale(p.spinRate, kGroundFriction);
        if (Vector3Length(p.velocity) < kSettleSpeed) {
            p.velocity = {0.0f, 0.0f, 0.0f};
            p.spinRate = {0.0f, 0.0f, 0.0f};
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
        rlRotatef(p.spinAngle.x, 1.0f, 0.0f, 0.0f);
        rlRotatef(p.spinAngle.y, 0.0f, 1.0f, 0.0f);
        rlRotatef(p.spinAngle.z, 0.0f, 0.0f, 1.0f);
        rlRotatef(p.yaw, 0.0f, 1.0f, 0.0f);
        rlRotatef(-p.pitch, 1.0f, 0.0f, 0.0f);
        rlRotatef(-p.roll, 0.0f, 0.0f, 1.0f);
        rlTranslatef(-p.pivot.x, -p.pivot.y, -p.pivot.z);
        if (p.group >= 0) DrawPlaneGroup(planeModel, p.group);
        else DrawPlanePart(planeModel, p.part);
        rlPopMatrix();
    }
}
