#include "plane.h"
#include "objects/glb_nodes.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>

namespace {

constexpr float kElevatorMaxDeg = 25.0f;
constexpr float kAileronMaxDeg = 25.0f;
constexpr float kRudderMaxDeg = 30.0f;
constexpr float kIdleRpm = 0.2f;
constexpr float kBlurStartRpm = 0.45f;
constexpr float kBlurFullRpm = 0.75f;
// The exported disc material is only ~0.22 alpha, which vanishes against terrain from the chase camera.
constexpr unsigned char kBlurMinAlpha = 140;
// The rigged export has only a hub and a blur disc (radius 0.87), no blades, so blades are drawn procedurally.
constexpr Vector3 kBladeSize = {0.13f, 1.72f, 0.04f};
constexpr Vector3 kBladeOffset = {0.0f, 0.0f, 0.02f};
constexpr Color kBladeColor = {96, 60, 34, 255};
constexpr float kPropMaxDegPerSec = 2400.0f;
constexpr float kWheelRadius = 0.44f;
constexpr float kMinHingeAngleRad = 1.0f * DEG2RAD;

struct PartSpec {
    PlanePart part;
    const char *nodeName;
    bool spins;
};

const PartSpec kPartSpecs[] = {
    {PART_PROPELLER, "propeller", true},
    {PART_RUDDER, "rudder_pivot", false},
    {PART_ELEVATOR, "elevator_pivot", false},
    {PART_AILERON_RIGHT, "aileron_right_pivot", false},
    {PART_AILERON_LEFT, "aileron_left_pivot", false},
    {PART_WHEEL_RIGHT, "wheel_right", true},
    {PART_WHEEL_LEFT, "wheel_left", true},
};

Mat4 Mul(const Mat4 &a, const Mat4 &b) {
    Mat4 r;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) sum += a.m[row + 4 * k] * b.m[k + 4 * col];
            r.m[row + 4 * col] = sum;
        }
    }
    return r;
}

Mat4 Translation(float x, float y, float z) {
    Mat4 r;
    r.m[12] = x;
    r.m[13] = y;
    r.m[14] = z;
    return r;
}

Mat4 AxisRotation(Vector3 axis, float angle) {
    float c = cosf(angle), s = sinf(angle), t = 1.0f - c;
    Mat4 r;
    r.m[0] = t * axis.x * axis.x + c;
    r.m[1] = t * axis.x * axis.y + s * axis.z;
    r.m[2] = t * axis.x * axis.z - s * axis.y;
    r.m[4] = t * axis.x * axis.y - s * axis.z;
    r.m[5] = t * axis.y * axis.y + c;
    r.m[6] = t * axis.y * axis.z + s * axis.x;
    r.m[8] = t * axis.x * axis.z + s * axis.y;
    r.m[9] = t * axis.y * axis.z - s * axis.x;
    r.m[10] = t * axis.z * axis.z + c;
    return r;
}

Matrix ToRaylib(const Mat4 &a) {
    const float *m = a.m;
    return (Matrix){m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]};
}

Mat4 FromRaylib(const Matrix &a) {
    Mat4 r;
    float v[16] = {a.m0, a.m1, a.m2, a.m3, a.m4, a.m5, a.m6, a.m7, a.m8, a.m9, a.m10, a.m11, a.m12, a.m13, a.m14, a.m15};
    for (int i = 0; i < 16; i++) r.m[i] = v[i];
    return r;
}

Mat4 WorldMatrix(const std::vector<GlbNode> &nodes, int index) {
    Mat4 world = nodes[index].local;
    for (int p = nodes[index].parent; p >= 0; p = nodes[p].parent) {
        world = Mul(nodes[p].local, world);
    }
    return world;
}

Mat4 ParentWorld(const std::vector<GlbNode> &nodes, int index) {
    return nodes[index].parent >= 0 ? WorldMatrix(nodes, nodes[index].parent) : Mat4();
}

// The export is a posed snapshot: each hinge's rotation is a deflection away from neutral, so only its axis is kept
// and the neutral pose is the same node with identity rotation (surfaces are modelled hanging aft of the pivot).
bool ExtractHingeAxis(const Mat4 &r, Vector3 &axis, float &angleRad) {
    const float *m = r.m;
    Vector3 v = {m[6] - m[9], m[8] - m[2], m[1] - m[4]};
    float len = Vector3Length(v);
    float cosine = (m[0] + m[5] + m[10] - 1.0f) * 0.5f;
    angleRad = atan2f(len * 0.5f, cosine);
    if (angleRad < kMinHingeAngleRad || len < 1e-6f) return false;
    axis = Vector3Scale(v, 1.0f / len);
    float ax = fabsf(axis.x), ay = fabsf(axis.y), az = fabsf(axis.z);
    float dominant = (ax >= ay && ax >= az) ? axis.x : (ay >= az ? axis.y : axis.z);
    if (dominant < 0.0f) axis = Vector3Negate(axis);
    return true;
}

