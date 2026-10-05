# Progression proposal (release blocker 4)

Status: PROPOSAL, AWAITING OWNER CHOICE. Do not build any of it until the owner picks a loop (or a mix).

Inputs: PLAYER_FEEDBACK.md is empty (no players yet), so this is designed against the niche (chill/arcade low-poly flight, comps A Short Hike / Sky Rogue), not player data. Revisit once itch.io feedback exists.

## What exists today (checked in src/)
- One level: 1 km corridor, checkpoint rings, finish gate, landing after the gate (`kRequireLandingAfterGate`).
- Results: `ComputeStars` (1 = finished, 2 = clean OR all rings, 3 = clean AND all rings), `Medal` by gate time (gold <= 24 s, silver <= 30 s, bronze <= 40 s; constants in `level.cpp`), best time kept only for the session (`bestTime`, `newBest`).
- Menu: `menu.cpp` is a list of entries (easy to add more); state switch menu / playing / crashed / finished.
- Persistence: only `graphics.cfg` next to the exe on desktop; `SETTINGS_MOBILE_OR_WEB` compiles it out. Nothing is saved on Web/iOS.
- Already built and reusable as "variety without new models": rain, snow, clouds, terrain colours, scatter density, damage visuals. Flight model is mid-rework (blocker 2), so aerobatics cannot be detected yet.
- Per-vehicle `PlaneParams` exist, so a second plane type is data, not a rewrite.

## Common to all loops
Progress data = one small versioned text file (`progress.cfg`, key=value lines): per level: best time, best stars, medals, plus loop-specific counters. Written when a result is shown (never mid-flight). Missing/corrupt file = fresh start, never a crash.
Where it lives:
- Desktop: next to the exe, same as `graphics.cfg` (or `GetApplicationDirectory()`; later move to the OS user-data dir).
- Web: raylib `SaveFileText` goes to Emscripten's virtual FS and is lost on reload. Needs a `/save` IDBFS mount plus `FS.syncfs` after each write (small Emscripten glue in CMake + one JS call), or `localStorage` via `EM_JS`. Note itch.io iframes can block storage in some browsers; fall back to memory silently.
- iOS: write to the app's Documents/Library directory (Application Support), via SDL's pref path (`SDL_GetPrefPath`) since the project uses SDL on iOS. No iCloud, no account, no data collected (keeps the privacy answers trivial).
This storage work (about 2 runs) is shared by every option, so it can start first once the owner picks anything.

## Loop A: "Star ladder with a teaser map" (lowest risk)
A level-select screen shows 5 levels as a row of cards. Level 1 is open; levels 2-5 are visible but locked, shown as silhouettes with the lock text "Earn 3 stars total" (cumulative stars across all levels: 3, 7, 12, 18). Each level has 3 stars (existing rule) plus a time medal. Unlock animation plays on the results screen ("New level: Canyon Run").
- Level 2+: same engine, new world data: level 2 = canyon with tighter obstacles (still daytime), 3 = coast with clouds and wind streaks, 4 = rain mountains, 5 = snow night/dusk. Each reuses the heightmap, obstacle and checkpoint layout functions with new parameters (length 1.2-2 km).
- Anticipation: locked cards are visible from the first minute; the next unlock always needs only 1-2 more stars.
- Track D: a card is a (mode, level) pair, so a parachute or copter card can sit in the same row later, unlocked by the same star total. Aerobatics: later add an optional "trick" objective per level, worth a bonus star, no separate system.
- Cost: storage 2 runs, level-select menu 2 runs, level data driven by a table (`LevelDef`) 2 runs, levels 2-5 about 1 run each plus tuning (medal times) 1-2 runs. About 11-12 runs for 5 levels; 6-7 runs for the first playable 2-level version.
- Risk: it is the most generic loop, so it may not give the "something is coming" feeling beyond the lock icons.

