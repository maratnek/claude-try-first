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

## 2026-10-02 night run (16)
- Did: A4 full, checklist item 1. Crossing the 1000 m gate now only stops the clock (`completed` became `gateCrossed`); the plane stays controllable, a "GATE! Land to finish" banner and a 40 x 300 m landing-zone slab appear, and the run ends when the plane is grounded with speed <= 8 m/s (`IsRunFinished`). A hard landing or obstacle after the gate goes to the Crashed screen, which now shows "Gate reached in X.XX s". `kRequireLandingAfterGate` in level.cpp (default true) restores the old immediate finish when false. ARCHITECTURE.md updated.
- Why: Sprint is done and A4 full is next in Track order; I took the design note's recommended defaults (landing on, rollout required, switch kept) since the owner has not answered its open questions.
- Verified: game-tester installed the X11 headers, built raylib 5.5 and all sources with -std=gnu++23, and ran about 4 min under Xvfb with real keys: banner and zone shown, no results at the gate, results only after braking to 7.8 m/s with the gate time (19.74 s), R/M work, crash before the gate has no gate line, crash after the gate shows it. code-reviewer read the diff: no blockers. CI status: see PR.
- Open: Owner decisions in design-notes/a4-full-level-loop.md still stand (rollout rule, landing limits, medal times). The landing zone is cosmetic (landing anywhere finishes). The plane has no ground friction, so braking to 8 m/s takes several seconds of S. The false switch, rings above 0 and touch were not exercised. The gate banner shows dimly under the crash overlay. Next: checklist item 2 (medals, star checklist, delta vs best).
- PR: see PR into dev from agents

## 2026-10-02 night run (15)
- Did: Design pass only, no code. New design-notes/a4-full-level-loop.md for A4 full: gate stops the clock, run ends on a landing and rollout (`gateCrossed && !plane.airborne && plane.speed <= 8`), post-gate hard landing goes to the Crashed screen, Gold/Silver/Bronze medals at 24/30/40 s (par about 19 s, derived from code constants, not flown), star checklist, best-time delta, and a 4-item implementation checklist. Also found an existing bug: touch Restart/Menu rects are hit-tested while invisible.
- Why: Sprint is done; A4 full is next in Track order and it changes the rules of the level, so it needs an owner-visible design before code.
- Verified: code-reviewer checked every cited constant, function and file against src/ and found one real contradiction (grounded gate crossing never finishing) plus several corrections; game-designer fixed them in one pass; second review found no blockers, and I applied its small nits. No build was run (doc-only change). CI status: see PR.
- Open: Owner decisions are listed in the note (landing mandatory on web v0.1 or not, medal times, rollout rule, 844x390 results layout). Medal times and par are estimates; flight acceptance checks need a human pilot. Next run can implement checklist item 1 once the owner confirms, or default to the recommended landing-on setting.
- PR: see PR into dev from agents

## 2026-10-02 night run (14)
- Did: A5 full, remaining pieces. Procedural UI click (60 ms blip) on menu actions and on R/M/Q buttons of the crash and results screens; procedural damage hit played once when a soft obstacle first damages the plane; master volume via SetMasterVolume, `-`/`=` keys in 10% steps (clamped 0..1), "Volume: N%" shown top-right for 1.5 s. ARCHITECTURE.md audio row updated.
- Why: Sprint is done, so back to the Track order; A5 was the smallest unfinished piece (A4 full needs a design pass and is larger).
- Verified: game-tester hand-built raylib 5.5, compiled all sources with -std=gnu++23 with no warnings, and ran about 68 s under Xvfb with injected keys (menu, play, volume keys hitting both clamps, restart, menu): alive, no errors beyond no-audio-device warnings. code-reviewer found one real blocker: the click was started and then cut in the same frame by StopOneShotSounds inside resetRun, so it was silent on Play, M and R. One fix pass: the click is no longer stopped by StopOneShotSounds; also fixed the Q click firing where Q does not quit, right-aligned the volume text, clamped the timer, removed added what-comments. The tester re-ran on the fixed tree: pass. CI status: see PR.
- Open: No sound has been heard (no audio device here); owner should check click, damage hit and volume steps by ear. Volume text placement not screenshotted. Volume has no touch control and is not persisted. Web build not run locally (no emsdk this run); CI only. Not done: crash screen "Exit" click on web, A4 full (landing-to-results flow, stars tuning) and then Track B.
- PR: see PR into dev from agents

