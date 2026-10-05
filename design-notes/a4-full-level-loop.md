# A4 full level loop: land after the gate, medals, replay target

Status: proposal, 2026-10-02. Builds on the existing minimal results screen (`DrawResultsScreen`, `ComputeStars`, `RecordFinish`, session `bestTime`). Nothing here needs new assets, new sounds, or flight-physics changes.

Player feedback: `PLAYER_FEEDBACK.md` is still empty (nothing shipped). This is designed against the niche (chill/arcade low-poly flight, comps *A Short Hike*, *Sky Rogue*) and the release plan, not player data. All numbers below are derived estimates and must be confirmed by timed play before the owner approves them (release-plan "Owner must do" item 5).

## What exists today (checked in code)
- Level: `targetDistance = 1000` m, measured as horizontal (x/z) distance from `startPosition`, so the gate is at z = 1000 on the flight line. `UpdateLevel` sets `completed` at 1000 m and `main.cpp` immediately switches to `Screen::Finished` and freezes the plane. The plane never lands after the gate.
- 4 checkpoint rings (`InitLevel`), offsets from start: (+10, +15, z150), (-10, +25, z350), (+8, +12, z550), (-8, +20, z750); pass radius 6 m. Fixed.
- Obstacles: hard at z 300/500/700/850 (x -8, 10, -6, 9; 6 m high, r 3); soft at z 120/220/420/620.
- Stars (`ComputeStars`): 3 = clean and 4/4 rings, 2 = one of the two, 1 = finished.
- Terrain: heightmap spans z -700..700; the flight corridor (|x| < 40) is flat at height 0; beyond z 700 the flat backdrop is also 0. So the ground after the gate (z 1000 and on) is flat and obstacle-free. The last obstacle is at z 850.
- Landing (`flight.cpp`): safe if sink <= 6 m/s, speed <= 30 m/s, pitch between -25 and +5 deg, |roll| <= 15 deg, after > 1.5 s airborne. Safe returns to ground roll; otherwise Hard = crash. Throttle input is an acceleration (`speed += throttle * 16 m/s^2`), so holding S on the ground stops the plane from 30 m/s in about 2 s. Coasting without S is very slow (drag only: 30 -> 5 m/s integrates to about 80 s with dragLinear 0.01 / dragQuad 0.001), so the rollout rule below relies on S/brake.

## 1. Should finishing require landing after the gate?

Recommendation: yes, with a two-stage finish. The gate stops the clock (race part); a safe landing ends the run (skill part). This delivers the roadmap's "finish -> landing -> results" without punishing the chill player twice.

Rule (all of it is a small state addition to `LevelState` plus one condition in `main.cpp`):
1. Crossing 1000 m sets `gateCrossed = true` (the existing `completed` flag can serve; `elapsed` already freezes there and is the gate time). The plane keeps flying; physics and controls stay live; `Screen` stays `Playing`. The clock stops at the gate (already true: `completed` stops `elapsed`).
2. After the gate the HUD shows a centred banner "GATE! Land to finish" and, on the ground ahead, a landing zone marker (flat quad, 40 m wide x 300 m long, from z 1000 to z 1300 centred on x 0; drawn with the checkpoint-style tint; behind the same switch as the gate, not a gfx toggle since it is gameplay-critical).
3. The run ends (`Screen::Finished`, results shown) when after the gate the plane is on the ground (`!plane.airborne`, reached via a safe touchdown) AND `plane.speed <= 8 m/s` (rolled out). See rule 5 for why `landing == Safe` is not tested. No requirement to be inside the zone; the marker is a hint, the terrain there is flat anyway. Simple and no geometry test needed.
4. A hard landing or hard obstacle after the gate goes to the normal Crashed screen (same as before the gate). The crash screen additionally shows "Gate reached in X.XX s" so the time is not lost emotionally. Do not invent a separate partial-credit state in v1.
5. Grounded gate crossing (taxiing through at speed without lifting off): `HasLandedSafely` needs `landing == Safe`, which is never set without an air touchdown, so rule 3 alone would never end such a run. Explicit fix: the finish condition is `gateCrossed && !plane.airborne && plane.speed <= 8` (grounded and rolled out), with no `landing == Safe` requirement. A grounded crossing therefore ends the run as soon as the player brakes, which is accepted as a harmless shortcut. Rule 3 is read with this condition.
6. Completion keeps using the existing radial distance from the start point (`distanceFlown >= targetDistance`), not a test of crossing the gate line/frame. A plane that arcs wide or crosses off-axis at x != 0 still triggers it, and this is accepted: the corridor is flat and the gate is only a visual target.

