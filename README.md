# AutoJump for Geometry Dash Android 2.2081

This is **experimental source code** for a native Geode mod targeting Android ARM64.
It attempts to jump automatically in normal forward-scrolling **cube** gameplay, using the level's registered hazard hitboxes and small solid blocks. It is NOT a completion bot, has no physics simulation or machine learning, and cannot beat all levels. It does not handle ship, UFO, ball, wave, robot, spider, swing, platformer, dual-player, moving hazards or complicated trigger setups reliably.

## Controls (works with touchscreen)

1. Install the Geode Android launcher that supports GD 2.2081.
2. Install the compiled `ollfx.autojump.geode` through the Geode mod menu (or Android's Geode mods directory).
3. In the Geode mod list, open **AutoJump (Android) → Settings**.
4. Turn on **Enable AutoJump**.
5. Start a regular cube level. Use **Jump distance** to move the click earlier/later (default 47 world units). Turn off **Jump over short solid blocks** if it responds badly to floor geometry.

No keyboard and no touch injection required: the mod asks GD itself to press/release jump. The settings menu is accessible on mobile.

## Build

Use the latest Geode SDK/CLI, Android NDK, Android arm64 binaries and CMake:

```sh
geode sdk install
geode sdk install-binaries -p android64
geode build -p android64
```

If building on GitHub, push this project to a repository and run **Actions → Android Geode build → Run workflow**. Its action is based on the official `geode-sdk/build-geode-mod` build action.

For local source-only planner tests (no Geode SDK needed):

```sh
clang++ -std=c++23 -Wall -Wextra -pedantic tests/planner_tests.cpp -o /tmp/autojump-tests
/tmp/autojump-tests
```

## Notes / limits

- Object hitboxes are captured when PlayLayer adds objects, and queried using a sorted, short lookahead window. This minimizes scanning on mobile, but moving hazards are NOT resampled.
- The bot presses one frame and releases the next, then suppresses repeated jumps for the same obstacle until a reset.
- The bot only checks obstacles near the cube's floor or upside-down ceiling path.
- Timing is heuristic rather than simulated; speed portals and geometry may cause misses.
- Update `mod.json` `geode` to the *actual SDK version* used if necessary. GD 2.2081 must be supported by that SDK build.
- Android64 is the default build target. An Android32 build needs an installed SDK and a separate build.
- Current environment lacks the Geode SDK and Android NDK. Only the independent C++ planning tests are verifiable here; native mod compilation and in-game behavior have **not** been validated.