## 2026-10-02 night run (13)
- Did: Weekend sprint item 6 (blob shadow). New DrawBlobShadow in src/objects/world.cpp: a dark translucent ellipse (2.2 m x 3.0 m, yawed with the plane, not plane-silhouette shaped) on the terrain under the plane, drawn with rlgl and no depth write after the world and before the plane. It grows 1x to 2.5x and fades out over 0-60 m altitude and is not drawn above 60 m. Ground height is the max of five nearest-cell samples to avoid sinking under the interpolated mesh. New GraphicsSettings::blobShadow flag, on in every preset including Low (about 20 triangles, and the altitude cue matters most on web/touch). ARCHITECTURE.md updated.
- Why: Last unchecked item of the Web v0.1 sprint; helps judge altitude and landing on touch.
- Verified: game-developer only got a syntax check. game-tester built raylib 5.5 from source, compiled all sources with -std=gnu++23, ran under Xvfb with injected keys: starts, takeoff, 53-60 FPS, no errors beyond no-audio warnings; screenshots show the shadow on the runway and behind the menu without covering the plane, HUD or buttons. code-reviewer read the diff: no blockers; rlgl draw order, depth-mask restore, batch flush and Emscripten-safety judged correct. CI status: see PR.
- Open: Shadow not seen at 5-55 m (fade/grow untested visually), nor over hills or on landing; edges may clip on steep slopes (flat disc). The flag is on in every preset, so F1 does not toggle it; owner may want Low off. Emscripten/Conan paths proven only by CI. Sprint items 1-6 are now all done; next run should return to the Track order (A4 full, A5 full).
- PR: see PR into dev from agents

## 2026-10-02 night run (12)
- Did: Weekend sprint item 5 (itch.io package). The web CMake target now has OUTPUT_NAME index (Emscripten branch only), so the build emits index.html/js/wasm/data. The CI `web` job copies those four files into web-dist/ and uploads them as the `FlightGame-web` artifact (GitHub zips it, index.html at the zip root, if-no-files-found: error). ARCHITECTURE.md CI section updated.
- Why: Next unchecked item of the Web v0.1 sprint; the owner needs a ready-to-upload itch.io zip.
- Verified: game-developer could not build web (no emsdk); game-tester installed emsdk, built the web target (index.html 2.7 KB, index.js 181 KB, index.wasm 501 KB, index.data 195 KB), ran the workflow's packaging script literally, confirmed index.html at zip root with matching js/wasm/data names, and loaded it in headless Chromium: all 200s, no page errors. code-reviewer read the diff: no blockers (nits: hard-coded file list, README does not mention the artifact). CI status: see PR.
- Open: upload-artifact step itself only proven by CI. Not uploaded to itch.io; itch size limits and the 134 MB INITIAL_MEMORY not checked. Flight feel/input/audio in the browser not exercised. Next: sprint item 6, blob shadow under the plane.
- PR: see PR into dev from agents

