# Little Helpers

E buys at the wall terminal, upgrades capacity or greets a nearby plush helper.

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [little-helpers-0.4.0.msmod](content/little-helpers-0.4.0.msmod), then drag and drop the `.msmod` file onto the [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) window. You can also use **Add mods…** (older loader builds: **Install mod package**).

Select the mod and its required frameworks, click **Prepare files**, then **Launch**.

Required mods: [unit-runtime](https://github.com/swooping-mods/unit-runtime), [sound-framework](https://github.com/swooping-mods/sound-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

## Controls

- Little Helpers — starter lobby, wall terminal to the RIGHT of the whiteboard. Aim at BUY / UPGRADE and press E.
- A helper costs $75. Capacity starts at one; upgrades unlock slots two, three and four.
- Helpers seek dirty patches and make small brush strokes. Efficient Moving Units provides host simulation, guest visuals and authenticated purchases; real Steam co-op qualification is pending.
- Aim at a helper and E to greet it. Stationary helpers wave; working helpers acknowledge with happy eyes. Interactions consume overlapping keys.

## Compatibility

Version 0.4.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).
