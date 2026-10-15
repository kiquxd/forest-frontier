# Forest Frontier

## What is it

Forest Frontier is a local, single-player exploration and settlement game built in **C++20 with raylib**. Run a small state by directly controlling its residents: explore forests, lakes and coastline, gather supplies, develop towns, exchange knowledge and eventually sail and dive for rare materials.

This is a playable development prototype. Terrain streams in chunks; towns occupy a finite seeded region. All UI, lore and interactions are in English. Startup is windowed and respects tiling window managers; fullscreen is optional.

An optional local Qwen model and joint launcher are included for explicit performance experiments. **Normal gameplay uses authored NPC interactions and never sends model requests.** Quests, rewards, equipment and progression are deterministic.

## Features/Gameplay

### Starting and controlling residents

Choose a state trait: **Balance, Craft, Exploration, Trade or Defense**. Its movement, gathering, combat and trade modifiers apply to every resident you control, including later arrivals. Use Left/Right or the arrow buttons to browse a wide card with the description, benefits and drawbacks. Text wraps to the available width; Wheel or Page Up/Down scrolls longer descriptions when text is enlarged. You start by the hall of one randomly chosen owned town; all other settlements are independent. There is no separate hero or ruler inventory.

Press **U** to choose any living, free resident of your towns, then move with **WASD / arrows**. Every resident keeps their own position, health, cargo, gear and vessel modes. Switching does not teleport the previous resident. Skills, technologies, recipes, exploration and activity milestones belong to the state and remain shared.

You begin without learned skills, tools or weapons. **K** opens the knowledge tree: learn **Gathering** free, gather sites, unlock **Craftsmanship**, learn **Tools** immediately, then queue your first **Axe** at the starting workbench. No prior crafts are needed for Tools. Weaponsmithing requires two completed crafts, which can be an axe and a campfire. Starting goods are in your town warehouse; **Y** transfers cargo on town territory. The state trait remains shared when you switch residents or the founding resident dies.

Skill branches must be learned in order, in addition to their practice requirements: **Gathering → Craftsmanship → Tools → Construction → Engineering**, **Craftsmanship → Weaponsmithing**, **Gathering → Fishing → Seamanship**, **Gathering → Herbalism** and **Gathering → Trading → Diplomacy**. Engineering also requires Trading. A later node cannot skip an unlearned ancestor; existing earned skills in older saves are retained.

### State and town management

Menus can be operated without a mouse. **Arrows** choose main-menu options, state tabs or knowledge-tree cells; **Enter** activates/learns. **Tab / Shift+Tab** enters and cycles visible widget focus; arrows then move that focus and Enter activates the outlined button or field. **Page Up/Down** changes list pages, and **Esc** closes the current window. This covers production/search, resources/transfers, growth, boards, settings and saved-world operations. In State, **C** opens Production and **Y** Resources when not typing a search.

**N** opens State with seven tabs: **Residents, Resources, Production, Projects, Orders, Towns and Growth**. It shows town treasuries, population/capacity, relations, specialty knowledge and deliveries. **P** opens a nearby hall with the same town boards. Management, the knowledge tree and Esc pause freeze world simulation. Production keeps running and refreshes live, including while the window is inactive or minimized.

Each city has a separate warehouse and treasury. **Y / Resources** deposits or withdraws goods and equipment carried by the selected resident. State knowledge and the state treasury are shared. Road-connected owned towns can send timed goods shipments, with materials reserved at dispatch. These are abstract deliveries, not autonomous caravans.

Grow population by accepting periodic wanderers when housing and food permit. Buy housing capacity in Growth. To relocate a resident, control them, walk to another owned town and change their home there. Build an **infirmary** for treatment; medical-supplies projects unlock **Medical care**, then an accessible school and 80 coins unlock **Hospitals**. Hospitals provide stronger treatment at lower cost.

### Projects, trade and money

NPCs provide authored lore, markets and specialist teaching; they no longer issue quests. **L / Projects** shows the selected town's projects, while **Orders** lists known foreign trade requests. Town halls and State refer to the same records.

Owned-town projects give recipes, technologies, care, housing or trust, **never coins**. The home survey unlocks Civic charter after accepting it, discovering two additional towns and returning. Deficit projects respond to warehouse shortages and pending deliveries. Different towns can need the same goods.

Foreign trade orders pay from the buyer's real treasury. Acceptance reserves payment; take the requested goods in a resident's cargo to that town and deliver to receive it. Abandoning returns the escrow. Buyers retain a protected reserve. Specialist orders supply rare goods for coins, with trust and knowledge gates and a simulation-time delivery delay.

Coins remain in state, city, regional-market or reserved-payment accounts. Construction/research/treatment fees fund the regional market, which buys bounded surpluses and returns money to towns. Projects, passive resident jobs and combat do not mint coins. This is a conservative prototype economy, without taxes/wages or a guarantee against hoarding; see Current limits.

### Production, equipment and building

