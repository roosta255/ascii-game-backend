# Puzzle Dungeon Agent — Implementation Plan

The agent should be a **small deterministic orchestration system around Claude/Codex/local models**, rather than one giant autonomous agent.

---

## 0. Environment reality & plan adjustments (updated 2026-09-09)

This plan was originally written against invented names. The real world:

| Plan term | Actual |
|---|---|
| `puzzle-dungeon-agent` repo | **`ascii-game-agent`** (github.com/roosta255/ascii-game-agent) — public, Python. Currently one stub commit: `agent.py` (token-frequency file scoring + TODO-banner insertion + PR creation, **no LLM**), `functions.json`, `design.txt`. **Decision: rewrite this repo in place** with the architecture below. |
| `backend` repo | **`ascii-game-backend`** — this repo. C++17, Drogon, CMake + Ninja, Catch2 (git submodule at `extern/Catch2`), Docker. **Private.** Builds only inside its devcontainer. |
| `frontend` repo | **`ascii-game-frontend`** — React + TypeScript + Vite, GitHub Pages, ~73 commits. Public. |
| `game-contract` repo | **Created as `roosta255/ascii-game-contract`** (public, currently empty — see Phase 0b). Today the contract is implicit: `src/view/api/` in backend, `match_api_final.zip`, `functions.json`, frontend hardcoding; that's what backfills it. |
| Workspace `/workspaces/puzzle-dungeon/` | Rename to `/workspaces/ascii-game/` (or keep book6's current layout). |

**Decisions locked:**

1. **Rewrite `ascii-game-agent`.** Archive the current `agent.py` / `functions.json` / `design.txt` under `legacy/` on first commit; do not carry their `/state/tasks` + `/instructions` model forward.
2. **`ascii-game-contract` repo created** (Phase 0b, done); wire it as a submodule of the workspace and backfill it from `src/view/api/` + frontend usage.
3. **Orchestrator runs on the DGX Spark; C++ builds are delegated to Docker.** `agent build` / `agent test` shell into the backend's devcontainer/Docker image rather than compiling natively on the Spark. The Spark hosts the Python orchestrator + local models only.
4. **Provision the Spark first.** Milestone 0 (Phase 0 + 0b) happens before any agent code, so everything is built and tested on the target host from day one.

**Resolved — agent repo visibility.** `ascii-game-agent` is being flipped to **private** (github.com → repo → Settings → Danger Zone → Change visibility) before Milestone 3, so `context/` and `memory/` can hold real backend architecture detail without leaking it publicly. Do this before generating the game model; afterward, anything that assumed anonymous/public clone access (CI, `clone_private_backend.sh`, doc links) needs a deploy key or PAT, same as `ascii-game-backend` already requires.

**Actual build/test commands to record in `workspace.yaml` (Phase 0):**

```text
backend  setup:  git submodule update --init --recursive
backend  build:  (in devcontainer/Docker) cmake -G Ninja -B build_ninja && cmake --build build_ninja
backend  test:   ./build_ninja/<test-binary>            # Catch2
backend  run:    docker compose -f docker-compose.dev.yml up
frontend setup:  npm install
frontend build:  npm run build
frontend dev:    npm run dev            # localhost:5173
frontend lint:   npx eslint .
```

CI already exists — reuse it, do not reinvent in Phase 18: backend `.github/workflows/pr-validation.yml` (+ `deploy-gamma.yml`, `deploy-prod.yml`), frontend has its own.

Milestone 3 must **seed** the game ontology from existing docs, not start blank: backend `architecture/` (`designdoc.txt`, `procgen_plan.md`, `puzzle_design_principles.md`, `fields-configuration.md`, `traits-configuration.md`, `keyframe_test_plan.md`) and frontend `animation-overlay-system.md`.

---

The key principle:

> **The agent decides what to do; deterministic tooling decides what actually happened.**

## 1. Target architecture

```text
                         DGX SPARK
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│                    Agent Orchestrator                       │
│                         │                                    │
│          ┌──────────────┼──────────────┐                     │
│          │              │              │                     │
│      Analyst        Architect       Implementer              │
│          │              │              │                     │
│          └──────────────┼──────────────┘                     │
│                         │                                    │
│                    Reviewer                                  │
│                         │                                    │
│                  Debugger / Fixer                            │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ Deterministic Tools                                    │  │
│  │                                                        │  │
│  │ code index │ git │ build │ tests │ validators │ diff  │  │
│  └────────────────────────────────────────────────────────┘  │
│                                                              │
└──────────────────────────┬───────────────────────────────────┘
                           │
                    Workspace
                           │
             ┌─────────────┴─────────────┐
             │                           │
       backend repo                 frontend repo
             │                           │
             └─────────────┬─────────────┘
                           │
                    ascii-game-contract
```

Do not initially build this as a distributed microservice system. One Python orchestrator running jobs is enough.

---

# 2. Rewrite the agent repository (`ascii-game-agent`)

Do **not** create a new repo. Rewrite `ascii-game-agent` in place. First commit: move the existing `agent.py`, `functions.json`, `design.txt` to `legacy/` (reference only), then lay down:

This repository contains the **knowledge and machinery for developing the game**, not the game itself.

```text
ascii-game-agent/
├── agent/
│   ├── cli/
│   ├── orchestrator/
│   ├── models/
│   ├── stages/
│   ├── tools/
│   ├── git/
│   ├── indexing/
│   ├── validation/
│   └── storage/
│
├── context/
│   ├── 01-game-overview.md
│   ├── 02-system-architecture.md
│   ├── 03-game-ontology.md
│   ├── 04-gameplay-invariants.md
│   ├── 05-backend-architecture.md
│   ├── 06-frontend-architecture.md
│   ├── 07-api-contracts.md
│   ├── 08-content-system.md
│   └── 09-current-state.md
│
├── schemas/
│   ├── job.schema.json
│   ├── requirements.schema.json
│   ├── implementation-plan.schema.json
│   └── review.schema.json
│
├── skills/
│   ├── analyze/
│   ├── architect/
│   ├── implement/
│   ├── review/
│   ├── debug/
│   ├── recover/
│   ├── remember/
│   └── sync-game-model/
│
├── jobs/
│
├── scripts/
│
├── tests/
│
├── AGENTS.md
└── README.md
```

The actual game workspace:

Workspace directory names match repo names exactly (as set up on the Spark):

```text
/workspaces/ascii-game/
├── ascii-game-backend/    (private)
├── ascii-game-frontend/   (public)
├── ascii-game-contract/   (public; created in Phase 0b)
└── ascii-game-agent/      (private)
```

---

# 3. Phase 0 — Provision the DGX Spark, then establish the workspace

The Spark has had **no setup**. Do this before writing any agent code.

**0a — Spark host provisioning**

```text
- OS + updates, SSH access, GPU drivers verified (nvidia-smi)
- Python 3.11+ + uv/venv
- Local model runtime (Ollama or vLLM) + pull baseline models
- Docker + Docker Compose (needed: builds are delegated to containers)
- git, gh CLI, credentials:
    - GitHub PAT / deploy keys for ascii-game-backend (private), ascii-game-frontend (public), ascii-game-agent (private), ascii-game-contract (public)
    - Hosted API keys (Claude / Codex) in a secrets store, not the repo
- Network egress to github.com + api.anthropic.com confirmed
- Clone the workspace layout under /workspaces/ascii-game/, one directory per repo, directory name == repo name:
    ascii-game-backend/ ascii-game-frontend/ ascii-game-agent/ ascii-game-contract/
- Build the backend devcontainer/Docker image once so `agent build` has something to shell into
```

**Status as of 2026-09-16: Phase 0a complete.** Repos cloned into `/workspaces/ascii-game/`; `docker build -t ascii-game-backend:dev .` succeeds natively on the Spark's ARM64 (Grace) CPU with the Dockerfile unchanged (262s, all steps green) — the ARM64 compatibility risk called out below is resolved. Docker itself needed `sudo usermod -aG docker $USER` + a fresh login (group membership wasn't applied automatically). Python and Ollama confirmed installed (DGX OS ships Ollama preinstalled, as its group already suggested). Baseline local model pulled: **`qwen3-coder:30b`** — candidate for the `implement`/`debugger` routing slots in Phase 16. Anthropic API key created (expiring; rotation procedure in `ascii-game-backend/runbook/claude-api-key-rotation.md`).

