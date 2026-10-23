# Incomplete systems and placeholders

Audit of the current 0.9 implementation. This inventory covers working prototypes, missing parts of requested mechanics and technical limitations. It is not a commitment to implement every possible extension. A working menu or prerequisite does not mean the underlying simulation is complete.

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

**Working:** Counted gear belongs to a resident bag or city warehouse. Directly controlled residents have persistent position, health, cargo, clothing and weapons; local defenders retain death/capture/ransom. Dead residents lose all carried equipment and cargo without drops; captives return with their reserved equipment. Infirmaries and hospitals treat residents on home-town territory.

**Incomplete:**

- Training is an immediate assignment enabled by technology and access to a local/road-connected school, without lessons, experience, training time or individual proficiency.
- Defenders periodically damage nearby wolves and hostile guards. They lack a complete attack animation, combat decision system and coordinated patrols; this is not an army simulation.
- Casualties and captivity come from local encounters only. There is no campaign casualty simulation, prisoner transport/exchange or persistent injury model.
- Ranged autonomous defenders are unavailable; resident ammunition logistics, ranged targeting and squad commands are not implemented. Directly controlled residents can use ranged weapons.

## Population and settlement management

**Working:** The first owned town starts with selectable residents rather than a separate hero. All active owned residents can be selected from State > Residents and explored with directly. Wanderers periodically appear in owned towns; acceptance consumes food and a housing place. Housing capacity can be expanded; walk a resident to another owned town and change their home there.

**Incomplete:**

- There are no births, dismissal, household simulation, food upkeep, wages or happiness. Wanderers are generated at towns, without physically traveling there.
- Resident search, comparison tools, bulk selection and group commands are absent. Selection controls one resident at a time; residents without work orders retain their last position.
- Housing is a capacity upgrade, not a freely placed residential building. Town layouts do not grow new residential districts in response to population changes.
- Wanderer history is bounded to 128 arrivals per campaign; declined/departed arrival recycling is not implemented.

## Resident work and production

**Working:** Production uses per-building FIFO queues, up to eight jobs per workshop, with reserved warehouse inputs and commissioning fees. Separate buildings operate in parallel. Forge construction is capped at max(1, floor(active town population / 6)); existing forges are retained after population loss. Jobs take 1/5/30/60/120 active-world minutes by rarity. A persisted production clock accumulates std::chrono::steady_clock elapsed time while the world is open, including management, Esc/settings, inactivity and minimized windows. Live FIFO rows show countdowns, progress bars and waiting/access/storage states. Simulation pauses do not stop production; returning to the lobby closes the world and suspends it. Output returns to the source city warehouse; cancellation refunds reserved inputs/fees once. Lost peaceful access suspends the queue. Old passive resident stock/coin generation is disabled in resident-control campaigns. Residents can receive repeating local gather/plant orders, including fishing with their own rod and the complete Fishing branch, and iron-ore mining with their own pickaxe, the complete Tools branch and Ironworking. These use actor-specific tools/cargo, shared state traits, manual cancellation, danger/conversation pauses, real warehouse returns and no passive resource or coin minting. Planters first fetch missing wood/berries from their warehouse into cargo, carry them to the site, and consume them only on successful planting; cancellation preserves carried supplies. Taking manual control cancels that resident's order; death/capture also cancels it. Resident orders continue outside the camera using separately requested service terrain; they pause with simulation or while required terrain is being prepared. The full recipe catalogue remains visible with case-insensitive name search, item PNGs and resource/skill/technology/building requirement PNGs. Missing counts or unlocks are muted, hovering explains them, and the same read-only quote controls both UI readiness and queue validation.

**Incomplete:**

- Workshop production is a timed management abstraction: no worker attendance, physical material transport, individual productivity, automatic priorities or operating costs.
- Resident orders remain local to explored home-town land, with terrain loaded independently of the camera. Mining other rare materials, underwater work, foreign workers, task priorities and specialized scholar/engineer output remain absent.
- Connected workshop output returns directly to its source warehouse when complete, without travel time or a hauling actor.
- Foreign towns have bounded abstract resource replenishment, not a full resident production simulation. There is no offline production catch-up.

