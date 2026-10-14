# Forest Frontier

## What is it

Forest Frontier is a local, single-player exploration and settlement game built in **C++20 with raylib**. Explore an isometric world of forests, lakes and coastline, earn knowledge through practical work and quests, develop your town, and eventually sail and dive for rare materials.

The current version is a playable development prototype. Terrain streams in chunks; the settlement network is a finite, seeded region. All in-game UI, lore and interactions are in English. The game starts in a window and works with tiling window managers; fullscreen is optional.

An optional local Qwen model and launcher are included for performance experiments. **Normal gameplay uses authored NPC interactions and never sends model requests.** Model generation currently runs only in explicitly enabled LLM benchmarks. Quests, rewards, equipment and progression are always controlled by deterministic game rules.

## Features/Gameplay

### Starting a campaign

Choose one of five backgrounds: **Traveler, Artisan, Scout, Envoy or Warden**. Each card shows its bonuses and penalties, affecting movement, gathering, combat or trade. A new campaign places you near the town hall of one randomly selected town you own. Other settlements are independent and have their own attitudes, specialties and knowledge.

You begin with basic clothing and no learned skills, tools or weapons. Visit your hall with **P** to learn **Gathering** for free. Gather resources, unlock **Craftsmanship**, make tools and equipment, and use trading and quests to build your resources and knowledge. Later lessons depend on activities such as gathering, crafting, fishing and completing town orders.

### Exploration and survival

- Forests, lakes, shoreline and a northern coast, with paths, towns and resource sites.
- Persistent fog of exploration on surface and underwater maps. Lakes and towns appear only as far as you have explored them; towns are drawn as miniature settlements.
- Trees, stone, berries, reeds, fish, ore and rarer materials. Tools improve gathering or enable specific activities; harvested sites remain depleted.
- Wolves with attack wind-up, lunges and pursuit; directional melee swings, knockback and ranged weapons with consumable ammunition.
- Food, potions and campfires for recovery. Campfires also deter wolves.

### NPCs, quests and progression

- Authored introductions and lore for 20 NPC types, including craftspeople, merchants, scholars and rare specialists. NPCs stay nearby during a conversation and display a speech indicator.
- Some NPCs offer quests; others provide lore only. Quest templates include deliveries, crafting, exploration and wolf hunting, with coins, items, recipes or knowledge as rewards.
- A persistent quest journal with tracked objectives. Each quest-giving NPC currently offers one non-repeatable quest.
- Eleven base skills, specialist town technologies and seven resident research nodes. Requirements can include activities, coins, residents, equipment, known skills, owned towns, trust and completed quests.
- A skill dependency graph with PNG icons, learned/locked colors and the actual acquisition requirements for each node.

### Equipment, crafting and settlements

- Fourteen weapon definitions and eleven outfits with gathering, movement, trade, protection or diving perks. Change player equipment anywhere.
- Counted equipment copies: an instance allocated to a resident cannot also be used by the player or another resident. Weapons have no durability and are never consumed by attacks; ammunition is expendable.
- Craft tools, weapons, clothes, consumables, vessels and underwater modules as skills, recipes and materials become available.
- Settlement markets with stocks and treasuries; buy and sell resources and earn trust. Rare materials and technologies depend on relationships with specialist towns.
- Town halls, rulers, gifts, truces, war declarations, alliances and diplomatic unions. Unions can bring another town under your ownership.
- Research and equip residents for gathering, crafting, guarding, trading, fishing, scholarship, engineering and diplomacy. Work contributes periodically to town stocks, income or relationships.
- Build and repair predefined town-wall segments; fortification knowledge improves them.

### Water exploration

Craft a boat, ship or submarine through progression. Switch vessels, sail across deep water, and use a submarine to explore the underwater layer. Oxygen, hull damage, reefs and emergency surfacing affect exploration. Unlock sonar and oxygen modules, collect pearls and crystals, and recover underwater salvage.

### Menus and saves

The main menu supports new campaigns, save selection, world summaries, deletion and settings. Summaries show explored maps, discovered towns, resources, equipment, skills and technologies. **Esc** pauses the world and opens the same summary with current progress.

