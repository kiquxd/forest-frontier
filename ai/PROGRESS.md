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