**0b — Create the `ascii-game-contract` repo** (see the dedicated Phase 0b section below). *(Done — created as `roosta255/ascii-game-contract`, public, currently empty.)*

**0c — workspace.yaml**

```yaml
# workspace.yaml

project: ascii-game
host: dgx-spark            # orchestrator + local models
build_delegation: docker  # C++ builds run in containers, not natively

repositories:
  backend:
    repo: roosta255/ascii-game-backend
    path: ./ascii-game-backend
    language: cpp
    visibility: private
    branch_policy: agent-branch
    setup: git submodule update --init --recursive
    build: docker-exec cmake -G Ninja -B build_ninja && cmake --build build_ninja
    test:  docker-exec ./build_ninja/<test-binary>
    run:   docker compose -f docker-compose.dev.yml up

  frontend:
    repo: roosta255/ascii-game-frontend
    path: ./ascii-game-frontend
    language: typescript
    visibility: public
    branch_policy: agent-branch
    setup: npm install
    build: npm run build
    test:  npm test
    lint:  npx eslint .

contracts:
  repo: roosta255/ascii-game-contract
  path: ./ascii-game-contract
  visibility: public

agent:
  repo: roosta255/ascii-game-agent
  path: ./ascii-game-agent
  visibility: private   # flipped before Milestone 3; see §0
```

