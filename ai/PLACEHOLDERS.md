# Incomplete systems and placeholders

Audit of the current 0.6 implementation. This inventory covers working prototypes, missing parts of requested mechanics and technical limitations. It is not a commitment to implement every possible extension. A working menu or prerequisite does not mean the underlying simulation is complete.

Keep this inventory current whenever mechanics change. Remove a limitation only after implementation and relevant validation. Deliberate scope choices belong in [DECISIONS.md](DECISIONS.md).

The public counterpart is the [README Current limits section](../README.md#current-limits). Keep every incomplete system and relevant technical limitation represented there in the same change; the README must describe the limitations directly rather than only link to this file.

The tracked item-by-item checklist is [TODO.md](../TODO.md). Remove validated completions there and synchronize this audit and README; split partial completions so remaining limitations are not lost. First-party C++ style migration is completed; dependency sources retain their upstream style.

## Strategic warfare

**Working:** war declarations, hostile local guards, wall damage, truces, alliances, union ownership, military research and equipment allocation.

**Incomplete:**
- War is a local-conflict placeholder. There are no campaign armies, marching routes, formations, tactical orders, siege simulation or military logistics.
- Defeating guards does not conquer a town. Ownership changes through diplomatic union, without occupation or a conquest campaign.
- Foreign towns do not autonomously wage wars, mobilize troops or provide allied military assistance.
- There is no naval combat or transport of armies by ship.

Sources: [gameplay.cpp](../src/gameplay.cpp), [adventure.cpp](../src/adventure.cpp).

## Defenders and resident training

**Working:** counted gear copies, exclusive allocation, research prerequisites, matching work clothes and resident roles.

**Incomplete:**
- Training is an immediate assignment enabled by a global technology, without lessons, experience, training time or individual proficiency.
- Defenders periodically damage nearby wolves. They lack a complete attack animation, combat decision system and coordinated patrols; this is not an army simulation.
- Residents have no health, injury or death model and cannot suffer military casualties.
- Ranged defenders are explicitly unavailable; resident ammunition, ranged targeting and squad commands are not implemented.

Sources: [adventure.cpp](../src/adventure.cpp), [gameplay.cpp](../src/gameplay.cpp).

## Population and settlement management

**Working:** an owned starting town, a resident roster, additional residents after unions and individual job/equipment assignment through NPC interaction.

**Incomplete:**
- Population comes from generated NPCs. There is no recruitment, immigration, birth, dismissal or population growth within an existing town.
- Housing does not determine capacity. Residents have no food needs, wages, happiness or household economy.
- There is no central roster interface for searching residents, comparing equipment or assigning jobs in bulk.
- Town layouts do not grow or develop new districts in response to population/economy changes.

Sources: [world.cpp](../src/world.cpp), [adventure.cpp](../src/adventure.cpp), [main.cpp](../src/main.cpp).

## Resident work and production

**Working:** trained residents add bounded stock/treasury batches every 20 seconds; ambassadors improve trust toward eligible discovered towns.

**Incomplete:**
- Work is a timer contribution, without physical collection, hauling, workplaces, input materials or production chains.
- Scholars currently contribute berries and engineers reeds; these stand in for specialized work rather than producing research or engineering projects.
- Traders create treasury income without moving goods or completing transactions. Ambassadors adjust trust without traveling or negotiating.
- There are no work queues, individual productivity progression or operating costs. Guild research changes batch size rather than creating a functioning guild economy.
- Unowned towns have no equivalent full resident production simulation. There is no offline production catch-up.

Source: [adventure.cpp](../src/adventure.cpp).

## Trade, rare resources and town economy

**Working:** resource purchases/sales, conserved transaction stock/treasury, town specialties, trust restrictions, price perks and a town-specific weapon offer.

**Incomplete:**
- Prices are fixed rules, not supply-and-demand calculations. NPCs share a town market rather than individual shop inventories and budgets.
- There are no trade routes, caravans, imports, exports or an autonomous foreign economy.
- Rare specialty stock is initially allocated, without a complete renewable supply chain. Foreign stocks and treasury can be depleted.
- The repeatable town order has fixed requirements/rewards, without a changing contract market.
- Income and worker contributions have no supporting tax, wage or maintenance simulation.
- General trading covers resource goods, not a full market for crafted weapons and clothing.

Source: [gameplay.cpp](../src/gameplay.cpp).

## Diplomacy and relations between settlements

**Working:** seeded inter-town relations, attitudes toward visitors, player trust, gifts, truces/alliances/unions, quest trust rewards and ambassadors.

**Incomplete:**
- Foreign relations are largely initialized values; rulers do not negotiate or change alliances through an autonomous diplomacy simulation.
- Treaties are immediate threshold/cost operations, without counteroffers, obligations, expiry or a negotiation process.
- Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.

Sources: [gameplay.cpp](../src/gameplay.cpp), [world.cpp](../src/world.cpp), [adventure.cpp](../src/adventure.cpp).

## City construction, walls and interiors

**Working:** houses/halls, a hall interaction scene, foreign rulers, an empty owned ruler seat and collidable wall segments with health.

**Incomplete:**
- Houses are static scenery, without walkable interiors or individual building functions. Hall interiors are interface scenes, not navigable levels.
- Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- There is no town founding, player construction of houses/workshops or building upgrade system.
- Wall damage supports local encounters, without a complete siege/destruction and rebuilding simulation.

Sources: [world.cpp](../src/world.cpp), [gameplay.cpp](../src/gameplay.cpp), [render.cpp](../src/render.cpp).

## NPC behavior and services

**Working:** weighted NPC types, authored introductions/lore, conversations keeping the speaker nearby, quest/service interaction and role-based options.

**Incomplete:**
- Movement wanders around a home point. There are no daily schedules, destination-based routines, inter-town travel or work animations tied to production.
- Movement updates are proximity-limited; distant residents do not receive a full behavior simulation.
- Individual relationship memory, changing personal stories and reactions to ongoing world events are absent.
- Residents generally expose town services rather than fully separate personal shops, teachers and service economies.

Sources: [world.cpp](../src/world.cpp), [main.cpp](../src/main.cpp).

## Quests and quest tracking

**Working:** selective quest givers, 11 authored templates, persistent acceptance/completion, progress baselines, delivery consumption, one-time rewards, recipe/knowledge unlocks, journal and tracking.

**Incomplete:**
- Each NPC offers at most one nonrepeatable quest. There are no chains, refreshes, branching outcomes or generated quests responding to actual town needs.
- Objectives use a small set of counters/deliveries. Crafting objectives can count generic crafts rather than a particular commissioned item.
- There are no multi-stage objectives, failure/expiry rules or abandonment mechanism.
- Tracking provides progress and giver information/distance, without a complete objective waypoint or route system.
- Rewards/unlocks do not form a fully balanced long-term campaign; multiple NPCs can offer the same template.

Sources: [adventure.cpp](../src/adventure.cpp), [content/quests](../content/quests), [render.cpp](../src/render.cpp).

## Skills, research and technology exchange

**Working:** milestone-gated base skills, seven additional research nodes, multi-criterion checks and town teaching for specialized material/technology access. Hall/skills menus share live lesson requirements, readiness, completion and numeric practice progress; learned lessons cannot be performed again through the interface.

**Incomplete:**
- Skills are unlocked flags, without levels, experience or specialization progression. Several milestones are generic activity counters.
- Research/lessons complete immediately; scholars, workshops and teachers do not supply research time or production.
- Town technologies are defined by type, without evolving discoveries, research competition or dynamic knowledge exchange.
- The summary now provides a dependency graph for base skills, resident research, activity counters and quest/town technologies. Automatic campaign guidance and a complete crafting-recipe dependency graph are still absent.
- Unlock pacing/economic balance have functional checks, not comprehensive long-play validation.

Sources: [gameplay.cpp](../src/gameplay.cpp), [adventure.cpp](../src/adventure.cpp), [render.cpp](../src/render.cpp).

## Crafting, equipment and character progression

**Working:** costs/unlock checks, expanded recipes, counted weapons/clothes, outfit perks, swaps anywhere and five backgrounds with bonuses/penalties.

**Incomplete:**
- Crafting is instant and available anywhere, without stations, crafting time, queues or production facilities.
- Tools/vessels are mostly ownership flags rather than separate world/inventory instances.
- Clothing is one full outfit, without independent garment/accessory slots. Equipment copies have no individual quality or instance properties.
- There are no storage chests, warehouse transfers or complete equipment management for multiple residents.
- Backgrounds provide fixed modifiers, without deeper individual progression or background-specific quest arcs.

Sources: [content.cpp](../src/content.cpp), [gameplay.cpp](../src/gameplay.cpp), [adventure.cpp](../src/adventure.cpp).

## Player combat and enemy behavior

**Working:** timed directional melee hits, visible swings, bow projectiles, attack windups, knockback, player health/protection, persistent defeated IDs and loot.

**Incomplete:**
- Melee damage uses range/cone checks at a fixed attack phase, not collision with the animated blade throughout its trajectory.
- Enemies chase/attack/return locally, without wolf pack tactics or broader encounter coordination.
- Weapon variety mostly changes numbers and a few attack categories, without a complete distinct move set per weapon.
- There is no blocking, parrying, combo or stamina system, or complete injury/status-effect model.
- Combat aboard vessels and underwater is unavailable.

Source: [gameplay.cpp](../src/gameplay.cpp).

## Survival, healing and respawning

**Working:** food/potion healing, recovery near peaceful towns/campfires and death relocation with a coin penalty.

**Incomplete:**
- Food is a healing/crafting resource; hunger, thirst, sleep and temperature are not simulated.
- Campfires provide local healing rather than a complete camping system.
- Death resets health and removes coins, without a corpse, recovery expedition or persistent injury state.

Source: [gameplay.cpp](../src/gameplay.cpp).

## Boats, ships and submarines

**Working:** craft/unlock requirements, vessel selection, distinct rendering/speeds, seated pilot, water travel, hull repair and submarine access.

**Incomplete:**
- Vessels are travel modes activated on water, not independent persistent craft with boarding, docking or storage at a world location.
- There are no crews, passengers, cargo holds, fuel costs or shipyard construction.
- Movement uses speed multipliers, without inertia, buoyancy, wind, currents or steering physics.
- Hull state is shared rather than tracked per vessel instance.

Sources: [gameplay.cpp](../src/gameplay.cpp), [render.cpp](../src/render.cpp).

## Underwater exploration and camera

**Working:** underwater map layer, dive/surface switching, limited visibility, oxygen, reef hazards, repair, sonar and gated resources.

**Incomplete:**
- Diving switches between two flat layers; there is no continuous depth, elevation, ballast, pressure or three-dimensional navigation/camera.
- The camera remains an isometric world view, rather than a separate depth-aware exploration system.
- Sonar reveals a disk of water cells, without obstacle-aware sensing, target identification or an energy/noise model.
- There are no walkable wreck interiors or a complete underwater ecosystem/encounter system.
- Sonar cooldown is runtime state and does not survive reload.

Sources: [gameplay.cpp](../src/gameplay.cpp), [render.cpp](../src/render.cpp).

## World generation, biomes and resources

**Working:** streamed deterministic chunks, forest/shore/shallow/deep water, surface/underwater resources, weighted settlement types and NPC rarity.

**Incomplete:**
- Lakes/coastline use predefined large-scale shapes with seeded variation, not general continent/biome generation.
- Settlements occupy a finite seeded region around fixed landmarks. Distant exploration generates terrain, not an endlessly expanding settlement network.
- Roads comprise local paths and one predefined inter-town connection, without a complete generated network.
- Harvested resources remain depleted; regrowth and renewable deposit management are absent.
- Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

Source: [world.cpp](../src/world.cpp).

## Collision and navigation

**Working:** bounded substep movement, obstacle/wall collision, sliding, water restrictions and local A* plans.

**Incomplete:**
- Collision is a two-dimensional radius test against simplified obstacles, not shape-accurate geometry or rigid bodies.
- Actors have no complete body-to-body collision model; gravity and elevation are absent.
- Navigation searches a bounded area of loaded terrain and approaches distant goals in short segments, rather than planning full world routes for caravans/armies.
- There is no hierarchical routing or complete handling of distant blocked/unloaded destinations.

Source: [world.cpp](../src/world.cpp).

## Map and exploration

**Working:** persistent fog on both layers, terrain, town miniatures, partially revealed world features/lakes, roads and save-preview maps.

**Incomplete:**
- Surface exploration follows the viewport, without line-of-sight occlusion. Window size/zoom affect exploration counters.
- Cartography is a cached coarse raster, without continuous detail at every scale.
- There are no manual annotations, route planning, strategic orders or complete political/economic overlays.
- Road detail inherits the incomplete world road network.

Sources: [cartography.cpp](../src/cartography.cpp), [gameplay.cpp](../src/gameplay.cpp), [menu.cpp](../src/menu.cpp).

## Graphics, animation and audio

**Working:** isometric atlas, detailed roof/wall planes, painted shadows, movement frames, outfit colors, weapon effects and distinct vessel drawings. Interface text is 1.25 times its previous effective size, rasterized at matching integer sizes. Menus use rounder buttons and four randomly selected procedural landscapes.

**Incomplete:**
- Sprites/drawing primitives do not provide real 3D buildings, geometric depth, dynamic lighting or physical shadows.
- Houses share limited variants/repeated layouts. NPCs share limited animation; complete job, interaction and combat animation sets are absent.
- Water graphics do not simulate waves, vessel wakes or physically changing shores.
- Music, sound effects and spatial audio are not implemented.

Source: [render.cpp](../src/render.cpp).

## Menus, controls and save management

**Working:** main menu/settings, background selection, save creation/selection/summary, map previews and confirmed deletion into recoverable trash. ESC opens/resumes a gameplay pause or closes an active panel; pause Quit saves before returning to the lobby, and failed saves keep the session open for retry. Lobby Quit explicitly closes the application. Movement supports WASD and arrows; town-hall actions use P and settings application uses F5.

Saved-world and live paused-world summaries share icon-backed vertical item/skill lists, a dependency graph, scroll/pan controls and selected-node requirements. Switching preview map layers is read-only.

**Incomplete:**
- Controls are fixed; rebinding/controller interfaces and complete accessibility configuration are absent.
- Separate lists/NPC panels do not provide a consolidated settlement/roster dashboard.
- Save summaries show key progress, not a complete citizen equipment/job report or campaign history.
- Rename/copy/import/export and restoration of deleted saves/backups are not complete menu workflows; recovery requires filesystem access.
- Historical dialogue settings remain even though normal gameplay no longer requests free-form model conversations; the public README now makes the authored-gameplay boundary explicit.

Sources: [menu.cpp](../src/menu.cpp), [save_slots.cpp](../src/save_slots.cpp), [main.cpp](../src/main.cpp), [README.md](../README.md).

## Persistence and content identity

**Working:** atomic saves, v1-v4 migration to v5 with backups, persistent quests/gear/roles/unlocks/relations/maps and harvested/defeated IDs.

**Incomplete:**
- Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. NPC/animal movement and active projectiles are also transient.
- There is no persistent calendar or offline simulation catch-up.
- Quest giver/resident identities rely on generated NPC indices. Version migrations do not provide general remapping for future settlement/NPC generation or content identity changes.
- Backup restoration remains manual.

Sources: [world.cpp](../src/world.cpp), [adventure.cpp](../src/adventure.cpp), [save_slots.cpp](../src/save_slots.cpp).

## Local model integration

**Working:** an asynchronous local model client for explicit LLM load benchmarks; normal introductions, quests and services are deterministic. `scripts/install.sh --with-model` builds the CPU server and downloads Qwen; `scripts/run.sh` starts the server and game together, checks readiness, reuses a ready `forest-npc` server and cleans up processes it owns. `--no-model` launches ordinary gameplay directly.

**Incomplete integration boundary:**
- The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.

Sources: [dialogue.cpp](../src/dialogue.cpp), [main.cpp](../src/main.cpp).

## Release and distribution

**Working:** Linux source setup downloads dependencies, builds the game and optional model server, and runs automated tests. The repository includes installation and launch instructions.

**Incomplete:**
- Windows/macOS packaging, platform validation and a complete distribution workflow are not established by the Linux build.
- Linux source setup is available and tested, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.

Sources: [perf.cpp](../src/perf.cpp), [PROGRESS.md](PROGRESS.md), [README.md](../README.md).

## Intentional choices, not unfinished mechanics

- Weapons do not wear out or disappear on attacks. Arrows and consumables may be spent.
- Player weapon/clothing swaps work anywhere; location restrictions are not pending work.
- Ordinary NPC interactions use authored English lore and authoritative quest/service rules. Removing unbounded free-form conversations was deliberate; restoring them is not a missing feature.
- English UI/content and windowed startup with explicit fullscreen are implemented choices.
- This is a local single-player game. Multiplayer and a full space/planet simulation are not implied deliverables from the original comparisons.

See [DECISIONS.md](DECISIONS.md) for the implementation decisions.
