# Rollout plan

Status as of 2026-10-05. Owner: release-manager prepares, project owner approves. Update when reality changes (a blocker fixed, a feedback result, a date slipping). Builds on `design-notes/release-plan.md` (scope and risks) and does not repeat it.

## Where things stand (checked 2026-10-05)

| Item | State |
| --- | --- |
| `release` branch | head `d225632`. Cut from `dev` as `9543bba`, plus blocker 1 (engine power level, glide, stall fall: `e688bc6`), blocker 2 (banked coordinated turns: `10a33ff`) and blocker 3 (visible soft-obstacle damage: `d225632`) |
| Blocker 4 (progression) | proposal only, `design-notes/progression.md`, three loops, owner has not picked. Nothing built. Not release-relevant for v0.1 |
| Blocker 5 (this plan) | this document |
| Blocker 6 (breakup v2) | not started; design note comes first. Not part of v0.1 |
| Owner flight test of blockers 1-3 | not done. Feel (power ramp, takeoff, stall, turns) is unchecked by a human, per PROGRESS_LOG runs 37-39 |
| Web package | CI job `web` uploads artifact `FlightGame-web` (`index.html` at zip root, custom `web/shell.html`: no scroll or zoom, fullscreen button, landscape hint). Per docs/STATUS.md the zip in `dist/` was built before blockers 1-3 (not verified here); rebuild from CI on the release head and do not upload that one |
| Never run on a real phone | Web build on a phone, touch feel, audio on Safari |
| iOS | only a simulator build in CI (`ios-sim`, continue-on-error). Never launched. No device build, no signing, no Apple account |
| Player feedback | `PLAYER_FEEDBACK.md` empty. No players yet |
| Game name, itch.io account | not decided / not created (docs/STATUS.md) |

## Channels

### 1. itch.io Web (free) - first, and the playtest channel

- What goes out: v0.1, level 1 (menu, flight, checkpoints, finish gate, landing, results screen with medal, stars and session best time), original biplane, Low/Medium/High presets, procedural sounds. Built from `release`, CI artifact `FlightGame-web`. Not included: progression, level 2, persistence of best time, breakup v2, anything on `dev` after the release head.
- When: as soon as the entry criteria below hold. No calendar date is promised because two of them (owner flight check, game name and account) depend on the owner. Proposed order: draft (hidden) page, then a restricted-access link to about 10-15 players (proposed), then public only on the owner's go.
- Why first: free, instant to update, reaches phones through the browser, and is the only way to get the 60% measurement that gates the App Store.

Entry criteria (all must hold; numbers are the bar):
1. All release blockers 1-3 cherry-picked into `release` (done, `d225632`). Blockers 4 and 6 are explicitly not required.
2. CI on `release` head: `build`, `web` green. `ios-sim` result noted but does not block (it is continue-on-error).
3. Zip comes from the CI run on the exact `release` head, with `index.html` at the zip root. Record the commit id in the release notes.
4. Web build loads in a browser and reaches the menu, takes off, passes a checkpoint, finishes and shows the results screen. Headless run acceptable for load; the owner or a tester must do the full run once.
5. Owner flies the new physics on desktop once and says the feel is acceptable (high urgency in docs/STATUS.md). If not acceptable: fix on `dev`, cherry-pick, rebuild; the date moves.
6. Phone check by the owner: one real phone, Safari or Chrome, 5 minutes. Pass if: no page scroll, zoom or pull-to-refresh; audio starts after the first tap; touch stick and throttle usable; frame rate at least 30 fps on Low (60 is the goal) [30 fps floor is proposed]. Failure of any item blocks the public step, not the hidden draft.
7. Name chosen, itch.io account and draft page exist (owner).
8. Zero known crashes or freezes in the happy path (menu to results) over 3 consecutive full runs on web (3 is proposed).

Checklist:
- [ ] Confirm `release` head and CI status (`gh run list --branch release --limit 1`)
- [ ] Download `FlightGame-web` from that run, unzip, check `index.html` at the root, serve locally and load it
- [ ] Release notes `design-notes/release-notes/v0.1.md` final: commit id, known issues up to date
- [ ] Owner uploads the zip to the draft page (kind of project: HTML, "play in browser", viewport about 960x540 (proposed), fullscreen on, mobile friendly)
- [ ] Feedback form and the instructions text in place (see measurement)
- [ ] Restricted-access link sent to players
- [ ] Owner gives the explicit go for the public page; only then tag `v0.1` and promote `release` into `main`

### 2. TestFlight (iOS)

- What goes out: the same v0.1 gameplay in an iOS build, internal testers only first.
- When: not scheduled. It starts when the owner enrols in the Apple Developer Program (US$99/yr, the docs say the owner does this later and the plan must not depend on it). Rough lead time to count on once enrolled: 1-several days for the account (identity checks can take longer), then a device build that has never been done (see risks below).
- Entry criteria:
  1. Apple Developer account active, bundle ID and app name chosen and free (owner).
  2. A device build that launches on a real iPhone: an SDL_main entry point exists in `src/main.cpp` but has never been run on a device; touch, main loop and signing on hardware are unproven. The simulator job has never launched the app, so the size of this work is unknown.
  3. Web v0.1 already shipped to the restricted-access testers, with its touch problems fixed (no iOS-specific unknowns left that web would have shown).
  4. At least 30 fps (proposed) on an iPhone 11-class device (proposed) on Low; safe areas and landscape orientation correct on a notched phone.
