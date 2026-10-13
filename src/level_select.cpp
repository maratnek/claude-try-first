#include "level_select.h"

namespace {
struct CardSeed {
    const char *id;
    const char *name;
    const char *tagline;
    const char *unlockedBy;
    const char *unlockFlag;
};

constexpr CardSeed kSeeds[] = {
    {"tut_takeoff", "First Wings", "Take off, climb and land", "", ""},
    {"tut_turns", "Slalom Field", "Learn to turn through the rings", "tut_takeoff", "level_tut_turns"},
    {"level1", "Valley Run", "Race the valley to the gate", "", ""},
    {"mountains", "Ridge Pass", "Thread the pass between the peaks", "level1", "level_mountains"},
    {"lake", "Blue Lagoon", "Land on the water with the seaplane", "mountains", "level_lake"},
};
}  // namespace

std::vector<LevelCard> BuildLevelCards() {
    std::vector<LevelCard> cards;
    for (const CardSeed &seed : kSeeds) {
        const LevelDef *def = FindLevel(seed.id);
        LevelCard card;
        card.id = seed.id;
        card.name = def ? def->name : seed.name;
        card.tagline = seed.tagline;
        card.unlockedBy = seed.unlockedBy;
        card.unlockFlag = seed.unlockFlag;
        card.comingSoon = def == nullptr;
        cards.push_back(card);
    }
    return cards;
}

float CardBestTime(const LevelCard &card, const ProgressData &data) {
    for (const LevelRecord &record : data.levels) {
        if (record.id == card.id) return record.bestTime > 0.0f ? record.bestTime : -1.0f;
    }
    return -1.0f;
}

bool IsLevelFinished(const ProgressData &data, const std::string &id) {
    for (const LevelRecord &record : data.levels) {
        if (record.id == id) return record.bestTime > 0.0f;
    }
    return false;
}

bool IsUnlocked(const LevelCard &card, const ProgressData &data) {
    if (card.unlockedBy.empty()) return true;
    return IsLevelFinished(data, card.unlockedBy) || (!card.unlockFlag.empty() && HasUnlock(data, card.unlockFlag));
}

bool IsOpenable(const LevelCard &card, const ProgressData &data) {
    return !card.comingSoon && IsUnlocked(card, data);
}

std::string UnlockConditionText(const LevelCard &card, const std::vector<LevelCard> &cards, const ProgressData &data) {
    if (card.comingSoon) return "Coming soon";
    if (IsUnlocked(card, data)) return "";
    for (const LevelCard &other : cards) {
        if (other.id == card.unlockedBy) return "Finish " + other.name + " to unlock";
    }
    return "";
}

int MoveSelection(int selected, int delta, int count) {
    int next = selected + delta;
    if (next < 0) next = 0;
    if (next > count - 1) next = count - 1;
    return next < 0 ? 0 : next;
}

int DefaultSelection(const std::vector<LevelCard> &cards, const ProgressData &data) {
    int last = 0;
    for (int i = 0; i < (int)cards.size(); i++) {
        if (!IsOpenable(cards[i], data)) continue;
        last = i;
        if (CardBestTime(cards[i], data) < 0.0f) return i;
    }
    return last;
}

bool UnlockNextLevel(ProgressData &data, const std::vector<LevelCard> &cards, const std::string &finishedId) {
    for (size_t i = 0; i + 1 < cards.size(); i++) {
        if (cards[i].id != finishedId) continue;
        const std::string &flag = cards[i + 1].unlockFlag;
        if (flag.empty() || HasUnlock(data, flag)) return false;
        AddUnlock(data, flag);
        return HasUnlock(data, flag);
    }
    return false;
}
