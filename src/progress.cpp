#include "progress.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace {

constexpr int kVersion = 1;

bool ValidId(const std::string &id) {
    if (id.empty()) return false;
    for (char c : id) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    }
    return true;
}

}  // namespace

LevelRecord *FindLevelRecord(ProgressData &data, const std::string &id) {
    for (LevelRecord &r : data.levels) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

bool RecordLevelResult(ProgressData &data, const std::string &id, float time, int stars) {
    if (!ValidId(id)) return false;
    LevelRecord *r = FindLevelRecord(data, id);
    if (!r) {
        data.levels.push_back({id, -1.0f, 0});
        r = &data.levels.back();
    }
    if (stars > r->stars) r->stars = stars > 3 ? 3 : stars;
    bool improved = time > 0.0f && (r->bestTime < 0.0f || time < r->bestTime);
    if (improved) r->bestTime = time;
    return improved;
}

bool HasUnlock(const ProgressData &data, const std::string &id) {
    for (const std::string &u : data.unlocks) {
        if (u == id) return true;
    }
    return false;
}

void AddUnlock(ProgressData &data, const std::string &id) {
    if (ValidId(id) && !HasUnlock(data, id)) data.unlocks.push_back(id);
}

std::string SerializeProgress(const ProgressData &data) {
    std::ostringstream out;
    out << "version=" << kVersion << "\n";
    char buf[64];
    snprintf(buf, sizeof buf, "%.3f", data.volume);
    out << "volume=" << buf << "\n";
    out << "preset=" << data.preset << "\n";
    for (const LevelRecord &r : data.levels) {
        snprintf(buf, sizeof buf, "%.3f", r.bestTime);
        out << "level." << r.id << "=" << buf << "," << r.stars << "\n";
    }
    for (const std::string &u : data.unlocks) out << "unlock." << u << "=1\n";
    return out.str();
}

ProgressData ParseProgress(const std::string &text) {
    ProgressData data;
    std::istringstream in(text);
    std::string line;
    bool versioned = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        if (key == "version") {
            versioned = atoi(value.c_str()) == kVersion;
            if (!versioned) return ProgressData{};
        } else if (!versioned) {
            continue;
        } else if (key == "volume") {
            float v = static_cast<float>(atof(value.c_str()));
            data.volume = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        } else if (key == "preset") {
            int p = atoi(value.c_str());
            data.preset = (p >= 0 && p <= 2) ? p : -1;
        } else if (key.rfind("level.", 0) == 0) {
            std::string id = key.substr(6);
            size_t comma = value.find(',');
            if (!ValidId(id) || comma == std::string::npos || FindLevelRecord(data, id)) continue;
            float time = static_cast<float>(atof(value.substr(0, comma).c_str()));
            int stars = atoi(value.substr(comma + 1).c_str());
            data.levels.push_back({id, time > 0.0f ? time : -1.0f, stars < 0 ? 0 : (stars > 3 ? 3 : stars)});
        } else if (key.rfind("unlock.", 0) == 0 && value == "1") {
            AddUnlock(data, key.substr(7));
        }
    }
    return data;
}