The orchestrator should execute:

```bash
agent workspace validate
```

and verify:

- Spark provisioning checklist (0a) passes
- ascii-game-backend / ascii-game-frontend / ascii-game-contract / ascii-game-agent all cloned
- Git repositories are valid; expected branches exist
- backend submodules initialised
- build commands work (via Docker delegation for backend)
- test commands work
- contract directory exists and is a valid submodule
- agent context exists
- local model runtime reachable; hosted API keys resolve

---

# 3b. Phase 0b — Create the `ascii-game-contract` repository

Public repo `roosta255/ascii-game-contract` — created, currently empty, added as a submodule of the workspace.

Initial contents, backfilled from what exists today (`src/view/api/` in backend, `match_api_final.zip`, `functions.json`, frontend API usage):

```text
ascii-game-contract/
├── api/            # endpoint definitions: method, path, request, response
│   └── match.md    # start from the existing /api/match surface
├── schemas/        # JSON Schema for every type crossing the boundary
├── enums/          # shared enum value sets (RoleEnum, WeatherType, …)
└── README.md       # "backend is source of truth; this is generated + reviewed"
```

Rules:

- Backend is the source of truth. `/sync-game-model` regenerates candidate contract artifacts; humans review the diff.
- The cross-repo reviewer (Phase 12) checks backend symbol ↔ contract ↔ frontend symbol against **this** repo.
- Do not block Milestone 1–2 on the contract being complete; a stub with just the match API is enough to start.

---

# 4. Phase 1 — Build repository intelligence

This is probably the **highest-value first implementation**.

The agent should have deterministic knowledge of the codebase before asking an LLM to reason about it.

Build an index containing:

```text
Files
Classes
Structs
Enums
Functions
Methods
Members
Includes
Inheritance
Callers
Callees
References
API endpoints
Types crossing API boundary
Tests
Build targets
```

For the C++ backend:

```text
class → methods
function → callers
function → callees
file → includes
enum → usages
struct → usages
endpoint → handler
```

For TypeScript:

```text
component → imports
function → callers
type/interface → usages
API call → endpoint
component → rendered state
```

Expose deterministic commands:

```bash
agent code search "move_character"

agent code symbol Character

agent code callers move_character

agent code references RoleEnum

agent code files MatchRenderer.tsx

agent code endpoint /api/match
```

The LLM should use these tools rather than repeatedly searching the entire repositories.

---

# 5. Phase 2 — Inventory the existing system

Before implementing autonomous coding, run a one-time **Game Model Synchronization** job.

Input:

```text
backend
frontend
existing documentation
existing functions.json
API definitions
configuration/content
```

Output:

```text
context/
    01-game-overview.md
    02-system-architecture.md
    03-game-ontology.md
    05-backend-architecture.md
    06-frontend-architecture.md
    07-api-contracts.md

ascii-game-contract/
    api/
    schemas/
```

Also produce:

```text
context/09-current-state.md
```

This should describe what actually exists **now**, rather than what was intended to be built.

The agent should distinguish:

```text
DESIGN INTENT
      ↓
DOCUMENTED ARCHITECTURE
      ↓
ACTUAL CODE
      ↓
ACTUAL API
```

Disagreements become explicit findings instead of silently being reconciled.

---

# 6. Phase 3 — Implement the job system

Everything the agent does should happen through a **job**.

Example:

```text
agent/jobs/2026-09-08-weather-system/
```

Structure:

```text
weather-system/
├── job.yaml
├── input/
│   └── design.md
│
├── artifacts/
│   ├── analysis.md
│   ├── requirements.yaml
│   ├── implementation_plan.yaml
│   └── review.yaml
│
├── logs/
│   ├── analyst.log
│   ├── architect.log
│   ├── implement.log
│   ├── build.log
│   └── review.log
│
├── checkpoints/
│
└── worktrees/
    ├── backend/
    └── frontend/
```

Job state:

```yaml
job_id: weather-system-20260908

status: running
current_stage: IMPLEMENT

repositories:
  backend:
    base_commit: abc123
    branch: agent/weather-system-20260908

  frontend:
    base_commit: def456
    branch: agent/weather-system-20260908

stages:
  analyze: complete
  architect: complete
  implement: running
  build: pending
  test: pending
  review: pending
  pr: pending

attempts:
  implementation: 1
  debugging: 0
  review: 0
```

