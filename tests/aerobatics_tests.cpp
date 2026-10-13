#include "aerobatics.h"
#include <cstdio>
#include <vector>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) {
        g_failures++;
        printf("FAIL %s\n", name);
    }
}

constexpr float kDt = 1.0f / 60.0f;

AttitudeSample At(float pitch, float roll, float heading) { return {pitch, roll, heading}; }

// Ramps each axis linearly to the target attitude over `seconds`, starting from `from`.
void Ramp(std::vector<AttitudeSample> &seq, AttitudeSample from, AttitudeSample to, float seconds) {
    const int n = static_cast<int>(seconds / kDt);
    for (int i = 1; i <= n; i++) {
        const float f = static_cast<float>(i) / static_cast<float>(n);
        seq.push_back(At(from.pitchDeg + (to.pitchDeg - from.pitchDeg) * f,
                         from.rollDeg + (to.rollDeg - from.rollDeg) * f,
                         from.headingDeg + (to.headingDeg - from.headingDeg) * f));
    }
}

// Feeds the sequence and returns every trick reported, in order.
std::vector<Trick> Fly(AerobaticsTracker &t, const std::vector<AttitudeSample> &seq) {
    std::vector<Trick> out;
    for (const AttitudeSample &s : seq) {
        UpdateAerobaticsAttitude(t, s, true, kDt);
        const Trick trick = TakeCompletedTrick(t);
        if (trick != Trick::None) out.push_back(trick);
    }
    return out;
}

std::vector<AttitudeSample> Level() { return {At(0, 0, 0), At(0, 0, 0)}; }

}  // namespace

int main() {
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(360, 0, 0), 6.0f);
        seq.push_back(At(360, 0, 0));
        const std::vector<Trick> r = Fly(t, seq);
        Expect("loop reported once", r.size() == 1 && r[0] == Trick::Loop);
        Expect("loop not repeated", TakeCompletedTrick(t) == Trick::None);
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(-360, 0, 0), 6.0f);
        const std::vector<Trick> r = Fly(t, seq);
        Expect("inside loop reported", r.size() == 1 && r[0] == Trick::Loop);
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, 360, 5), 3.0f);
        const std::vector<Trick> r = Fly(t, seq);
        Expect("barrel roll reported once", r.size() == 1 && r[0] == Trick::BarrelRoll);
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, -360, 0), 3.0f);
        const std::vector<Trick> r = Fly(t, seq);
        Expect("left barrel roll reported", r.size() == 1 && r[0] == Trick::BarrelRoll);
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(180, 0, 0), 3.0f);
        Ramp(seq, At(180, 0, 0), At(180, 180, 0), 1.5f);
        Ramp(seq, At(180, 180, 0), At(180, 180, 0), 0.5f);
        const std::vector<Trick> r = Fly(t, seq);
        Expect("immelmann reported once", r.size() == 1 && r[0] == Trick::Immelmann);
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, 40, 90), 6.0f);
        Ramp(seq, At(0, 40, 90), At(0, 0, 90), 2.0f);
        Expect("gentle bank turn is no trick", Fly(t, seq).empty());
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, 90, 0), 1.0f);
        Ramp(seq, At(0, 90, 0), At(0, 0, 0), 1.0f);
        Expect("90 degree roll is no trick", Fly(t, seq).empty());
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(180, 0, 0), 3.0f);
        Ramp(seq, At(180, 0, 0), At(180, 0, 0), 5.0f);
        Expect("half loop alone is no trick", Fly(t, seq).empty());
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, 360, 0), 3.0f);
        Ramp(seq, At(0, 360, 0), At(360, 360, 0), 6.0f);
        const std::vector<Trick> r = Fly(t, seq);
        Expect("roll then loop reports both in order", r.size() == 2 && r[0] == Trick::BarrelRoll && r[1] == Trick::Loop);
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, 200, 0), 2.0f);
        for (const AttitudeSample &s : seq) UpdateAerobaticsAttitude(t, s, true, kDt);
        UpdateAerobaticsAttitude(t, At(0, 200, 0), false, kDt);
        std::vector<AttitudeSample> rest;
        Ramp(rest, At(0, 200, 0), At(0, 360, 0), 2.0f);
        Expect("landing mid roll resets progress", Fly(t, rest).empty());
    }
    {
        AerobaticsTracker t;
        PlaneState plane;
        plane.airborne = true;
        for (int i = 0; i < 600; i++) {
            plane.pitch = 60.0f;
            plane.roll = (i % 2 == 0) ? 75.0f : -75.0f;
            UpdateAerobatics(t, plane, kDt);
        }
        Expect("clamped PlaneState attitude never completes a trick", TakeCompletedTrick(t) == Trick::None);
    }

    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(200, 0, 0), 3.0f);
        Ramp(seq, At(200, 0, 0), At(0, 0, 0), 3.0f);
        Ramp(seq, At(0, 0, 0), At(0, 150, 0), 1.0f);
        Ramp(seq, At(0, 150, 0), At(0, 150, 0), 0.5f);
        Expect("abandoned half loop then roll is no trick", Fly(t, seq).empty());
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(200, 0, 0), 3.0f);
        Ramp(seq, At(200, 0, 0), At(100, 0, 0), 2.0f);
        Ramp(seq, At(100, 0, 0), At(100, 160, 0), 1.0f);
        Expect("half loop undone to 100 degrees then roll is no trick", Fly(t, seq).empty());
    }
    {
        AerobaticsTracker t;
        std::vector<AttitudeSample> seq = Level();
        Ramp(seq, At(0, 0, 0), At(0, 200, 0), 25.0f);
        Ramp(seq, At(0, 200, 0), At(0, 360, 0), 1.0f);
        Expect("roll slower than the 20 s timeout is no trick", Fly(t, seq).empty());
    }

    if (g_failures == 0) printf("AerobaticsTests passed\n");
    return g_failures == 0 ? 0 : 1;
}
