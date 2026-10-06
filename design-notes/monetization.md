# Monetization options

Status: OPTIONS ONLY, OWNER DECIDES, UNDECIDED FOR NOW. Nothing here is built, and nothing should be built before the owner picks. v0.1 on itch.io stays free with no payment prompts of any kind.

Inputs: PLAYER_FEEDBACK.md is empty (no players yet), so this is designed against the niche (chill/arcade low-poly flight, comps A Short Hike / Sky Rogue) and the release criteria, not player data. Revisit once itch.io feedback exists.

Store-rule statements below come from memory, not from a fresh read of the current Apple or itch.io text. Every one is marked "verify before relying" and must be checked by the owner against the live guidelines before any submission.

## Constraints from the repo
- `rollout-plan.md`: v0.1 on itch.io is free and "nobody is pushed to pay". The 60% positive-reaction measure (>= 10 survey responses, "Yes" only counts) is about fun, not money. Owner item 8: "Decide monetization before any paid step".
- `progression.md`: no persistence exists on Web/iOS today (only `graphics.cfg` on desktop). Any "owned item" (skin, supporter flag) needs the shared `progress.cfg` storage work (about 2 runs) first. Loop C adds a hangar, planes and paint unlocked by pilot rank; that is the natural place a cosmetic would live. A second plane model is a human asset step (`asset-requests/`).
- Per-vehicle `PlaneParams` exist, so a plane is data, but a skin needs a texture or material swap on the single `biplane-1920.glb`; a paint variant is cheaper than a new model. Not checked in code beyond that; the developer would confirm.
- No accounts, no data collection today; the iOS privacy answers are trivial. Any option should keep that true (no ad SDK, no analytics).
- Web (itch.io iframe) and iOS are different worlds: itch.io HTML5 has no in-app purchase API at all; iOS has StoreKit only.
- Project rules: the owner approves anything that ships; the scheduled routine never pushes to production.

## Rules to check (verify before relying)
- Apple 3.1.1: unlocking features, content or digital goods inside an iOS app must use Apple's in-app purchase; a "buy me a coffee" link or external payment to unlock in-app content is not allowed in general. Region-specific exceptions for external links have changed over recent years; verify before relying.
- Apple 3.1.1 also covers tipping inside the app: a tip jar must be an in-app purchase (consumable); verify before relying.
- Apple 4.2 (minimum functionality): a paid or IAP-heavy app is judged more harshly; one level is a rejection risk regardless of price. `rollout-plan.md` already blocks the App Store on progression for this reason. Verify before relying.
- Apple 2.1 / 3.1.2: paid apps and IAP need a working purchase flow in review, and the owner needs the paid-apps agreement, tax and bank details in App Store Connect; verify before relying.
- Apple fee: 30% standard, 15% under the Small Business Program for developers under a revenue threshold; verify before relying (amounts and eligibility change).
- Apple Kids / age rating: ads and purchases affect the age rating and privacy labels; verify before relying.
- Ad SDKs: on iOS they bring App Tracking Transparency and privacy label work, and consent rules in some regions; verify before relying.
- itch.io: a project can be free, "pay what you want" (with a minimum, can be 0) or paid; "donate/support" tiers exist on the page; itch takes a revenue share that the seller sets (default 10%, adjustable) plus payment processor fees; payouts need a payment account. Web (HTML5) games can be paid-gated downloads but there is no in-game purchase API; verify before relying.
- itch.io policy on selling: no hidden extras; asset licensing of anything sold must be clear. The owner-made biplane model comes from an external generator, so its commercial-use terms should be checked before any paid release; verify before relying.

## Options

### 1. Free everywhere, no monetization (baseline)
- What: the game stays free on itch.io and, later, on the App Store; no IAP, no ads, no tip links.
- Pros: nothing to build; zero risk for the 60% criterion and for review; no tax setup; no store-rule exposure.
- Cons: no income; no signal on whether anyone would pay.
- Fit with progression: any loop works unchanged.
- Cost: 0 runs.

### 2. Tip jar on the itch.io page only (Web)
- What: itch.io "pay what you want" with a minimum of 0 (or the donate section), a line in the page text: "Free to play. If you enjoyed it, you can tip." No in-game prompt, nothing gated.
- Pros: owner-only setup on the itch page, no code; fully consistent with "nobody is pushed to pay"; gives a first real data point (do strangers tip at all). The iOS build is unaffected, so no 3.1.1 exposure.
- Cons: low income at a small audience; the wording must not guilt anyone; the survey should stay separate from the tip so the 60% measure is not skewed.
- Fit with progression: independent. Cost: 0 runs (owner action). Do not link it from inside the iOS app (3.1.1; verify before relying).

