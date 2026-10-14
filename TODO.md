# Incomplete systems checklist

Tracked development inventory for the current build. Each unchecked item describes an existing limitation, not a promise to implement every possible extension. See [README Current limits](README.md#current-limits) for working behavior and [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md) for the detailed audit.

Remove an item only after its full scope is implemented and validated. Split it into smaller items if only part is completed. Update README Current limits and ai/PLACEHOLDERS.md in the same change; intentional design choices do not belong in this checklist. Git history retains completed items.

## Strategic warfare

- [ ] War is a local-conflict placeholder. There are no campaign armies, marching routes, formations, tactical orders, siege simulation or military logistics.
- [ ] Defeating guards does not conquer a town. Ownership changes through diplomatic union, without occupation or a conquest campaign.
- [ ] Foreign towns do not autonomously wage wars, mobilize troops or provide allied military assistance.
- [ ] There is no naval combat or transport of armies by ship.

## Defenders and resident training

- [ ] Training is an immediate assignment enabled by technology and access to a local/road-connected school, without lessons, experience, training time or individual proficiency.
- [ ] Defenders periodically damage nearby wolves and hostile guards. They lack a complete attack animation, combat decision system and coordinated patrols; this is not an army simulation.
- [ ] Resident casualties/capture currently come from local defender encounters only. There is no campaign casualty simulation, prisoner transport/exchange, player captivity or resident medical treatment system.
- [ ] Ranged defenders are explicitly unavailable; resident ammunition, ranged targeting and squad commands are not implemented.

## Population and settlement management

- [ ] Population comes from generated NPCs. There is no recruitment, immigration, birth, dismissal or population growth within an existing town.
- [ ] Housing does not determine capacity. Residents have no food needs, wages, happiness or household economy.
- [ ] The town roster supports individual job/equipment assignment; resident search, comparison tools and bulk assignment are absent.
- [ ] Town layouts do not grow or develop new districts in response to population/economy changes.

## Resident work and production

- [ ] Work checks required local/road-connected facilities but remains a timer contribution, without physical collection, hauling, attendance at workplaces, input materials or production chains.
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
- [ ] Treaties and surrender use immediate fixed terms, without counteroffers, treaty expiry or a negotiation process. Tribute covers local player-controlled towns; there is no autonomous foreign subjugation or state-level political integration.
- [ ] Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- [ ] Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.

## City construction, walls and interiors

- [ ] Residential houses are static scenery without walkable interiors or individual household functions. Activity buildings provide access gates; hall interiors are interface scenes, not navigable levels.
- [ ] Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- [ ] There is no town founding, player construction of residential houses, free building placement or building upgrade system. Territory expansion uses a fixed circular radius and automatically chosen safe activity-building sites.
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

- [ ] Craft outputs are immediate, followed by a persistent rarity-scaled global cooldown. There are no timed production queues, reserved-input jobs or simultaneous per-building workshop queues.
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
- [ ] Player death resets health, destroys the equipped weapon and removes coins, without player captivity, a recoverable corpse, recovery expedition or persistent injury state.

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
- [ ] Player roads/bridges are manually plotted complete routes, without maintenance, demolition, traffic capacity or territorial right-of-way negotiations. Local town paths remain generated scenery; inter-town roads are not generated automatically.
- [ ] Harvested resources remain depleted; regrowth and renewable deposit management are absent.
- [ ] Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- [ ] Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

## Collision geometry

- [ ] Body contacts use local disc separation rather than a general rigid-body simulation. Vessels use radius footprints rather than a separate collision hull for each persistent craft.


## Map and exploration

- [ ] Cartography uses scale-dependent cached raster detail and exploration cells, not an unlimited-resolution geographic model.
- [ ] Political/economic layers show discovered towns and remembered deposits; complete regional borders, production forecasts and strategic orders await the deferred resident-command/navigation systems.
- [ ] Road planning is a surface-map mouse interface with at most 64 points and 128 roads. A route must be fully explored and paid for in one operation; alternate keyboard/controller plotting and detailed road editing are absent.


## Graphics, animation and audio

- [ ] Buildings have twelve facade/detail variants and fade when they occlude the player; interiors, more architectural styles and complete profession/interaction animation sets remain incomplete.
- [ ] Water has animated ripples, foam and vessel wakes; physical waves, tides and changing shores are not simulated.
- [ ] Audio provides three synthesized music loops and event/ambient effects with independent volume controls and distance/pan for nearby hits, wolves and fires; a larger soundtrack, a full ambient mixing system and smooth soundtrack crossfades are not implemented.


## Menus, controls and save management

- [ ] Gamepad support covers basic movement/actions and directional menu navigation, without complete controller mappings, remapping or text entry. Accessibility has text size and tree contrast, without a complete keyboard-only interface, screen-reader or color customization.
- [ ] Campaign history retains the most recent 512 creation, discovery, quest, unlock and treaty events. It does not reconstruct events from older saves or provide a complete searchable campaign timeline.

## Persistence and content identity

- [ ] Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. NPC/animal movement and active projectiles are also transient.
- [ ] A persistent simulation clock drives crafting cooldowns and tribute, but there is no calendar, offline progression or simulation catch-up.
- [ ] Quest giver/resident identities rely on generated NPC indices. Version migrations do not provide general remapping for future settlement/NPC generation or content identity changes.

## Local model integration

- [ ] The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- [ ] Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.

## Release and distribution

- [ ] Windows/macOS packaging, platform validation and a complete distribution workflow are not established by the Linux build.
- [ ] Linux source setup is available and tested, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- [ ] Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.

## Navigation and autonomous movement (deferred)

- [ ] Full destination orders and route planning are deferred until command systems exist for owned and foreign residents/professions, caravans and armies; current job/equipment assignments are timer-based prototypes, not this prerequisite.
- [ ] Navigation searches a bounded area of loaded terrain and approaches distant goals in short segments, rather than planning full world routes for caravans/armies.
- [ ] There is no hierarchical routing or complete handling of distant blocked/unloaded destinations.