bool BuildRig(PlaneModel &planeModel, const std::vector<GlbNode> &nodes) {
    int triangleMeshes = 0;
    for (const GlbNode &n : nodes) triangleMeshes += n.trianglePrimitives;
    // raylib creates one mesh per triangle primitive, in node order; anything else means the mapping below would be wrong.
    if (triangleMeshes != planeModel.model.meshCount) return false;

    int partNode[PART_COUNT];
    for (const PartSpec &spec : kPartSpecs) {
        partNode[spec.part] = -1;
        for (size_t i = 0; i < nodes.size(); i++) {
            if (nodes[i].name == spec.nodeName) partNode[spec.part] = (int)i;
        }
        if (partNode[spec.part] < 0) return false;

        PlanePartRig &rig = planeModel.parts[spec.part];
        int idx = partNode[spec.part];
        rig.spins = spec.spins;
        rig.restLocal = nodes[idx].local;
        rig.parentWorld = ParentWorld(nodes, idx);
        rig.invRestWorld = FromRaylib(MatrixInvert(ToRaylib(WorldMatrix(nodes, idx))));
        if (!spec.spins) {
            float angle;
            if (!ExtractHingeAxis(nodes[idx].local, rig.axis, angle)) return false;
            TraceLog(LOG_INFO, "PLANE: %s hinge axis (%.2f, %.2f, %.2f), exported deflection %.1f deg", spec.nodeName, rig.axis.x, rig.axis.y, rig.axis.z, angle * RAD2DEG);
        }
        rig.found = true;
    }

    planeModel.meshPart.assign(planeModel.model.meshCount, -1);
    planeModel.meshIsBlur.assign(planeModel.model.meshCount, false);
    int mesh = 0;
    for (size_t i = 0; i < nodes.size(); i++) {
        int part = -1;
        for (int p = (int)i; p >= 0 && part < 0; p = nodes[p].parent) {
            for (int k = 0; k < PART_COUNT; k++) {
                if (partNode[k] == p) part = k;
            }
        }
        bool blur = nodes[i].name == "prop_blur_disc";
        for (int k = 0; k < nodes[i].trianglePrimitives; k++, mesh++) {
            planeModel.meshPart[mesh] = part;
            planeModel.meshIsBlur[mesh] = blur;
        }
    }
    return true;
}

float Approach(float current, float target, float rate, float dt) {
    return current + (target - current) * (1.0f - expf(-rate * dt));
}

Mat4 AnimatedWorld(const PlanePartRig &rig, float angleDeg) {
    Mat4 animatedLocal;
    float angle = angleDeg * DEG2RAD;
    if (rig.spins) {
        animatedLocal = Mul(rig.restLocal, AxisRotation({0.0f, 0.0f, 1.0f}, angle));
    } else {
        Mat4 neutral = Translation(rig.restLocal.m[12], rig.restLocal.m[13], rig.restLocal.m[14]);
        animatedLocal = Mul(neutral, AxisRotation(rig.axis, angle));
    }
    return Mul(rig.parentWorld, animatedLocal);
}

Matrix PartTransform(const PlanePartRig &rig, float angleDeg) {
    // Vertices are already baked into rest-pose world space, so undo that pose before applying the animated one.
    return ToRaylib(Mul(AnimatedWorld(rig, angleDeg), rig.invRestWorld));
}

float SmoothStep(float edge0, float edge1, float x) {
    float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void DrawPropellerBlades(const PlanePartRig &rig, float angleDeg, float alpha) {
    if (alpha <= 0.0f) return;
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(ToRaylib(AnimatedWorld(rig, angleDeg))));
    Color color = kBladeColor;
    color.a = (unsigned char)(alpha * 255.0f);
    DrawCubeV(kBladeOffset, kBladeSize, color);
    rlPopMatrix();
}

}  // namespace

void LoadPlaneModel(PlaneModel &planeModel, const char *path) {
    planeModel.model = LoadModel(path);
    planeModel.loaded = planeModel.model.meshCount > 0;
    if (!planeModel.loaded) return;

    std::vector<GlbNode> nodes;
    planeModel.animated = LoadGlbNodes(path, nodes) && BuildRig(planeModel, nodes);
    if (!planeModel.animated) {
        TraceLog(LOG_WARNING, "PLANE: node rig unavailable, drawing %s statically", path);
    }
}