## Loop B: "Open island" world that unlocks regions (strongest anticipation, biggest cost)
One persistent low-poly island/archipelago in free flight (A Short Hike feel). The 1 km level becomes the "Starter Strip" airfield. Other regions are visible from the air but gated by clear invisible rules shown in-world: a storm wall over the mountains, a closed canyon gate, a distant lighthouse. Unlocks are by skill: stars at the airfield open the canyon gate, a clean landing on the island pad opens the coast, an aerobatic trick (barrel roll over the lake) disperses the storm wall. Missions are short "flight jobs" (fly through rings, land on the pad, deliver a letter) started at airfields.
- Level 2+: each is a mission region of the same world, not a separate level.
- Persisted: set of opened gates/regions, best time per mission, trick flags, collectibles found.
- Track D: each future mode is a new island (parachute cliff, helipad island, a launch pad in the sky for space); aerobatics are the natural gate keys, which fits the owner's "skilled players learn manoeuvres" idea best of the three.
- Cost: large. Needs a bigger terrain (currently a 1 km corridor with a flat horizon backdrop), free-flight world streaming or a fixed larger heightmap, mission framework, waypoint/compass HUD, performance tuning on Low for Web/iPhone. Estimate 20-30 runs, with a risky first 5-6 (large terrain perf on mobile, navigation UI).
- Risk: scope and performance on the iPhone 11 target; free flight with no goals can feel empty if missions are thin. Delays the App Store date the most.

## Loop C: "Hangar and skills" (unlock planes and tricks, light world)
Levels stay a short linear list (levels 2-3 as in the cost line, but all unlocked by finishing the previous one with any stars, so nobody gets stuck). The anticipation comes from a hangar screen: a second and third plane (faster, tighter turns; own `PlaneParams`) stand there as dimmed silhouettes with a requirement ("Gold on level 2"), and a "Flight school" page lists aerobatic tricks (barrel roll, loop, Immelmann, split-S) as skill cards that fill in when the game detects them in free flight or in a school course. Tricks give "pilot rank" points which unlock planes and cosmetic paint, never required for story levels.
- Persisted: levels done, stars, medals, trick flags and best trick scores, owned planes, rank.
- Track D: modes appear as new hangar vehicles (a glider/wingsuit, a parachute, a copter) each with its own levels, which matches "things that fly or fall". Aerobatics: first-class (flight school).
- Cost: plane swap per `PlaneParams` and a hangar screen 3 runs (needs a second plane model via the asset-requests flow, human step), trick detector (needs blocker 2 flight model; integrates roll/pitch/heading over a window, headless sim to verify) 4-5 runs, flight school course (ring paths shaped for each trick) 2-3 runs, levels 2-3 about 4 runs, storage 2 runs. About 15-18 runs.
- Risk: trick detection false positives or negatives on touch controls (a barrel roll with one thumb); requires the new flight model to be settled first; depends on a human-made 3D asset.

## Apple guideline 4.2 (minimum functionality)
Reviewers reject thin "tech demo" apps. Defences, all loops: a real menu, several levels or a world, persistent progress, settings, and no web-wrapper feel (native build via raylib/SDL). Loop A reaches this with 4-5 short levels, but a reviewer may still call it repetitive. Loops B and C add a second axis (world or vehicles/skills) which reads as a complete game. None of the options depends on an Apple account; the owner enrols separately. The Web (itch.io) criterion (60% positive) comes first and needs only Loop A's first slice (level 2 + save) to test the "come back" feel.

## Recommendation (awaiting owner choice)
Build Loop A as the foundation, then graduate to Loop C. Reason: A's storage, `LevelDef` table and level select are needed by C and also by B, so no work is thrown away, and it gets a second level plus saved stars into itch.io testers' hands in about 6-7 runs. Then C's hangar/flight school adds the anticipation (silhouette planes, trick cards) and carries aerobatics and Track D without the cost and mobile-perf risk of B. Take Loop B only if the owner wants the A Short Hike open-world feel as the identity of the game and accepts a later App Store date. This is a suggestion only. The owner picks; nothing is built until then.

## Questions for the owner
1. Is the identity "short arcade levels you replay" (A/C) or "a world to explore" (B)?
2. Are locked levels gated by stars (can get stuck, replay value) or by simply finishing the previous one (friendlier, chill)?
3. Is a second plane model acceptable as a human asset step (needed for C)?
