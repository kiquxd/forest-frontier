# Forest Frontier

Native C++20 / raylib game. Keep all shipped UI, content and NPC interactions in English. Communicate with the user in Russian.

## Maintainer notes
Only the coding assistant edits `ai/DECISIONS.md`, `ai/PLACEHOLDERS.md`, `ai/PROGRESS.md` and other files in `ai/`, unless the user explicitly instructs otherwise. Update them when implementation or scope changes. These files record actual behavior; never describe a placeholder as a finished system.

Keep the English `README.md` current in the same change as gameplay, content, controls, saves, dependencies, installation or launcher changes. Preserve its four main sections: What is it, Features/Gameplay, Installation and Tech Stuff. Put all unfinished mechanics and relevant technical limitations in the existing `Current limits` subsection of Features/Gameplay, synchronized with `ai/PLACEHOLDERS.md`; a link alone is insufficient. State what works and what remains incomplete. Remove a limitation only after implementation and relevant validation. Keep intentional design choices separate from missing mechanics, and do not present possible extensions as promised features.

`TODO.md` is the tracked checklist of incomplete mechanics and technical debt. Keep it synchronized with README Current limits and `ai/PLACEHOLDERS.md`. Remove completed items after implementation and validation; split partially completed items instead of removing their remaining scope. Keep TODO.md in Git, never in .gitignore. Git history records completed work.

## C++ code style

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for new and changed C++ code. Existing compressed formatting is legacy debt, not a style to copy.

- Format changed C++ files with `clang-format -i --style=file` using the root `.clang-format`: Google base, two-space indentation, 80-column limit, spaces instead of tabs. Do not pack multiple statements, branches or function bodies onto one line.
- Use descriptive names: `UpperCamelCase` types/functions, `snake_case` variables, `kUpperCamelCase` constants and trailing underscores for private data members. Formatting does not enforce naming or design rules; review them separately.
- Make headers self-contained, include dependencies directly, use guards for new headers and follow Google's include grouping/order. Avoid global `using namespace` directives.
- Keep functions focused, ownership explicit and interfaces readable. Explain non-obvious decisions and invariants in comments.
- Repository exceptions: retain `.cpp`/`.hpp` extensions and current exception-based error handling. Preserve serialized content IDs and saved-world compatibility during symbol renames.
- Migrate legacy code in reviewable changes. Keep broad formatting/renaming separate from gameplay changes; do not reformat third_party or generated files. Run relevant checks after refactoring.

## Implementation
- Read relevant source and content before changing behavior. Stable content IDs and NPC IDs are part of saved-world compatibility.
- Keep authoritative quests, rewards, equipment allocation and progression deterministic. Language-model output cannot mutate game state.
- GPU work stays on the render thread. World generation stays on the worker. Do not reconfigure global world metadata while that worker is alive.
- Save atomically, support older saves, and preserve a backup before a schema migration. Never reset an existing user's progression.
- Equipment uses counted instances. Equipped or citizen-assigned instances cannot be simultaneously allocated elsewhere. Weapons have no durability and are not consumed by attacks; ranged ammunition remains consumable.
- Default to windowed mode. Fullscreen must be explicitly selected; respect the tiling window manager.

## Validation
Run the affected core tests and `ctest --test-dir build --output-on-failure`. For UI changes, check real raylib screenshots and the isolated menu smoke test when applicable. Run a compact performance benchmark for rendering/simulation changes. Target 60 FPS; user accepts a worst-window p95 ceiling of 30 ms. Log aggregates and meaningful events, not every function call.