## Trade, rare resources and town economy

**Working:** Each town has its own goods warehouse, product stock and treasury; state skills and treasury are shared. Transfer cargo locally, build counted road freight carts and send real goods between road-connected owned towns, or order specialist goods for coins with knowledge/trust gates. New internal transfers use physical carts; old timer shipments keep their saved behavior. A cart costs 20 city wood, 5 stone and 4 leather after the complete Construction branch, carries up to 40 goods, follows existing road polylines/bridges and uses bounded local connectors around town halls. Cargo is reserved once at dispatch and unloaded only on arrival; full storage retains the remainder and Return physically carries it back. Hostile endpoints/transit, nearby active-area threats and obstructions suspend travel. A main-thread event queue requests service terrain for off-camera workers and upcoming freight road sections without activating distant combat or ordinary NPC movement. No resident driver or equipment copy is allocated to a cart. Foreign deficit orders reserve the buyer's payment and pay only on physical delivery by a resident. State/city/regional funds, production fees and order escrow conserve the campaign money supply. Own-town market sales are replaced by warehouse transfers; own projects do not pay coins. A regional fund buys bounded surpluses and receives research/construction/treatment fees; foreign buyers keep a protected reserve.

**Incomplete:**

- Prices are fixed rules, not supply-and-demand calculations. NPCs share town markets rather than individual inventories and budgets.
- Road freight carts have no assigned resident drivers, animal care, wear, traffic separation/loading queues, interception/combat or transport losses. They wait for required service terrain and blocked roads; there is no automatic road repair/rerouting. Specialist deliveries and timer shipments from older saves remain abstract; specialist delivery does not require a built road.
- Rare specialist orders create commissioned goods without simulating their production inputs. Foreign basic-stock replenishment and regional surplus purchases are abstract; there is no autonomous foreign economy.
- Closed money accounting prevents minting, not hoarding. There is no taxation, wage, maintenance or automatic redistribution policy guaranteeing liquidity for every town or preventing wealth concentration.
- General trading covers resource goods, not a complete market for crafted weapons and clothing. Inter-town shipments carry goods, not equipment copies.

## Diplomacy and relations between settlements

**Working:** seeded inter-town relations, visitor attitudes, player trust, gifts, truces/alliances/unions, project trust rewards, surrender retaining local rule, recurring tribute and buying independence/previous allegiance. Unpaid tribute resumes local hostility.

**Incomplete:**

- Foreign relations are largely initialized values; rulers do not negotiate or change alliances through an autonomous diplomacy simulation.
- Treaties and surrender use immediate fixed terms, without counteroffers, treaty expiry or a negotiation process. Tribute covers local player-controlled towns; there is no autonomous foreign subjugation or state-level political integration.
- Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.

Sources: [gameplay.cpp](../src/gameplay.cpp), [world.cpp](../src/world.cpp), [adventure.cpp](../src/adventure.cpp).

## City construction, walls and interiors

**Working:** Town halls contain projects, orders and management. Circular territory expansion, collidable walls and eight activity-building types are available: workbench, forge, tailor, school, shipyard, alchemy, infirmary and hospital. Housing is a paid capacity upgrade. Complete player-plotted roads share facility access through peaceful transit towns; water sections are more expensive bridges.

**Incomplete:**

- Residential houses are static scenery without walkable interiors or individual household functions. Activity buildings provide access gates; hall interiors are interface scenes, not navigable levels.
- Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- There is no town founding, free residential/building placement or general building upgrade system. Territory expansion uses a fixed circular radius and automatically chosen safe activity-building sites; the hospital is a separate medical building, not a general upgrade framework.
- Wall damage supports local encounters, without a complete siege/destruction and rebuilding simulation.

