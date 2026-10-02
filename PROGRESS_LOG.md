# Progress log

One dated entry per scheduled agent run (newest first), so the project
owner can skim what happened without digging through git log. Format:

```
## YYYY-MM-DD HH:MM UTC — <night|day> run
- Did: <what, one line per item>
- Why: <reasoning, one line>
- Verified: <build/run result, or "not verifiable without a human">
- Open: <anything waiting on the human, e.g. a pending asset request>
- PR: <link or branch, if one was opened>
```

## 2026-10-02 day run
- Did: Weekend sprint item 2 (A4-min results screen). New Screen::Finished: reaching 1000 m freezes the run and shows time, checkpoints x/total, CLEAN/DAMAGED, 1-3 stars (3 = clean and all checkpoints, 2 = either, 1 = finished), session-only best time with NEW BEST, and R Restart / M Menu (touch buttons reuse the crash ones). Timer starts when Play is pressed, so takeoff counts. Removed the old in-HUD "LEVEL COMPLETE" text that showed through the overlay and moved the stats block up so it no longer sits under the touch Restart button. ARCHITECTURE.md updated.
- Why: Next unchecked item of the Web v0.1 sprint; players need an end-of-level result.
- Verified: game-tester built a scratch copy (hand-built raylib 5.5, Conan removed) and flew the real 1 km under Xvfb with real keys: Finished screen appears, R restarts, M returns to menu, best time kept across restart and menu, slower run does not overwrite it, crash flow unchanged. code-reviewer read the diff: no blockers; I applied the overlap and duplicate-text nits and rebuilt (compiles). CI status: see PR.
- Open: 3-star case, damaged finish and checkpoint count above 0 never seen. Touch buttons on the results screen untested. Emscripten build only syntax-checked for level.cpp. Propeller still spins on the frozen Finished plane (cosmetic); input.cpp parameter `crashed` now also means finished (rename later). Timer is unclamped on a backgrounded web tab. Next: sprint item 3, A5-min sounds.
- PR: see PR into dev from agents

## 2026-10-02 night run (8)
- Did: Weekend sprint item 1 (Web build runs in a browser). Built the web target with a fresh emsdk (emcc 6.0.10), served it and loaded it in headless Chromium. It loads: menu, biplane model, terrain, HUD and gameplay all render (about 31-38 FPS under software GL). Found one real bug: miniaudio threw `Cannot read properties of undefined (reading 'buffer')` on every audio callback because newer emsdk no longer exports Module.HEAPF32, so the web build would have had no sound. Fix: one link flag, `-sEXPORTED_RUNTIME_METHODS=HEAPF32`, in the Emscripten branch of CMakeLists.txt.
- Why: First unchecked item of the weekend Web v0.1 sprint; the web build had only ever been compiled by CI. CI's setup-emsdk is unpinned (latest), so it would hit the same thing.
- Verified: game-developer found and fixed it; game-tester independently built with and without the flag and ran both in headless Chromium: 240 page errors without, 0 with. code-reviewer read the diff: no blockers (flag is inside the EMSCRIPTEN branch only, no conflicts with other link flags). The desktop build was compiled against hand-built raylib 5.5 and ran 5 s under Xvfb (Conan is blocked in the sandbox). CI status: see PR.
- Open: Sound was never heard (headless, only confirmed the callback no longer throws). No real GPU, touch or phone test. Only about 10 s of play exercised; checkpoints, crash, restart and finish not run in the browser. Safari/iOS Safari untested. Menu uses IsMouseButtonDown polling, so a sub-frame tap can be missed at low FPS (worth checking on a phone). minshell.html pulls FileSaver.js from a CDN (item 4, custom shell, removes it). Next: sprint item 2, A4-min results screen.
- PR: see PR into dev from agents