The state file is what lets you kill the agent and resume it later.

---

# 7. Phase 4 — Implement the stage machine

The orchestrator should be a straightforward state machine.

```text
INGEST
   ↓
ANALYZE
   ↓
ARCHITECT
   ↓
IMPLEMENT
   ↓
BUILD
   ↓
TEST
   ↓
REVIEW
   ↓
   ├── PASS ───────→ PR
   │
   └── CHANGES
          ↓
        FIX
          ↓
        BUILD
          ↓
        TEST
          ↓
        REVIEW
```

With hard limits:

```yaml
limits:
  implementation_attempts: 2
  debug_attempts: 3
  review_cycles: 3
```

If the limit is exceeded:

```text
FAILED
```

Do not let an LLM loop indefinitely.

---

# 8. Phase 5 — Implement `/analyze`

The Analyst receives:

```text
design.md
game ontology
relevant architecture
repository metadata
relevant indexed code
```

It produces:

```yaml
requirements:

  - id: WEATHER-001
    description: ...
    acceptance_criteria:
      - ...

  - id: WEATHER-002
    description: ...
```

And:

```text
analysis.md
```

The Analyst should explicitly record:

```yaml
ambiguities:
  - ...

assumptions:
  - ...

out_of_scope:
  - ...
```

The model should **not invent game mechanics** merely because the design document is incomplete.

---

# 9. Phase 6 — Implement `/architect`

The Architect gets:

```text
requirements.yaml
game ontology
backend architecture
frontend architecture
API contracts
code index
relevant source
```

It produces:

```yaml
implementation_plan:

  backend:
    files:
      - path: ...
        symbols:
          - ...
        changes:
          - ...

  frontend:
    files:
      - path: ...
        symbols:
          - ...
        changes:
          - ...

  contract:
    changes:
      - ...

  tests:
    backend:
      - ...

    frontend:
      - ...

    integration:
      - ...

  requirements:
    WEATHER-001:
      - ...
```

The requirement mapping is crucial.

It lets the Reviewer answer:

> Did the implementation actually satisfy WEATHER-001?

rather than merely:

> Does this code look reasonable?

---

# 10. Phase 7 — Build the worktree manager

This is one of the most important safety components.

The agent never works directly on:

```text
main
```

Instead:

```text
backend:
    main
      │
      └── agent/weather-system-20260908

frontend:
    main
      │
      └── agent/weather-system-20260908
```

The orchestrator creates isolated worktrees.

Conceptually:

```python
worktree.create(job)
worktree.status(job)
worktree.diff(job)
worktree.destroy(job)
```

Every operation should know:

```text
job ID
repository
branch
worktree path
base commit
```

The coding agent receives **only those worktrees**.

---

# 11. Phase 8 — Implement the coding agent

Do not make the coding agent responsible for orchestration.

Its job is simply:

```text
read plan
↓
inspect code
↓
modify code
↓
run local validation
↓
report
```

Its tool set should be deliberately constrained:

```text
read_file
search_code
find_symbol
find_callers
find_callees
write_file
edit_file
run_command
git_diff
git_status
```

No:

```text
git push
git merge
production credentials
arbitrary SSH
```

The implementation prompt should explicitly say:

> Implement the provided plan. Do not redesign the architecture unless the plan is demonstrably impossible. If the plan conflicts with the actual code, stop and report the conflict.

This prevents autonomous-agent behavior from silently rewriting the specification.

---

# 12. Phase 9 — Deterministic build/test system

This should **not use an LLM**.

Create:

```bash
agent build      # backend: shells into the backend Docker image / devcontainer
agent test       # backend: runs the Catch2 binary inside the container
agent validate
agent lint
```

Because the orchestrator runs on the Spark and the backend toolchain lives in Docker, `agent build`/`agent test` for the backend must `docker exec` (or `docker compose run`) against the backend image over the mounted worktree. The frontend builds natively on the Spark (Node). Capture the same structured result regardless of where it ran.

The orchestrator captures:

```yaml
build:
  exit_code: 0
  duration_seconds: 41
  stdout: ...
  stderr: ...

tests:
  exit_code: 1
  passed: 147
  failed: 2
```

For the game, eventually:

```text
agent validate content
agent validate contracts
agent validate game-model
agent validate puzzle
```

This gives the LLM concrete evidence instead of asking it whether it thinks the implementation works.

---

# 13. Phase 10 — Implement the Debugger

Only invoke the debugger if deterministic validation fails.

