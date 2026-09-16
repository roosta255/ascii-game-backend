# API Findings — Match API surface

Companion to `api/match.md`. These are explicit findings from documenting
the current `ApiController` implementation, not silently resolved or
normalized away — per the project's review philosophy: disagreements
between what exists and what a consumer would reasonably expect become
recorded findings, not guesses. Filed against both `ascii-game-backend`
(`architecture/api-findings.md`) and `ascii-game-contract`
(`api/findings.md`) so backend maintainers and API consumers see the same
list. Status: **open** — none of these have been triaged as intentional or
scheduled for a fix yet.

Source: `ascii-game-backend/src/controllers/ApiController.cpp` and
`src/view/api/*.hpp`, as of 2026-09-16.

---

## 1. Response format is inconsistent (text vs. JSON)

`join`, `leave`, and `start` return a bare text body on success
(`ApiController.cpp:332,374,406` — `invokeResponse200("Joined match", ...)`
etc.), while `createMatch`, `getMatch`, `getMatchList`, and every
character-action route return JSON. A generic HTTP client can't treat
"200 OK" uniformly across this API — it has to know per-route whether to
expect a JSON body or a plain string.

**Impact**: frontend/agent integration code must special-case these three
routes. An integration test should assert the *current* inconsistency
explicitly (so a future fix is a deliberate, visible change) rather than
assume uniformity.

**Open question**: should `join`/`leave`/`start` move to JSON for
consistency, or is plain text intentional (e.g. for simpler non-JS
clients)?

---

## 2. `409 Conflict` is overloaded with two unrelated meanings

`409` is returned for two different situations that a client cannot tell
apart from the status code alone:

- **Game-rule rejection of a well-formed action** — e.g. character-action
  routes (`ApiController.cpp:838`, `invokeResponseJson(status, ...)` where
  `status` is `409` when `activated == false`), and `end_turn`
  (`ApiController.cpp:647`, `codeset.describe("End turn rejected due to: ")`).
- **Optimistic-lock save conflict** — `join`/`leave`/`start`/`createMatch`
  all return `409` when `matchRepository.save()` fails due to a version
  mismatch (`ApiController.cpp:326,368,400` and the equivalent in
  `createMatch`).

**Impact**: a client that wants to distinguish "retry with fresh state"
(save conflict — genuinely transient) from "this action was rejected by
game rules" (not a conflict — retrying with the same input will fail again)
cannot do so from the status code; it has to parse the body.

**Open question**: should save conflicts use a different status (e.g.
`412 Precondition Failed`), or should the body's `codeset`/error shape
carry an explicit discriminator?

---

## 3. `activate_inventory_item`'s `item`/`source_item`/`target_item` precedence is implemented but undocumented

`performCharacterActionImpl` accepts three overlapping fields
(`ApiController.cpp:768,773,789`):

```
targetItemIndex  = target_item, else item, else empty
sourceItemIndex   = source_item, else item, else empty
targetPreactivationEntity = PreactivationTargetItem{ target_item, else item }
```

So a bare `item` field is used as **both** source and target when the more
specific fields are absent, but `source_item`/`target_item` take precedence
independently — meaning a request could set `source_item` and rely on
`item` for the target, or vice versa. This precedence is real and
intentional-looking code, but no caller-facing documentation states it, and
`activate_inventory_item`'s own required-field check
(`ApiController.cpp:520`) only requires *one of the three*, not a specific
combination.

**Impact**: a frontend implementer has to read the backend source to know
which field wins when more than one is present. High risk of a client
sending `item` + `target_item` together and getting the "wrong" one used as
source, silently.

**Ask**: a worked example (2-3 request bodies with expected
source/target resolution) should be added to `api/match.md` once the
intended precedence is confirmed with whoever owns this route.

---

## 4. `GET /api/flyweight/{name}` only implements `name=rules`

`ApiController::getFlyweight` (`ApiController.cpp:972` onward) special-cases
exactly one value, `"rules"`; every other `name` returns `404 Unknown
flyweight: <name>`. Every other flyweight type (roles, doors, locks, items,
animations) is only reachable in bulk via `GET /api/flyweights`.

**Impact**: the route reads as a general `{name}`-keyed lookup but isn't
one. Not clear whether this is a deliberate "only rules needs individual
lookup" decision or an unfinished generalization.

**Open question**: is per-name lookup for the other flyweight types
planned, or should this route be renamed/narrowed to reflect that it's
rules-only today?

---

## 5. `RuleFlyweightApiView` resolves `items` to text but leaves `doors`/`roles` as raw enum ints

In `RuleFlyweightApiView::serializeMatch` (`src/view/api/RuleFlyweightApiView.hpp`):

```cpp
itemsArr.push_back(item_to_text(m.items.getOrDefault(i, ITEM_UNALLOCATED)));   // resolved to text
doorsArr.push_back((int)m.doors.getOrDefault(i, DOOR_COUNT));                  // raw int
rolesArr.push_back((int)m.roles.getOrDefault(i, ROLE_COUNT));                  // raw int
```

Every other enum-backed field in the entire Match API is resolved to its
symbolic name before being sent (roles, doors, locks, items, animations,
actions, traits, generators, layouts, rooms — see `api/match.md`
throughout). This is the one place a client receives a bare integer and has
to already know the enum ordering to interpret it — a real inconsistency
with the rest of the contract, not a style nit.

**Impact**: any consumer of `GET /api/flyweight/rules` needs backend enum
ordering (`DoorEnum`, `RoleEnum`) to make sense of `matches.*.doors` /
`matches.*.roles`, unlike every other field they'll encounter in this API.

**Suggested fix**: resolve via `DoorFlyweight`/`RoleFlyweight` name lookup,
same pattern already used for `items` two lines above.

---

## 6. (Discovered while documenting the nested types, not one of the original five)

**Non-finding, recorded so it isn't re-flagged later**: `RuleFlyweightApiView`'s
trait serialization (`traitsToJson`, and the `operator<<` in
`TraitEnum.hpp`) calls a function named `action_to_text(int)`. This looks
at first read like a copy-paste bug pulling in the *action* enum's
name lookup for *trait* indices. It is not: `action_to_text` is
**overloaded** — `TraitEnum.cpp` defines `action_to_text(int)` for trait
indices, distinct from `ActionEnum.cpp`'s `action_to_text(const
ActionEnum&)`. Overload resolution picks the right one by parameter type.
Confusing naming (two unrelated enums' text-lookup functions share a name),
but functionally correct. Worth a rename for clarity in a future cleanup,
not a correctness fix.