**Quit to main menu saves the world first.** If saving fails, the session stays open for retry. The main menu has a separate Quit button. Save writes are atomic; older formats are migrated with a backup, and deleted slots are moved to `saves/.trash/`. Active worlds autosave periodically.

### Controls

| Key | Action |
| --- | --- |
| WASD / Arrow keys | Move |
| Shift | Sprint |
| E | Talk to a nearby NPC |
| P | Town hall: lessons, ruler and diplomacy |
| F (hold) | Gather a nearby resource |
| Space / Left mouse button | Attack outside UI panels |
| C / T | Crafting / Trading |
| I / O | Weapons / Outfits |
| K / L | Skills and research / Quest journal |
| 1 in a conversation | Accept or turn in a quest |
| F4 in a conversation | Specialist teaching, where available |
| Q / H / B | Eat / Use potion / Place campfire |
| V / R | Select vessel / Dive or surface |
| G / J | Sonar / Repair vessel |
| M | Map |
| Mouse wheel | Zoom the world/map; scroll a summary |
| Shift + wheel / Middle drag | Pan the skill graph |
| Enter / Shift + Enter | Selected action / Sell in the trade menu |
| Page Up / Page Down | Page through menus or scroll summaries |
| Esc | Close a panel, otherwise pause/resume |
| F3 / F11 | Performance HUD / Toggle fullscreen |
| F5 in settings | Apply settings |

### Current limits

The mechanics below are working prototypes or incomplete systems. Their current behavior and missing parts are listed explicitly; menus, unlocks and visuals do not imply a complete simulation. This inventory describes the current build, rather than promising every possible extension.

#### Strategic warfare

**Working:** war declarations, hostile local guards, wall damage, truces, alliances, union ownership, military research and equipment allocation.

**Incomplete:**
- War is a local-conflict placeholder. There are no campaign armies, marching routes, formations, tactical orders, siege simulation or military logistics.
- Defeating guards does not conquer a town. Ownership changes through diplomatic union, without occupation or a conquest campaign.
- Foreign towns do not autonomously wage wars, mobilize troops or provide allied military assistance.
- There is no naval combat or transport of armies by ship.

#### Defenders and resident training

**Working:** counted gear copies, exclusive allocation, research prerequisites, matching work clothes and resident roles.

**Incomplete:**
- Training is an immediate assignment enabled by a global technology, without lessons, experience, training time or individual proficiency.
- Defenders periodically damage nearby wolves. They lack a complete attack animation, combat decision system and coordinated patrols; this is not an army simulation.
- Residents have no health, injury or death model and cannot suffer military casualties.
- Ranged defenders are explicitly unavailable; resident ammunition, ranged targeting and squad commands are not implemented.

#### Population and settlement management

**Working:** an owned starting town, a resident roster, additional residents after unions and individual job/equipment assignment through NPC interaction.

**Incomplete:**
- Population comes from generated NPCs. There is no recruitment, immigration, birth, dismissal or population growth within an existing town.
- Housing does not determine capacity. Residents have no food needs, wages, happiness or household economy.
- There is no central roster interface for searching residents, comparing equipment or assigning jobs in bulk.
- Town layouts do not grow or develop new districts in response to population/economy changes.

#### Resident work and production

**Working:** trained residents add bounded stock/treasury batches every 20 seconds; ambassadors improve trust toward eligible discovered towns.

**Incomplete:**
- Work is a timer contribution, without physical collection, hauling, workplaces, input materials or production chains.
- Scholars currently contribute berries and engineers reeds; these stand in for specialized work rather than producing research or engineering projects.
- Traders create treasury income without moving goods or completing transactions. Ambassadors adjust trust without traveling or negotiating.
- There are no work queues, individual productivity progression or operating costs. Guild research changes batch size rather than creating a functioning guild economy.
- Unowned towns have no equivalent full resident production simulation. There is no offline production catch-up.

#### Trade, rare resources and town economy

**Working:** resource purchases/sales, conserved transaction stock/treasury, town specialties, trust restrictions, price perks and a town-specific weapon offer.

