# Incomplete systems checklist

Tracked development inventory for the current build. Each unchecked item describes an existing limitation, not a promise to implement every possible extension. See [README Current limits](README.md#current-limits) for working behavior and [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md) for the detailed audit.

Remove an item only after its full scope is implemented and validated. Split it into smaller items if only part is completed. Update README Current limits and ai/PLACEHOLDERS.md in the same change; intentional design choices do not belong in this checklist. Git history retains completed items.

## Strategic warfare

- [ ] War is a local-conflict placeholder. There are no campaign armies, marching routes, formations, tactical orders, siege simulation or military logistics.
- [ ] Defeating guards does not conquer a town. Ownership changes through diplomatic union, without occupation or a conquest campaign.
- [ ] Foreign towns do not autonomously wage wars, mobilize troops or provide allied military assistance.
- [ ] There is no naval combat or transport of armies by ship.

## Defenders and resident training

- [ ] Training is an immediate assignment enabled by a global technology, without lessons, experience, training time or individual proficiency.
- [ ] Defenders periodically damage nearby wolves. They lack a complete attack animation, combat decision system and coordinated patrols; this is not an army simulation.
- [ ] Residents have no health, injury or death model and cannot suffer military casualties.
- [ ] Ranged defenders are explicitly unavailable; resident ammunition, ranged targeting and squad commands are not implemented.

## Population and settlement management

- [ ] Population comes from generated NPCs. There is no recruitment, immigration, birth, dismissal or population growth within an existing town.
- [ ] Housing does not determine capacity. Residents have no food needs, wages, happiness or household economy.
- [ ] There is no central roster interface for searching residents, comparing equipment or assigning jobs in bulk.
- [ ] Town layouts do not grow or develop new districts in response to population/economy changes.

## Resident work and production

- [ ] Work is a timer contribution, without physical collection, hauling, workplaces, input materials or production chains.
- [ ] Scholars currently contribute berries and engineers reeds; these stand in for specialized work rather than producing research or engineering projects.
- [ ] Traders create treasury income without moving goods or completing transactions. Ambassadors adjust trust without traveling or negotiating.
- [ ] There are no work queues, individual productivity progression or operating costs. Guild research changes batch size rather than creating a functioning guild economy.
- [ ] Unowned towns have no equivalent full resident production simulation. There is no offline production catch-up.

## Trade, rare resources and town economy

- [ ] Prices are fixed rules, not supply-and-demand calculations. NPCs share a town market rather than individual shop inventories and budgets.
- [ ] There are no trade routes, caravans, imports, exports or an autonomous foreign economy.
- [ ] Rare specialty stock is initially allocated, without a complete renewable supply chain. Foreign stocks and treasury can be depleted.
- [ ] The repeatable town order has fixed requirements/rewards, without a changing contract market.
- [ ] Income and worker contributions have no supporting tax, wage or maintenance simulation.
- [ ] General trading covers resource goods, not a full market for crafted weapons and clothing.

## Diplomacy and relations between settlements

- [ ] Foreign relations are largely initialized values; rulers do not negotiate or change alliances through an autonomous diplomacy simulation.
- [ ] Treaties are immediate threshold/cost operations, without counteroffers, obligations, expiry or a negotiation process.
- [ ] Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- [ ] Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.

## City construction, walls and interiors

- [ ] Houses are static scenery, without walkable interiors or individual building functions. Hall interiors are interface scenes, not navigable levels.
- [ ] Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- [ ] There is no town founding, player construction of houses/workshops or building upgrade system.
- [ ] Wall damage supports local encounters, without a complete siege/destruction and rebuilding simulation.

## NPC behavior and services

- [ ] Movement wanders around a home point. There are no daily schedules, destination-based routines, inter-town travel or work animations tied to production.
- [ ] Movement updates are proximity-limited; distant residents do not receive a full behavior simulation.
- [ ] Individual relationship memory, changing personal stories and reactions to ongoing world events are absent.
- [ ] Residents generally expose town services rather than fully separate personal shops, teachers and service economies.

## Quests and quest tracking

- [ ] Each NPC offers at most one nonrepeatable quest. There are no chains, refreshes, branching outcomes or generated quests responding to actual town needs.
- [ ] Objectives use a small set of counters/deliveries. Crafting objectives can count generic crafts rather than a particular commissioned item.
- [ ] There are no multi-stage objectives, failure/expiry rules or abandonment mechanism.
- [ ] Tracking provides progress and giver information/distance, without a complete objective waypoint or route system.
- [ ] Rewards/unlocks do not form a fully balanced long-term campaign; multiple NPCs can offer the same template.

## Skills, research and technology exchange

- [ ] Skills are unlocked flags, without levels, experience or specialization progression. Several milestones are generic activity counters.
- [ ] Research/lessons complete immediately; scholars, workshops and teachers do not supply research time or production.
- [ ] Town technologies are defined by type, without evolving discoveries, research competition or dynamic knowledge exchange.
- [ ] The summary now provides a dependency graph for base skills, resident research, activity counters and quest/town technologies. Automatic campaign guidance and a complete crafting-recipe dependency graph are still absent.
- [ ] Unlock pacing/economic balance have functional checks, not comprehensive long-play validation.

## Crafting, equipment and character progression

- [ ] Crafting is instant and available anywhere, without stations, crafting time, queues or production facilities.
- [ ] Tools/vessels are mostly ownership flags rather than separate world/inventory instances.
- [ ] Clothing is one full outfit, without independent garment/accessory slots. Equipment copies have no individual quality or instance properties.
- [ ] There are no storage chests, warehouse transfers or complete equipment management for multiple residents.
- [ ] Backgrounds provide fixed modifiers, without deeper individual progression or background-specific quest arcs.

## Player combat and enemy behavior

- [ ] Melee damage uses range/cone checks at a fixed attack phase, not collision with the animated blade throughout its trajectory.
- [ ] Enemies chase/attack/return locally, without wolf pack tactics or broader encounter coordination.
- [ ] Weapon variety mostly changes numbers and a few attack categories, without a complete distinct move set per weapon.
- [ ] There is no blocking, parrying, combo or stamina system, or complete injury/status-effect model.
- [ ] Combat aboard vessels and underwater is unavailable.

## Survival, healing and respawning

- [ ] Food is a healing/crafting resource; hunger, thirst, sleep and temperature are not simulated.
- [ ] Campfires provide local healing rather than a complete camping system.
- [ ] Death resets health and removes coins, without a corpse, recovery expedition or persistent injury state.

## Boats, ships and submarines

- [ ] Vessels are travel modes activated on water, not independent persistent craft with boarding, docking or storage at a world location.
- [ ] There are no crews, passengers, cargo holds, fuel costs or shipyard construction.
- [ ] Movement uses speed multipliers, without inertia, buoyancy, wind, currents or steering physics.
- [ ] Hull state is shared rather than tracked per vessel instance.

## Underwater exploration and camera

- [ ] Diving switches between two flat layers; there is no continuous depth, elevation, ballast, pressure or three-dimensional navigation/camera.
- [ ] The camera remains an isometric world view, rather than a separate depth-aware exploration system.
- [ ] Sonar reveals a disk of water cells, without obstacle-aware sensing, target identification or an energy/noise model.
- [ ] There are no walkable wreck interiors or a complete underwater ecosystem/encounter system.
- [ ] Sonar cooldown is runtime state and does not survive reload.

## World generation, biomes and resources

- [ ] Lakes/coastline use predefined large-scale shapes with seeded variation, not general continent/biome generation.
- [ ] Settlements occupy a finite seeded region around fixed landmarks. Distant exploration generates terrain, not an endlessly expanding settlement network.
- [ ] Roads comprise local paths and one predefined inter-town connection, without a complete generated network.
- [ ] Harvested resources remain depleted; regrowth and renewable deposit management are absent.
- [ ] Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- [ ] Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

## Collision and navigation

- [ ] Collision is a two-dimensional radius test against simplified obstacles, not shape-accurate geometry or rigid bodies.
- [ ] Actors have no complete body-to-body collision model; gravity and elevation are absent.
- [ ] Navigation searches a bounded area of loaded terrain and approaches distant goals in short segments, rather than planning full world routes for caravans/armies.
- [ ] There is no hierarchical routing or complete handling of distant blocked/unloaded destinations.

## Map and exploration

- [ ] Surface exploration follows the viewport, without line-of-sight occlusion. Window size/zoom affect exploration counters.
- [ ] Cartography is a cached coarse raster, without continuous detail at every scale.
- [ ] There are no manual annotations, route planning, strategic orders or complete political/economic overlays.
- [ ] Road detail inherits the incomplete world road network.

## Graphics, animation and audio

- [ ] Sprites/drawing primitives do not provide real 3D buildings, geometric depth, dynamic lighting or physical shadows.
- [ ] Houses share limited variants/repeated layouts. NPCs share limited animation; complete job, interaction and combat animation sets are absent.
- [ ] Water graphics do not simulate waves, vessel wakes or physically changing shores.
- [ ] Music, sound effects and spatial audio are not implemented.

## Menus, controls and save management

- [ ] Controls are fixed; rebinding/controller interfaces and complete accessibility configuration are absent.
- [ ] Separate lists/NPC panels do not provide a consolidated settlement/roster dashboard.
- [ ] Save summaries show key progress, not a complete citizen equipment/job report or campaign history.
- [ ] Rename/copy/import/export and restoration of deleted saves/backups are not complete menu workflows; recovery requires filesystem access.
- [ ] Historical dialogue settings remain even though normal gameplay no longer requests free-form model conversations; the public README now makes the authored-gameplay boundary explicit.

## Persistence and content identity

- [ ] Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. NPC/animal movement and active projectiles are also transient.
- [ ] There is no persistent calendar or offline simulation catch-up.
- [ ] Quest giver/resident identities rely on generated NPC indices. Version migrations do not provide general remapping for future settlement/NPC generation or content identity changes.
- [ ] Backup restoration remains manual.

## Local model integration

- [ ] The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- [ ] Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.

## Performance, balance and release coverage

- [ ] Results cover short routes on the tested Linux machine, not long campaigns, maximum population/settlement counts or future army simulations.
- [ ] Timing/RSS logs are not GPU profiler coverage or combined game-plus-model memory accounting.
- [ ] Economy, rewards, unlock pacing and background balance lack broad long-play validation.
- [ ] Windows/macOS packaging, platform validation and a complete distribution workflow are not established by the Linux build.
- [ ] Linux source setup is available and tested, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- [ ] Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.
- [ ] The 60 FPS target and accepted worst-window p95 ceiling of 30 ms constrain future changes; current measurements do not guarantee them for future systems.

## C++ style migration

- [ ] Existing C++ sources and tests still use densely packed statements, inconsistent naming and header/include conventions. Migrate them to the Google C++ Style Guide and the repository .clang-format in reviewable changes; keep formatting changes separate from behavior changes, preserve saved-content IDs and validate any symbol renames.
