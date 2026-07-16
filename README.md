# SML Validator (Mod Validation)

An editor-time validation plugin for **Satisfactory** mods. It catches a class of
configuration mistakes that compile and cook fine but **crash the shipped game at
runtime** — before you package with Alpakit, not after a player reports a crash.

Because modders typically can't test the packaged (Shipping) build the way it actually
runs, these mistakes are easy to ship. This plugin turns them into up-front validation
errors during Blueprint/asset validation and packaging.

## What it checks

Two kinds of rules, both data-driven (configure in Project Settings, no recompile needed):

### 1. Required function-call input pins (`RequiredPins`)
Flags a Blueprint graph that calls a function but leaves a required input pin unconnected
(and without a non-null default). Seeded example:

- `FGPowerConnectionComponent::SetPowerInfo(powerInfo)` — passing a null/None here crashes
  the game when the connection is left unwired.

Fields: `ClassName` (optional owning class, U/A-prefix-insensitive), `FunctionName`, `PinName`.

### 2. Class-valued property constraints (`ClassPropertyRules`)
Inspects a `TSubclassOf` / soft-class property on a Blueprint's class defaults and fails the
asset when it's wrong. Catches "attached the wrong kind of class" and dangling/renamed-away
references — the classic `nullpeter` build-menu crash. Each rule supports:

- **Must-be-set** (`bMustBeSet`) — the property must reference a non-null class; an unset or
  dangling (renamed-away) reference fails.
- **Type check** (`RequiredBaseClass`) — the referenced class must derive from this.
- **Pairing check** (`WhenValueDerivesFrom` + `RequiredOwnerBaseClass`) — when the referenced
  class derives from X, the owning class must derive from Y.

Seeded examples (general FactoryGame footguns):

| Owner | Property | Rule |
|---|---|---|
| `FGBuildable` | `mHologramClass` | must derive from `FGHologram`; if it's an `FGRailroadTrackHologram`, the buildable must be an `FGBuildableRailroadTrack` |
| `FGBuildingDescriptor` | `mBuildableClass` | must be set (null/dangling → build-menu crash on select) |
| `FGVehicleDescriptor` | `mVehicleClass` | must be set |
| `FGVehicle` | `mHologramClass` | must be set |

Fields: `OwnerClassName`, `PropertyName`, `bMustBeSet`, `RequiredBaseClass`,
`WhenValueDerivesFrom`, `RequiredOwnerBaseClass`. All class names match with or without the
`U`/`A` prefix, anywhere up the parent chain.

## Installing

Drop the `ModValidation` folder into your project's `Mods/` (or `Plugins/`) directory and
regenerate/build. It's an **Editor-only** module, enabled by default — nothing ships in your
packaged mod.

## Configuring

**Project Settings → Plugins → Mod Validation.** Add or edit entries in `RequiredPins` and
`ClassPropertyRules`; they're saved to `Config/DefaultEditor.ini`. The seeded defaults above
are only applied on first run (when no entry exists yet); once you've edited the lists, the
`.ini` wins.

## Notes

- Built against the Coffee Stain Unreal Engine fork used by [SML](https://github.com/satisfactorymodding/SatisfactoryModLoader).
- Rule matching is name-based (strings) so it works without hard C++ dependencies on the
  classes being validated.