- Internal testing needs no Apple review. External TestFlight (public link) needs review and the owner's explicit go.
- Checklist: [ ] account and Team ID, [ ] signing set up by the owner (an App Store Connect API key in a GitHub secret, never the account password), [ ] device build in CI or on the owner's Mac, [ ] upload, [ ] 5-minute test on device, [ ] touch retuning list.

### 3. App Store

- What goes out: v1.0, not v0.1. Only when the game is a complete game.
- When: not before both of these hold: (a) the web criterion below is met, and (b) there is enough content and progression that guideline 4.2 (minimum functionality) is not a risk. Today there is one level and no progression (blocker 4 is only a proposal), so (b) is not met. No date is set. Do not plan around it.
- Entry criteria:
  1. 60% positive reaction measured on web (below), with at least 10 responses.
  2. Owner has picked a progression loop and it is built (at least the first of the options in `design-notes/progression.md`).
  3. TestFlight internal build tested on a device, no crash in 3 consecutive full runs (proposed).
  4. Store metadata ready: name, subtitle, description, 3-5 screenshots for the required phone sizes, icon (current icon is crude, owner to judge or supply), age rating, privacy answers. The game collects no data today; keep the privacy answer true if telemetry is ever added.
  5. Owner presses Submit.
- Review takes days and a rejection restarts the wait; leave at least a week of buffer after the planned submission before promising a public date.

## What the owner must do by hand

1. Fly the new physics on desktop (power, takeoff, glide, turns, damage look) and say whether feel is acceptable. Now, high urgency.
2. Choose the game name; create the itch.io account and a draft page (cover 630x500 or larger, 3-5 screenshots, short description, HTML5 embed settings above).
3. Upload the zip from the CI artifact (agents never upload).
4. Test on one real phone (see criterion 6) and report.
5. Create the feedback form (one question, see below) and paste its link into the page text; copy itch.io comments into `PLAYER_FEEDBACK.md`.
6. Give the explicit go for: public page, tag `v0.1`, promoting `release` into `main`, external TestFlight, App Store submission. Each is a separate go.
7. Later: enrol in the Apple Developer Program, provide signing as a CI secret, pick a progression loop, approve store text, press Submit.
8. Decide monetization before any paid step; v0.1 is free and nobody is pushed to pay.

## How the 60% positive-reaction criterion is measured

Definition (proposed; the owner may change the numbers):
- Unit: one response per player. Source 1: a one-question survey linked from the itch.io page and shown again in the page text: "Would you play this again?" with Yes / Maybe / No and an optional comment. Source 2: itch.io likes/ratings and comments, read by hand and classified positive, neutral or negative; used as a cross-check, not added to the survey count.
- Positive = "Yes". "Maybe" and "No" count as not positive. This is deliberately strict.
- Measure: positive responses divided by all survey responses.
- Met when: at least 10 survey responses and 60% or more positive, so at least 6 of 10 (7 of 11, 8 of 13). Fewer than 10 responses means not measurable yet, not met and not failed.
- Window: from the restricted-access link going out the window closes at the later of 10 responses or 14 days, capped at 28 days; then the owner calls it.
- Who: the release-manager tallies the form export and `PLAYER_FEEDBACK.md` entries and writes the result into this plan and PROGRESS_LOG.md. The owner confirms.
- Reliability notes: this is small-sample and friends-heavy, so friends and strangers should be separated in the tally; if strangers are under 5 responses, say so. The form needs no personal data.
- Free-text answers go into `PLAYER_FEEDBACK.md` so game-designer weighs them.

## Release notes plan

- One file per version in `design-notes/release-notes/vX.Y.md`: what players get, known issues. Draft for v0.1 exists (`design-notes/release-notes/v0.1.md`).
- Known issues come from the "Open" lines of PROGRESS_LOG.md; strip them of internal names before they go on the itch.io page. Final version names the `release` commit the zip was built from.
- Player-facing text for the itch.io page and the store is derived from the same file after the owner approves it. No mention of how the game was built.
- A new file is made per candidate; the previous one stays.

## Risks that can move dates

- Owner flight check fails on feel: blocks everything; fix goes `dev` then `release` by cherry-pick.
- The web build has never been seen on a phone: browser audio and touch problems could appear only there.
- iOS device build is unproven (the simulator job has not launched the app) and needs the owner's account and signing.
- Level 1 only: even if web criteria pass, the App Store waits for content and progression.
- Blocker 6 (breakup v2) and blocker 4 (progression) are on `dev`'s roadmap, not on `release`; a v0.2 candidate would need the owner's ask to cut from `dev`.

## Next step

Owner flies `release` head `d225632` (or the matching CI web zip) and answers items 1 and 2 of the owner list. Then release-manager rebuilds the zip from CI on that head and re-checks the web entry criteria.