Why: the full loop needs an ending that uses the existing landing code (not yet play-tested), the zone after the gate is already flat, and rollout via S already works. Cost: small, contained in `level.cpp`/`main.cpp`.

Time budget: gate to landing is typically 8-15 s (cut throttle, S to brake, descend 20 m with pitch <= 0, 6 m/s sink limit, roll level).

Cheap fallback (keeps today's behavior): a `constexpr bool kRequireLandingAfterGate` in `level.cpp`. When false, `UpdateLevel`/`main.cpp` behave exactly as now (gate -> `Finished` immediately, freeze). Ship with it true only if the owner's feel test says landing is fun; otherwise flip to false with no other change. Medals and stars below work identically in both modes because they use `elapsed`, which is frozen at the gate either way.

## 2. Star and medal thresholds (from code constants)

Derivation of par (biplane: maxSpeed 60 m/s, accel 16 m/s^2, liftoffSpeed 18, liftoffPitch 5 deg, controlSpeed 25):
- Ground roll to 18 m/s: about 1.2 s and 11 m.
- Reaching ~60 m/s with full throttle: about 4.5 s and about 150 m (net accel falls from 16 to about 12 m/s^2 near top speed because of drag).
- Remaining ~850 m at 60 m/s: about 14.2 s.
- Ideal straight run: about 19 s. Climbing to ring heights (12-25 m) and weaving 8-10 m laterally for the rings costs speed through `climbDecel` and a few extra metres.

Stars: keep `ComputeStars` as-is (3 = clean + 4/4 rings, 2 = one, 1 = finish). It is already a simple "1 for finishing, +1 all rings, +1 clean" sum; the change is presentation only: show the three goals as a checklist so the player knows what to retry:
- [x] Finished (a grounded gate crossing also counts)
- [ ] All rings (3/4 now; needs 4/4)
- [x] Clean (no damage)

Time medal, separate from stars (gate time `elapsed`, requires a finish; shown beside the stars):

| Medal | Gate time | Meaning |
|---|---|---|
| Gold | <= 24.0 s | clean full-throttle run, rings taken |
| Silver | <= 30.0 s | competent run |
| Bronze | <= 40.0 s | finishing at about 25 m/s average, casual |

Put them in `constexpr float kMedalGold/Silver/Bronze` at the top of `level.cpp`. Rationale: gold sits about 5 s above the ~19 s ideal (leaves room for the ring weaving and a human's imperfect takeoff); bronze is generous so nearly every finisher gets a medal. The numbers are estimates: game-tester (or owner) must fly 5 runs (casual, normal, hot, with and without rings) and the numbers get adjusted so a first-time player lands around silver/bronze and gold needs a clean second or third attempt. Record the measured times in the PR.

## 3. Replay motivation

Everything small, all in memory (release plan: no persistence on web v0.1; a desktop file only if trivial):
- Best time (exists). Add the delta vs best on the results screen: "+0.84 s" or "NEW BEST".
- Medal and "next target": "Silver! Gold at 24.0 s" (one line). On gold: "Gold - perfect run needs 3 stars".
- Three-star checklist as above; the unmet line is drawn dimmed in orange as the nudge.
- Do not add a ghost, leaderboard or ring-streak in A4 (Track C material).
- Persisting best time (desktop file; web localStorage via a tiny Emscripten JS call; iOS file) is explicitly post-launch; keep the `bestTime` field the single place so it can be added later.


## 4. Touch and web considerations
- Brake: the post-gate landing needs the throttle `-` touch button. It already exists; add a hint line during the post-gate phase ("Hold - to brake after landing") when `inputState.touchUsed`.
- Results hint currently reads "R: Restart  M: Menu"; on touch (`touchUsed`) hide that keyboard text, since the 260x80 touch Restart/Menu buttons from `DrawTouchOverlay` (shown when `Finished`) are the control.
- Known bug to fix in this loop: `RestartRect()`/`MenuRect()` in `src/input.cpp` are centred on screen and hit-tested every frame in `ReadFlightInput` even when not drawn. `main.cpp` accepts `input.restart` when `HasLandedSafely(plane)`, so a centre tap after landing silently restarts the run (and could skip the results). Checklist item 3 gates those hit tests on crashed or Finished. Caveat: a finger already resting in the centre rect when the screen appears counts as a fresh press, so ignore touches that began before the results screen appeared. The flag passed to `ReadFlightInput` lags one frame, which is harmless.
- Landing precision on a stick-only phone is the risk: the landing window (pitch -25..+5, roll <= 15, sink <= 6) was never play-tested. If touch testers fail it often, loosen via `PlaneParams` (e.g. `landingMaxRoll` 15 -> 25, `landingMaxSinkRate` 6 -> 8) rather than adding assist code. This is A3's reconcile item; do it with data from phone tests.
- No wait/animation state is added, so no web frame-pacing concerns. Web: nothing new to load; the landing zone marker is one quad.
- Do not lengthen the loop: target a total run (menu to results) of about 60-75 s (takeoff ~5 s, flight ~25 s, landing ~12 s, results) so a phone user replays easily.

## 5. Implementation checklist (game-developer, ordered, each one run)

Self-verification by game-developer is limited to: desktop and web build succeed, the game launches and reaches the menu without log errors, forced-value checks that need no piloting (e.g. a temporary local time offset for medal boundaries, reverted), and diff reading. Flight-based acceptance lines below (gate, landing, crash after gate) are confirmed by the owner flying; mark them "owner to confirm" in the PR rather than claiming them verified.

1. Gate does not end the run (the `kRequireLandingAfterGate` switch, default true).
   - Files: `src/level.h` (only if `completed` is not reused: `gateCrossed`), `src/level.cpp` (`UpdateLevel` sets it, `ResetLevelProgress` clears it, banner in `DrawLevelHUD`, landing zone quad in `DrawFinishGate`), `src/main.cpp` (replace `else if (level.completed)` with: finish when `gateCrossed && !plane.airborne && plane.speed <= 8` (rule 5) if the switch is true; when false keep today's immediate finish; hard crash after the gate stays Crashed with a "Gate reached in X.XX s" line in `DrawCrashScreen`).
   - Acceptance: build + launch; take off and fly straight through the gate: no results yet, banner visible, plane controllable, clock frozen at gate time. Landing gently and braking to <= 8 m/s shows results with the gate time (not landing time). A hard landing after the gate shows CRASHED with the gate line. Flipping the constexpr to false reproduces the old immediate results. R and M still work from every screen; web and desktop builds still compile (CI green).
2. Results screen: star checklist, medal, delta vs best, next target.
   - Files: `src/level.cpp` (`DrawResultsScreen`, new `ComputeMedal`), `src/level.h` (declaration, `kMedal*` constants live in the .cpp).
   - Acceptance: forced runs (use a temporary local time offset, then revert) show Bronze/Silver/Gold boundaries exactly at 40.0/30.0/24.0 s; delta line correct on the 2nd run in a session and "NEW BEST" on improvement; unmet star goals are visually distinct; text fits at 1280x720. Known limit: `DrawResultsScreen` lays out relative to `h/2` with the title at `h/2 - 255`, so at 844x390 (phone landscape) the title and stars are off-screen. Either rework the layout to scale from `h` (preferred, small) or record it as a known limit for web v0.1 and fix before the phone test; the owner decides in question 7.
3. Touch polish for the loop.
   - Files: `src/level.cpp` (hide "R: Restart  M: Menu" when touch used, signature takes a `touchUsed` bool), `src/main.cpp` (pass `g.inputState.touchUsed`; post-gate brake hint; pass `crashed || Finished` into `ReadFlightInput`), `src/input.h`/`src/input.cpp` (only hit-test `RestartRect`/`MenuRect` when that flag is set).
   - Acceptance: reading the diff, restart/menu touch hit tests are skipped in `Playing`; with the touch flag forced on the results screen shows no keyboard hint and the buttons do not overlap text; keyboard play unchanged. A real tap test needs a touch device (owner).
4. Timed tuning pass (owner, no code unless numbers move). Agents cannot fly the plane headlessly, so any acceptance that says "fly", "land" or "timed run" here and above needs a human pilot (the owner) on desktop or phone.
   - Acceptance: 5 timed runs by the owner, recorded in the PR body; adjust `kMedal*` if first-attempt time is not between silver and bronze for a normal run; owner approves final numbers.

## 6. Open questions for the owner
1. Is landing after the gate a must for the full loop, or should the web v0.1 ship with the fallback (immediate results at the gate) until landing is play-tested? Recommendation: ship with landing on, but keep the switch.
2. A crash after the gate: always the Crashed screen (proposed, simple), or a "rough landing" result capped at one star that still records the gate time? The second is friendlier but needs one more state.
3. Approve medal times 24 / 30 / 40 s (after the timed pass) and whether medals should replace or sit beside the stars. Proposed: beside.
4. Should "rolled out" be required (speed <= 8 m/s), or end on touchdown? Rollout adds ~2 s and teaches the brake, but may annoy touch players.
5. Is persisting best time on desktop (a file) worth doing in A4, or strictly post-launch as the release plan says?
6. Are the landing limits (sink 6, roll 15, pitch -25..+5) acceptable for a chill game, or should they relax before the phone test?
7. If results at 844x390 are off-screen, rework the layout before web v0.1 or accept it as a known limit until the phone test?