## 2026-10-02 night run (7)
- Did: Roadmap A3 owner issues + A3b. New src/menu.{h,cpp}: title screen (Play, plus Exit on desktop only) built from a vector of entries, so later entries are one line in InitMenu. main.cpp has a Screen {Menu, Playing, Crashed} switch; the game starts in the menu, Play resets the run, the crash screen offers M (or a touch MENU button) to return to the menu. One-shot procedural crash sound (thump, noise burst, rumble, crunch) plays once on crash. Fixed an existing bug: the engine was not really silenced on crash (UpdateEngineAudio(0) still looped at about 0.35 volume); the engine is now stopped on every non-Playing screen. Menu also accepts a mouse click (small addition beyond the brief).
- Why: Next unfinished Track A items (owner-reported crash sound and post-crash menu).
- Verified: game-developer hand-built raylib 5.5 + all sources, 6 s under Xvfb clean. game-tester rebuilt, injected keys under Xvfb: starts in menu, Enter plays, takeoff, two hard-landing crashes show the crash screen, R restarts, M returns to menu, Q exits, M ignored during play; -D__EMSCRIPTEN__ syntax check passes. code-reviewer found one real bug (touch MENU button overlapped menu entries and a stale held-touch flag could fire Exit/Play on the first menu frame); one fix pass added EnterMenu() which treats a press held across the switch as not yet a tap, and removed a "what" comment. The fix was rebuilt and started by the developer but the touch path was NOT exercised. Conan/CMake, Emscripten and iOS builds NOT run locally; CI status: see PR.
- Open: Crash sound has never been heard (no audio device here); owner should check level and character. Touch/mouse menu taps and the touch MENU button are untested. Crash into a hard obstacle (vs hard landing) not exercised. After a hard landing the HUD still reads AIRBORNE / Plane: OK under the crash overlay (cosmetic). Esc quits on desktop in any state. Window title still says "Step 5: Engine Audio". Next: A4 full level loop.
- PR: see PR into dev from agents

## 2026-10-02 night run (6)
- Did: Roadmap A3 (v1). Soft obstacles (one bush, three treetops) via AddSoftObstacle: a hit sets plane.damaged and flight continues; hard obstacles and hard landings still crash. HUD shows "Plane: OK/DAMAGED"; R restart clears it. Crash now freezes the sim and shows a crash screen (dim overlay, "R: Restart", "Esc / Q: Exit" on desktop only). Engine sound is silenced while crashed. SETTINGS_MOBILE_OR_WEB moved to settings.h so level.cpp/main.cpp share it. Landing thresholds unchanged.
- Why: Next unfinished Track A item after A2. Breakup animation and smoke are Track B.
- Verified: game-developer and game-tester hand-built raylib 5.5 + all sources and ran under Xvfb (clean, only sandbox audio-device warnings). code-reviewer read the diff: no blockers; I applied three nits (engine audio at crash, bush moved from x=3 to x=9 so the takeoff roll cannot clip it, blank line) and rebuilt/ran 6 s clean. Soft/hard hit behaviour, crash screen and Q exit were checked by code reading only, NOT exercised in a run. Conan/CMake, Emscripten and iOS NOT run locally; CI status: see PR.
- Open: Owner should fly into a bush/treetop and a red sphere to check feel and placement. Touch has no Exit button, and the touch Restart button may overlap the "R: Restart" hint. Esc closes the game anywhere on desktop (raylib default). "Damaged" is not in any results text yet (A4 must add it). Damage has no gameplay effect beyond the HUD flag. Crash screen text offsets untested on small screens.
- PR: see PR into dev from agents

## 2026-10-02 night run (5)
- Did: Roadmap A2. New PlaneParams struct + BiplaneParams() in flight.h holds all flight constants (rates, accel, drag, stall speed, liftoff, landing limits, wheel height); kMaxSpeed removed (main.cpp/plane.cpp use params.maxSpeed). Pitch/roll/yaw authority scales with airspeed squared (full at 25 m/s), so a standing plane does not rotate from input. Ground: roll input ignored, rudder steers at taxi speed, pitch settles to the terrain slope (clamped to half the liftoff pitch so slope alone never lifts off). Drag decays speed without throttle (about 2 m/s per second at 40 m/s), climbing costs and diving gains speed. Stall below 14 m/s drops the nose (floored at -20 deg) with reduced authority; landing window widened to 25 deg nose-down so a gentle stalled touchdown is Safe.
- Why: Next unfinished Track A item; later planes need different physics from per-plane params.
- Verified: game-tester hand-built raylib 5.5 + all sources, ran 8 s under Xvfb (no non-audio errors), and ran flight.cpp in a headless sim: standstill no rotation, takeoff at ~1.3 s, slope alone no liftoff, decay, stall recovery, soft stalled touchdown Safe, hard dive Hard. code-reviewer found spec gaps and a stalled-landing regression on the first pass; one fix pass resolved all; re-review found no blockers. Conan/CMake and Emscripten builds NOT run locally; CI status: see PR.
- Open: Flight FEEL is not play-tested (cannot send keyboard input here) - owner should play: takeoff, cruise without holding W (speed now bleeds slowly), stall, landing. Throttle is still a rate input (W/S), so cruise needs occasional W. No ground friction, so an unthrottled plane rolls a long way. Slope sampling in main.cpp uses hard-coded 2 m offsets; kMinAirTimeForLanding still file-local.
- PR: see PR into dev from agents