### 3. Free + optional cosmetic plane skins (paint schemes)
- What: the biplane gets a handful of paint schemes. A few are earned in-game (via Loop C pilot rank or stars, no payment), some are paid (IAP on iOS). Nothing affects flight (`PlaneParams` untouched), so no pay-to-win.
- Pros: the best fit for a chill game: players show off, nobody is blocked; reuses Loop C's hangar and rank; the same skins make good store screenshots.
- Cons: needs persistence and a hangar screen first; needs a skin pipeline (material colour swap, or texture variants from the asset flow); on iOS needs StoreKit (non-consumable IAP), "restore purchases" button (required by Apple; verify before relying) and receipt handling; on Web there is nothing to charge with, so skins would be earned-only there, so the two builds differ.
- Fit with progression: only makes sense after Loop A storage and Loop C hangar. Free-earned skins first, paid later.
- Cost estimate (not measured): paint swap on the existing model 1-2 runs, hangar screen is part of Loop C (3 runs covers plane swap plus hangar together), storage 2 runs (shared), StoreKit IAP integration in the SDL/raylib iOS build with restore flow 3-5 runs (high uncertainty: the device build has never run, `rollout-plan.md`), plus a human step in App Store Connect to create products.

### 4. One-time "supporter" unlock (non-gating)
- What: a single non-consumable purchase: removes nothing from the game, adds a "Supporter" badge, a couple of exclusive paints and a thank-you credit. Everything in the game stays free.
- Pros: simple to explain; one product in App Store Connect; fits "chill"; players who want to pay can, nobody else notices.
- Cons: same StoreKit and restore-purchases work as option 3 but with less visible reward; low conversion expected; on Web it can only be a link to the itch.io page (outside the app, never gating anything); Apple may question an IAP that unlocks nothing functional, so frame it as unlocking the paints (verify before relying).
- Fit with progression: needs only the storage and a flag in `progress.cfg`; no hangar needed if the reward is one paint option in the menu.
- Cost estimate: storage (shared) 2 runs, flag plus menu entry 1 run, StoreKit 3-5 runs.

### 5. Premium paid upfront (iOS, and/or itch.io)
- What: a fixed price (for example a few dollars), no IAP. On itch.io, a paid page with a free demo (level 1).
- Pros: no purchase code in the game at all on iOS (Apple handles the paid app); no ads; honest and simple; a demo on itch.io keeps level 1 free.
- Cons: a paid app with one level fails the 4.2 bar and the 60% test says nothing about willingness to pay; paid-app discoverability on iOS is poor without marketing; refunds and reviews are harsher; a free test audience cannot be moved to paid without feeling bait-and-switch; the owner must be sure about the model licence.
- Fit with progression: requires at least Loop A plus Loop C content to justify a price. Cost: 0 runs of code (store setup is the owner's), plus the content of progression.md.

### 6. Ads (banner or rewarded)
- What: an ad SDK on iOS (and Web).
- Pros: income without asking players for money.
- Cons: conflicts with the chill tone and with "nobody pushed to pay" in spirit; an ad SDK means tracking prompts and privacy labels, a CMake/Conan or Emscripten integration that does not exist, and a Web ad network integration that the itch.io iframe can limit; hurts frame rate on an iPhone 11 target; contradicts the "no data collected" privacy position in `progression.md`. Not recommended now.
- Cost estimate: 4-8 runs plus owner paperwork; high uncertainty. Verify before relying on every platform rule here.

## Web vs iOS summary
| | itch.io Web | iOS App Store |
| --- | --- | --- |
| Free play | yes (v0.1) | yes, later |
| Tip / donate | page-level, no code | in-app tip must be IAP (verify before relying) |
| Cosmetic purchase | not possible in-game; earned-only | IAP non-consumable plus restore |
| Premium | paid page or PWYW, demo possible | paid app, but 4.2 risk with one level |
| Ads | possible but poor fit | needs ATT and privacy labels |
| Owner paperwork | itch payout account | Developer Program (US$99/yr per `rollout-plan.md`), paid-apps agreement, tax and bank |

## Recommended order (suggestion only)
1. Now to v0.1: option 1 in the game, plus option 2 as page text only if the owner wants a first signal. Nothing is gated; the 60% measurement stays clean.
2. When Loop A storage and Loop C hangar exist: earned-only cosmetic paints (no payment), which also test whether people care about skins. This costs nothing beyond progression work already proposed.
3. Before App Store submission, decide: either stay free with a supporter or paint IAP (options 3/4, 3-5 runs of StoreKit work on top of a device build that does not yet exist), or free with no IAP. Do not start StoreKit before TestFlight works on a real device.
4. Premium (5) and ads (6) only if the owner explicitly wants them; both are the worst fit for the stated niche and the free-first criterion.

## Questions for the owner
1. Is money a goal at all for v1.0, or is the aim reach and learning first (option 1/2)?
2. Is a tip link on the itch.io page acceptable now, or should the first playtest have no payment text whatsoever?
3. If skins: are earned-only paints enough, or should paid ones exist on iOS? Is the biplane model licence fine for a paid cosmetic (check the generator's terms)?
4. Free app with optional IAP, or paid app? Is the difference between Web (no purchases) and iOS acceptable?
5. Is the owner willing to set up the Apple paid-apps agreement, tax and bank details, and an itch.io payout account, and when?
6. Are ads ruled out entirely, or only "at first"?