Give it:

```text
original requirements
implementation plan
git diff
compiler output
test output
relevant source
previous attempts
```

Then:

```text
diagnose
↓
identify root cause
↓
modify code
↓
build
↓
test
```

Limit:

```text
3 attempts
```

After that:

```text
FAILED
```

with a useful diagnostic artifact.

---

# 14. Phase 11 — Implement the independent Reviewer

The Reviewer should be a **different model invocation** from the coding agent.

Ideally:

```text
Coder: local model
Reviewer: Claude
```

or vice versa.

Give the Reviewer:

```text
design
requirements
architecture plan
git diff
test results
```

Not the coder's private reasoning.

Output:

```yaml
result: PASS

requirements:
  WEATHER-001:
    status: satisfied
    evidence:
      - ...

architecture:
  status: pass

tests:
  status: pass

scope:
  status: pass

findings: []
```

Or:

```yaml
result: CHANGES_REQUESTED

findings:
  - id: REVIEW-001
    severity: high
    description: ...
    location: ...
    required_change: ...
```

---

# 15. Phase 12 — Add the cross-repository reviewer

For a full-stack change:

```text
Backend
   ↓
API contract
   ↓
Frontend
```

the Reviewer needs to verify the entire chain.

Example:

```text
Backend:
    WeatherState.weather

Contract:
    weather

Frontend:
    WeatherState.weatherState
```

Unit tests could all pass while the game still breaks.

Therefore add:

```text
agent integration-test
```

which tests the actual boundary.

---

# 16. Phase 13 — Implement `/recover`

Recovery should be deterministic first.

Given a failed job:

```bash
agent recover weather-system-20260908
```

The recovery system reads:

```text
original design
requirements
implementation plan
git diff
build output
test output
review findings
job state
previous attempts
```

Then determines:

```text
What was the intended state?
What changed?
Where did the implementation diverge?
What is the safest checkpoint?
```

Possible outcomes:

```text
resume
rollback last attempt
request re-architecture
mark failed
```

Do not let an agent blindly "try again" from a broken state.

---

# 17. Phase 14 — Implement `/remember`

At the end of every successful job, generate structured memory.

For example:

```text
agent/memory/
├── architecture/
├── decisions/
├── discoveries/
├── mistakes/
└── sessions/
```

A decision:

```yaml
decision: API weather state is server-authoritative

reason:
  Frontend should never determine game mechanics.

rejected:
  Client-side weather simulation

consequence:
  Frontend only renders weather received from server.
```

This becomes future context.

---

# 18. Phase 15 — Implement `/sync-game-model`

This should be a special deterministic + LLM-assisted job.

Run periodically:

```bash
agent sync-game-model
```

It compares:

```text
C++ implementation
       │
       ├── enums
       ├── classes
       ├── systems
       └── state

against

game ontology
       │
       ├── documented systems
       ├── invariants
       └── contracts
```

Example:

```text
C++ contains:
    WeatherType::ACID_RAIN

Ontology contains:
    WeatherType:
        CLEAR
        RAIN
        SNOW

→ MODEL DRIFT DETECTED
```

The agent reports this rather than automatically rewriting the ontology.

---

# 19. Phase 16 — Add model routing

Do not hard-code a single model.

Use:

```python
class AgentModel:
    def run(...)
```

with adapters:

```text
Claude
Codex
OpenAI-compatible endpoint
Ollama
```

Then routing:

```yaml
models:

  analyst:
    provider: claude

  architect:
    provider: claude

  implement:
    provider: local
    model: qwen3-coder:30b   # pulled on the Spark 2026-09-16

  debugger:
    provider: local
    model: qwen3-coder:30b

  reviewer:
    provider: claude

  summarizer:
    provider: local-small
```

The Spark becomes particularly valuable here.

Routine work can stay local while higher-value architectural/review decisions can use stronger hosted models.

---

# 20. Phase 17 — Build the CLI

The first usable interface should be boring.

```bash
agent init

agent workspace validate

agent index

agent sync-game-model

agent job create design.md

agent job status <job>

agent job run <job>

agent job pause <job>

agent job resume <job>

agent job recover <job>

agent job review <job>

agent job diff <job>

agent job logs <job>

agent pr <job>
```

Eventually:

```bash
agent run design/weather.md
```

could execute the entire pipeline.

---

# 21. Phase 18 — GitHub integration

Only after everything above works locally. **Reuse existing CI, don't reinvent it:** backend `.github/workflows/pr-validation.yml` (+ `deploy-gamma.yml`, `deploy-prod.yml`); frontend has its own. The agent opens PRs into these existing checks.

The final pipeline becomes:

