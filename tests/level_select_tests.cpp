#include "level_select.h"
#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void Expect(const char *name, bool cond) {
    if (!cond) {
        g_failures++;
        printf("FAIL %s\n", name);
    }
}

const LevelCard &Card(const std::vector<LevelCard> &cards, const char *id) {
    for (const LevelCard &c : cards) {
        if (c.id == id) return c;
    }
    return cards[0];
}

}  // namespace

int main() {
    std::vector<LevelCard> cards = BuildLevelCards();
    Expect("five cards", cards.size() == 5);
    const char *order[] = {"tut_takeoff", "tut_turns", "level1", "mountains", "lake"};
    bool orderOk = cards.size() == 5;
    for (size_t i = 0; orderOk && i < 5; i++) orderOk = cards[i].id == order[i];
    Expect("campaign order", orderOk);
    Expect("names", cards[0].name == "First Wings" && cards[1].name == "Slalom Field" && cards[2].name == "Valley Run" &&
                        cards[3].name == "Ridge Pass" && cards[4].name == "Blue Lagoon");
    Expect("tutorial and level1 have content", !cards[0].comingSoon && !cards[2].comingSoon && cards[1].comingSoon && cards[3].comingSoon && cards[4].comingSoon);
    bool taglines = true;
    for (const LevelCard &c : cards) taglines = taglines && !c.tagline.empty();
    Expect("every card has a tagline", taglines);

    ProgressData fresh;
    const LevelCard &level1 = Card(cards, "level1");
    const LevelCard &mountains = Card(cards, "mountains");
    const LevelCard &lake = Card(cards, "lake");
    Expect("level1 always unlocked and openable", IsUnlocked(level1, fresh) && IsOpenable(level1, fresh));
    Expect("mountains locked on fresh save", !IsUnlocked(mountains, fresh) && !IsOpenable(mountains, fresh));
    Expect("lake locked on fresh save", !IsUnlocked(lake, fresh));
    Expect("level1 condition empty", UnlockConditionText(level1, cards, fresh).empty());
    Expect("teaser says coming soon", UnlockConditionText(mountains, cards, fresh) == "Coming soon");
    Expect("first card playable from the start", IsOpenable(cards[0], fresh) && UnlockConditionText(cards[0], cards, fresh).empty());

    ProgressData finished;
    RecordLevelResult(finished, "level1", 30.0f, 2);
    Expect("finishing level1 unlocks mountains", IsUnlocked(mountains, finished));
    Expect("coming soon never openable even when unlocked", !IsOpenable(mountains, finished));
    Expect("lake still locked", !IsUnlocked(lake, finished));
    Expect("unfinished record does not count", (RecordLevelResult(fresh, "level1", -1.0f, 0), !IsLevelFinished(fresh, "level1")));

    ProgressData flagged;
    AddUnlock(flagged, "level_mountains");
    Expect("unlock flag alone unlocks", IsUnlocked(mountains, flagged));
    Expect("flag does not unlock others", !IsUnlocked(lake, flagged));

    LevelCard playable = mountains;
    playable.comingSoon = false;
    Expect("future real level locked needs finish text",
           UnlockConditionText(playable, cards, fresh) == "Finish Valley Run to unlock");
    Expect("future real level unlocked after finish", IsOpenable(playable, finished) && UnlockConditionText(playable, cards, finished).empty());

    ProgressData p;
    Expect("finish level1 writes mountains flag", UnlockNextLevel(p, cards, "level1") && HasUnlock(p, "level_mountains"));
    Expect("flag written once", !UnlockNextLevel(p, cards, "level1") && p.unlocks.size() == 1);
    Expect("last card unlocks nothing", !UnlockNextLevel(p, cards, "lake"));
    Expect("unknown id unlocks nothing", !UnlockNextLevel(p, cards, "nope"));
    Expect("level before always-open card writes nothing", !UnlockNextLevel(p, cards, "tut_turns"));
    Expect("flags survive a save round trip", HasUnlock(ParseProgress(SerializeProgress(p)), "level_mountains"));

    Expect("move right", MoveSelection(1, 1, 5) == 2);
    Expect("move left", MoveSelection(3, -1, 5) == 2);
    Expect("clamp at left end", MoveSelection(0, -1, 5) == 0);
    Expect("clamp at right end", MoveSelection(4, 1, 5) == 4);
    Expect("empty list", MoveSelection(0, 1, 0) == 0);

    Expect("default selection is the first unfinished card on a fresh save", DefaultSelection(cards, fresh) == 0);
    ProgressData tutDone;
    RecordLevelResult(tutDone, "tut_takeoff", 40.0f, 1);
    Expect("default selection moves on after the tutorial is finished", DefaultSelection(cards, tutDone) == 2);
    Expect("best time", CardBestTime(level1, finished) > 29.9f && CardBestTime(level1, ProgressData{}) < 0.0f);

    if (g_failures == 0) printf("level select tests passed\n");
    return g_failures == 0 ? 0 : 1;
}