- **C / Production** lists every recipe, including locked ones. Search item names without case sensitivity; **Clear** restores the full catalogue. Each row has an item PNG and resource/skill/technology/building PNGs below it. Resource counts show available/required; unmet requirements and unavailable recipes are muted. Hover an item for all missing requirements, or a requirement icon for its name and status. Queue buttons are enabled only when the authoritative checks pass.
- Workshop jobs reserve warehouse inputs immediately and produce items later into the source city's warehouse. Canceling refunds the reserved materials and fee once.
- Every physical workshop has a FIFO queue of up to eight jobs. Different buildings run in parallel. Rarity determines **1 / 5 / 30 / 60 / 120 active-world minutes**; there is no campaign-wide crafting cooldown.
- Queued rows show live progress bars and seconds remaining. Waiting jobs show their queue position and start delay. Canceling partially finished work gives the next item its full craft duration; full output storage holds the workshop FIFO until space is available. Production uses monotonic elapsed time and accounts for delayed background frames without slowing down.
- Your first town has a **workbench**. Build **forge, tailor, school, shipyard, alchemy, infirmary or hospital** with Construction and warehouse materials. A town permits **max(1, floor(active residents / 6)) forges**; other facility types currently permit one each.
- Workshops may be local or connected through a complete peaceful road network. Hostile endpoints/transit suspend access and queued work; reserved materials are retained. Remote workshop output currently returns without simulated hauling.
- Fourteen weapons and eleven outfits have different perks. Take counted copies from stock, equip them anywhere, or deposit unused copies. An equipped/captive/carried copy cannot also be used by another resident. Weapons never wear out; ranged ammunition is consumed.
- Renewable town plots grow **trees in 10 minutes**, **berries/reeds in 3 minutes**. Plant using warehouse wood/berries, harvest into storage and replant. Plots are management timers; wild harvested sites stay depleted.
- Expand circular town territory from **350 to 1100**, build/repair predefined walls and construct full custom road polylines through **M → Plan road**. All route tiles must be explored and the entire cost paid atomically; water sections become more expensive bridges.

Resident role/training research and local defenders remain available as prototypes. **There are no autonomous resident task orders, group movement or strategic armies.** Direct resident exploration is the current control model.

### Exploration, combat and diplomacy

Explore forests, lakes, shoreline and a northern coast. Sight has a fixed world-space range with forest/building/wall occlusion; zoom and window size cannot reveal more territory. Surface/seafloor exploration is shared and persistent; partially explored lakes and towns remain partially masked.

Gather wood, stone, berries, reeds, fish, ores and rare materials with appropriate tools. Fight local wolves/hostile guards using visible directional swings, knockback or ranged weapons. Food, potions and campfires heal; campfires deter wolves. If a controlled resident is defeated, they die or can be captured by hostile guards. Death destroys carried gear/cargo without drops; captivity preserves it for ransom. Choose another resident instead of respawning a hero; state knowledge and coins remain.

Town halls support gifts, truces, war, alliances and diplomatic union. Union transfers ownership; local fighting does not conquer towns. Surrender retains local rule for **30 coins now + 10 every 120 game seconds**; unpaid tribute restores hostility. Buy independence/previous allegiance for **300 coins**. Strategic warfare remains a placeholder.

### Water, map journal, presentation and audio

Progress through raft, canoe, rowboat, skiff, cutter, sailing ship, brigantine, flagship and submarine. Advanced vessels require specialist knowledge, school research, materials and commissioning fees; they offer speed/sight and distinct decks/masts. Hulls use an isometric side view with raised decks, shaded visible sides and a narrower waterline; large vessels add a raised aft cabin and folded sails. Vessels remain resident-owned travel modes. Diving opens a separate underwater layer with oxygen, hull damage, reef hazards, emergency surfacing, sonar and rare salvage.

Discovering a town opens a summary card with its name, population, specialties, knowledge, infrastructure and attitude toward your state. **Esc** dismisses it without opening pause. Multiple discoveries appear in sequence. The world pauses while workshop production continues; already known towns are not announced again after loading.

**M** shows terrain/politics/economy layers (**1/2/3**), explored towns and remembered deposits. Click an explored location or note marker to edit a titled journal entry; **J / Journal** opens the list, **Ctrl+Enter** saves. Arrows move a centre cursor, **+/-** zoom and **Enter** creates/opens a note there; middle-drag and wheel remain available. In road planning, Enter adds a point, Backspace undoes it and Ctrl+Enter builds the validated whole route. Notes are manual and saved, up to 128 per world.

Buildings have detailed facade variants and fade when obscuring the controlled resident. Water has animated ripples, foam and wakes; vessels show hulls, rigging and cabins. Settings provides **Automatic / Forest / Harbor / Depths / Off** music, separate music/effects volumes and three original synthesized loops. Audio-device failure permits silent play. Regenerate assets with `python3 scripts/generate-audio.py`; see [audio assets](assets/audio/README.md).