```text
Design
  ↓
Requirements
  ↓
Architecture
  ↓
Implementation
  ↓
Build
  ↓
Tests
  ↓
Review
  ↓
Fix
  ↓
Review
  ↓
Git commit
  ↓
Push agent branch
  ↓
Create PR
```

For two repositories:

```text
Job: weather-system-20260908

PR #184 backend
PR #91 frontend
```

The job records that they're related.

The agent **does not merge** them.

---

# 22. Phase 19 — Add human approval boundaries

Initially, require approval at these points:

```text
Design
  ↓
[HUMAN]
  ↓
Architecture
  ↓
[HUMAN]
  ↓
Implementation
  ↓
automatic
Build
  ↓
automatic
Tests
  ↓
automatic
Review
  ↓
[HUMAN if changes requested]
  ↓
PR
  ↓
[HUMAN MERGE]
```

Once you trust it, architecture approval can become optional for low-risk changes.

---

# 23. Implementation order

Do not build everything at once.

## Milestone 0 — Provision & scaffold (do first)

```text
DGX Spark host provisioning (Phase 0a)
ascii-game-contract repo created + backfilled with the match API (Phase 0b)
ascii-game-agent rewritten: legacy/ moved aside, skeleton laid down
workspace cloned on the Spark under /workspaces/ascii-game/
backend Docker image built once (build delegation target)
```

## Milestone 1 — Agent foundation

Build:

```text
workspace.yaml (with real repo names + commands)
CLI
job.yaml
job state machine
logging
configuration
```

No LLM yet.

**Status: done (2026-09-16), `ascii-game-agent` branch `milestone-1/agent-foundation`,
pending merge.** `agent init`/`workspace validate`/`job create,list,status,logs,pause,resume`
are real and covered by 54 passing tests (config, job persistence, the
state machine, logging, validate, CLI — no network/model/Docker
required). `job run/diff/recover/review`, `index`, `sync-game-model`, `pr`
are wired into the CLI's final shape but stubbed (exit 2, name the
milestone that implements them) since their stages don't exist yet. See
`ascii-game-agent/AGENTS.md` → Current status for the file-by-file
breakdown.

## Milestone 2 — Repository intelligence

Build:

```text
C++ index
TypeScript index
code search
symbol lookup
caller/callee lookup
API endpoint discovery
```

**Status: done (2026-09-16), `ascii-game-agent` branch `milestone-2/repo-intelligence`,
pending merge.** tree-sitter-based index (`agent/indexing/`), validated
directly against the real backend (466 files) and frontend (45 files),
zero crashes. `agent index` + `agent code
search/symbol/callers/callees/references/files/endpoint` all real,
including the exact example commands from plan §4 (`search
"move_character"`, `symbol Character`, `references RoleEnum`, `files
MatchRenderer.tsx`, `endpoint /api/match`). Found and worked around a real
tree-sitter grammar limitation on Drogon's `ADD_METHOD_TO` macro pattern,
and a more serious one where the codebase's `X.enum` macro-body convention
(used by nearly every real enum — Role, Action, Door, Trait, ...) hides
the enum from tree-sitter entirely; both documented in
`ascii-game-agent/agent/indexing/README.md` rather than silently worked
around. Call edges are name-based, not overload-resolved — also
documented there.
## Milestone 3 — Game model

Generate (seeding from existing docs — backend `architecture/*.md`, frontend `animation-overlay-system.md` — not from scratch):

```text
game ontology
backend architecture
frontend architecture
API contract  → populates ascii-game-contract/
current-state document
```

Run `/sync-game-model`. (The public-repo caveat from §0 is resolved — `ascii-game-agent` is private — so `context/`/`memory/` can hold real backend detail.)

**Status: done (2026-09-16), `ascii-game-agent` branch `milestone-3/game-model`
(stacked on the still-unmerged `milestone-2/repo-intelligence`), plus a
direct-to-main commit on `ascii-game-contract`, both pending review.**
All 9 `context/*.md` files populated by direct synthesis + source reads
(not this milestone's own tooling — `/sync-game-model` as an automated
command is still Milestone 9). Found and documented three real DESIGN
INTENT vs. ACTUAL CODE drifts in `context/09-current-state.md`: a planned
`DungeonLayoutMapper` class that was never built, `procgen_plan.md`'s
planned file layout not matching the real `iAtomEmbedder`/
`GraphPathEmbedder`/`SerpentineEmbedder` implementation, and a stale
room-count constant (96 planned vs. 64 actual). Also populated
`ascii-game-contract/schemas/` (3 validated JSON Schema files) and
`enums/` (9 of 25 enum families, script-extracted from the real `.enum`
files since the Milestone 2 code index can't resolve macro-body enum
values) — both repos' READMEs record exactly what's still missing.