## NPC behavior and services

**Working:** Weighted NPC types have authored introductions/lore and deterministic trade/specialist services. Conversation keeps the speaker nearby. NPCs no longer issue quests in resident-control campaigns; town boards own projects/orders. Owned residents without orders stay where they were left, while workers follow explicit local orders; selected residents are not drawn twice.

**Incomplete:**

- Foreign movement wanders around a home point. There are no daily schedules, destination-based routines, inter-town travel or work animations tied to production.
- Movement updates are proximity-limited; distant foreign NPCs do not receive a full behavior simulation.
- Individual relationship memory, changing personal stories and reactions to ongoing world events are absent.
- Residents generally expose town services rather than fully separate personal shops, teachers and service economies.

## Quests and quest tracking

**Working:** Town boards create authored knowledge projects and stock-deficit requests. Home-town surveys reward Civic charter after two new discoveries; supply projects unlock recipes, specialist knowledge, medical care, housing or care/trust. Projects have no coin rewards. State > Orders lists foreign trade requests; town halls show the same authoritative records. Accepted orders reserve buyer money, require delivery and can be abandoned with a refund. Unaccepted deficit requests expire; migrated accepted legacy objectives retain their progress without coin rewards.

**Incomplete:**

- The template set is small: supplies, knowledge and a town-discovery survey. Branching chains, multi-stage story projects, commissioned-item objectives and a complete balanced campaign are absent.
- Need calculations use local stock plus pending orders/shipments, without production forecasts, citizen consumption or sophisticated request scheduling. Different towns can legitimately request the same goods.
- Accepted tasks have no failure/deadline consequences; abandonment is available, while automatic expiry currently affects only unaccepted deficit requests.
- The board does not provide a complete objective waypoint/route system or advanced filtering. Task history is capped at 2048 records; archival/recycling is not implemented.

## Skills, research and technology exchange

**Working:** Skills, technologies, recipes and explored maps belong to the state and survive resident switching/death. K opens the existing dependency tree without a duplicate skill list. Eleven base skills and seven resident research nodes use authoritative activity/population/equipment/town/trust checks. Tools opens immediately after Craftsmanship; the first axe needs no previous crafts, while Weaponsmithing still requires two completed crafts. Skill prerequisites are authoritative and transitive: activity counts cannot bypass earlier skills; tree edges use the same definitions. Resident research checks its full prerequisite branch, and specialist projects/teaching require the Craftsmanship branch. Already earned save flags are preserved. Specialist knowledge is available through town projects or paid teaching. Medical care projects unlock hospital research with an accessible school and 80 state coins.

**Incomplete:**

- Skills are unlocked flags, without levels, experience or specialization progression. Several milestones are generic activity counters.
- Research/lessons complete immediately; scholars, workshops and teachers do not supply research time or production.
- Town technologies are defined by type, without evolving discoveries, research competition or dynamic knowledge exchange.
- The dependency tree covers base skills, resident research, activity counters and town/project technologies. Automatic campaign guidance and a complete crafting-recipe dependency graph are absent.
- Unlock pacing and the new production/economy/population loop have functional checks, not comprehensive long-play balance validation.

## Crafting, equipment and character progression

**Working:** Craft requests reserve source-city materials and create delayed workshop jobs; completed products enter the warehouse. Counted weapon/clothing copies can be carried, equipped or stored but never allocated twice. Local warehouse transfers include gear, tools, consumables and vessel ownership. Equipment swaps work anywhere using the selected resident's carried copies. Five starting state traits apply movement, gathering, combat and trade modifiers to every controlled resident, including new arrivals. Selection uses a wide description card with measured wrapping and scrolling for enlarged text.

**Incomplete:**