### Menus and saves

**Esc** opens **Resume, Save world, Save & return, Settings**; saving keeps the session paused. Save & return first saves, then opens the lobby. Failure leaves the live session available for retry. The lobby has separate application Quit, new-game/state-trait selection and useful read-only saved-world summaries with overview, map, resource rows, towns and skill tree; there is no duplicate skill list. Live **K** focuses on the state knowledge tree; State handles city/resident management.

Saved-world **F2** expands summary; **F6/F7** rename/copy, **F8/F9** export/import, **F10** opens recovery, **Delete** confirms moving a slot to `saves/.trash/`. Files are validated; recovery restores a new slot without overwriting its source. Exports require a new `.json` destination.

Saves are atomic **v11**, with v1–v10 loading/migration backups and no automatic progression reset. Residents, warehouses, queues, plots, tasks, escrow, shipments, arrivals and money accounts persist. Production runtime and world simulation time are saved separately; neither includes offline elapsed time. Production pauses when you return to the lobby or close the application. Normal installation/launch never deletes saves.

Settings works in the lobby and pause: window/fullscreen, frame cap, zoom, HUD, text scale, tree contrast, audio and primary/alternate bindings. A visible **Music: On/Off** toggle is always at the top; detailed tracks/volumes are also reachable with Page Up/Down or wheel. Use **Apply** to save changes, including Music Off during a live world. New actions preserve older custom bindings and receive free keys if defaults conflict. The bottom action bar uses actual bindings and nearby available actions on a dark background. The upper-left title and health panels fit their contents and adapt to text scale; management shortcuts stay in the bottom action bar.

### Controls

Defaults can be changed in Settings.

| Key | Action |
| --- | --- |
| WASD / Arrows; Shift | Move selected resident; sprint |
| U / N | Select resident / State management |
| Y | Resident cargo and town warehouse |
| K | Shared state knowledge tree |
| P / L | Nearby town hall / Town projects |
| C / T | Production queues / Nearby market |
| E / F (hold) | Talk / Gather |
| Space / Left mouse | Attack outside panels |
| I / O | Selected resident weapons / outfits |
| F4 in conversation | Specialist teaching |
| Q / H / B | Eat / Potion / Campfire |
| V / R / G / J | Vessel / Dive-surface / Sonar / Repair |
| M; 1/2/3 in map | Map; Terrain/Politics/Economy |
| Wheel / Middle-drag | Zoom/scroll; map/tree pan |
| J in map; Ctrl+Enter in notes | Journal; save note |
| Tab / Shift+Tab; Enter | Focus UI controls; activate the focused control |
| Arrows; +/-; Enter on map | Move map cursor; zoom; note/road waypoint |
| Backspace; Ctrl+Enter in road planning | Undo point; build the complete route |
| Esc | Close panel, otherwise pause/resume |
| F3 / F11 | Performance HUD / Explicit fullscreen |

### Current limits

The mechanics below are working prototypes or incomplete systems. Menus, unlocks and visuals do not imply a complete simulation. This inventory describes the current build, rather than promising every possible extension.

#### Strategic warfare

**Working:** war declarations, hostile local guards, wall damage, truces, alliances, union ownership, military research and equipment allocation.

**Incomplete:**

- War is a local-conflict placeholder. There are no campaign armies, marching routes, formations, tactical orders, siege simulation or military logistics.
- Defeating guards does not conquer a town. Ownership changes through diplomatic union, without occupation or a conquest campaign.
- Foreign towns do not autonomously wage wars, mobilize troops or provide allied military assistance.
- There is no naval combat or transport of armies by ship.


#### Defenders and resident training

**Working:** Counted gear belongs to a resident bag or city warehouse. Directly controlled residents have persistent position, health, cargo, clothing and weapons; local defenders retain death/capture/ransom. Dead residents lose all carried equipment and cargo without drops; captives return with their reserved equipment. Infirmaries and hospitals treat residents on home-town territory.

**Incomplete:**

- Training is an immediate assignment enabled by technology and access to a local/road-connected school, without lessons, experience, training time or individual proficiency.
- Defenders periodically damage nearby wolves and hostile guards. They lack a complete attack animation, combat decision system and coordinated patrols; this is not an army simulation.
- Casualties and captivity come from local encounters only. There is no campaign casualty simulation, prisoner transport/exchange or persistent injury model.
- Ranged autonomous defenders are unavailable; resident ammunition logistics, ranged targeting and squad commands are not implemented. Directly controlled residents can use ranged weapons.

#### Population and settlement management

**Working:** The first owned town starts with selectable residents rather than a separate hero. All active owned residents can be selected from State > Residents and explored with directly. Wanderers periodically appear in owned towns; acceptance consumes food and a housing place. Housing capacity can be expanded; walk a resident to another owned town and change their home there.

**Incomplete:**

