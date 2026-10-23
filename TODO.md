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
- [ ] Casualties and captivity come from local encounters only. There is no campaign casualty simulation, prisoner transport/exchange or persistent injury model.
- [ ] Ranged autonomous defenders are unavailable; resident ammunition logistics, ranged targeting and squad commands are not implemented. Directly controlled residents can use ranged weapons.

## Population and settlement management

- [ ] There are no births, dismissal, household simulation, food upkeep, wages or happiness. Wanderers are generated at towns, without physically traveling there.
- [ ] Resident search, comparison tools, bulk selection and group commands are absent. Selection controls one resident at a time; residents without work orders retain their last position.
- [ ] Housing is a capacity upgrade, not a freely placed residential building. Town layouts do not grow new residential districts in response to population changes.
- [ ] Wanderer history is bounded to 128 arrivals per campaign; declined/departed arrival recycling is not implemented.

## Resident work and production

- [ ] Workshop production is a timed management abstraction: no worker attendance, physical material transport, individual productivity, automatic priorities or operating costs.
- [ ] Resident orders remain local to explored home-town land, with terrain loaded independently of the camera. Mining other rare materials, underwater work, foreign workers, task priorities and specialized scholar/engineer output remain absent.
- [ ] Connected workshop output returns directly to its source warehouse when complete, without travel time or a hauling actor.
- [ ] Foreign towns have bounded abstract resource replenishment, not a full resident production simulation. There is no offline production catch-up.

## Trade, rare resources and town economy

- [ ] Prices are fixed rules, not supply-and-demand calculations. NPCs share town markets rather than individual inventories and budgets.
- [ ] Road freight carts have no assigned resident drivers, animal care, wear, traffic separation/loading queues, interception/combat or transport losses. They wait for required service terrain and blocked roads; there is no automatic road repair/rerouting. Specialist deliveries and timer shipments from older saves remain abstract; specialist delivery does not require a built road.
- [ ] Rare specialist orders create commissioned goods without simulating their production inputs. Foreign basic-stock replenishment and regional surplus purchases are abstract; there is no autonomous foreign economy.
- [ ] Closed money accounting prevents minting, not hoarding. There is no taxation, wage, maintenance or automatic redistribution policy guaranteeing liquidity for every town or preventing wealth concentration.
- [ ] General trading covers resource goods, not a complete market for crafted weapons and clothing. Inter-town shipments carry goods, not equipment copies.

## Diplomacy and relations between settlements

- [ ] Foreign relations are largely initialized values; rulers do not negotiate or change alliances through an autonomous diplomacy simulation.
- [ ] Treaties and surrender use immediate fixed terms, without counteroffers, treaty expiry or a negotiation process. Tribute covers local player-controlled towns; there is no autonomous foreign subjugation or state-level political integration.
- [ ] Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- [ ] Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.

## City construction, walls and interiors

- [ ] Residential houses are static scenery without walkable interiors or individual household functions. Activity buildings provide access gates; hall interiors are interface scenes, not navigable levels.
- [ ] Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- [ ] There is no town founding, free residential/building placement or general building upgrade system. Territory expansion uses a fixed circular radius and automatically chosen safe activity-building sites; the hospital is a separate medical building, not a general upgrade framework.
- [ ] Wall damage supports local encounters, without a complete siege/destruction and rebuilding simulation.

## NPC behavior and services

- [ ] Foreign movement wanders around a home point. There are no daily schedules, destination-based routines, inter-town travel or work animations tied to production.
- [ ] Movement updates are proximity-limited; distant foreign NPCs do not receive a full behavior simulation.
- [ ] Individual relationship memory, changing personal stories and reactions to ongoing world events are absent.
- [ ] Residents generally expose town services rather than fully separate personal shops, teachers and service economies.

## Quests and quest tracking

- [ ] The template set is small: supplies, knowledge and a town-discovery survey. Branching chains, multi-stage story projects, commissioned-item objectives and a complete balanced campaign are absent.
- [ ] Need calculations use local stock plus pending orders/shipments, without production forecasts, citizen consumption or sophisticated request scheduling. Different towns can legitimately request the same goods.
- [ ] Accepted tasks have no failure/deadline consequences; abandonment is available, while automatic expiry currently affects only unaccepted deficit requests.
- [ ] The board does not provide a complete objective waypoint/route system or advanced filtering. Task history is capped at 2048 records; archival/recycling is not implemented.

## Skills, research and technology exchange

- [ ] Skills are unlocked flags, without levels, experience or specialization progression. Several milestones are generic activity counters.
- [ ] Research/lessons complete immediately; scholars, workshops and teachers do not supply research time or production.
- [ ] Town technologies are defined by type, without evolving discoveries, research competition or dynamic knowledge exchange.
- [ ] The dependency tree covers base skills, resident research, activity counters and town/project technologies. Automatic campaign guidance and a complete crafting-recipe dependency graph are absent.
- [ ] Unlock pacing and the new production/economy/population loop have functional checks, not comprehensive long-play balance validation.

## Crafting, equipment and character progression

