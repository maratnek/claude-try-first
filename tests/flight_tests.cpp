#include "flight.h"
#include <cmath>
#include <cstdio>

namespace {

constexpr float kDt = 1.0f / 60.0f;
constexpr float kPi = 3.14159265f;

int g_failures = 0;
bool g_verbose = false;

void Report(const char *name, float value, float lo, float hi) {
    bool ok = value >= lo && value <= hi;
    if (!ok) g_failures++;
    if (!ok || g_verbose) printf("%s %-34s = %9.3f  [%.3f, %.3f]\n", ok ? "ok  " : "FAIL", name, value, lo, hi);
}

void Expect(const char *name, bool cond) {
    if (!cond) g_failures++;
    if (!cond || g_verbose) printf("%s %s\n", cond ? "ok  " : "FAIL", name);
}

PlaneState Airborne(float speed, float altitude, float power) {
    PlaneState s;
    s.airborne = true;
    s.airTime = 10.0f;
    s.speed = speed;
    s.position.y = altitude;
    s.enginePower = power;
    return s;
}

struct Liftoff { float time; float distance; };

Liftoff MeasureLiftoff(const PlaneParams &p) {
    PlaneState s;
    s.position.y = p.wheelHeight;
    FlightInput in;
    in.throttle = 1.0f;
    float t = 0.0f;
    while (!s.airborne && t < 60.0f) {
        in.pitch = s.speed >= p.liftoffSpeed ? 1.0f : 0.0f;
        if (s.enginePower >= 1.0f) in.throttle = 0.0f;
        UpdatePlaneControls(s, p, in, kDt, 0.0f, 0.0f);
        t += kDt;
    }
    return {s.airborne ? t : -1.0f, s.position.z};
}

float MeasureGlideRatio(const PlaneParams &p) {
    PlaneState s = Airborne(30.0f, 1000.0f, 0.0f);
    FlightInput in;
    for (int i = 0; i < 60 * 8; i++) UpdatePlaneControls(s, p, in, kDt, 0.0f, 0.0f);
    float z0 = s.position.z, y0 = s.position.y;
    for (int i = 0; i < 60 * 10; i++) UpdatePlaneControls(s, p, in, kDt, 0.0f, 0.0f);
    return (s.position.z - z0) / (y0 - s.position.y);
}

struct Bank { float radius; float altLoss; };

Bank MeasureBank(const PlaneParams &p, float speed, float bankDeg, float seconds) {
    PlaneState s = Airborne(speed, 1000.0f, 0.0f);
    FlightInput in;
    auto step = [&] {
        s.roll = bankDeg;
        s.enginePower = p.levelPower;
        s.speed = speed;
        UpdatePlaneControls(s, p, in, kDt, 0.0f, 0.0f);
    };
    for (int i = 0; i < 60; i++) step();
    float y0 = s.position.y, yaw0 = s.yaw;
    float dist = 0.0f;
    int n = (int)(seconds * 60.0f);
    for (int i = 0; i < n; i++) {
        Vector3 before = s.position;
        step();
        float dx = s.position.x - before.x, dz = s.position.z - before.z;
        dist += sqrtf(dx * dx + dz * dz);
    }
    float turned = (s.yaw - yaw0) * kPi / 180.0f;
    return {dist / fabsf(turned), y0 - s.position.y};
}

void TestLiftoff(const PlaneParams &p) {
    Liftoff l = MeasureLiftoff(p);
    Report("liftoff time s", l.time, 1.7f, 2.5f);
    Report("liftoff distance m", l.distance, 15.0f, 22.0f);
}

void TestGlide(const PlaneParams &p) {
    Report("glide ratio", MeasureGlideRatio(p), 7.6f, 10.4f);
}

void TestBank(const PlaneParams &p) {
    Report("turn radius 45deg @20 m/s", MeasureBank(p, 20.0f, 45.0f, 6.0f).radius, 34.0f, 50.0f);
    Report("turn radius 45deg @35 m/s", MeasureBank(p, 35.0f, 45.0f, 6.0f).radius, 103.0f, 154.0f);
    Report("turn radius 45deg @50 m/s", MeasureBank(p, 50.0f, 45.0f, 6.0f).radius, 210.0f, 315.0f);
    Bank b = MeasureBank(p, 35.0f, 45.0f, 4.0f);
    Report("altitude loss 45deg bank 4s @35", b.altLoss, 4.0f, 6.5f);
}

void TestStandstill(const PlaneParams &p) {
    PlaneState s;
    s.position.y = p.wheelHeight;
    FlightInput in;
    in.pitch = 1.0f;
    in.roll = 1.0f;
    in.yaw = 1.0f;
    for (int i = 0; i < 120; i++) UpdatePlaneControls(s, p, in, kDt, 0.0f, 0.0f);
    Report("standstill yaw change", s.yaw, -0.01f, 0.01f);
    Report("standstill pitch change", s.pitch, -0.01f, 0.01f);
    Report("standstill roll change", s.roll, -0.01f, 0.01f);
    Report("standstill speed", s.speed, 0.0f, 0.0f);
}

void TestStallFall(const PlaneParams &p) {
    PlaneState s = Airborne(p.stallSpeed * 0.5f, 500.0f, 0.0f);
    FlightInput in;
    float y0 = s.position.y;
    for (int i = 0; i < 120; i++) UpdatePlaneControls(s, p, in, kDt, 0.0f, 0.0f);
    Report("stall altitude drop in 2s", y0 - s.position.y, 9.5f, 14.5f);
    Expect("stall still falling (no hover)", s.fallSpeed > 1.8f);
}

void TestLanding(const PlaneParams &p) {
    PlaneState soft = Airborne(22.0f, 8.0f, 0.0f);
    FlightInput in;
    for (int i = 0; i < 60 * 20 && soft.airborne; i++) UpdatePlaneControls(soft, p, in, kDt, 0.0f, 0.0f);
    Expect("gentle descent lands Safe", soft.landing == LandingResult::Safe && !soft.airborne);

    PlaneState hard = Airborne(30.0f, 40.0f, 0.0f);
    hard.pitch = -30.0f;
    for (int i = 0; i < 60 * 20 && hard.landing == LandingResult::None; i++) {
        in.pitch = -1.0f;
        UpdatePlaneControls(hard, p, in, kDt, 0.0f, 0.0f);
    }
    Expect("steep dive impact is Hard", hard.landing == LandingResult::Hard);

    PlaneState rolled = Airborne(22.0f, 8.0f, 0.0f);
    for (int i = 0; i < 60 * 20 && rolled.landing == LandingResult::None; i++) {
        rolled.roll = 40.0f;
        UpdatePlaneControls(rolled, p, in, kDt, 0.0f, 0.0f);
        in.pitch = 0.0f;
    }
    Expect("landing banked 40deg is Hard", rolled.landing == LandingResult::Hard);
}

}  // namespace

int main(int argc, char **argv) {
    g_verbose = argc > 1;
    PlaneParams p = BiplaneParams();
    TestLiftoff(p);
    TestGlide(p);
    TestBank(p);
    TestStandstill(p);
    TestStallFall(p);
    TestLanding(p);
    if (g_failures) printf("%d check(s) failed\n", g_failures);
    else printf("all flight checks passed\n");
    return g_failures ? 1 : 0;
}