- There are no births, dismissal, household simulation, food upkeep, wages or happiness. Wanderers are generated at towns, without physically traveling there.
- Resident search, comparison tools, bulk selection and group commands are absent. Selection controls one resident at a time; inactive residents retain their last position.
- Housing is a capacity upgrade, not a freely placed residential building. Town layouts do not grow new residential districts in response to population changes.
- Wanderer history is bounded to 128 arrivals per campaign; declined/departed arrival recycling is not implemented.

#### Resident work and production

**Working:** Production uses per-building FIFO queues, up to eight jobs per workshop, with reserved warehouse inputs and commissioning fees. Separate buildings operate in parallel. Forge construction is capped at max(1, floor(active town population / 6)); existing forges are retained after population loss. Jobs take 1/5/30/60/120 active-world minutes by rarity. A persisted production clock accumulates std::chrono::steady_clock elapsed time while the world is open, including management, Esc/settings, inactivity and minimized windows. Live FIFO rows show countdowns, progress bars and waiting/access/storage states. Simulation pauses do not stop production; returning to the lobby closes the world and suspends it. Output returns to the source city warehouse; cancellation refunds reserved inputs/fees once. Lost peaceful access suspends the queue. Old passive resident stock/coin generation is disabled in resident-control campaigns. The full recipe catalogue remains visible with case-insensitive name search, item PNGs and resource/skill/technology/building requirement PNGs. Missing counts or unlocks are muted, hovering explains them, and the same read-only quote controls both UI readiness and queue validation.

**Incomplete:**

- Workshop production is a timed management abstraction: no worker attendance, physical material transport, individual productivity, automatic priorities or operating costs.
- Roles/equipment and research remain prerequisites/prototypes; residents do not receive autonomous job orders, gather on their own or produce specialized scholar/engineer output.
- Connected workshop output returns directly to its source warehouse when complete, without travel time or a hauling actor.
- Foreign towns have bounded abstract resource replenishment, not a full resident production simulation. There is no offline production catch-up.

#### Trade, rare resources and town economy

**Working:** Each town has its own goods warehouse, product stock and treasury; state skills and treasury are shared. Transfer cargo locally, send timed goods shipments between road-connected owned towns, or order specialist goods for coins with knowledge/trust gates. Foreign deficit orders reserve the buyer's payment and pay only on physical delivery by a resident. State/city/regional funds, production fees and order escrow conserve the campaign money supply. Own-town market sales are replaced by warehouse transfers; own projects do not pay coins. A regional fund buys bounded surpluses and receives research/construction/treatment fees; foreign buyers keep a protected reserve.

**Incomplete:**

- Prices are fixed rules, not supply-and-demand calculations. NPCs share town markets rather than individual inventories and budgets.
- Shipments are scheduled abstractions, without caravans, moving haulers, interception or transport losses. Specialist delivery does not require a built road.
- Rare specialist orders create commissioned goods without simulating their production inputs. Foreign basic-stock replenishment and regional surplus purchases are abstract; there is no autonomous foreign economy.
- Closed money accounting prevents minting, not hoarding. There is no taxation, wage, maintenance or automatic redistribution policy guaranteeing liquidity for every town or preventing wealth concentration.
- General trading covers resource goods, not a complete market for crafted weapons and clothing. Inter-town shipments carry goods, not equipment copies.

#### Diplomacy and relations between settlements

**Working:** seeded inter-town relations, visitor attitudes, player trust, gifts, truces/alliances/unions, project trust rewards, surrender retaining local rule, recurring tribute and buying independence/previous allegiance. Unpaid tribute resumes local hostility.

**Incomplete:**

- Foreign relations are largely initialized values; rulers do not negotiate or change alliances through an autonomous diplomacy simulation.
- Treaties and surrender use immediate fixed terms, without counteroffers, treaty expiry or a negotiation process. Tribute covers local player-controlled towns; there is no autonomous foreign subjugation or state-level political integration.
- Alliances do not establish shared defense, coordinated trade or joint policy. Unions transfer ownership without political integration.
- Visitors are generated residents with an origin town, not travelers arriving along routes. Hostility changes local positioning rather than simulating refusal or diplomatic incidents.


#### City construction, walls and interiors

**Working:** Town halls contain projects, orders and management. Circular territory expansion, collidable walls and eight activity-building types are available: workbench, forge, tailor, school, shipyard, alchemy, infirmary and hospital. Housing is a paid capacity upgrade. Complete player-plotted roads share facility access through peaceful transit towns; water sections are more expensive bridges.

**Incomplete:**

- Residential houses are static scenery without walkable interiors or individual household functions. Activity buildings provide access gates; hall interiors are interface scenes, not navigable levels.
- Walls fill a predefined ring with a fixed opening; there is no free placement, layout editor, movable/openable gate or dismantling interface.
- There is no town founding, free residential/building placement or general building upgrade system. Territory expansion uses a fixed circular radius and automatically chosen safe activity-building sites; the hospital is a separate medical building, not a general upgrade framework.
- Wall damage supports local encounters, without a complete siege/destruction and rebuilding simulation.

