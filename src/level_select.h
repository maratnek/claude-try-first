#pragma once
#include "level_def.h"
#include "progress.h"
#include <string>
#include <vector>

struct LevelCard {
    std::string id;
    std::string name;
    std::string tagline;
    std::string unlockedBy;
    std::string unlockFlag;
    bool comingSoon = false;
};

// Campaign order. Levels without a LevelDef yet are teasers (comingSoon).
std::vector<LevelCard> BuildLevelCards();

bool IsLevelFinished(const ProgressData &data, const std::string &id);
// An empty unlockedBy means the card is open from the start.
bool IsUnlocked(const LevelCard &card, const ProgressData &data);
bool IsOpenable(const LevelCard &card, const ProgressData &data);
// Empty when the card is unlocked and playable.
std::string UnlockConditionText(const LevelCard &card, const std::vector<LevelCard> &cards, const ProgressData &data);

// -1 when the level has no recorded time.
float CardBestTime(const LevelCard &card, const ProgressData &data);

int MoveSelection(int selected, int delta, int count);
int DefaultSelection(const std::vector<LevelCard> &cards, const ProgressData &data);

// Writes the unlock flag of the card after finishedId; true if a flag was newly added.
bool UnlockNextLevel(ProgressData &data, const std::vector<LevelCard> &cards, const std::string &finishedId);

// Index of the card right after finishedId when it is openable, else -1.
int NextOpenableCard(const std::vector<LevelCard> &cards, const ProgressData &data, const std::string &finishedId);