## 2026-10-02 night run (4)
- Did: Roadmap A1. New src/settings.{h,cpp}: GraphicsSettings with Low/Medium/High presets and four toggles (distance-marker pillars, obstacle wireframes, decorative characters, propeller blur disc), each gating a real draw call. Default is Low on Web/iOS, High on desktop; F1 cycles the preset, the HUD shows it, and desktop persists it to graphics.cfg next to the executable (gitignored). Terrain and gameplay rendering are not gated; physics untouched.
- Why: Next unfinished Track A item; every later visual feature has to check this.
- Verified: game-tester hand-built against raylib 5.5 (Conan blocked in sandbox), ran 8 s under Xvfb: clean, model loaded. code-reviewer: no blockers; I applied two nits (loop-condition toggle, gitignore). F1 key press, graphics.cfg round trip and the Emscripten build were NOT run locally. CI status: see PR.
- Open: Today the toggles save little frame time (terrain is the main cost and is not gated), so Low is mostly cosmetic until Track B features exist. No touch/on-screen way to change the preset on iOS/Web, and the HUD "(F1)" hint is misleading on touch. Per-toggle UI not built. Owner should press F1 on desktop and confirm the look of Low.
- PR: see PR into dev from agents

## 2026-10-02 day run
- Did: Roadmap A0. Wrote design-notes/release-plan.md: minimum scope for Web v0.1 (~day 10) and iOS v1.0 (submit ~day 21), cuts/deferrals, nine risks with mitigations, day-by-day order of A1-A5, and the owner's hand tasks (Apple Developer enrolment, itch.io page, device tests, promoting dev to main). Docs only, no code changed.
- Why: Next unfinished Track A item (A-fix already merged); the plan sets the order for later runs.
- Verified: code-reviewer checked every "actual code state" claim against src/, CMakeLists.txt and CI config: no blockers. Applied its nits (CI wording, explicit note of deviations from strict A order, run-frequency assumption). Docs only, so no build run. Base dev CI was green on the last merge (run 43).
- Open: Owner should confirm the plan's assumptions (no Apple account yet, one iPhone 11-class device, a Mac with Xcode, day-21 target). Day estimates are rough. Next up: A1 graphics settings, plus a browser smoke test of the web build.
- PR: see PR into dev from agents

## 2026-10-02 night run (3)
- Did: Roadmap A-fix. Verified the propeller blur disc by rendering it (RPM forced to 1.0 in a temporary local change, reverted) from the chase camera under Xvfb. It was not culled, but at the asset's ~0.22 alpha it was a barely visible ghost circle. Added kBlurMinAlpha = 140 in plane.cpp so the disc's effective alpha is max(material alpha, 140) * fade; the RPM fade-in is unchanged and the .glb is untouched.
- Why: First Track A item; owner reported seeing no disc at speed.
- Verified: game-developer hand-built raylib 5.5 + sources (not the Conan path), screenshots before/after show the disc clearly visible after the change. code-reviewer read the diff: no blockers (alpha restored after draw, no overflow, <cmath> included). CI status: see PR.
- Open: Needs a human look at actual flight speed (screenshot was at 0 m/s with RPM forced) and whether alpha 140 (~55%) is the right strength, over sky and terrain. Next up: A0 release plan, then A1 graphics settings.
- PR: see PR into dev from agents

## 2026-10-02 night run (2)
- Did: Added landing. Touching the 1 m ground floor after 1.5 s airborne is judged: safe (pitch within the liftoff pitch, speed <= 30 m/s, roll <= 15 deg) returns the plane to ground roll so it can take off again; anything else is a hard landing through the existing crash path. HUD shows the result; keyboard R restarts after a safe landing. Touch restart button still only appears after a crash.
- Why: Landing was the next roadmap item after the mobile work I could do without a toolchain for iOS.
- Verified: game-tester built all sources against raylib 5.5 by hand (not the Conan path) and ran 9 s under Xvfb: clean, model loaded, no warnings. code-reviewer found a real bug (safe-landing window overlapped the liftoff test, causing a bounce loop); one fix pass tied the pitch limit to kLiftoffPitch, re-review found no blockers. Flight feel NOT play-tested. CI status: see PR.
- Open: Behaviour change needing an owner look: the old 1 m soft floor is now a landing check, so flying low (<1 m above ground, >30 m/s, or after 1.5 s of low climb-out) crashes. Landing needs pitch within 5 deg, so flared landings are hard landings. kLandingMaxSinkRate can never trigger (dead given the other limits). Landing does not affect level completion. Touch users get no restart button after a safe landing (they can just take off). iOS build still outstanding.
- PR: see PR into dev from agents