**Incomplete:**
- Prices are fixed rules, not supply-and-demand calculations. NPCs share a town market rather than individual shop inventories and budgets.
- There are no trade routes, caravans, imports, exports or an autonomous foreign economy.
- Rare specialty stock is initially allocated, without a complete renewable supply chain. Foreign stocks and treasury can be depleted.
- The repeatable town order has fixed requirements/rewards, without a changing contract market.
- Income and worker contributions have no supporting tax, wage or maintenance simulation.
- General trading covers resource goods, not a full market for crafted weapons and clothing.

#### Diplomacy and relations between settlements

**Working:** seeded inter-town relations, attitudes toward visitors, player trust, gifts, truces/alliances/unions, quest trust rewards and ambassadors.

**Incomplete:**
- Foreign relations are largely initialized values; rulers do not negotiate or change alliances through an autonomous diplomacy simulation.
- Treaties are immediate threshold/cost operations, without counteroffers, obligations, expiry or a negotiation process.
- Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.

#### City construction, walls and interiors

**Working:** houses/halls, a hall interaction scene, foreign rulers, an empty owned ruler seat and collidable wall segments with health.

**Incomplete:**
- Houses are static scenery, without walkable interiors or individual building functions. Hall interiors are interface scenes, not navigable levels.
- Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- There is no town founding, player construction of houses/workshops or building upgrade system.
- Wall damage supports local encounters, without a complete siege/destruction and rebuilding simulation.

#### NPC behavior and services

**Working:** weighted NPC types, authored introductions/lore, conversations keeping the speaker nearby, quest/service interaction and role-based options.

**Incomplete:**
- Movement wanders around a home point. There are no daily schedules, destination-based routines, inter-town travel or work animations tied to production.
- Movement updates are proximity-limited; distant residents do not receive a full behavior simulation.
- Individual relationship memory, changing personal stories and reactions to ongoing world events are absent.
- Residents generally expose town services rather than fully separate personal shops, teachers and service economies.

#### Quests and quest tracking

**Working:** selective quest givers, 11 authored templates, persistent acceptance/completion, progress baselines, delivery consumption, one-time rewards, recipe/knowledge unlocks, journal and tracking.

**Incomplete:**
- Each NPC offers at most one nonrepeatable quest. There are no chains, refreshes, branching outcomes or generated quests responding to actual town needs.
- Objectives use a small set of counters/deliveries. Crafting objectives can count generic crafts rather than a particular commissioned item.
- There are no multi-stage objectives, failure/expiry rules or abandonment mechanism.
- Tracking provides progress and giver information/distance, without a complete objective waypoint or route system.
- Rewards/unlocks do not form a fully balanced long-term campaign; multiple NPCs can offer the same template.

#### Skills, research and technology exchange

**Working:** milestone-gated base skills, seven additional research nodes, multi-criterion checks and town teaching for specialized material/technology access. Hall/skills menus share live lesson requirements, readiness, completion and numeric practice progress; learned lessons cannot be performed again through the interface.

**Incomplete:**
- Skills are unlocked flags, without levels, experience or specialization progression. Several milestones are generic activity counters.
- Research/lessons complete immediately; scholars, workshops and teachers do not supply research time or production.
- Town technologies are defined by type, without evolving discoveries, research competition or dynamic knowledge exchange.
- The summary now provides a dependency graph for base skills, resident research, activity counters and quest/town technologies. Automatic campaign guidance and a complete crafting-recipe dependency graph are still absent.
- Unlock pacing/economic balance have functional checks, not comprehensive long-play validation.

#### Crafting, equipment and character progression

**Working:** costs/unlock checks, expanded recipes, counted weapons/clothes, outfit perks, swaps anywhere and five backgrounds with bonuses/penalties.

**Incomplete:**
- Crafting is instant and available anywhere, without stations, crafting time, queues or production facilities.
- Tools/vessels are mostly ownership flags rather than separate world/inventory instances.
- Clothing is one full outfit, without independent garment/accessory slots. Equipment copies have no individual quality or instance properties.
- There are no storage chests, warehouse transfers or complete equipment management for multiple residents.
- Backgrounds provide fixed modifiers, without deeper individual progression or background-specific quest arcs.

