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

Strategic warfare is a placeholder: there are local hostile guards and diplomatic war states, but no marching armies, sieges, conquest or naval combat. Resident work is a simplified periodic simulation. Walls use fixed layouts; there is no freeform city builder. Underwater exploration uses a separate flat layer rather than a 3D depth simulation. Markets use fixed pricing, and quests have no branching chains or regeneration.

The full inventory of incomplete systems is maintained in [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md).

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

Implementation choices, validation results and unfinished systems are recorded in [ai/DECISIONS.md](ai/DECISIONS.md), [ai/PROGRESS.md](ai/PROGRESS.md) and [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md). Contribution rules are in [AGENTS.md](AGENTS.md). Keep content and NPC IDs stable, preserve existing saves, and keep public UI/content in English.
