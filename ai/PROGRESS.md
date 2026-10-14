# Release 0.6

Implemented:
- Authored English NPC introductions and 11 deterministic quest templates; not every resident offers a quest.
- Persistent accept/turn-in state, objective baselines, one-time rewards, quest recipe unlocks, journal and tracking.
- Five selectable backgrounds with actual movement/gathering/combat/trading bonuses and penalties.
- Eleven outfits with perks and animation frames; weapons and clothing can be swapped anywhere.
- Counted gear copies, atomic citizen allocations, training requirements and eight active resident roles.
- Workers contribute to town stocks, traders to treasury, ambassadors to bounded trust, melee defenders to local wolf protection.
- Seven new resident research nodes and a dedicated progression menu with population, gear, skill, trust, territory and quest requirements.
- Isometric wall/roof planes, shadows, windows, foundations and detailed town-hall sprites in one atlas.
- Windowed default; fullscreen only by explicit flag or session toggle.
- Save v5 with automatic v1-v4 migration and backups; legacy leather armor protection preserved.

Validation:
- All seven CTest groups pass, including quests, real character damage bonus, resident defense, gear conservation and old-save migration.
- Isolated X11/XWayland menu smoke passes: settings, creation/background screen, game-to-menu save, summary tabs, deletion cancel/confirm and recovery.
- Windowed 55-second community expedition with Qwen load: 2048x1101, 60 FPS, worst-window p95 16.683 ms, no frames over 20 ms, game RSS up to 63.0 MiB. All 16 scenario stages succeed.
- Summary: logs/v6-summary.txt; trace: logs/perf-20261013-232858-1460024.jsonl.
- Screenshots: logs/v6-newgame.png, logs/v6-quests.png, logs/v6-skills.png, logs/v6-clothes.png, logs/v6-houses.png.

Incomplete systems and prototype limitations are documented in PLACEHOLDERS.md; strategic warfare is not implemented.

Documentation audit:
- Expanded PLACEHOLDERS.md from four entries to a full inventory of incomplete gameplay systems, simulation boundaries, persistence limitations and validation coverage, with source links.
- Distinguished intentional implemented choices from missing features. Documentation only; no mechanics added or new runtime validation claimed.

Joint launcher:
- Added executable scripts/run.sh with model/game path overrides, shared port, readiness/alias checks, bounded startup, external server reuse, separate model logs and owned-process cleanup on exit/signals.
- Preserved explicit --llm-port 8080 over older menu settings; rebuilt the game.
- All eight CTest groups pass, including new launcher regression coverage for forwarding, exit status, reuse, port conflicts, failed startup, timeout and SIGTERM cleanup.
- Real Qwen plus windowed main-menu launch succeeds on port 18089; after the game exits that port no longer responds. Screenshot: logs/launcher-menu.png; model log: logs/model-20261013-234748-1469434.log.
- Isolated menu smoke passes after the port-priority change. No rendering/simulation mechanics changed; no new performance claim.

Lesson feedback and interface readability:
- Learning already modified authoritative skill state; the hall presentation kept its generic requirement/detail and repeat action. Added shared lesson state, immediate learned/readiness labels, exact requirement counters, completion bars and a learned-skills count. Learned lessons no longer execute again through the UI.
- Increased all effective interface font sizes by 1.25, baked matching integer glyph sizes and aligned text positions to pixels. Measurement and wrapping use the same scale; no nearby-size font resampling.
- Enlarged menu rows/panels and adapted page size to window height; input hitboxes share the rendering geometry. Background cards paginate on narrow windows.
- Rounded menu buttons and added four native landscapes: forest lake, coast with sailboat/lighthouse, town and underwater submarine scene. Random choice per menu entry, no consecutive repeats within the same process; deterministic preview override available.
- All eight CTest groups pass. Expanded campaign assertions cover lesson readiness, immediate completion, prerequisite unlocks, exact practice counters and duplicate rejection.
- Real-window smoke covers mouse-button Gathering learning, save/reload, repeated Enter, plus existing menu/settings/deletion flows. It passes with isolated saves.
- Inspected real screenshots of menu scenes and character/skills/hall/crafting/NPC screens, including 900x650 character/skills/hall windows. Artifacts: logs/ui-*.png.
- 35-second windowed community route with real Qwen: 2048x1101, weighted 60.0 FPS, worst-window frame p95 16.683 ms (LLM-busy subset p95 16.686 ms), zero frames over 20 ms, game RSS at most 65.9 MiB. Two model requests succeeded; 15 benchmark stages completed without failures (short run, not the complete 55-second scenario).
- Trace: logs/ui-benchmark/perf-20261014-001018-1481494.jsonl; model log: logs/model-20261014-001017-1481434.log. No save schema change or new XP-level mechanic.