#### NPC behavior and services

**Working:** Weighted NPC types have authored introductions/lore and deterministic trade/specialist services. Conversation keeps the speaker nearby. NPCs no longer issue quests in resident-control campaigns; town boards own projects/orders. Owned residents keep their controlled positions instead of wandering away; selected residents are not drawn twice.

**Incomplete:**

- Foreign movement wanders around a home point. There are no daily schedules, destination-based routines, inter-town travel or work animations tied to production.
- Movement updates are proximity-limited; distant foreign NPCs do not receive a full behavior simulation.
- Individual relationship memory, changing personal stories and reactions to ongoing world events are absent.
- Residents generally expose town services rather than fully separate personal shops, teachers and service economies.

#### Quests and quest tracking

**Working:** Town boards create authored knowledge projects and stock-deficit requests. Home-town surveys reward Civic charter after two new discoveries; supply projects unlock recipes, specialist knowledge, medical care, housing or care/trust. Projects have no coin rewards. State > Orders lists foreign trade requests; town halls show the same authoritative records. Accepted orders reserve buyer money, require delivery and can be abandoned with a refund. Unaccepted deficit requests expire; migrated accepted legacy objectives retain their progress without coin rewards.

**Incomplete:**

- The template set is small: supplies, knowledge and a town-discovery survey. Branching chains, multi-stage story projects, commissioned-item objectives and a complete balanced campaign are absent.
- Need calculations use local stock plus pending orders/shipments, without production forecasts, citizen consumption or sophisticated request scheduling. Different towns can legitimately request the same goods.
- Accepted tasks have no failure/deadline consequences; abandonment is available, while automatic expiry currently affects only unaccepted deficit requests.
- The board does not provide a complete objective waypoint/route system or advanced filtering. Task history is capped at 2048 records; archival/recycling is not implemented.

#### Skills, research and technology exchange

**Working:** Skills, technologies, recipes and explored maps belong to the state and survive resident switching/death. K opens the existing dependency tree without a duplicate skill list. Eleven base skills and seven resident research nodes use authoritative activity/population/equipment/town/trust checks. Tools opens immediately after Craftsmanship; the first axe needs no previous crafts, while Weaponsmithing still requires two completed crafts. Skill prerequisites are authoritative and transitive: activity counts cannot bypass earlier skills; tree edges use the same definitions. Resident research checks its full prerequisite branch, and specialist projects/teaching require the Craftsmanship branch. Already earned save flags are preserved. Specialist knowledge is available through town projects or paid teaching. Medical care projects unlock hospital research with an accessible school and 80 state coins.

**Incomplete:**

- Skills are unlocked flags, without levels, experience or specialization progression. Several milestones are generic activity counters.
- Research/lessons complete immediately; scholars, workshops and teachers do not supply research time or production.
- Town technologies are defined by type, without evolving discoveries, research competition or dynamic knowledge exchange.
- The dependency tree covers base skills, resident research, activity counters and town/project technologies. Automatic campaign guidance and a complete crafting-recipe dependency graph are absent.
- Unlock pacing and the new production/economy/population loop have functional checks, not comprehensive long-play balance validation.

#### Crafting, equipment and character progression

**Working:** Craft requests reserve source-city materials and create delayed workshop jobs; completed products enter the warehouse. Counted weapon/clothing copies can be carried, equipped or stored but never allocated twice. Local warehouse transfers include gear, tools, consumables and vessel ownership. Equipment swaps work anywhere using the selected resident's carried copies. Five starting state traits apply movement, gathering, combat and trade modifiers to every controlled resident, including new arrivals. Selection uses a wide description card with measured wrapping and scrolling for enlarged text.

**Incomplete:**

- Tools/vessels are mostly per-resident ownership flags, not separate world instances. Duplicate copies can be stored in warehouses but one resident carries at most one of each flagged tool/vessel.
- Clothing is one full outfit, without independent garment/accessory slots. Equipment copies have no individual quality or instance properties.
- Warehouse transfers lack filters, bulk equipment loadouts and a full logistics/automation interface; equipment cannot yet be shipped between towns through the shipment menu.
- State traits provide fixed modifiers, without evolving specialization or trait-specific progression.

#### Player combat and enemy behavior

**Working:** timed directional melee hits, visible swings, bow projectiles, attack windups, knockback, player health/protection, persistent defeated IDs and loot.

**Incomplete:**

- Melee damage uses range/cone checks at a fixed attack phase, not collision with the animated blade throughout its trajectory.
- Enemies chase/attack/return locally, without wolf pack tactics or broader encounter coordination.
- Weapon variety mostly changes numbers and a few attack categories, without a complete distinct move set per weapon.
- There is no blocking, parrying, combo or stamina system, or complete injury/status-effect model.
- Combat aboard vessels and underwater is unavailable.