- Tools/vessels are mostly per-resident ownership flags, not separate world instances. Duplicate copies can be stored in warehouses but one resident carries at most one of each flagged tool/vessel.
- Clothing is one full outfit, without independent garment/accessory slots. Equipment copies have no individual quality or instance properties.
- Warehouse transfers lack filters, bulk equipment loadouts and a full logistics/automation interface; equipment cannot yet be shipped between towns through the shipment menu.
- State traits provide fixed modifiers, without evolving specialization or trait-specific progression.

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

**Working:** Food/potions/campfires heal the selected resident. Infirmaries provide paid berry-based treatment; researched hospitals provide stronger, cheaper treatment. A defeated controlled resident dies or may be captured by hostile guards; control returns to the resident list rather than respawning a hero. Death destroys carried gear/cargo, captivity preserves it, and ransom returns the resident home. State money and knowledge survive.

**Incomplete:**

- Food is a healing/crafting resource; hunger, thirst, sleep and temperature are not simulated.
- Campfires provide local healing rather than a complete camping system.
- Medical treatment is immediate, without treatment queues, staffing, disease or persistent injuries. There is no recoverable corpse/recovery expedition or complete campaign defeat/recovery flow when no controllable residents remain.

## Boats, ships and submarines

**Working:** Craft/unlock requirements, nine vessel choices, distinct rendering/speeds, seated pilots, water travel, repair and diving remain available to controlled residents. Vessel ownership, selected mode, oxygen and hull are stored per resident and restored on selection.

**Incomplete:**

- Vessels are travel modes activated on water, not independent persistent craft with boarding, docking or storage at a world location.
- There are no crews, passengers, cargo holds, fuel costs or a physical shipyard assembly process.
- Movement uses speed multipliers, without inertia, buoyancy, wind, currents or steering physics.
- Hull is tracked per resident, shared across their vessel modes, rather than per independent vessel instance.

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

**Working:** Streamed deterministic chunks, forest/shore/shallow/deep water, weighted town/NPC rarity and surface/underwater resources. Owned towns can plant renewable wood, berries and reeds: trees mature in 10 simulation minutes, berry/reed plots in 3. Resident planting creates persisted world locations with a growing marker and mature resource sprite. Harvesting on site with F or a gather order yields 20 wood or 12 berries/reeds into that resident's cargo; workers then carry it to the warehouse. Each resident planting consumes 2 carried wood and 2 berries at completion, after a physical warehouse pickup when needed. Territory expansion increases plot capacity. Growth-menu plots retain their timer/remote-harvest behavior, including existing plots from older saves.

**Incomplete:**

- Lakes/coastline use predefined large-scale shapes with seeded variation, not general continent/biome generation.
- Settlements occupy a finite seeded region around fixed landmarks. Distant exploration generates terrain, not an endlessly expanding settlement network.
- Player roads/bridges are manually plotted complete routes, without maintenance, demolition, traffic capacity or territorial right-of-way negotiations. Local town paths remain generated scenery; inter-town roads are not generated automatically.
- Wild harvested sites stay depleted. New planted sites use automatically selected town-edge locations and simple shared, nonblocking sprites; manual placement, fertility/seasons and renewable mineral extraction are absent. Growth-menu plots, including older saves, remain abstract management timers.
- Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

## Collision geometry

**Working:** circle bodies, four-point isometric building foundations with box broad-phase checks, thick wall segments/capsules, polygonal rock footprints and tile-boundary geometry, with substepped sliding and local player/NPC/enemy contacts. Nearby NPCs also separate without adding destinations or orders. The world intentionally retains 2D physics.

**Incomplete:**

- Body contacts use local disc separation rather than a general rigid-body simulation. Vessels use radius footprints rather than a separate collision hull for each persistent craft.


## Map and exploration