Pause, safe exit and movement bindings:
- ESC closes the current gameplay panel or opens/resumes pause. Resume and Quit to main menu support keyboard and mouse; simulation and player actions stop while paused, and the resume click is consumed instead of triggering an attack.
- Quit writes the current slot before returning to the lobby. A failed write preserves the running session and displays a retryable error; successful Quit avoids a second final write.
- Interactive --play now skips only the initial lobby and returns there after Quit. Interactive temporary sessions also return to the lobby; automated benchmark/preview/frame-limit completion is preserved. ESC in the lobby no longer exits the app; its explicit Quit button does.
- Restored WASD alongside arrows, combining equivalent directions before normalization. Moved hall/diplomacy from D to P and Apply settings from S to F5; updated all current hints and controls documentation.
- All eight CTest groups pass. Expanded real-window smoke passes: mouse Gathering lesson, pause/resume, frozen movement, failed save/retry without damaging the old save, direct --play return, both Quit buttons by mouse, each WASD direction, alternative left/right arrows, P hall access and F5 settings application.
- Pause screenshot inspected: logs/quit-pause.png. The pause/exit build's 20-second community benchmark: 2048x1101, 60.0 FPS, worst-window p95 16.676 ms, zero frames over 20 ms, game RSS up to 65.5 MiB; no model load. Trace: logs/quit-benchmark/perf-20261014-002137-1487936.jsonl. This short run completed 13 stages without failures, not the full expedition. Movement rebinding subsequently passed the real-window checks; no additional benchmark claim.

NPC / hall interaction separation:
- Removed the hall's priority interception of E. E starts a nearby NPC conversation; only P opens the town-hall interface. A fresh campaign's first P visit selects the Gathering lesson automatically.
- Updated doorway/HUD/tutorial/gathering hints and README controls. Conversation opening emits one compact conversation_started event for diagnosis.
- All eight CTest groups pass. Real-window regression passes with both an NPC and hall nearby: E records a conversation and leaves skills unchanged, then P opens Gathering and mouse learning persists. Existing pause, save/retry, movement and menu checks also pass. No save schema or simulation/rendering workload change.

Shared summaries and skill graph:
- Saved-world and live paused-world summaries now share Overview, World, Inventory, Skills and Skill tree tabs. ESC opens the live summary on pause; Resume and save-before-Quit remain available.
- Inventory displays resources, counted weapons/clothes, tools, vessels, modules and consumables as vertical icon/name/count rows. Skills and technologies are separate vertical rows with actual learned/readiness/requirement state.
- Added 87 native pixel-art PNG icons plus a 1024x384 batched atlas and manifest in assets/ui; reproducible standard-library generator: scripts/generate-ui-icons.py.
- The 31-cell dependency graph covers 11 base skills, four activity counters, seven resident research nodes, eight town technologies and Civic charter. PNG icons sit above captions; learned skills/knowledge are light green, unlearned ones dark green, and ready nodes have a gold marker. Activity cells report recorded amounts; child requirements retain their own thresholds.
- Clicking a node shows acquisition requirements; wheel/PGUP/PGDN scroll, SHIFT+wheel or middle drag pans the graph. Small saved-world panels also support horizontal scrolling.
- Research readiness is shared with authoritative research without copying/mutating Save. Preview map layers use a render override instead of changing player submersion. Live summaries include current unsaved progress; saved summaries retain the file timestamp.
- All nine CTest groups pass, including summary counts, PNG coverage, nonoverlapping/cycle-free graph, live lesson progress, town-teacher readiness and read-only resource conservation.
- Final real-window smoke passes: all summary tabs, graph/list scrolling, NPC/hall separation, skills, pause/resume, seafloor preview without diving, failed-save retry, both Quit buttons and WASD/arrow movement.
- Inspected inventory/skills/tree screenshots at normal and 900x650 sizes using a valid isolated fixture; artifacts: logs/summary-inventory.png, logs/summary-skills.png, logs/summary-tree-small.png. Live graph: logs/summary-live-tree.png.
- 25-second benchmark with the graph opened over loaded terrain: 2048x1101, weighted 60.0 FPS, worst-window p95 16.681 ms, zero frames over 20 ms, game RSS up to 66.7 MiB. Four aggregate windows explicitly record paused=true and summary_tab=4. No model generation in this UI benchmark.
- Trace: logs/summary-benchmark-final/perf-20261014-010626-1516522.jsonl; compact result: logs/summary-ui-perf.json. No save schema change or new skill/research unlock rules.