- [ ] Tools/vessels are mostly per-resident ownership flags, not separate world instances. Duplicate copies can be stored in warehouses but one resident carries at most one of each flagged tool/vessel.
- [ ] Clothing is one full outfit, without independent garment/accessory slots. Equipment copies have no individual quality or instance properties.
- [ ] Warehouse transfers lack filters, bulk equipment loadouts and a full logistics/automation interface; equipment cannot yet be shipped between towns through the shipment menu.
- [ ] State traits provide fixed modifiers, without evolving specialization or trait-specific progression.

## Player combat and enemy behavior

- [ ] Melee damage uses range/cone checks at a fixed attack phase, not collision with the animated blade throughout its trajectory.
- [ ] Enemies chase/attack/return locally, without wolf pack tactics or broader encounter coordination.
- [ ] Weapon variety mostly changes numbers and a few attack categories, without a complete distinct move set per weapon.
- [ ] There is no blocking, parrying, combo or stamina system, or complete injury/status-effect model.
- [ ] Combat aboard vessels and underwater is unavailable.

## Survival, healing and respawning

- [ ] Food is a healing/crafting resource; hunger, thirst, sleep and temperature are not simulated.
- [ ] Campfires provide local healing rather than a complete camping system.
- [ ] Medical treatment is immediate, without treatment queues, staffing, disease or persistent injuries. There is no recoverable corpse/recovery expedition or complete campaign defeat/recovery flow when no controllable residents remain.

## Boats, ships and submarines

- [ ] Vessels are travel modes activated on water, not independent persistent craft with boarding, docking or storage at a world location.
- [ ] There are no crews, passengers, cargo holds, fuel costs or a physical shipyard assembly process.
- [ ] Movement uses speed multipliers, without inertia, buoyancy, wind, currents or steering physics.
- [ ] Hull is tracked per resident, shared across their vessel modes, rather than per independent vessel instance.

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
- [ ] Wild harvested sites stay depleted. New planted sites use automatically selected town-edge locations and simple shared, nonblocking sprites; manual placement, fertility/seasons and renewable mineral extraction are absent. Growth-menu plots, including older saves, remain abstract management timers.
- [ ] Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- [ ] Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

## Collision geometry

- [ ] Body contacts use local disc separation rather than a general rigid-body simulation. Vessels use radius footprints rather than a separate collision hull for each persistent craft.

## Map and exploration

- [ ] Cartography uses scale-dependent cached raster detail and exploration cells, not an unlimited-resolution geographic model.
- [ ] Political/economic layers show discovered towns and remembered deposits; complete regional borders, production forecasts and strategic orders await the deferred resident-command/navigation systems.
- [ ] Road planning supports mouse or keyboard plotting, with at most 64 points and 128 roads. A route must be fully explored and paid for in one operation; controller plotting and detailed road editing are absent.

## Graphics, animation and audio

- [ ] Buildings have twelve facade/detail variants and fade when they occlude the player; interiors, more architectural styles and complete profession/interaction animation sets remain incomplete.
- [ ] Water has animated ripples, foam and vessel wakes; physical waves, tides and changing shores are not simulated.
- [ ] Audio provides three synthesized music loops and event/ambient effects with independent volume controls and distance/pan for nearby hits, wolves and fires; a larger soundtrack, a full ambient mixing system and smooth soundtrack crossfades are not implemented.

## Menus, controls and save management

- [ ] Gamepad support covers basic movement/actions and legacy directional menu navigation, without complete controller mappings/remapping/text entry. Screen-reader support and color customization are absent.
- [ ] Campaign history retains the most recent 512 significant events. It does not reconstruct older events or provide a complete searchable campaign timeline.

## Persistence and content identity

- [ ] Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. Foreign NPC/animal movement and active projectiles are also transient.
- [ ] Growing, deliveries and tribute use the persisted simulation clock; production uses a separate persisted active-world runtime clock. There is no calendar or offline progression; production does not run for closed worlds or after application shutdown.
- [ ] Generated resident identities still rely on stable NPC indices. Migrations do not provide general remapping for future settlement/NPC generation or content identity changes.

## Local model integration

- [ ] The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- [ ] Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.

## Release and distribution

- [ ] macOS arm64 gameplay/window/audio validation remains manual; Windows support, macOS app packaging/signing and a complete distribution workflow are not established. The optional model server is not validated on macOS.
- [ ] Linux and macOS arm64 source setup are available, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- [ ] Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.

## Navigation and autonomous movement (deferred)

- [ ] Unrestricted distant destinations, autonomous road discovery and armies remain deferred pending broader owned/foreign resident and profession command systems. Worker orders stay local; freight follows existing explicit roads, with service terrain loaded independently of the camera.
- [ ] Collision-aware local connectors use bounded loaded-terrain searches. Freight selects a deterministic existing-road graph route, without general off-road or hierarchical world routing.
- [ ] There is no hierarchical routing or automatic handling of distant blocked destinations.
- [ ] Off-camera work/freight use a bounded cache of 512 service chunks and at most 256 queued events per simulation step. Excess demand can delay work; missing terrain causes preparation waits without free catch-up output. Work and freight pause in menus and closed worlds; offline progression is absent.
