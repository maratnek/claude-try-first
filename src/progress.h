#pragma once
#include <string>
#include <vector>

struct LevelRecord {
    std::string id;
    float bestTime = -1.0f;
    int stars = 0;
};

struct ProgressData {
    std::vector<LevelRecord> levels;
    std::vector<std::string> unlocks;
    float volume = 1.0f;
    int preset = -1;
};

LevelRecord *FindLevelRecord(ProgressData &data, const std::string &id);
// Keeps the better of the stored and new result; returns true if the time improved.
bool RecordLevelResult(ProgressData &data, const std::string &id, float time, int stars);
bool HasUnlock(const ProgressData &data, const std::string &id);
void AddUnlock(ProgressData &data, const std::string &id);

std::string SerializeProgress(const ProgressData &data);
// Unknown keys and malformed lines are skipped; an empty or unversioned text gives defaults.
ProgressData ParseProgress(const std::string &text);

// Desktop: progress.cfg next to the executable. Web: localStorage. iOS: no-op stub.
ProgressData LoadProgress();
void SaveProgress(const ProgressData &data);