## 2026-10-02 night run
- Did: Mobile polish. HUD shows touch hints (instead of keyboard ones) once a touch has been seen; plane animation state resets on restart; propeller and wheels wind down after a crash; kMaxSpeed now defined once in flight.h (was duplicated in plane.cpp/flight.cpp/main.cpp). Physics unchanged.
- Why: Closes small open items from the previous two runs; keyboard-only HUD text was misleading on mobile.
- Verified: NOT compiled or run locally (sandbox has no raylib; Conan and the raylib 5.5 download both failed). code-reviewer checked the diff by reading: no blockers; its HUD-length nit was fixed by shortening the string. CI result: see PR.
- Open: Human check on a touch device / web build of HUD switch and restart reset. Landing and iOS build remain. Note: PR #3's body ended with a "Generated by Claude Code" footer, which breaks the no-attribution rule; this run's PR avoids it.
- PR: see PR into dev from agents

## 2026-10-01 night run (2)
- Did: Animated the rigged biplane. New src/objects/glb_nodes.{h,cpp} parses the GLB node tree (own small JSON parser, no cgltf, so no duplicate-symbol risk with raylib and identical on Conan/Web builds); plane.cpp maps raylib's per-primitive meshes to nodes and draws them with per-part transforms. Propeller spins with speed/throttle (blur disc only at high RPM), elevator follows pitch, ailerons follow roll (opposite), rudder follows yaw, wheels spin with ground speed. Falls back to the old static DrawModel if the node mapping doesn't match. Physics and rotation convention untouched.
- Why: owner-requested next roadmap item (CLAUDE.md). Hinge axes/neutral pose are derived from the exported matrices, not assumed (exported pivots are posed: rudder 7.4 deg, elevator 11.8, ailerons 20; neutral = identity, so the plane at zero input looks different from the old static pose).
- Verified: game-developer hand-built raylib 5.5 + project sources, ran 9s under Xvfb, screenshots with deflected parts; game-tester independently rebuilt/ran (pass, no static-fallback warning, no regressions by code reading); code-reviewer found no blockers. Reviewer flagged the rudder direction as backwards (developer had reasoned from tail motion); I applied that one fix pass (negated rudder angle, re-syntax-checked) and did not re-render it. Conan/CMake and Emscripten builds NOT run locally; CI status: see PR.
- Open: Human look needed at in-flight visuals and direction signs (game +X appears screen-left, existing behaviour). Pilot head not animated. Not reset on R restart / keeps spinning after crash (cosmetic). kMaxSpeed duplicated in plane.cpp and flight.cpp. Parser has no depth limit / cyclic-children guard (first-party asset only). Roadmap text in CLAUDE.md still lists this item as next-up; owner may want to prune it.
- PR: see PR into dev from agents

## 2026-10-01 night run
- Did: Added touch controls. New src/input.cpp builds a FlightInput from keyboard + touch (left-half virtual stick for pitch/roll, bottom-right +/- throttle buttons, on-screen restart after a crash, overlay shown only once a touch is seen). Flight code now consumes FlightInput instead of reading keys. Fixed a misleading comment about mouse-to-touch mapping.
- Why: Mobile launch is the priority; Web/iOS builds are useless without a non-keyboard way to fly.
- Verified: Built all sources against real raylib 5.5 (desktop, hand-rolled link, not the Conan path), ran 8s under Xvfb: no crash, model loaded (audio warnings are sandbox-only). Keyboard sign mapping diffed against old code: identical. Web path code-reviewed only (no emcc). CI status: see PR.
- Open: Touch is untested on a real device (stick feel/sensitivity, multi-touch, page scroll/zoom prevention on web). Desktop mouse does not register as touch in raylib 5.5, so it can only be tried on web/touch hardware. HUD text still lists only keyboard controls. Landing and iOS build remain.
- PR: see PR into dev from agents

## 2026-10-01 15:15 UTC — day run
- Did: Added an Emscripten (Web) build target: main loop split into UpdateFrame + emscripten_set_main_loop_arg on web, AssetPath helper, CMake FetchContent raylib 5.5 + HTML output with preloaded assets, new `web` CI job. Follow-up commit fixed Conan provider still running on web (EMSCRIPTEN var is undefined before project(); now detected via toolchain file).
- Why: Mobile launch is the priority and Web (itch.io) is the first step; CI had no web target.
- Verified: Sandbox has no emcc and Conan can't install, so nothing was built locally (sources syntax-checked against raylib 5.5 headers). GitHub Actions: first push's `web` job failed (Conan provider), fix commit 6833c20 made both macOS `build` and `web` green. Not browser-tested.
- Open: Browser test of the web build (model load, audio after first click, FPS); touch controls; iOS build; landing. Link time `-sSTACK_SIZE`/`-sINITIAL_MEMORY` accepted by CI emsdk.
- PR: https://github.com/maratnek/claude-try-first/pull/1