Public README and setup:
- Replaced the historical Russian README with an English guide using What is it, Features/Gameplay, Installation and Tech Stuff. Covers current campaign start, progression, quests, equipment, settlements, water, summaries, controls, stack, setup and validation. Explicitly distinguishes authored NPC gameplay from optional LLM benchmarks and links to the current incomplete-system inventory.
- Added scripts/install.sh with opt-in --with-model, shared prerequisite/build-job/download helpers and scripts/download-model.sh. Model downloads resume from .part, completed files are reused, failed/invalid-format downloads are not promoted. The header check is not a checksum or complete GGUF validation.
- Added --no-model to the existing joint launcher, preserving the default model/game readiness and cleanup behavior. Custom model log paths are honored. Standalone model startup validates port/threads and reports missing explicitly configured server binaries instead of silently substituting another executable. Disabled llama.cpp prebuilt UI downloads as well as UI builds.
- Main-project setup with --with-model succeeds with cached dependencies/model. All nine CTest groups pass, including game-only launch with missing model/server files, forwarded options, project working directory and propagated exit status.
- Installation from an isolated source copy under a /tmp path containing spaces succeeds with freshly downloaded raylib/JSON/HTTP dependencies and a new Release build; all nine CTest groups pass. The font is copied from the available system font. No large model redownload was performed.
- Isolated downloader checks cover interrupted download preservation, resumed retry, atomic promotion, cache reuse, invalid GGUF-header rejection, help and invalid setup options. Real Qwen plus raylib menu launch on port 18093 succeeds; the owned server stops on game exit. Screenshot inspected: logs/readme-joint-launch.png. No gameplay/rendering changes or new performance claim.

Initial Git publication preparation:
- Initialized the local repository with main and configured origin as https://github.com/kiquxd/forest-frontier.git after confirming the remote has no refs. Existing Git author configuration is available.
- Expanded .gitignore for generated binaries/build artifacts, GGUF files outside models/, local credentials/assistant settings and editor caches. Publishable sources, content, icons and the font license are retained; models, saves, settings, dependencies and logs are excluded.
- README now includes the real clone URL, complete --with-model installation and custom-GGUF setup without downloading Qwen. Documents the generic streaming API and alias, Qwen-oriented reasoning/template settings, externally started server reuse and the absence of validation for other models. No inference behavior changed.
- All nine CTest groups pass before the initial commit. Checked the publishable file list for accidental large files and recognizable credential patterns; none found. Publication itself is performed separately after this preparation.

Public incomplete-mechanics inventory:
- Expanded the existing README Current limits subsection to cover every audited incomplete system and technical limitation in ai/PLACEHOLDERS.md, with working behavior and missing parts stated separately. Kept all four requested main sections and distinguished intentional scope/design choices from unfinished mechanics.
- AGENTS.md now requires README updates in the same change as mechanics, content, controls, saves, dependencies or setup changes, with Current limits synchronized to the maintainer inventory. Added the reciprocal maintenance reference in ai/PLACEHOLDERS.md. Documentation only; no mechanics, save format or rendering changed.
- Verified all 23 audited system categories appear in README Current limits, the four main sections are preserved, text is English and local links resolve. git diff --check is clean; all nine CTest groups pass.