#### Survival, healing and respawning

**Working:** Food/potions/campfires heal the selected resident. Infirmaries provide paid berry-based treatment; researched hospitals provide stronger, cheaper treatment. A defeated controlled resident dies or may be captured by hostile guards; control returns to the resident list rather than respawning a hero. Death destroys carried gear/cargo, captivity preserves it, and ransom returns the resident home. State money and knowledge survive.

**Incomplete:**

- Food is a healing/crafting resource; hunger, thirst, sleep and temperature are not simulated.
- Campfires provide local healing rather than a complete camping system.
- Medical treatment is immediate, without treatment queues, staffing, disease or persistent injuries. There is no recoverable corpse/recovery expedition or complete campaign defeat/recovery flow when no controllable residents remain.

#### Boats, ships and submarines

**Working:** Craft/unlock requirements, nine vessel choices, distinct rendering/speeds, seated pilots, water travel, repair and diving remain available to controlled residents. Vessel ownership, selected mode, oxygen and hull are stored per resident and restored on selection.

**Incomplete:**

- Vessels are travel modes activated on water, not independent persistent craft with boarding, docking or storage at a world location.
- There are no crews, passengers, cargo holds, fuel costs or a physical shipyard assembly process.
- Movement uses speed multipliers, without inertia, buoyancy, wind, currents or steering physics.
- Hull is tracked per resident, shared across their vessel modes, rather than per independent vessel instance.

#### Underwater exploration and camera

**Working:** underwater map layer, dive/surface switching, limited visibility, oxygen, reef hazards, repair, sonar and gated resources.

**Incomplete:**

- Diving switches between two flat layers; there is no continuous depth, elevation, ballast, pressure or three-dimensional navigation/camera.
- The camera remains an isometric world view, rather than a separate depth-aware exploration system.
- Sonar reveals a disk of water cells, without obstacle-aware sensing, target identification or an energy/noise model.
- There are no walkable wreck interiors or a complete underwater ecosystem/encounter system.
- Sonar cooldown is runtime state and does not survive reload.


#### World generation, biomes and resources

**Working:** Streamed deterministic chunks, forest/shore/shallow/deep water, weighted town/NPC rarity and surface/underwater resources. Owned towns can plant renewable wood, berries and reeds: trees mature in 10 simulation minutes, berry/reed plots in 3. Manual harvest delivers 20 wood or 12 berries/reeds to the town warehouse; replanting consumes wood/berries. Territory expansion increases plot capacity.

**Incomplete:**

- Lakes/coastline use predefined large-scale shapes with seeded variation, not general continent/biome generation.
- Settlements occupy a finite seeded region around fixed landmarks. Distant exploration generates terrain, not an endlessly expanding settlement network.
- Player roads/bridges are manually plotted complete routes, without maintenance, demolition, traffic capacity or territorial right-of-way negotiations. Local town paths remain generated scenery; inter-town roads are not generated automatically.
- Wild harvested sites stay depleted. Renewable plots are timer records in management, without individually placed/rendered trees/fields, forestry workers or renewable mineral extraction.
- Animals wander/flee locally, without breeding, food chains or persistent population simulation.
- Terrain is flat, without caves, elevation or physical geology. Weather, seasons and a day/night simulation are absent.

#### Collision geometry

**Working:** circle bodies, four-point isometric building foundations with box broad-phase checks, thick wall segments/capsules, polygonal rock footprints and tile-boundary geometry, with substepped sliding and local player/NPC/enemy contacts. Nearby NPCs also separate without adding destinations or orders. The world intentionally retains 2D physics.

**Incomplete:**

- Body contacts use local disc separation rather than a general rigid-body simulation. Vessels use radius footprints rather than a separate collision hull for each persistent craft.


#### Map and exploration

**Working:** exploration independent of window/zoom, fixed world-space sight ranges, forest-limited sight and building/wall occlusion. Current visibility and explored memory are separate; older exploration stays intact. Large/live/saved maps keep stable terrain colors across explored areas; only the minimap dims explored cells outside current sight. Unexplored cells remain dark. Newly identified towns open an Esc-dismissed discovery card with their name, type, rarity, population, state relations, specialty resources, knowledge and infrastructure; multiple discoveries appear in sequence, and known towns are not announced again after reload. Maps adapt raster detail to scale, preserve partially explored lakes/towns, provide terrain/politics/economy views, retain up to 4096 known deposits after chunks unload, and remember up to 128 titled surface/seafloor notes. Clicking a marker opens its journal entry; notes can be edited/deleted and viewed through Journal/J.

**Incomplete:**

- Cartography uses scale-dependent cached raster detail and exploration cells, not an unlimited-resolution geographic model.
- Political/economic layers show discovered towns and remembered deposits; complete regional borders, production forecasts and strategic orders await the deferred resident-command/navigation systems.
- Road planning supports mouse or keyboard plotting, with at most 64 points and 128 roads. A route must be fully explored and paid for in one operation; controller plotting and detailed road editing are absent.