## Milestone 4 — Safe implementation

Build:

```text
Git worktrees
branch manager
coding-agent adapter
diff collection
```

Test it on a deliberately tiny change.

**Status: done (2026-09-16), `ascii-game-agent` branch `milestone-4/safe-implementation`,
pending review.** `WorktreeManager` (create/status/diff/destroy, one
`agent/<job_id>` branch + worktree per job per repo, main checkout never
touched) and the coding-agent adapter (`read_file`/`write_file`/
`edit_file`/`search_code`/`find_symbol`/`find_callers`/`find_callees`/
`git_diff`/`git_status`/`run_command`, path-escape-blocked, `run_command`
refusing `git push`/`git merge`/SSH) are both real, backed by the
Milestone 2 code index where cached. `agent worktree create/status/diff/destroy`
and a real `agent job diff` are wired into the CLI. Tested per this
milestone's own instruction on a real, tiny change against the actual
`ascii-game-backend` repo (not just fixtures) — created a real worktree +
branch, edited a file through the adapter, diffed it, destroyed it,
confirmed `main` was untouched throughout, then cleaned up completely.

## Milestone 5 — Deterministic validation

Build:

```text
agent build
agent test
agent validate
```

and connect them to the job state machine.

**Status: done (2026-09-16), `ascii-game-agent` branch
`milestone-5/deterministic-validation`, pending review.** `agent
build`/`agent test`/`agent lint` run a job's declared command in its
worktree, capture exit_code/duration/stdout/stderr, persist it as a job
artifact, and — only when the job is at the matching stage — advance the
state machine with the real verdict. `agent validate
content/contracts/game-model/puzzle` (the game-specific semantic
validators) are stubbed, correctly deferred to Milestone 8/9. Docker
delegation is implemented and unit-tested for correct command
construction, but its bind-mount step couldn't be validated end-to-end in
book6's sandboxed Docker (mounts an empty tmpfs there regardless of host
path) — confirm on the Spark before trusting it in a real job. The
direct-execution path was validated for real against `ascii-game-backend`
(including a genuine pass and a genuine failure), which also surfaced
that a fresh worktree has no out-of-tree build directory until something
configures it there — a gap for Milestone 6's IMPLEMENT stage to own.

## Milestone 6 — Full pipeline

Implement:

```text
ANALYZE
ARCHITECT
IMPLEMENT
BUILD
TEST
```

**Revised 2026-09-16 — model architecture is three paths, not one
provider config.** Discussed once it became clear Milestone 4's
`agent/tools/adapter.py` duplicates what Claude Code itself already does
well, and that IMPLEMENT specifically should be able to hand off to an
actual Claude Code agent running on the Spark rather than a bespoke tool
loop for everything:

1. **Raw Claude API** (`anthropic` SDK + `ANTHROPIC_API_KEY` from the
   existing secrets convention) — ANALYZE/ARCHITECT/REVIEW. These are
   non-agentic: text/context in, structured YAML out, no file access
   needed, so the full Claude Code agent loop would be overkill.
2. **Local Ollama + the Milestone 4 tool adapter** — the *default* for
   IMPLEMENT/DEBUG, per the original Phase 16 routing
   (`qwen3-coder:30b`). This is why the Milestone 4 adapter still
   matters: Ollama isn't Claude Code, so the local path needs its own
   agentic tool loop, and that's what it is.
3. **Headless Claude Code** (`claude -p`, agentic, real tool use) — the
   *escalation* path for IMPLEMENT/DEBUG when the local model fails
   (tying into the existing `attempts.debugging`/`attempts.implementation`
   counters already in the state machine), scoped to the job's worktree
   via cwd. Fully headless/unattended — no human watches it run; review
   happens afterward via `agent job diff`/the PR, same as everything
   else. This is the concrete answer to "can a Claude Code agent on the
   Spark work on the dev stack" — yes, this is how.

Connecting VSCode (Remote-SSH + Dev Containers) from book6 to a
devcontainer running on the Spark is a separate, already-supported dev
workflow (same `devcontainer.json` this repo already has) — useful for a
human's own manual work there, orthogonal to how the headless orchestrator
runs.

**Session constraint, found while starting this milestone:** book6 has no
`ANTHROPIC_API_KEY`, no local Ollama, and no `claude` CLI binary at all —
none of the three model paths can be live-tested from here. Each adapter
is built as real, correct code and tested against a real-but-fake stand-in
(a fake local HTTP server shaped like Ollama/Anthropic's API, a fake
`claude` executable on PATH) rather than pure mocking — but genuine live
calls need validating on the Spark, where the real key/Ollama/CLI belong.
Building one stage at a time, starting with ANALYZE (path 1, simplest —
no tools, no escalation logic).

