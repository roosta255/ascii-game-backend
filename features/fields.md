# Fields Feature

## Goal

Introduce environmental fields that affect characters and objects.

Examples:
- gas
- heat
- smoke
- magical zones

## Design Principles

The dungeon is a collection of interacting state machines.

Mechanics should:
- create puzzles
- modify state
- remain deterministic

## Existing Ideas

Fields should not simulate continuous physics.
They should apply discrete effects.

Examples:
- OXYGENATED
- SMOKE_INHALATION
- HEAT_STROKE
- UNCOMBUSTIBLE_COLD

## Open Questions

- Where should field propagation live?
- Should fields be room-level or tile-level?
- How do traits consume field effects?