#### Player combat and enemy behavior

**Working:** timed directional melee hits, visible swings, bow projectiles, attack windups, knockback, player health/protection, persistent defeated IDs and loot.

**Incomplete:**
- Melee damage uses range/cone checks at a fixed attack phase, not collision with the animated blade throughout its trajectory.
- Enemies chase/attack/return locally, without wolf pack tactics or broader encounter coordination.
- Weapon variety mostly changes numbers and a few attack categories, without a complete distinct move set per weapon.
- There is no blocking, parrying, combo or stamina system, or complete injury/status-effect model.
- Combat aboard vessels and underwater is unavailable.

#### Survival, healing and respawning

**Working:** food/potion healing, recovery near peaceful towns/campfires and death relocation with a coin penalty.

**Incomplete:**
- Food is a healing/crafting resource; hunger, thirst, sleep and temperature are not simulated.
- Campfires provide local healing rather than a complete camping system.
- Death resets health and removes coins, without a corpse, recovery expedition or persistent injury state.

#### Boats, ships and submarines

**Working:** craft/unlock requirements, vessel selection, distinct rendering/speeds, seated pilot, water travel, hull repair and submarine access.

**Incomplete:**
- Vessels are travel modes activated on water, not independent persistent craft with boarding, docking or storage at a world location.
- There are no crews, passengers, cargo holds, fuel costs or shipyard construction.
- Movement uses speed multipliers, without inertia, buoyancy, wind, currents or steering physics.
- Hull state is shared rather than tracked per vessel instance.

#### Underwater exploration and camera

**Working:** underwater map layer, dive/surface switching, limited visibility, oxygen, reef hazards, repair, sonar and gated resources.

**Incomplete:**
- Diving switches between two flat layers; there is no continuous depth, elevation, ballast, pressure or three-dimensional navigation/camera.
- The camera remains an isometric world view, rather than a separate depth-aware exploration system.
- Sonar reveals a disk of water cells, without obstacle-aware sensing, target identification or an energy/noise model.
- There are no walkable wreck interiors or a complete underwater ecosystem/encounter system.
- Sonar cooldown is runtime state and does not survive reload.

#### World generation, biomes and resources

**Working:** streamed deterministic chunks, forest/shore/shallow/deep water, surface/underwater resources, weighted settlement types and NPC rarity.

**Incomplete:**
- Lakes/coastline use predefined large-scale shapes with seeded variation, not general continent/biome generation.
- Settlements occupy a finite seeded region around fixed landmarks. Distant exploration generates terrain, not an endlessly expanding settlement network.
- Roads comprise local paths and one predefined inter-town connection, without a complete generated network.
- Harvested resources remain depleted; regrowth and renewable deposit management are absent.
- Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

#### Collision and navigation

**Working:** bounded substep movement, obstacle/wall collision, sliding, water restrictions and local A* plans.

**Incomplete:**
- Collision is a two-dimensional radius test against simplified obstacles, not shape-accurate geometry or rigid bodies.
- Actors have no complete body-to-body collision model; gravity and elevation are absent.
- Navigation searches a bounded area of loaded terrain and approaches distant goals in short segments, rather than planning full world routes for caravans/armies.
- There is no hierarchical routing or complete handling of distant blocked/unloaded destinations.

#### Map and exploration

**Working:** persistent fog on both layers, terrain, town miniatures, partially revealed world features/lakes, roads and save-preview maps.

**Incomplete:**
- Surface exploration follows the viewport, without line-of-sight occlusion. Window size/zoom affect exploration counters.
- Cartography is a cached coarse raster, without continuous detail at every scale.
- There are no manual annotations, route planning, strategic orders or complete political/economic overlays.
- Road detail inherits the incomplete world road network.

#### Graphics, animation and audio

**Working:** isometric atlas, detailed roof/wall planes, painted shadows, movement frames, outfit colors, weapon effects and distinct vessel drawings. Interface text is 1.25 times its previous effective size, rasterized at matching integer sizes. Menus use rounder buttons and four randomly selected procedural landscapes.