**Working:** exploration independent of window/zoom, fixed world-space sight ranges, forest-limited sight and building/wall occlusion. Current visibility and explored memory are separate; older exploration stays intact. Large/live/saved maps keep stable terrain colors across explored areas; only the minimap dims explored cells outside current sight. Unexplored cells remain dark. Newly identified towns open an Esc-dismissed discovery card with their name, type, rarity, population, state relations, specialty resources, knowledge and infrastructure; multiple discoveries appear in sequence, and known towns are not announced again after reload. Maps adapt raster detail to scale, preserve partially explored lakes/towns, provide terrain/politics/economy views, retain up to 4096 known deposits after chunks unload, and remember up to 128 titled surface/seafloor notes. Clicking a marker opens its journal entry; notes can be edited/deleted and viewed through Journal/J.

**Incomplete:**

- Cartography uses scale-dependent cached raster detail and exploration cells, not an unlimited-resolution geographic model.
- Political/economic layers show discovered towns and remembered deposits; complete regional borders, production forecasts and strategic orders await the deferred resident-command/navigation systems.
- Road planning supports mouse or keyboard plotting, with at most 64 points and 128 roads. A route must be fully explored and paid for in one operation; controller plotting and detailed road editing are absent.


## Graphics, animation and audio

**Working:** detailed roof/wall/facade variants, material grain, clothing details, grass/flowers/bark, occlusion fading, animated harvesting/resident tools and combat impact/defeat feedback. Sword impact now occurs when the animated swing crosses its facing direction; damage is still an authoritative one-hit phase. Water includes ripples/foam/wakes; nine vessel definitions use projected deck planes, raised shaded hull sides, waterlines, oars and sail/mast details, with volumetric aft cabins on large ships. Original Forest/Harbor/Depths music can follow terrain or be selected/off, with separate music/effects volume and synthesized footsteps, gathering, combat, UI and environment sounds.

**Incomplete:**

- Buildings have twelve facade/detail variants and fade when they occlude the player; interiors, more architectural styles and complete profession/interaction animation sets remain incomplete.
- Water has animated ripples, foam and vessel wakes; physical waves, tides and changing shores are not simulated.
- Audio provides three synthesized music loops and event/ambient effects with independent volume controls and distance/pan for nearby hits, wolves and fires; a larger soundtrack, a full ambient mixing system and smooth soundtrack crossfades are not implemented.


## Menus, controls and save management

**Working:** Esc handles Resume/Save/Save & return/Settings; K shows only state knowledge in live resident-control play. N opens State with Residents, Resources, Production, Projects, Orders, Towns and Growth; U selects residents, Y opens cargo/warehouses, C production and L projects. P opens a nearby town hall using the same boards. Management pauses world simulation while production continues. Menus expose keyboard focus through Tab/Shift+Tab, arrows and Enter, including warehouse transfers, queue actions, settings and saved-world operations; disabled production actions can receive focus to reveal their requirements. Tree arrows select neighboring cells in all four directions. Persistent HUD shows selected-resident health and relevant underwater gauges, with stocks accessible by button. Lobby summaries remain read-only, retain overview/world/resources/towns/tree and never duplicate a skill list. Existing atomic file workflows, remapping, settings Apply and contextual footer remain.

**Incomplete:**

- Gamepad support covers basic movement/actions and legacy directional menu navigation, without complete controller mappings/remapping/text entry. Screen-reader support and color customization are absent.
- Campaign history retains the most recent 512 significant events. It does not reconstruct older events or provide a complete searchable campaign timeline.

## Persistence and content identity

**Working:** Atomic v14 saves contain persisted work/freight event deadlines, counted freight carts/cargo/road routes/progress/returns, resident orders/progress/targets/carried planting supplies, planted-resource locations, resident positions/cargo/gear/health/vessels, city product stocks/housing, queues/plots/shipments, boards/escrow, arrivals and the money ledger alongside shared exploration/progression and existing roads/treaties. Versions 1-13 load with migration backups; older orders/carts start scheduling from the saved simulation time without offline credit; pre-v12 orders default to idle, old plots stay abstract and old timer shipments retain their remaining delivery time; v12 saves receive no free carts; older state choices are recovered from resident backgrounds, including a deceased founder, without re-awarding coins; v9 queue deadlines retain their exact remaining time when migrated to the separate production clock; legacy loose goods move to the home warehouse and legacy equipment to the first resident without resetting knowledge. Resident positions and orders persist across reload; routes rebuild against current geometry; dead residents remain tombstones. A failed autosave keeps the current session alive, shows a warning and logs the error; automatic retries are spaced by 30 seconds, and manual Save remains available.

