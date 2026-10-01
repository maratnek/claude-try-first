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

## 2026-10-01 15:15 UTC — day run
- Did: Added an Emscripten (Web) build target: main loop split into UpdateFrame + emscripten_set_main_loop_arg on web, AssetPath helper, CMake FetchContent raylib 5.5 + HTML output with preloaded assets, new `web` CI job. Follow-up commit fixed Conan provider still running on web (EMSCRIPTEN var is undefined before project(); now detected via toolchain file).
- Why: Mobile launch is the priority and Web (itch.io) is the first step; CI had no web target.
- Verified: Sandbox has no emcc and Conan can't install, so nothing was built locally (sources syntax-checked against raylib 5.5 headers). GitHub Actions: first push's `web` job failed (Conan provider), fix commit 6833c20 made both macOS `build` and `web` green. Not browser-tested.
- Open: Browser test of the web build (model load, audio after first click, FPS); touch controls; iOS build; landing. Link time `-sSTACK_SIZE`/`-sINITIAL_MEMORY` accepted by CI emsdk.
- PR: https://github.com/maratnek/claude-try-first/pull/1