Tracked checklist and C++ style rules:
- Added root TODO.md with 104 unchecked items: every audited incomplete-system limitation plus legacy C++ style migration. Keep it tracked, remove fully implemented/validated items, split partial completions and synchronize README and ai/PLACEHOLDERS.md. Verified neither TODO.md nor .clang-format is ignored.
- AGENTS.md adopts the Google C++ Style Guide for new/changed code with naming, header/include, readability and reviewable-migration rules. Added a Google-based .clang-format with two-space indentation, 80-column limit and expanded function/conditional/loop bodies. Preserved existing .cpp/.hpp extensions, exception policy and saved-content compatibility.
- README and maintainer notes link the checklist and distinguish style policy from actual migration. No existing C++ sources were reformatted or renamed. clang-format is not installed locally, so no formatter invocation is claimed. Checklist coverage, links and git diff --check pass; all nine CTest groups pass.

First-party C++ style migration:
- Migrated all 31 first-party C++ implementation/header/test files under src/ and tests/. Third-party sources and content/assets are untouched. Expanded declarations and control flow, added braces and Google formatting, renamed functions/fields/locals/constants/enumerators, replaced primitive functional casts with explicit static_cast, removed global using-directives, moved helper functions into anonymous namespaces and added guards/direct dependencies to headers. Mechanics, enum numeric order and saved-content identity are unchanged.
- Used temporary clang-format 23.1.3 and libclang 18.1.1 tooling outside the repository; these are developer tools, not new runtime/install requirements. Added executable scripts/format.sh with --check and CLANG_FORMAT override, scoped to first-party C++ only. Root formatting requires clang-format >=15 for InsertBraces.
- Removed the completed style-migration item from TODO.md; all 103 audited limitations remain represented. Updated README, AGENTS.md and maintainer notes to reflect the implemented migration and repeatable formatting check.
- Final Release build succeeds without compiler warnings. All nine CTest groups pass. All ten project headers compile independently. clang-format --check and git diff --check pass. Verified every original string, character and numeric literal group across all 31 C++ files matches HEAD before the migration, excluding includes and concatenating formatter-split strings. Audit artifact: logs/style-literal-audit.log.
- Isolated real-window menu smoke passes, including lessons, NPC/hall distinction, paused summaries/graph, map layer preview, save failure/retry, settings, WASD/arrows and quit/save management. Inspected final gameplay screenshot: logs/style-benchmark.png.
- 55-second windowed community benchmark without model generation: 2048x1101, weighted 60.0 FPS, worst-window p95 16.688 ms, zero frames over 20 ms, game RSS max 66.5 MiB. All 16 recorded scenario stages succeed. Trace: logs/style-benchmark/perf-20261014-151141-1687427.jsonl; compact result: logs/style-benchmark-summary.json. No new gameplay features or save schema.

Placeholder scope cleanup:
- Reduced the final TODO/README/maintainer inventory category to Release and distribution, retaining Windows/macOS coverage, standalone packaging and download integrity limitations. Removed performance-measurement coverage/profiling/FPS caveats and the redundant general balance-validation bullet; quest/progression balance limitations remain. All three inventories are synchronized, with 99 checklist items. Development performance targets and validation rules remain in AGENTS.md and README Tech Stuff. Documentation only.