**Incomplete:**

- Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. Foreign NPC/animal movement and active projectiles are also transient.
- Growing, deliveries and tribute use the persisted simulation clock; production uses a separate persisted active-world runtime clock. There is no calendar or offline progression; production does not run for closed worlds or after application shutdown.
- Generated resident identities still rely on stable NPC indices. Migrations do not provide general remapping for future settlement/NPC generation or content identity changes.

## Local model integration

**Working:** an asynchronous local model client for explicit LLM load benchmarks; normal introductions, quests and services are deterministic. `scripts/install.sh --with-model` builds the CPU server and downloads Qwen; `scripts/run.sh` starts the server and game together, checks readiness, reuses a ready `forest-npc` server and cleans up processes it owns. `--no-model` launches ordinary gameplay directly.

**Incomplete integration boundary:**
- The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.

Sources: [dialogue.cpp](../src/dialogue.cpp), [main.cpp](../src/main.cpp).

## Release and distribution

**Working:** Linux source setup downloads dependencies, builds the game and optional model server, and runs automated tests. Native macOS arm64 setup builds the game and raylib in a separate directory and runs automated tests without opening a game window. The repository includes installation and launch instructions.

**Incomplete:**

- macOS arm64 gameplay/window/audio validation remains manual; Windows support, macOS app packaging/signing and a complete distribution workflow are not established. The optional model server is not validated on macOS.
- Linux and macOS arm64 source setup are available, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.

Sources: [perf.cpp](../src/perf.cpp), [PROGRESS.md](PROGRESS.md), [README.md](../README.md).

## Navigation and autonomous movement (deferred)

**Working:** owned residents follow bounded local gather/plant routes, avoid blocked/unsafe/unknown routes and carry harvests/planting supplies between sites and the home-town hall entrance. Freight carts traverse existing player-road polylines between owned endpoints, including peaceful foreign transit towns; local city connectors avoid building geometry. The existing debug/benchmark auto-route remains available. Work and freight continue away from the player through a main-thread event queue and separately streamed service terrain. Ordinary NPC movement, animals and combat remain limited to the player's active area; service loading never reveals unexplored terrain. General off-road agent routing stays deferred.

**Incomplete / prerequisites:**

- Unrestricted distant destinations, autonomous road discovery and armies remain deferred pending broader owned/foreign resident and profession command systems. Worker orders stay local; freight follows existing explicit roads, with service terrain loaded independently of the camera.
- Collision-aware local connectors use bounded loaded-terrain searches. Freight selects a deterministic existing-road graph route, without general off-road or hierarchical world routing.
- There is no hierarchical routing or automatic handling of distant blocked destinations.
- Off-camera work/freight use a bounded cache of 512 service chunks and at most 256 queued events per simulation step. Excess demand can delay work; missing terrain causes preparation waits without free catch-up output. Work and freight pause in menus and closed worlds; offline progression is absent.

## Intentional choices, not unfinished mechanics

- Weapons do not wear out or disappear on attacks. Arrows and consumables may be spent.
- Resident weapon/clothing swaps work anywhere; location restrictions are not pending work.
- Ordinary NPC interactions use authored English lore and authoritative board/service rules. Removing unbounded free-form conversations was deliberate; restoring them is not a missing feature.
- English UI/content and windowed startup with explicit fullscreen are implemented choices.
- This is a local single-player game. Multiplayer and a full space/planet simulation are not implied deliverables from the original comparisons.

See [DECISIONS.md](DECISIONS.md) for the implementation decisions.
