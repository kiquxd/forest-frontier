# Forest Frontier

Native C++20 / raylib game. Keep all shipped UI, content and NPC interactions in English. Communicate with the user in Russian.

## Maintainer notes
Only the coding assistant edits `ai/DECISIONS.md`, `ai/PLACEHOLDERS.md`, `ai/PROGRESS.md` and other files in `ai/`, unless the user explicitly instructs otherwise. Update them when implementation or scope changes. These files record actual behavior; never describe a placeholder as a finished system.

Keep the English `README.md` current in the same change as gameplay, content, controls, saves, dependencies, installation or launcher changes. Preserve its four main sections: What is it, Features/Gameplay, Installation and Tech Stuff. Put all unfinished mechanics and relevant technical limitations in the existing `Current limits` subsection of Features/Gameplay, synchronized with `ai/PLACEHOLDERS.md`; a link alone is insufficient. State what works and what remains incomplete. Remove a limitation only after implementation and relevant validation. Keep intentional design choices separate from missing mechanics, and do not present possible extensions as promised features.

## Implementation
- Read relevant source and content before changing behavior. Stable content IDs and NPC IDs are part of saved-world compatibility.
- Keep authoritative quests, rewards, equipment allocation and progression deterministic. Language-model output cannot mutate game state.
- GPU work stays on the render thread. World generation stays on the worker. Do not reconfigure global world metadata while that worker is alive.
- Save atomically, support older saves, and preserve a backup before a schema migration. Never reset an existing user's progression.
- Equipment uses counted instances. Equipped or citizen-assigned instances cannot be simultaneously allocated elsewhere. Weapons have no durability and are not consumed by attacks; ranged ammunition remains consumable.
- Default to windowed mode. Fullscreen must be explicitly selected; respect the tiling window manager.

## Validation
Run the affected core tests and `ctest --test-dir build --output-on-failure`. For UI changes, check real raylib screenshots and the isolated menu smoke test when applicable. Run a compact performance benchmark for rendering/simulation changes. Target 60 FPS; user accepts a worst-window p95 ceiling of 30 ms. Log aggregates and meaningful events, not every function call.