Menu separation, progression dashboard and save workflows:
- Esc now offers Resume, manual Save world, Save & return to main menu and shared Settings. Manual Save remains paused; failed saves keep the session open. K Progress pauses simulation and provides Overview/history, World, Resources, Settlements and Skill tree. Saved-world previews are read-only with F2 expansion; separate skill lists were removed.
- The tree calls authoritative base training/research, shows readiness/requirements and prevents duplicate acquisition. Specialist teaching remains tied to NPC/hall visits. Town reports include discovered locations, type/rarity, resident/visitor counts, trust/treaty, walls and specialty knowledge; owned rosters support individual role and counted-gear assignment.
- Contextual action keycaps have an opaque bottom background and current key labels, including nearby NPC, town-hall, harvesting, vessel and resident actions. Removed the floating hall-entry prompt and repeated action hints/skill card from the side HUD.
- Added primary/alternate keyboard rebinding, explicit conflict swaps/default reset, shared text size/contrast settings and basic gamepad inputs. Fixed same-frame Enter activating the first setting while opening pause Settings, and recovery opening/confirming from one Enter. Full controller/accessibility support is still incomplete.
- Lobby F6/F7 rename/copy, F8/F9 export/import and F10 recovery validate sources and preserve existing destinations/progress. Recovery restores a new slot. A completed world operation remains completed if preferences cannot be written. No world metadata is reconfigured while its worker is alive.
- Save v6 remembers acquired goods and the last 512 significant events. Existing v1-v5 formats remain readable and migration writes keep a backup. No content/NPC IDs or equipment instance rules changed. Old history is not reconstructed.
- README, TODO and PLACEHOLDERS are synchronized: 95 remaining checklist items across the existing system categories. Retained partial roster limitations (search/comparison/bulk allocation) and complete-controller/accessibility/history limitations. No performance-placeholder category was reintroduced.
- Final Release build succeeds without warnings; all ten CTest groups pass. New menu_core tests cover control swaps/reserved keys/settings, legacy settings, read-only/authoritative unlocks, resource memory, bounded history, malformed imports, copy/rename/export/import/recovery and v5 backup preservation. All 14 project headers compile independently; formatting and diff checks pass.
- Real-window menu smoke passes the split pause/progress, manual save/frozen movement, failed save/retry, learning, default WASD/arrows and quit flows. Extended workflow smoke passes conflict/swap/reset, read-only saved tree, all file operations, roster assignment, unchanged fullscreen when entering pause settings and runtime movement on rebound Z.
- Inspected real raylib screenshots for footer, live/saved tree, settings, recovery and resident allocation; also used a PID-restricted Sway floating test window at 900x650 with text size 140%. Card/roster heights now adapt to the enlarged font. Artifacts: logs/menu-context-footer.png, logs/menu-live-tree.png, logs/menu-saved-tree.png, logs/menu-resident-assignment.png, logs/menu-pause-settings.png and logs/menu-*-small-140.png. No physical controller test was performed.
- A 55-second windowed community benchmark without model generation at actual 2048x1101 holds weighted 60.0 FPS, worst-window p95 16.689 ms, zero frames over 20 ms and game RSS max 72.0 MiB; all 16 scenario stages succeed. Trace: logs/menu-benchmark/perf-20261014-164034-1745044.jsonl. The final graph/roster text-layout refinement is measured separately with the paused-tree run below.
- Final 25-second paused-tree benchmark without a model at actual 2048x1152 holds weighted 60.0 FPS, worst-window p95 16.684 ms, zero frames over 20 ms and RSS max 71.7 MiB. Five aggregate windows record paused=true/summary_tab=4; no gameplay scenario stages were requested. Trace: logs/menu-progress-benchmark/perf-20261014-164949-1749682.jsonl. Compact results: logs/menu-perf-summary.json. Final screenshot inspected: logs/menu-progress-benchmark.png.

## 2026-10-14 — 2D collision geometry, cartography, vessels and audio

- Added exact 2D building polygons, wall capsules, terrain tile boxes and resource shapes, including local NPC/player separation. Long-distance route planning remains deferred until owned/foreign resident command systems exist; tracked separately in the incomplete-work documents.
- Exploration now uses world-space sight and building/wall occlusion, with persistent discovery and remembered rare deposits. Live maps offer terrain/politics/economy overlays and explored-location clicks opening an editable, saved journal; saved-world previews stay read-only. Save v7 retains legacy progression and migration backups.
- Nine vessel choices include raft, canoe, skiff, cutter, brigantine and flagship, with deterministic craft knowledge/material/coin gates. Added richer procedural facades/material details, vegetation/outfits, vessel sails/rigging, animated waves/foam/wakes/submarine bubbles, harvesting motion and combat feedback. Prestige currently describes cost/travel/presentation, not diplomatic influence or crew mechanics.
- Added original synthesized music/effects, automatic/manual track selection, Off and independent persisted volumes. Audio-device failure allows silent gameplay. Full mixing/crossfades and a larger soundtrack remain incomplete.
- All 11 CTest groups pass in 5.53 seconds, including new geometry/exploration/journal/vessel/save tests. Earlier real-window menu/workflow and extended world checks passed; inspected journal, map, settings and flagship screenshots. The representative community run with new geometry/audio/graphics held 60.0 FPS at 2048x1101 without model generation; worst-window p95 16.680 ms. No additional long benchmark was run after small final refinements. The final quick real-window journal/save/return check passes; its screenshot was inspected. Formatting and diff checks pass.
- Updated validation policy following user feedback: short focused checks by default, extended GUI workflows opt-in, occasional profiling only for credible performance risks or observed slowdowns. README, TODO and PLACEHOLDERS reflect actual remaining scope.