**Status: ANALYZE done and live-confirmed (2026-09-17).** Merged
(`ascii-game-agent` main). `agent job run <job_id>` executes it for real
(reads the job's design doc + `context/01,03,04,05,06,07,09`, calls the
raw Claude API, writes `requirements.yaml`/`analysis.md`, advances the
state machine) — called at any other stage it says so plainly rather than
doing nothing.

**Live-validated on the Spark for real**, closing the gap book6
structurally couldn't (no `ANTHROPIC_API_KEY`/Ollama/`claude` CLI there —
by design, not an oversight, since the orchestrator's only intended
runtime is the Spark, §0 decision 3). `scripts/live_validate_analyze.sh`
ran a real job ("add a `foo` boolean field to match state, default
false") through a genuine Claude API call: one correctly-scoped
requirement, real acceptance criteria, and ambiguities/assumptions that
were actually the right ones to flag (API/store exposure, later
mutation, which exact model — nothing invented beyond the design doc).
`current_stage` advanced ANALYZE → ARCHITECT correctly. Two real
environment-friction bugs found and fixed along the way (the script
assuming `agent` was on PATH, and a relative `--workspace` path breaking
under the `uv run` fallback) — see `fix/live-validate-script-robustness`
in `ascii-game-agent`.

Next: ARCHITECT (same raw-API pattern), then IMPLEMENT (needs paths 2/3,
not built).

## Milestone 7 — Review loop

Add:

```text
REVIEW
FIX
REVIEW
```

with hard limits.

## Milestone 8 — Cross-repo integration

Add:

```text
contract validation
backend/frontend integration tests
multi-repository jobs
multi-repository branches
```

## Milestone 9 — Recovery and memory

Add:

```text
RECOVER
REMEMBER
SYNC-GAME-MODEL
```

## Milestone 10 — GitHub

Finally:

```text
branch → push → PR
```

and eventually automated CI feedback.

---

# 24. The first end-to-end test

Do not initially test this with a complicated game feature.

Use something like:

> Add a `WeatherState` field to the match state, expose it through the backend API, and display the current weather in the frontend.

That exercises almost everything important:

```text
Design
 ↓
Requirements
 ↓
Architecture
 ↓
Backend
 ↓
API contract
 ↓
Frontend
 ↓
Build
 ↓
Tests
 ↓
Cross-repo validation
 ↓
Review
 ↓
PR
```

If it can successfully perform that change without human intervention beyond approval gates, you've demonstrated the architecture.

---

# 25. What not to build yet

Avoid these initially:

- vector database
- elaborate RAG pipeline
- multi-agent message bus
- Kubernetes
- distributed workers
- browser automation
- autonomous GitHub merging
- autonomous production deployment
- giant agent prompt
- complex web UI
- custom model training

None of those solve the fundamental problem yet.

The biggest leverage is:

**good project model + deterministic code intelligence + isolated worktrees + explicit job state + deterministic validation.**

---

# Final architecture

The mature system should ultimately look like:

```text
                         DESIGN
                           │
                           ▼
                    ┌─────────────┐
                    │   ANALYST   │
                    └──────┬──────┘
                           │
                    requirements.yaml
                           │
                           ▼
                    ┌─────────────┐
                    │ ARCHITECT   │
                    └──────┬──────┘
                           │
                 implementation_plan.yaml
                           │
                           ▼
              ┌─────────────────────────┐
              │    ISOLATED WORKTREES   │
              └────────────┬────────────┘
                           │
                           ▼
                    ┌─────────────┐
                    │    CODER    │
                    └──────┬──────┘
                           │
                           ▼
                ┌──────────────────────┐
                │ BUILD / TEST / VALID │
                │    deterministic     │
                └──────────┬───────────┘
                           │
                    ┌──────▼──────┐
                    │   REVIEWER  │
                    └──────┬──────┘
                           │
                ┌──────────┴──────────┐
                │                     │
              PASS              CHANGES_REQUESTED
                │                     │
                │                     ▼
                │                  FIXER
                │                     │
                │              ───────┘
                │
                ▼
             GIT BRANCH
                │
                ▼
              GitHub
                │
                ▼
               PR
```

## Core architectural decision

**The LLMs are plugins to this system, not the system itself.**

That means you can:

- swap Claude for Codex or a local model
- restart a job
- inspect exactly what happened
- recover from failures
- enforce repository boundaries
- validate behavior deterministically
- maintain project knowledge across sessions

without losing control of the development process.