**Incomplete:**
- Sprites/drawing primitives do not provide real 3D buildings, geometric depth, dynamic lighting or physical shadows.
- Houses share limited variants/repeated layouts. NPCs share limited animation; complete job, interaction and combat animation sets are absent.
- Water graphics do not simulate waves, vessel wakes or physically changing shores.
- Music, sound effects and spatial audio are not implemented.

#### Menus, controls and save management

**Working:** main menu/settings, background selection, save creation/selection/summary, map previews and confirmed deletion into recoverable trash. ESC opens/resumes a gameplay pause or closes an active panel; pause Quit saves before returning to the lobby, and failed saves keep the session open for retry. Lobby Quit explicitly closes the application. Movement supports WASD and arrows; town-hall actions use P and settings application uses F5.

Saved-world and live paused-world summaries share icon-backed vertical item/skill lists, a dependency graph, scroll/pan controls and selected-node requirements. Switching preview map layers is read-only.

**Incomplete:**
- Controls are fixed; rebinding/controller interfaces and complete accessibility configuration are absent.
- Separate lists/NPC panels do not provide a consolidated settlement/roster dashboard.
- Save summaries show key progress, not a complete citizen equipment/job report or campaign history.
- Rename/copy/import/export and restoration of deleted saves/backups are not complete menu workflows; recovery requires filesystem access.
- Historical dialogue settings remain even though normal gameplay no longer requests free-form model conversations; the authored-gameplay boundary is documented above.

#### Persistence and content identity

**Working:** atomic saves, v1-v4 migration to v5 with backups, persistent quests/gear/roles/unlocks/relations/maps and harvested/defeated IDs.

**Incomplete:**
- Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. NPC/animal movement and active projectiles are also transient.
- There is no persistent calendar or offline simulation catch-up.
- Quest giver/resident identities rely on generated NPC indices. Version migrations do not provide general remapping for future settlement/NPC generation or content identity changes.
- Backup restoration remains manual.

#### Local model integration

**Working:** an asynchronous local model client for explicit LLM load benchmarks; normal introductions, quests and services are deterministic. `scripts/install.sh --with-model` builds the CPU server and downloads Qwen; `scripts/run.sh` starts the server and game together, checks readiness, reuses a ready `forest-npc` server and cleans up processes it owns. `--no-model` launches ordinary gameplay directly.

**Incomplete:**
- The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.

#### Performance, balance and release coverage

**Working:** automated mechanic tests, real-window menu smoke, compact logs and community/expedition benchmarks with model load.

**Incomplete:**
- Results cover short routes on the tested Linux machine, not long campaigns, maximum population/settlement counts or future army simulations.
- Timing/RSS logs are not GPU profiler coverage or combined game-plus-model memory accounting.
- Economy, rewards, unlock pacing and background balance lack broad long-play validation.
- Windows/macOS packaging, platform validation and a complete distribution workflow are not established by the Linux build.
- Linux source setup is available and tested, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.
- The 60 FPS target and accepted worst-window p95 ceiling of 30 ms constrain future changes; current measurements do not guarantee them for future systems.

Weapons intentionally have no durability, equipment swaps work anywhere, and ordinary NPC interactions use authored English content. Those are implemented design choices. Multiplayer and a space/planet simulation are outside the current scope.

The tracked [TODO.md checklist](TODO.md) lists remaining work item by item. Remove completed items after implementation and validation, splitting partial completions so remaining scope is retained. Maintain it and this section alongside [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md) whenever implementation changes.

## Installation

### Requirements

**Linux x86_64 is the tested setup.** You need a C++20 compiler, CMake 3.20+, a build tool such as Make, Bash, curl, tar and Python 3. Optional model setup also needs Git and a C compiler. Playing requires a working graphical session and OpenGL drivers; this is a desktop game.

For example, install the basic tools on Ubuntu/Debian:

```sh
sudo apt install build-essential cmake curl git python3
```

Or on Fedora:

```sh
sudo dnf install gcc gcc-c++ make cmake curl git python3
```

