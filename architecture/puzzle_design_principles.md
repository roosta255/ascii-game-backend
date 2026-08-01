# ASCII Dungeon Design Principles

## Core Philosophy

The dungeon is a collection of interacting state machines rather than a physics simulation. Mechanics should primarily alter the state of the world, characters, or objects in discrete and understandable ways.

---

# 1. Mechanics Create Puzzles, Not Barriers

A mechanic should introduce a problem that can be reasoned about and manipulated rather than simply preventing progress.

Examples:

* Orange/blue toggle doors.
* Item-stealing monkeys.
* Traits that can be gained or lost.

---

# 2. Prefer Reversible Failure Over Hard Failure

Negative outcomes should create complications rather than dead ends.

Failure should produce new decisions and new puzzles rather than requiring a restart.

---

# 3. Every Mechanic Should Have a Tradeoff

Every benefit should have a cost, limitation, or competing use.

Examples:

* A key opens locks but may be required elsewhere.
* A red cube can power machinery or enable time travel.
* A trait grants access to some areas while preventing access to others.

---

# 4. State Should Persist and Matter

Actions should leave lasting effects on the dungeon.

Examples:

* Switch positions.
* Item locations.
* Character traits.
* NPC memories.
* Opened or relocked passages.

Persistent state makes actions meaningful.

---

# 5. Problems Should Become Resources

Obstacles should eventually become tools.

Examples:

* A monkey may transport items.
* A dangerous trait may become useful.
* A hostile creature may be manipulated into solving another puzzle.

---

# 6. NPCs Are Puzzles First and Enemies Second

NPCs should primarily behave as moving systems that can be studied and manipulated.

The most interesting NPCs:

* change state,
* alter the world,
* have predictable behaviors,
* can be exploited.

---

# 7. Mechanics Should Compose

A mechanic should become more valuable when combined with other mechanics.

The interaction between systems is often more interesting than the systems themselves.

---

# 8. Mechanics Should Produce Interesting Decisions

The goal is to create meaningful choices rather than routine actions or repetitive tasks.

---

# 9. The Dungeon Should Allow Temporary Self-Sabotage

The player should be able to place the world into an undesirable state and later recover from it.

Experimentation should be encouraged.

---

# 10. Cause and Effect Must Be Understandable

Actions should produce predictable and learnable consequences.

The player should be able to form accurate mental models of the dungeon.

---

# 11. Mechanics Should Generate Stories

Memorable mechanics produce anecdotes and surprising situations.

A story is often more memorable than a statistic.

---

# 12. Prefer State-Based Costs Over Damage-Based Costs

Interesting costs often involve:

* losing access,
* moving resources,
* changing relationships,
* altering world state,
* gaining or losing traits.

---

# 13. Apparent Soft-Locks Should Often Be Solvable

The dungeon should occasionally appear broken while secretly containing a solution.

Discovering recovery paths is satisfying.

---

# 14. Mechanics Should Have Multiple Uses

The same mechanic should support both obvious and advanced applications.

Depth emerges from reuse.

---

# 15. Nothing Should Exist Solely to Punish

Even negative mechanics should create new opportunities, decisions, or puzzles.

---

# 16. Prefer Discrete State Changes Over Continuous Simulation

The dungeon should avoid mechanics that require continuous updates over time.

Good:

* switch toggles,
* trait gain or loss,
* item transformation,
* state transitions.

Avoid:

* countdown timers,
* slowly moving hazards,
* objects that require constant ticking,
* mechanics whose state depends on every intermediate moment.

Reason:

Pathfinding and state-space searches become dramatically more expensive when time becomes another dimension of the puzzle.

The dungeon should evolve primarily through events rather than through continuous simulation.

---

# 17. Avoid Mechanics That Multiply Temporal State Space

Every timed mechanic potentially creates additional versions of the dungeon that must be considered during planning and search.

Mechanics should ideally be:

* timeless,
* event-driven,
* turn-driven,
* activated only by interaction.

Exceptions should be rare and provide significant value.

---

# 18. Design for Touch-Targeted Interaction

The interface is based on selecting destinations and objects directly rather than issuing directional commands.

Mechanics should naturally map to:

* tapping objects,
* tapping characters,
* selecting destinations.

Avoid mechanics that fundamentally require:

* facing direction,
* directional input,
* pushing from a specific side,
* precise movement execution.

---

# 19. Prefer State Manipulation Over Physics Manipulation

The game is not a physics sandbox.

Strong mechanics:

* transform objects,
* move objects between containers,
* toggle states,
* change ownership,
* change traits,
* alter accessibility.

Weak mechanics:

* momentum,
* pushing chains,
* friction,
* directional forces.

The dungeon should feel like a collection of interacting rules rather than a simulation of physical objects.

---

# 20. Prefer Intent-Based Actions Over Dexterity-Based Actions

Actions should be chosen because they are strategically interesting rather than because they require precise execution.

The challenge should arise from reasoning about systems, not from controlling movement.

---

# Summary

1. Mechanics create puzzles, not barriers.
2. Prefer reversible failure.
3. Every mechanic has a tradeoff.
4. State changes persist.
5. Problems can become resources.
6. NPCs are puzzles first and enemies second.
7. Mechanics should compose.
8. Decisions should be interesting.
9. Allow temporary self-sabotage.
10. Cause and effect must be understandable.
11. Mechanics should generate stories.
12. Prefer state-based costs over damage-based costs.
13. Apparent soft-locks should often be solvable.
14. Mechanics should have multiple uses.
15. Nothing exists solely to punish.
16. Prefer discrete state changes over continuous simulation.
17. Avoid mechanics that multiply temporal state space.
18. Design for touch-targeted interaction.
19. Prefer state manipulation over physics manipulation.
20. Prefer intent-based actions over dexterity-based actions.
