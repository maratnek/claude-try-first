# Campaign: levels 1-5, stages, teasers, saves, planes

Status: DESIGN for the owner's decision of 2026-10-09 (CLAUDE.md "Campaign"). It replaces the open loop choice in `progression.md`: a linear chain of levels, each unlocked by finishing the previous one, each level made of stages of different types. Hangar/aerobatics ideas from Loop C stay later and are not blocked by this.

Inputs: `PLAYER_FEEDBACK.md` is empty (no players yet), so this is designed against the niche (chill/arcade low-poly flight, comps A Short Hike / Sky Rogue) and the owner's two rules: a new player learns the controls before anything hard, and locked levels are visible and tease. Revisit after the first itch.io feedback; the likeliest thing to change is tutorial length.

## Design rules
- Unlock = finish the previous level (reach its last stage), any stars. Never gate by stars: nobody gets stuck, which suits a chill game and the 60%-positive Web goal. Stars and medals are optional replay value.
- Every stage says in one short line what to do, shown at stage start and kept small on the HUD ("Stage 2/4: Climb. Hold up arrow, reach 60 m"). Hints are text only, no voice, no pause.
- A crash restarts at the start of the current stage in tutorial levels (1-2) and at the level start in 3-5. One flag on the level, not two code paths.
- Hard difficulty comes from layout, never from stricter landing/damage rules (those were just relaxed for the owner).

## Levels