The setup script downloads the official raylib 5.5 Linux x86_64 binary. On other platforms, provide a raylib 5+ installation discoverable by CMake; those setups have not been validated here. The installer does not run sudo or install system packages.

### Build and play

Clone the repository:

```sh
git clone https://github.com/kiquxd/forest-frontier.git
cd forest-frontier
```

For the complete setup, including the local model:

```sh
./scripts/install.sh --with-model
./scripts/run.sh
```

This downloads the game dependencies, builds the game and CPU inference server, downloads Qwen, and runs the game tests. The launcher then starts both processes. System tools listed above must already be installed.

For a smaller setup with all current gameplay mechanics and no inference server:

```sh
./scripts/install.sh
./scripts/run.sh --no-model
```

Setup downloads missing dependencies and the font, builds the game in Release mode and runs the tests. Completed downloads are reused. Saves and settings are preserved. Initial setup requires internet access; normal play is offline after setup.

Build parallelism defaults to four jobs. For a machine with less available memory:

```sh
BUILD_JOBS=2 ./scripts/install.sh
```

### Optional local model

To install the CPU-only llama.cpp server and **Qwen3-1.7B Q4_K_M**, then launch both processes:

```sh
./scripts/install.sh --with-model
./scripts/run.sh
```

The model download is approximately **1.11 GB**, plus server source and build files. An interrupted model download remains a `.part` file and can be resumed by rerunning setup. Allow additional RAM for inference; actual usage and performance depend on your machine and thread count. The model is optional for all current gameplay mechanics.

The joint launcher waits for a healthy `forest-npc` server, opens the game, and stops its own server when the game exits. A compatible server already running on the selected port is reused and left running. Default port: `8080`; model threads: `4`; startup timeout: `120` seconds.

```sh
LLM_PORT=8081 LLM_THREADS=2 ./scripts/run.sh --startup-timeout 180
./scripts/run.sh --model /path/to/another.gguf
./scripts/run.sh --no-model -- --width 1280 --height 800
./scripts/run.sh -- --fullscreen
```

### Using your own model

The inference client is **not tied to the Qwen architecture**: it calls llama.cpp's streaming `/v1/chat/completions` endpoint using the server alias `forest-npc`. You can supply another **instruction/chat GGUF supported by the pinned llama.cpp release**, with a working embedded chat template and support for system messages. Qwen3-1.7B is the tested default; compatibility and performance of other models have not been validated.

To use your own model without downloading Qwen:

```sh
./scripts/install.sh
./scripts/build-llama.sh
./scripts/run.sh --model /absolute/path/to/your-chat-model.gguf
```

The bundled server runs CPU-only, uses a 2048-token context and Jinja chat templates, and disables reasoning output. The client also passes `enable_thinking=false`, sampling options and streaming usage requests. These settings suit the tested Qwen setup; a replacement template must tolerate them. Base/completion models without a chat template are not drop-in replacements. To exercise generation rather than only load the model:

```sh
./scripts/run.sh --model /absolute/path/to/your-chat-model.gguf -- --benchmark 55 --benchmark-llm
```

You can also start a compatible llama-server yourself with the alias `forest-npc` and your required template/server options; the joint launcher will reuse it once healthy. Reuse checks the alias and health, not the underlying model identity. Ordinary gameplay remains authored regardless of the loaded model.

`LLAMA_SERVER`, `LLM_MODEL`, `GAME_BINARY` and `LLM_LOG_FILE` override the server binary, model, game binary and model log. Relative custom model/game paths are resolved from the calling directory. Pass game options after `--`; use `LLM_PORT` rather than forwarding `--llm-port` through the launcher.

Individual setup steps remain available:

```sh
./scripts/bootstrap.sh       # Game dependencies, build and tests
./scripts/build-llama.sh     # CPU-only server
./scripts/download-model.sh # Model download
./scripts/run-model.sh      # Standalone local server
```

Use `./scripts/install.sh --help` or `./scripts/run.sh --help` for setup/launcher options. `./build/forest_frontier --help` lists game options. Model startup errors point to a separate file in `logs/`. For port conflicts, stop your existing service or choose another `LLM_PORT`. If compilation runs out of memory, reduce `BUILD_JOBS`.