#### Graphics, animation and audio

**Working:** detailed roof/wall/facade variants, material grain, clothing details, grass/flowers/bark, occlusion fading, animated harvesting/resident tools and combat impact/defeat feedback. Sword impact now occurs when the animated swing crosses its facing direction; damage is still an authoritative one-hit phase. Water includes ripples/foam/wakes; nine vessel definitions use projected deck planes, raised shaded hull sides, waterlines, oars and sail/mast details, with volumetric aft cabins on large ships. Original Forest/Harbor/Depths music can follow terrain or be selected/off, with separate music/effects volume and synthesized footsteps, gathering, combat, UI and environment sounds.

**Incomplete:**

- Buildings have twelve facade/detail variants and fade when they occlude the player; interiors, more architectural styles and complete profession/interaction animation sets remain incomplete.
- Water has animated ripples, foam and vessel wakes; physical waves, tides and changing shores are not simulated.
- Audio provides three synthesized music loops and event/ambient effects with independent volume controls and distance/pan for nearby hits, wolves and fires; a larger soundtrack, a full ambient mixing system and smooth soundtrack crossfades are not implemented.


#### Menus, controls and save management

**Working:** Esc handles Resume/Save/Save & return/Settings; K shows only state knowledge in live resident-control play. N opens State with Residents, Resources, Production, Projects, Orders, Towns and Growth; U selects residents, Y opens cargo/warehouses, C production and L projects. P opens a nearby town hall using the same boards. Management pauses world simulation while production continues. Menus expose keyboard focus through Tab/Shift+Tab, arrows and Enter, including warehouse transfers, queue actions, settings and saved-world operations; disabled production actions can receive focus to reveal their requirements. Tree arrows select neighboring cells in all four directions. Persistent HUD shows selected-resident health and relevant underwater gauges, with stocks accessible by button. Lobby summaries remain read-only, retain overview/world/resources/towns/tree and never duplicate a skill list. Existing atomic file workflows, remapping, settings Apply and contextual footer remain.

**Incomplete:**

- Gamepad support covers basic movement/actions and legacy directional menu navigation, without complete controller mappings/remapping/text entry. Screen-reader support and color customization are absent.
- Campaign history retains the most recent 512 significant events. It does not reconstruct older events or provide a complete searchable campaign timeline.

#### Persistence and content identity

**Working:** Atomic v11 saves contain resident positions/cargo/gear/health/vessels, city product stocks/housing, queues/plots/shipments, boards/escrow, arrivals and the money ledger alongside shared exploration/progression and existing roads/treaties. Versions 1-10 load with migration backups; older state choices are recovered from resident backgrounds, including a deceased founder, without re-awarding coins; v9 queue deadlines retain their exact remaining time when migrated to the separate production clock; legacy loose goods move to the home warehouse and legacy equipment to the first resident without resetting knowledge. Inactive controlled residents keep positions across reload; dead residents remain tombstones. A failed autosave keeps the current session alive, shows a warning and logs the error; automatic retries are spaced by 30 seconds, and manual Save remains available.

**Incomplete:**

- Living enemy health, position and attack state are transient; chunk regeneration/reload can reset a surviving enemy. Foreign NPC/animal movement and active projectiles are also transient.
- Growing, deliveries and tribute use the persisted simulation clock; production uses a separate persisted active-world runtime clock. There is no calendar or offline progression; production does not run for closed worlds or after application shutdown.
- Generated resident identities still rely on stable NPC indices. Migrations do not provide general remapping for future settlement/NPC generation or content identity changes.

#### Local model integration

**Working:** an asynchronous local model client for explicit LLM load benchmarks; normal introductions, quests and services are deterministic. `scripts/install.sh --with-model` builds the CPU server and downloads Qwen; `scripts/run.sh` starts the server and game together, checks readiness, reuses a ready `forest-npc` server and cleans up processes it owns. `--no-model` launches ordinary gameplay directly.

**Incomplete integration boundary:**
- The model has no bounded gameplay role such as context-grounded quest narration or constrained negotiation. A successful benchmark does not establish such a mechanic.
- Model measurements do not validate persistent NPC memory or model-driven world behavior; neither exists in normal gameplay.


#### Release and distribution

**Working:** Linux source setup downloads dependencies, builds the game and optional model server, and runs automated tests. The repository includes installation and launch instructions.

**Incomplete:**

- Windows/macOS packaging, platform validation and a complete distribution workflow are not established by the Linux build.
- Linux source setup is available and tested, but there is no standalone relocatable package: assets/content/default writable paths remain bound to the checkout at build time. System packages must be installed separately.
- Downloads use HTTPS, curl success and a GGUF header check for the model, without pinned checksums or complete model validation. The model/font URLs track upstream main, and existing dependency files/source checkouts are reused rather than reverified against a lock manifest.


#### Navigation and autonomous movement (deferred)