## 2026-10-14 — Town infrastructure and limits on crafting

- Added six activity buildings, a starter workbench, seeded foreign facilities, school-gated resident research/training and local/peaceful road-connected crafting/work access. Construction expands town radius in 150-unit steps with increasing costs; building sites are automatically chosen, and full layout/upgrade systems remain incomplete.
- Every successful craft starts a persistent global 5/15/40/90/180-second simulation cooldown by rarity. Failed attempts do not consume resources or extend it. It advances in live gameplay/crafting, freezes during session pause/Progress/note or road editing, and never uses offline wall time. Equipment swaps remain unrestricted.
- Removed the automatic inter-town road from terrain and map rendering. Surface map Plan road accepts custom waypoints and confirms a complete explored route atomically; land tiles cost 1 wood/1 stone and bridge tiles 4 wood/3 stone. Bridges support walking over deep water; peaceful connected towns share facilities. Approaching a hall identifies its centre cell despite building occlusion, without revealing the entire town.
- Residents persist health, dead tombstones or captivity. Local defender encounters can cause death or deterministic capture by hostile guards. Death destroys assigned weapon/clothes without drops; captured gear stays allocated, and ransom restores the same copies. Player death destroys the equipped weapon; player captivity and strategic casualties remain unfinished.
- Surrender retains local town control with 30 coins upfront, tribute 10 per 120 simulation seconds and a 300-coin exit. Missed tribute resumes local hostility. Exit can retain player independence or restore the recorded previous ruler. Autonomous wars/state integration remain placeholders.
- Save v8 persists clocks, cooldown, infrastructure, territory, captivity/losses and tribute; v1-v7 progress remains readable and migrations preserve backups. README, TODO and PLACEHOLDERS stay synchronized at 93 remaining items; navigation is still deferred behind resident command systems.
- Twelve CTest groups pass; the final full run took 5.51 seconds. Checks cover atomic road costs/bridges, cooldown reload, local guard capture, ransom allocation, dead-roster synchronization, tribute/default, previous-owner restoration and v7 migration. Short real-window smoke passes road planning/cancel, journal/save and return to menu. Inspected logs/world-road-plan.png; moved confirmation panel inside the map after finding a clipped button. Formatting/diff checks pass. No performance benchmark was run for this change.

## 2026-10-14 — Longer crafting cooldowns

- Increased rarity tiers to 1, 5, 30, 60 and 120 simulation minutes. Updated the save-validation ceiling to the shared two-hour maximum; schema stays v8, and existing shorter in-progress timers remain readable and unchanged.
- Added deterministic clock-boundary and two-hour save/reload checks; no real-time waiting, GUI simulation or performance benchmark is needed for this balance change. README and PLACEHOLDERS describe the new durations; the tracked incomplete scope in TODO is unchanged.
- Validation: Release build and all 12 CTest groups pass; the complete test run takes approximately 5.5 seconds. Formatting and diff checks pass.

## 2026-10-14 — Visible Apply action in settings

- Moved the shared settings Apply action to a prominent top-right button labelled Apply. Removed applying through F5 and released F5 for gameplay rebinding; unsaved draft behavior and atomic settings writes remain unchanged. Lobby, pause and keyboard controls share this implementation.
- Updated existing UI harnesses to click Apply. A short settings smoke verifies draft changes save through the button and F5 does not write them. Real raylib screenshot logs/settings-apply.png inspected; Release build, all 12 CTest groups (5.81 seconds), formatting and diff checks pass. No extended UI scenario or benchmark was run.