Saves, settings, models, downloaded dependencies, builds and logs are local and excluded from Git. The executable currently uses the source directory for content and default writable paths: **keep the checkout available and rebuild if you move it**. This is a source build, not a standalone packaged release.

## Tech Stuff

### Stack and structure

| Component | Purpose |
| --- | --- |
| C++20 | World, simulation, progression and UI |
| raylib 5.5 | Native window, input and rendering |
| CMake / CTest | Builds and regression tests |
| nlohmann/json 3.12.0 | Content, saves, settings and JSONL logs |
| cpp-httplib 0.20.0 | Local model HTTP client |
| llama.cpp b9982 (optional) | CPU inference server |
| Qwen3-1.7B Q4_K_M (optional) | Explicit LLM performance experiments |
| Noto Sans | UI font, with [font license](assets/FONT-LICENSE.txt) |
| Python 3 | Test harnesses, icon generation and performance summaries |

`src/` holds the game; `content/` holds JSON definitions for NPCs, weapons, outfits, backgrounds, quests, recipes and settlements. `assets/ui/` contains reproducible pixel-art PNG icons and their shared atlas. Regenerate them with `python3 scripts/generate-ui-icons.py`; no image library is required.

World generation runs on a worker; GPU work stays on the render thread. Chunk streaming, visibility culling and batched icons keep rendering work bounded. Text is baked at the sizes used by the UI and aligned to pixels. Model requests run outside the render loop and cannot change authoritative game state.

Pinned game dependencies are downloaded by setup. The optional server starts from the pinned llama.cpp release; an existing source checkout is preserved. Dependency projects and model distributions retain their own licenses: [raylib](https://github.com/raysan5/raylib), [nlohmann/json](https://github.com/nlohmann/json), [cpp-httplib](https://github.com/yhirose/cpp-httplib), [llama.cpp](https://github.com/ggml-org/llama.cpp), [Qwen GGUF distribution](https://huggingface.co/unsloth/Qwen3-1.7B-GGUF).

### Validation and performance

After changing code:

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

Tests cover world generation, gameplay, campaigns, quests/equipment, save compatibility, summaries, dialogue transport and launcher lifecycle. A real-window menu regression harness is available on Linux/X11 or XWayland:

```sh
python3 scripts/menu-smoke.py
```

For core development without a graphics dependency, fetch the JSON header if it is not already present, then configure a separate build:

```sh
mkdir -p third_party
test -s third_party/json.hpp || curl -fL https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp -o third_party/json.hpp
cmake -S . -B build-core -DFRONTIER_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core -j 4
ctest --test-dir build-core --output-on-failure
```

Performance targets are **60 FPS** and a worst-window frame p95 ceiling of **30 ms**. These are development targets, not a guarantee for every machine. Logs contain aggregate frame timings, memory and meaningful gameplay/model events rather than function-call traces.

Run a short windowed community benchmark without a model:

```sh
./scripts/run.sh --no-model -- --benchmark 55 --benchmark-community
python3 scripts/summarize-perf.py
```

Measure the same scenario with model generation enabled:

```sh
./scripts/run.sh -- --benchmark 55 --benchmark-community --benchmark-llm
python3 scripts/summarize-perf.py
```

Benchmarks use temporary game state and do not save their simulated progression. The lower-level `scripts/benchmark.sh` also provides uncapped baseline, gameplay and expedition scenarios; its LLM modes expect a server to be running already.

Implementation choices, validation results and unfinished systems are recorded in [ai/DECISIONS.md](ai/DECISIONS.md), [ai/PROGRESS.md](ai/PROGRESS.md) and [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md). The tracked work checklist is [TODO.md](TODO.md). Contribution rules are in [AGENTS.md](AGENTS.md). Keep content and NPC IDs stable, preserve existing saves, and keep public UI/content in English.

New and changed C++ code follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) and the repository [.clang-format](.clang-format). Existing code still needs a gradual style migration; the configuration does not mean all sources already comply. Install clang-format separately for development, then format changed source files with `clang-format -i --style=file path/to/file.cpp`. It is not required to build or play.