| # | id | Name | Plane | Terrain | Stages |
| --- | --- | --- | --- | --- | --- |
| 1 | `tut_takeoff` | First Wings | biplane | flat | Takeoff, Climb, Approach, Runway landing |
| 2 | `tut_turns` | Slalom Field | biplane | flat, gentle hills | Rings (arrows turn), Rings (A/D turn), Rings (throttle), Gate |
| 3 | `level1` | Valley Run | biplane | the existing 1 km corridor | Takeoff, Rings, Gate, Runway landing (today's level 1, unchanged) |
| 4 | `mountains` | Ridge Pass | biplane | tall ridges, S-shaped pass | Takeoff, Mountain pass, Valley run, Runway landing |
| 5 | `lake` | Blue Lagoon | seaplane | lake with islands and a shore | Water takeoff, Rings over water, Island run, Water landing |

`level1` keeps its id so any saved best time stays valid; it is campaign slot 3, so order is a list in code, not the id.

### Level 1 First Wings (about 90 s)
Start on the runway at the world origin like level 1 does today. Each stage ends on a trigger, then the next hint appears.
1. Takeoff: hint "Hold W for full throttle, then ease the nose up with the up arrow (touch: drag up)". Done when airborne above 5 m AGL.
2. Climb: two large rings (radius 14 m) straight ahead at y = 25 m and y = 50 m, 150 m apart. Hint "Keep the nose up and climb". Rings are not required, the stage ends at 50 m AGL.
3. Approach: a line of 5 flat marker rings (radius 12 m, centred on a 4 degree glide path to the runway threshold, spacing 80 m) so a player can fly the descent by following rings. Hint "Reduce throttle (S) and glide down through the rings".
4. Runway landing: hint "Touch down gently and roll out". The runway is the existing landing zone type (300 x 24 m). Level ends on a safe touchdown plus stopped/rolling slow.
The level is one straight line: the runway for the landing lies about 700 m ahead of the start, so the approach is a long straight descent. No turning in level 1 on purpose.

### Level 2 Slalom Field (about 2 min)
1. Arrows turn: 4 rings alternating left/right of the centreline (x = -60, +60, -60, +60 m, z every 120 m, y = 40 m). Hint "Bank with the left/right arrows to turn". Because turn rate comes from bank, this is the natural first lesson.
2. A/D turn: 4 more rings, offsets of 40 m, hint "A and D swing the nose without banking: use them for small corrections". Both give the player the owner-fixed 22 deg/s feel.
3. Throttle: 3 rings with a speed band (see `StageDef.minSpeed/maxSpeed`): a slow ring (counts only below 24 m/s, placed after a low-altitude pass to bleed speed) and a fast ring (above 34 m/s, downhill). Hint "W/S set engine power: slow down for the first ring, speed up for the last". Fails soft: missing a band ring just does not count it; the stage still ends at its end plane (fix for stuck players).
4. Gate: finish gate with the existing medal times, no landing required (`requireLanding = false`) so the tutorial does not repeat level 1's ending.

### Level 3 Valley Run
Exactly today's `Level1Def()` content, wrapped as 4 stages. The approach stage is the tutorial's in level 1, so no extra hints here beyond the stage titles.

### Level 4 Ridge Pass (about 3 min, new terrain)
Terrain `Mountains`: the heightmap raises maxHeight to about 220 m outside a winding pass roughly 90 m wide. The corridor is no longer a straight line: it is an S-curve (x offset = 120 sin(z / 260) in the flat part). The player must turn to stay in the pass; the "go around" is the point.
1. Takeoff on a flat apron.
2. Mountain pass: 8 rings placed along the S-curve at y = ground + 60 m, radius 16 m; hard obstacles are the ridge sides themselves (terrain collision already crashes).
3. Valley run: low ground-hugging rings (y = ground + 18 m, radius 12 m), the pass narrows. Rewards the low-level skill AGL warning teaches.
4. Runway landing on a short strip (250 m) at the far end, slightly uphill to gate carelessness.
Weather uses existing presets: overcast clouds. No new effects needed.

### Level 5 Blue Lagoon (about 3 min, water)
Terrain `Lake`: `waterLevel` set; any terrain below it is replaced by a flat blue plane (dull sparkle only; no waves for the first PR). Start on a dock (a flat grass pad at the shore) with the seaplane.
1. Water takeoff: taxi from the dock onto the lake and lift off (low drag on water, same takeoff speed rules).
2. Rings over water: 6 rings 12 m above the surface, in a gentle arc over the lagoon.
3. Island run: 3 rings between small island hills and 2 soft canopies (existing obstacle type).
4. Water landing: a marked zone 400 x 60 m of water with floating buoys at the corners. A safe touchdown follows the landing limits already in `flight.cpp`; the plane then glides to a stop (water drag).
A wheeled biplane cannot start on water and cannot be picked here; see Planes.

## Stage types as data in LevelDef
The stage system (PR 1) generalises today's fixed "checkpoints, gate, landing" into an ordered list, keeping the current fields as the source for level 3 until moved:

```
enum class StageType { Takeoff, Rings, MountainPass, ValleyRun, Gate, RunwayLanding, WaterLanding };
enum class TerrainKind { Flat, Hills, Mountains, Lake };

struct StageDef {
    StageType type;
    const char *title;           // HUD: "Climb"
    const char *hint;            // one line, shown at stage start
    float endZ;                  // stage completes when plane z passes this, unless the type has its own rule
    bool requiredRings;          // false = rings are optional (stars only)
    float minSpeed, maxSpeed;    // 0 = no band, otherwise a ring counts only inside the band
    std::vector<CheckpointDef> checkpoints;
};
```
LevelDef gets: `id`, `name`, `tagline`, `unlockedBy` (previous level id, empty for level 1), `terrainKind`, `waterLevel` (0 = none), `planeIds` (allowed, first is default), `restartAtStage`, `std::vector<StageDef> stages`, plus a function `const std::vector<const LevelDef *> &CampaignLevels()` and `const LevelDef *FindLevel(const std::string &id)` beside `Level1Def()`.

How each type completes (the only logic the stage system adds, one `UpdateStage` function in `level.cpp`):
- Takeoff: airborne >= 5 m AGL. Climb stage = `Rings` with `requiredRings=false` and endZ/altitude rule in the hint.
- Rings / MountainPass / ValleyRun: all stage rings resolved when z passes `endZ` (each counted via the existing `CrossesRingHole`; counted vs missed feeds stars). They differ only in data (ring heights/radii, terrain), so these three are one code path with different tags used for the HUD title and teaser.
- Gate: z passes `gateDistance` equivalent (move to `endZ`); medal clock stops here. Medal thresholds stay on LevelDef.
- RunwayLanding / WaterLanding: the existing landing-zone check, with the zone surface being ground or water. `landingZoneLength/Width/rolloutSpeed` move into the stage.
Cargo drop and flying under a bridge are future `StageType` values (a ring variant with a dropped-object test, and a long low ring); not designed here.

Rule for the stage-system PR: level 3 (`level1`) built from stages must produce the same gate time and result as today (`LevelDefTests` and the gate-time flight test prove it).

## Level select and the teaser look
A row of five cards (horizontal scroll on touch, left/right + Enter on keyboard), reached from Play in the main menu. Each card shows: the level number and name, a one-line tagline, a thin silhouette strip, best time with a medal pip and 0-3 stars, and the plane icon of its default plane.

- Unlocked: full colour, silhouette strip in the level's real sky/ground palette.
- Locked: whole card darkened to about 30% brightness, silhouette only (a single dark ridge line plotted from the level's terrain profile: flat = a faint horizon line, mountains = jagged peaks, lake = a wave line with small island bumps; computed at startup from `terrainKind`, no art assets), a padlock, and the name still readable ("Ridge Pass"). Below: the plain unlock condition, "Finish Valley Run to unlock". Level 5 also shows the plane silhouette of the seaplane in its card.
- One tagline per card that sells it: L4 "Thread the pass between the peaks", L5 "Land on the water with the seaplane".
- When a level is finished the next card brightens in a short 1 s animation on the results screen ("New level: Ridge Pass" with Next button). Selecting a locked card shows the condition as a shake, no error dialog.
- Levels with no content yet (before PR 5/6 ship) are shown as locked teasers with "Coming soon" instead of the unlock condition and cannot be opened. That ships the "advertise the next levels" effect for the first itch.io build for free.

Reuse `menu.cpp`'s entry list; a level card is a menu entry kind. Results screen gets Next / Retry / Level select.

## What is saved (`src/progress.*`)
No new file format needed; everything fits `ProgressData` as it is:
- Per level `LevelRecord{id, bestTime, stars}`. Medal is not stored: it is derived from `bestTime` against the level's `medalGold/Silver/Bronze` (so retuning medal times never invalidates saves). Tutorial levels 1-2 have no medals, only stars and finished.
- `unlocks` string list: `level:mountains`, `level:lake`, `plane:seaplane`. Unlock is written when the previous level is finished (`AddUnlock`), then `SaveProgress` once, on the results screen only (never mid-flight). A level counts as unlocked if its `unlockedBy` is finished or the flag exists; level 1 is always unlocked. Storing the flag means a later rebalance cannot re-lock a player.
- Level 1's stars are credited from stage rings only if all required stages finished; a crash never writes.
- Selected plane per level: not saved for the first release (default plane is used). When the hangar exists, add one key `plane.<levelid>=<planeid>` as a new unknown-key-safe line; `ParseProgress` already skips unknown keys, so old builds are not harmed.
- Settings (`volume`, `preset`) are already in `ProgressData`.
- Web uses `localStorage` already; if storage is blocked inside the itch.io iframe, play continues from memory and everything unlocked before is simply lost on reload, a known limitation to put in the page text.
- Test-only unlock: the existing `--crash-test` style debug hook gets `--unlock-all` so testers can reach levels 4-5 without playing.

## Planes per level
- Levels 1-4: the biplane (`assets/models/biplane-1920.glb`, `BiplaneParams`). Level 4 is deliberately flyable with the biplane's climb rate: ridges are tall but the pass floor is level, so no plane swap is needed to finish it.
- Level 5: seaplane, a floatplane variant of the biplane with the same rig node names (asset request to file via asset-planner; the world-art track already lists "a seaplane variant for the campaign"). Own flight config file (`seaplane.flight.txt`) per the Aircraft configs task: slightly higher mass and drag, lower top speed, floats instead of wheels. Landing/takeoff surface rule: floats are safe on water only; on ground the seaplane crash-lands (keeps rules simple). The biplane cannot start on water: `planeIds = {"seaplane"}` for level 5.
- Plane choice (hangar): a row under the level card, shown only when `planeIds.size() > 1`. Until a second allowed plane exists for a level, selection is hidden (not a dead control). First real choice: level 4 may allow `{biplane, monoplane}` later if a second wheeled plane model arrives; the design does not depend on it. `plane:seaplane` is unlocked together with level 5 so it also appears in the hangar for later free-flight levels.
- Each `PlaneParams` comes from its data file; the level only names the plane id, the game loads model + params from that id. Plane model creation stays behind `src/objects/plane.cpp`, so a floatplane is a different load path call, no call-site changes.

## Suggested PR order
Each PR is one behaviour change, small, and ends with the game still starting, level 3 identical, CI autoplay green.

1. Stage system as data (no visible change). `StageType`, `StageDef`, `UpdateStage`, level 3 (`level1`) wrapped as stages, HUD stage title line; `LevelDefTests` extended. Includes the `CampaignLevels()` registry with only level 3 in it.
2. Level select with locked teasers and saved unlocks. Level cards, silhouettes from `terrainKind`, unlock flags in `ProgressData`, results Next/Retry. Levels 1, 2, 4, 5 appear as "Coming soon" teasers (no content). Add `--unlock-all`.
3. Tutorial level 1 (First Wings): takeoff/climb/approach/landing stages, hints, restart at stage. Needs a one-time layout check by flying it.
4. Tutorial level 2 (Slalom Field): rings with speed bands, optional rings, the two turn stages.

**Smallest shippable for the first itch.io Web release: PRs 1 to 4.** That is 3 playable levels (two tutorials plus today's Valley Run), saved progress and visible locked teasers for Ridge Pass and Blue Lagoon, using only the biplane and no new terrain or art. It tests the "learn to fly, then want more" loop with real players. PR 5-6 follow in later builds as the owner sees feedback.

5. Mountains (Ridge Pass): `TerrainKind::Mountains` heightmap in `TerrainDef`/`ComputeRawHeight` (the hill formula is still hard-coded; making it data is part of this PR), S-curve pass, ring layout, short uphill strip. Its own PR plus a medal-time tuning PR afterward.
6. Water (Blue Lagoon) in two PRs: (a) water surface + `waterLevel` + water landing zone/drag using the biplane as a stand-in so it can be tested, behind the level id; (b) seaplane model integration (asset request fulfilled) + `seaplane.flight.txt` + hangar line for level 5. (a) can run in parallel with the asset request.
7. Plane choice (hangar row) only when a level allows two planes; depends on aircraft configs and is not needed for the Web release.

## Open questions for the owner
1. Is "reach the last stage" the right finish for unlocking, or should the tutorials demand the landing too? (Proposed: yes, landing, because it is the skill taught.)
2. Seaplane on land: crash-landing is simplest; a wheeled-plus-floats "amphibian" would be friendlier but needs more rules. OK to start simple?
3. May level 1 and 2 hints be shown as English text only for the first Web build? (No localization planned.)
