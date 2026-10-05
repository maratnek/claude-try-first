#include "debris.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {
constexpr float kGravity = 9.81f;
constexpr float kRestHeight = 0.15f;
constexpr float kBounceKeep = 0.4f;
constexpr float kGroundFriction = 0.6f;
constexpr float kSettleSpeed = 1.2f;
constexpr float kKickSpeed = 7.0f;
constexpr float kKickUp = 6.0f;
constexpr float kMaxSpinRate = 540.0f;

constexpr PlanePart kSpawnOrder[] = {
    PART_PROPELLER, PART_WHEEL_RIGHT, PART_WHEEL_LEFT, PART_RUDDER,
    PART_ELEVATOR, PART_AILERON_RIGHT, PART_AILERON_LEFT,
};

constexpr bool SpawnOrderExcludesHead() {
    for (PlanePart part : kSpawnOrder)
        if (part == PART_PILOT_HEAD) return false;
    return true;
}
static_assert(sizeof(kSpawnOrder) / sizeof(kSpawnOrder[0]) == kMaxDebrisPieces,
              "kMaxDebrisPieces must match kSpawnOrder");
static_assert(SpawnOrderExcludesHead(), "pilot head must never be thrown as debris");

float Random(float range) {
    return GetRandomValue(-1000, 1000) / 1000.0f * range;
}

Vector3 PlaneToWorld(Vector3 v, const PlaneState &plane) {
    v = Vector3RotateByAxisAngle(v, {0.0f, 0.0f, 1.0f}, -plane.roll * DEG2RAD);
    v = Vector3RotateByAxisAngle(v, {1.0f, 0.0f, 0.0f}, -plane.pitch * DEG2RAD);
    return Vector3RotateByAxisAngle(v, {0.0f, 1.0f, 0.0f}, plane.yaw * DEG2RAD);
}
}  // namespace

void ClearDebris(DebrisState &debris) {
    debris.count = 0;
}

void SpawnDebris(DebrisState &debris, const PlaneModel &planeModel, const PlaneState &plane, Vector3 planeVelocity, int pieceCount) {
    debris.count = 0;
    if (!planeModel.animated || pieceCount <= 0) return;
    if (pieceCount > kMaxDebrisPieces) pieceCount = kMaxDebrisPieces;

    for (int i = 0; i < pieceCount; i++) {
        PlanePart part = kSpawnOrder[i];
        DebrisPiece &p = debris.pieces[debris.count++];
        p.part = part;
        p.pivot = planeModel.parts[part].pivot;
        p.position = Vector3Add(plane.position, PlaneToWorld(p.pivot, plane));
        p.velocity = Vector3Add(planeVelocity, {Random(kKickSpeed), kKickUp * (0.4f + 0.6f * fabsf(Random(1.0f))), Random(kKickSpeed)});
        p.spinAngle = {0.0f, 0.0f, 0.0f};
        p.spinRate = {Random(kMaxSpinRate), Random(kMaxSpinRate), Random(kMaxSpinRate)};
        p.yaw = plane.yaw;
        p.pitch = plane.pitch;
        p.roll = plane.roll;
        p.settled = false;
    }
}

void UpdateDebris(DebrisState &debris, const WorldState &world, int pieceCount, float dt) {
    if (pieceCount <= 0) {
        ClearDebris(debris);
        return;
    }
    for (int i = 0; i < debris.count; i++) {
        DebrisPiece &p = debris.pieces[i];
        if (p.settled) continue;

        p.velocity.y -= kGravity * dt;
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.spinAngle = Vector3Add(p.spinAngle, Vector3Scale(p.spinRate, dt));

        float floor = GetGroundHeight(world, p.position.x, p.position.z) + kRestHeight;
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

unsigned DebrisDetachedMask(const DebrisState &debris) {
    unsigned mask = 0;
    for (int i = 0; i < debris.count; i++) mask |= 1u << debris.pieces[i].part;
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
        DrawPlanePart(planeModel, p.part);
        rlPopMatrix();
    }
}