**Working:** the existing bounded debug/benchmark auto-route remains available. Long-distance agent simulation is deferred behind resident command systems.

**Incomplete / prerequisites:**

- Full destination orders and route planning are deferred until command systems exist for owned and foreign residents/professions, caravans and armies; current direct selection and job/equipment assignments do not satisfy this prerequisite.
- Navigation searches a bounded area of loaded terrain and approaches distant goals in short segments, rather than planning full world routes for caravans/armies.
- There is no hierarchical routing or complete handling of distant blocked/unloaded destinations.


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

`src/` holds the game; `content/` holds JSON definitions for NPCs, weapons, outfits, state traits (stable IDs in `content/characters/`), quests, recipes and settlements. `assets/ui/` contains reproducible pixel-art PNG icons and their shared atlas. Regenerate them with `python3 scripts/generate-ui-icons.py`; no image library is required.

World generation runs on a worker; GPU work stays on the render thread. Chunk streaming, visibility culling and batched icons keep rendering work bounded. Text is baked at the sizes used by the UI and aligned to pixels. Model requests run outside the render loop and cannot change authoritative game state.

Pinned game dependencies are downloaded by setup. The optional server starts from the pinned llama.cpp release; an existing source checkout is preserved. Dependency projects and model distributions retain their own licenses: [raylib](https://github.com/raysan5/raylib), [nlohmann/json](https://github.com/nlohmann/json), [cpp-httplib](https://github.com/yhirose/cpp-httplib), [llama.cpp](https://github.com/ggml-org/llama.cpp), [Qwen GGUF distribution](https://huggingface.co/unsloth/Qwen3-1.7B-GGUF).

### Validation and performance

After changing code:

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

Tests cover world generation, gameplay, campaigns, workshop queues, closed money accounting, resident selection/warehouses/growth, facilities/roads/bridges, resident losses/ransom, tribute, quests/equipment, save compatibility, controls, tree unlocks, resource memory, save-file workflows, summaries, dialogue transport and launcher lifecycle. Run checks once after related changes are complete. A short real-window journal/save check is available on Linux/X11 or XWayland; screenshot capture requires Wayland and grim. Longer workflow checks are optional when changing those workflows or preparing a release:

```sh
python3 scripts/world-smoke.py  # Quick journal/save check
python3 scripts/settings-smoke.py  # Quick Apply-button check
python3 scripts/nation-smoke.py  # Fresh resident/warehouse/production/board check
python3 scripts/production-smoke.py  # Catalogue, requirements, search and queue
python3 scripts/production-clock-smoke.py  # Live/background production clock
python3 scripts/discovery-smoke.py  # Town summary, Escape priority and reload
python3 scripts/state-trait-smoke.py  # Trait selection, small window and large text
python3 scripts/keyboard-smoke.py  # Keyboard-only starter/tree/state/audio/map/save flow
# Optional extended workflows:
python3 scripts/world-smoke.py --extended
python3 scripts/menu-smoke.py
python3 scripts/menu-workflow-smoke.py
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

Profile only after an observed slowdown or changes with a credible performance impact, such as major renderer/geometry changes, near-3D graphics, large asset/actor counts or model integration. Small cosmetic changes do not require benchmarks. Start with a short windowed sample without a model:

```sh
./scripts/run.sh --no-model -- --benchmark 8
python3 scripts/summarize-perf.py
```

For release preparation or a specific regression, optionally exercise the longer community scenario with model generation enabled:

```sh
./scripts/run.sh -- --benchmark 55 --benchmark-community --benchmark-llm
python3 scripts/summarize-perf.py
```

Benchmarks use temporary game state and do not save their simulated progression. Extended scripted scenarios pre-provision activity buildings and advance craft time explicitly; they measure execution load rather than natural campaign pacing. The lower-level `scripts/benchmark.sh` also provides uncapped baseline, gameplay and expedition scenarios; its LLM modes expect a server to be running already.

Implementation choices, validation results and unfinished systems are recorded in [ai/DECISIONS.md](ai/DECISIONS.md), [ai/PROGRESS.md](ai/PROGRESS.md) and [ai/PLACEHOLDERS.md](ai/PLACEHOLDERS.md). The tracked work checklist is [TODO.md](TODO.md). Contribution rules are in [AGENTS.md](AGENTS.md). Keep content and NPC IDs stable, preserve existing saves, and keep public UI/content in English.

First-party C++ sources and tests follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) and the repository [.clang-format](.clang-format): consistent naming, expanded declarations/control flow, header guards and direct includes. Dependency sources retain their upstream style. Install clang-format >=15 separately for development; the migration was validated with 23.1.3. It is not required to build or play.

```sh
./scripts/format.sh         # Format first-party C++ sources and tests
./scripts/format.sh --check # Verify formatting without edits
```

Set `CLANG_FORMAT=/path/to/clang-format` to use a specific formatter executable. Formatting checks cover layout; naming and design still require review.