## 2026-10-02 night run (11)
- Did: Weekend sprint item 4b (C++23). CMAKE_CXX_STANDARD 17 -> 23; README.md and docs/ARCHITECTURE.md updated. No source changes were needed. cmake/conan_provider.cmake derives compiler.cppstd from CMAKE_CXX_STANDARD, so the generated Conan profile now says gnu23 (valid for apple-clang 15); the Conan-Center raylib recipe removes compiler.cppstd in configure(), so raylib's package id should not change.
- Why: Owner asked to leave C++17; 23 chosen because every toolchain we could test accepts it (clang 18, g++ 13, emcc 6.0.10), so there was no reason to stop at 20.
- Verified: game-developer and game-tester compiled every src file with -std=gnu++23 on clang++ 18 and g++ 13 (no errors), built the desktop executable against hand-built raylib 5.5 and ran it 6 s under Xvfb (clean apart from no-audio-device warnings), and linked the web target with emsdk 6.0.10 (flags confirmed -std=gnu++23). code-reviewer read the diff and the Conan provider: no blockers. CI status: see PR.
- Open: macOS Apple clang and the Conan path were NOT run here; CI's macos-14 job is the only proof for them. Web output not loaded in a browser. Next: sprint item 5, itch.io zip artifact in CI.
- PR: see PR into dev from agents

## 2026-10-02 night run (10)
- Did: Weekend sprint item 4 (web page shell). New web/shell.html replaces raylib's minshell: touch-action none, overscroll-behavior none, no pinch zoom (viewport meta plus gesture/touchmove preventDefault), full-window canvas, fullscreen button (hidden where the API is missing, e.g. iOS Safari), a "rotate to landscape" hint on portrait phones (max-width 600px) with a Play anyway dismiss, and no CDN dependency (FileSaver.js gone). main.cpp sets FLAG_WINDOW_RESIZABLE on web only so the canvas follows the browser viewport. Added an empty data: favicon to silence the 404.
- Why: Next unchecked item of the Web v0.1 sprint; phone players need a page that does not scroll, zoom or refresh under their thumbs.
- Verified: game-developer built the web target with emsdk 6.0.10 and loaded it in headless Chromium. game-tester independently rebuilt and checked 1280x720, 390x844 and 844x390: canvas and buffer match the viewport exactly, hint shows only in portrait, Play works with keyboard and touch, mid-run resize re-lays out the HUD, no page errors (only a favicon 404, since fixed). Desktop code hand-built against raylib 5.5 and ran 5 s under Xvfb. code-reviewer read the diff: no blockers. CI status: see PR.
- Open: Real phone not tested: pull-to-refresh/pinch suppression, iOS Safari, the fullscreen toggle itself, safe-area insets. In portrait the menu title is clipped at the edges, and at 844x390 HUD row 2 sits close to the fullscreen button. No loading indicator while assets download (black page). Portrait tablets get no hint. Window title in main.cpp still says Step 5. Next: sprint item 5, itch.io zip artifact in CI.
- PR: see PR into dev from agents

## 2026-10-02 night run (9)
- Did: Weekend sprint item 3 (A5-min sounds), all procedural. Checkpoint chime (two notes a fifth apart, once per ring), wind loop whose volume and pitch rise with airspeed (quiet on the ground, stopped on Menu/Crashed/Finished), touchdown thump on a safe landing only. resetRun now stops the one-shots and the wind. CountPassed is exposed in level.h so main.cpp can detect a ring being passed. ARCHITECTURE.md audio row updated.
- Why: Next unchecked item of the Web v0.1 sprint.
- Verified: game-developer and game-tester built with a hand-built raylib 5.5 and ran under Xvfb with injected keys: menu, takeoff, crash screen, R restart and M menu all work, no errors beyond the expected no-audio-device warnings. code-reviewer read the diff: no blockers. It found the wind loop crossfade was wrong (the tail faded toward the head instead of toward the noise that precedes it); I fixed that by generating extra noise and blending the head from it, and re-checked that audio.cpp compiles. CI status: see PR.
- Open: No sound has been heard (no audio device here); owner should check chime, wind level and thump by ear. raylib PlaySound does not loop, so wind is re-triggered when it ends, which may leave a gap of up to one frame every 2 s; a stream would fix it if audible. Finish screen, a live checkpoint pass and a live safe touchdown were not run. Emscripten build not run locally. Wind has no hysteresis at its 0.01 volume cutoff. Next: sprint item 4, web page shell.
- PR: see PR into dev from agents

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