void UpdatePlaneAnimation(PlaneAnim &anim, const PlaneState &plane, const FlightInput &input, bool crashed, float dt) {
    anim.elevator = Approach(anim.elevator, input.pitch, 12.0f, dt);
    anim.aileron = Approach(anim.aileron, input.roll, 12.0f, dt);
    anim.rudder = Approach(anim.rudder, input.yaw, 12.0f, dt);

    float rpmTarget = kIdleRpm + (1.0f - kIdleRpm) * (plane.speed / kMaxSpeed);
    if (crashed) rpmTarget = 0.0f;
    else if (input.throttle > 0.0f) rpmTarget = fminf(1.0f, rpmTarget + 0.2f);
    anim.rpm = Approach(anim.rpm, rpmTarget, 3.0f, dt);
    anim.propAngle = fmodf(anim.propAngle + anim.rpm * kPropMaxDegPerSec * dt, 360.0f);

    float wheelTarget = (plane.airborne || crashed) ? 0.0f : plane.speed / kWheelRadius * RAD2DEG;
    anim.wheelRate = Approach(anim.wheelRate, wheelTarget, (plane.airborne && !crashed) ? 1.5f : 8.0f, dt);
    anim.wheelAngle = fmodf(anim.wheelAngle + anim.wheelRate * dt, 360.0f);
}

void DrawPlaneObject(const PlaneModel &planeModel, const PlaneAnim &anim, Vector3 position, float yawDegrees, float pitchDegrees, float rollDegrees) {
    if (!planeModel.loaded) return;

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(yawDegrees, 0.0f, 1.0f, 0.0f);
    // Pitch/roll signs are negated here to match PlaneState's flight
    // convention (positive pitch climbs, positive roll banks right) under
    // rlRotatef's right-hand rotation direction.
    rlRotatef(-pitchDegrees, 1.0f, 0.0f, 0.0f);
    rlRotatef(-rollDegrees, 0.0f, 0.0f, 1.0f);

    if (!planeModel.animated) {
        DrawModel(planeModel.model, (Vector3){0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    } else {
        // The tail is at -Z, so a positive rotation about +Y swings the trailing edge toward -X; the rudder is negated
        // so a +yaw (nose toward +X) deflects its trailing edge toward +X like a real rudder.
        float angles[PART_COUNT] = {};
        angles[PART_PROPELLER] = anim.propAngle;
        angles[PART_RUDDER] = -anim.rudder * kRudderMaxDeg;
        angles[PART_ELEVATOR] = anim.elevator * kElevatorMaxDeg;
        angles[PART_AILERON_RIGHT] = anim.aileron * kAileronMaxDeg;
        angles[PART_AILERON_LEFT] = -anim.aileron * kAileronMaxDeg;
        angles[PART_WHEEL_RIGHT] = anim.wheelAngle;
        angles[PART_WHEEL_LEFT] = anim.wheelAngle;

        Matrix transforms[PART_COUNT];
        for (int k = 0; k < PART_COUNT; k++) {
            transforms[k] = PartTransform(planeModel.parts[k], angles[k]);
        }

        // Blades fade out as the blur disc fades in, so the propeller reads as spinning at speed.
        float blurAlpha = SmoothStep(kBlurStartRpm, kBlurFullRpm, anim.rpm);

        // Translucent blades and blur disc go last so they blend over the opaque parts behind them.
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1) DrawPropellerBlades(planeModel.parts[PART_PROPELLER], anim.propAngle, 1.0f - blurAlpha);
            for (int i = 0; i < planeModel.model.meshCount; i++) {
                bool blur = planeModel.meshIsBlur[i];
                if (blur != (pass == 1)) continue;
                if (blur && blurAlpha <= 0.0f) continue;
                int part = planeModel.meshPart[i];
                Matrix transform = part >= 0 ? transforms[part] : MatrixIdentity();
                Material &material = planeModel.model.materials[planeModel.model.meshMaterial[i]];
                Color original = material.maps[MATERIAL_MAP_DIFFUSE].color;
                if (blur) {
                    material.maps[MATERIAL_MAP_DIFFUSE].color.a = (unsigned char)(fmaxf(original.a, kBlurMinAlpha) * blurAlpha);
                    // The disc is a single-sided plane facing forward and raylib ignores glTF doubleSided,
                    // so it would be culled from the chase camera behind the plane.
                    rlDisableBackfaceCulling();
                }
                DrawMesh(planeModel.model.meshes[i], material, transform);
                if (blur) rlEnableBackfaceCulling();
                material.maps[MATERIAL_MAP_DIFFUSE].color = original;
            }
        }
    }

    rlPopMatrix();
}

void UnloadPlaneModel(PlaneModel &planeModel) {
    if (planeModel.loaded) {
        UnloadModel(planeModel.model);
    }
}
