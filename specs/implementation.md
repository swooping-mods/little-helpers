# Little Helpers: behavior and implementation

Describes the retained payload **0.4.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.4.1** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.4.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

A wall terminal beside the starter whiteboard sells small cleaning helpers for $75. Capacity starts at one; purchases unlock helpers up to the current capacity and upgrades expand it to four. Unit Runtime supplies fixed-step host simulation, authenticated guest commands and guest snapshots. Helpers select dirty patches, follow bounded floor/obstacle routes and perform small native cleaning strokes; aiming and E greets a helper with a wave or happy-eye acknowledgement.

## State and lifecycle

A checksummed host sidecar saves capacity, owned count and latest transaction identity/result. Live route/animation state is session simulation. Host purchase admission validates proximity/quote/clear space and exact payment; retries use deduplication instead of another charge. Guest visuals do not debit wallets or clean. Authority change suspends commands; teardown saves valid dirty host state and releases scene/sound roots.

## Integration and configuration

Runtime dependencies: [unit-runtime](https://github.com/swooping-mods/unit-runtime), [sound-framework](https://github.com/swooping-mods/sound-framework), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/little-helpers/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `unit-runtime.units` | 1 | at least 80 bytes |
| Requires `unit-runtime.contacts` | 1 | at least 16 bytes |
| Requires `sound-framework.sound` | 1 | at least 48 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |

## Implementation sources

- [mod.c](../source/mods/little-helpers/mod.c)
- [helper_core.h](../source/mods/little-helpers/helper_core.h)
- [unit_purchase.h](../source/mods/little-helpers/unit_purchase.h)
- [unit_replica.h](../source/mods/little-helpers/unit_replica.h)
- [route_core.h](../source/mods/little-helpers/route_core.h)

## Verification boundary

Real Steam co-op, arbitrary maps/doors, skinning/animation transitions and performance need matching build checks. Session command deduplication does not create a cross-process wallet/sidecar transaction.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
