# Release plan: level 1 on Web (itch.io), then iOS

Status as of 2026-10-02. Update when reality changes.

## Actual code state (checked, not assumed)
- Web: Emscripten target exists (CMakeLists.txt, `__EMSCRIPTEN__` main loop, assets preloaded, CI `web` job last reported green with PR #1; later runs passed CI on dev). Never opened in a browser.
- Touch: src/input.cpp has a left-half virtual stick, throttle +/- buttons, restart button (crash only). Never tried on a real device.
- Landing: exists (safe touchdown returns to ground roll, hard landing crashes). Not play-tested. Does not affect level completion.
- Audio: engine only (src/audio.cpp, ~49 lines, procedural).
- Missing: settings/quality presets (A1), airspeed-scaled controls and per-plane params (A2), soft obstacles / crash screen with Restart+Exit (A3), results screen/score (A4), SFX (A5), iOS build, any menu.
- No player feedback yet (PLAYER_FEEDBACK.md is empty), so this plan is designed against the niche (chill/arcade low-poly flight), not player data.
- Assets are tiny (200 KB), so web download size is not a risk.

## Assumptions (flag if wrong)
- Owner has no Apple Developer account yet and no iOS device decision; one phone (ideally iPhone 11-class) is available for tests.
- "Month" runs from now, so iOS must be submitted by about day 21 to leave a review buffer.
- Agents work in short runs (1-2 items each); the owner does the hands-on steps below.
- Web ships first as a free itch.io page and doubles as the playtest channel feeding PLAYER_FEEDBACK.md.

## Minimum scope: Web v0.1 (target about day 10)
Must have: A1 (Low default on web), A2 (controls scaled by airspeed, stall, ground handling), A3 (crash screen with Restart; Exit may be hidden on web), A4 (loop with results screen, time, checkpoints, damaged/clean, stars, best time in memory), A5 (SFX with master volume).
Deliberate deviations from strict Track A order: a day-1 browser smoke test, a minimal tap-to-start screen (not the Track C main menu), and A5 may overlap A4.
Deliberate deviations from strict Track A order: a day-1 browser smoke test, a minimal tap-to-start screen (not the Track C main menu), and A5 may overlap A4.
Must also have: a one-screen title/"tap to start" (needed anyway to unlock web audio on first gesture), and touch HUD hints already present.

Timing assumption: day numbers are rough estimates assuming about 1-2 agent runs per day with one A-item per run; if run frequency is lower, dates slip proportionally.

Timing assumption: day numbers are rough estimates assuming about 1-2 agent runs per day with one A-item per run; if run frequency is lower, dates slip proportionally.

## Minimum scope: iOS v1.0 (submit about day 21)
Web v0.1 plus: iOS build, safe-area and orientation handling (landscape only), app icon and launch screen, touch retuning from device feedback, privacy answers (no data collected), screenshots.

## Cut or defer
- All Track B (terrain colour, weather, smoke, breakup, pilot head). Exception if time remains: B1 blob shadow only, because it helps judge altitude on touch.
- Track C: main menu, level select, plane shop, save/load progression, more levels, plane generator.
- Bladed propeller model (asset request pending), customizable parts.
- Persistence: in-memory best time on web; a local file only on desktop. Web localStorage and iOS persistence are post-launch.
- Exit button on web/iOS (not meaningful there).
- Deeper flight model beyond A2.

## Risks and mitigations
1. iOS toolchain: raylib has no official iOS CMake path in this repo; Conan does not target iOS here. Needs Xcode, raylib built with `-DPLATFORM=iOS`-style settings or a CMake iOS toolchain (`CMAKE_SYSTEM_NAME=iOS`), asset bundling, and a CI job on macos-14. Likely the largest unknown. Mitigation: spike an iOS Simulator build by day 8 (separate from gameplay work); CI builds for simulator only (no signing).
2. Signing and Apple account: Developer Program enrollment can take 1-several days (longer for identity checks). Owner must enrol now. Signing and upload cannot be done by agents or CI without secrets the owner provides.
3. App Store review: typically 1-3 days but a rejection resets the clock. Mitigation: submit by day 21, via TestFlight first (internal testing needs no review) to catch problems; avoid minimum-functionality rejection by having menu-less but complete loop plus results and settings.
4. Touch feel: stick sensitivity, thumb occlusion, multi-touch, no haptics. Only real-device testing finds it. Mitigation: ship web early, test on phone browser at day 10-12, expose sensitivity constants for quick tuning, consider tilt as optional later. Web also needs page scroll/zoom/pull-to-refresh prevention (CSS `touch-action: none`, custom shell instead of minshell.html).
5. Web audio: browsers block audio until a user gesture; procedural audio generated on the main thread may stutter. Mitigation: start audio on the title tap; keep SFX tiny; check Safari/iOS WebAudio separately.
6. Weak-device performance: performance rule requires every non-critical visual to be switchable. Mitigation: A1 first so all later features hook in; Low default on web/mobile; measure FPS on the iPhone 11-class device and a low-end laptop browser; HUD FPS counter behind a debug key. Target 30 fps floor, 60 fps goal.
7. Web build never run in a browser: could fail at load (memory, model loading, GLB parsing path). Mitigation: first browser smoke test is day 1-2, before more features.
8. Flight feel unplaytested (landing and low-flying crash rule from the last run may be too strict for a "chill" game). Mitigation: owner play session after A2 and A3; reconcile as A3 already requires.
9. CI only builds, never runs. Mitigation: owner or agent runs the web build locally via `emrun`/static server each milestone.

## Day-by-day order (Track A strictly in order; days are working days, rough)
- Day 1: Web smoke test in a browser (owner or headless), fix load issues; add tap-to-start screen + touch-action/scroll prevention. Start A1.
- Day 2-3: A1 settings struct, Low/Med/High presets, per-feature toggles, Low default on web. Start iOS toolchain spike in parallel if capacity allows (owner enrols in Apple Developer Program now).
- Day 4-6: A2 per-plane parameter struct, airspeed-scaled authority, ground handling, drag, stall. Owner play check on desktop.
- Day 7-8: A3 crash screen (Restart/Exit), soft obstacles + damaged state, reconcile landing rules (relax the 1 m floor and the pitch window). iOS simulator build in CI.
- Day 9-10: A4 results screen, stars, best time; A5 minimal SFX + master volume (A5 can overlap A4 if separate files). Cut web v0.1 build, owner uploads to itch.io (hidden/draft page first).
- Day 11-13: Collect feedback from phone browser and a few friends into PLAYER_FEEDBACK.md; fix touch feel, perf on Low, critical bugs only.
- Day 14-17: iOS: device build, TestFlight internal, safe-area/orientation, icon, launch screen; retune touch on device.
- Day 18-21: App Store metadata, screenshots, privacy answers; submit. Public itch.io page goes live around the same time.
- Day 22-30: Review buffer and fixes; Track B items only if all above are green.
Slip rule: if A2-A4 run over, cut A5 down to engine + crash + checkpoint chime rather than moving the iOS submission date.

## Owner must do by hand
1. Enrol in Apple Developer Program (US$99/yr) now; note the Team ID. Decide bundle ID (e.g. com.<you>.flightgame) and app name; check the name is free on the App Store.
2. Provide signing: create certificates/provisioning (or give agents an App Store Connect API key stored as a GitHub secret). Agents must not be given the account password.
3. Create the itch.io account and a draft game page: title, cover image (630x500 min), 3-5 screenshots, short description, "HTML5, play in browser" embed, viewport size (e.g. 960x540), enable fullscreen button, set to Mobile-friendly. Upload a zipped web build (index.html at the zip root).
4. Device tests: open the itch.io/web build on a real phone (Safari and Chrome), play for 5 minutes, report stick/throttle feel, audio start, FPS, and any scroll/zoom problems; repeat on a TestFlight build. Include at least one older iPhone if available.
5. Play-test flight feel on desktop after A2 and A3 (does landing/crash strictness feel chill?) and approve the star/score thresholds.
6. Review and approve App Store listing text, screenshots, age rating and privacy questionnaire; press Submit.
7. Promote dev to main for each release; copy feedback from itch.io comments into PLAYER_FEEDBACK.md.
