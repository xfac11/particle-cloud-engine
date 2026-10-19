# Architecture Decisions and Open Questions

**Status:** Living record of decisions made during architecture discovery. Update this document when decisions are confirmed or implementation evidence changes the design.

This file summarizes the accepted direction and links to detailed contracts. It does not claim that target features are already implemented.

## Accepted decisions

### D-001 — Authoritative simulation state

**Decision:** The simulation engine owns authoritative particle state. API, .NET transport, and browser layers may hold derived copies/snapshots but do not independently calculate authoritative physics.

**Why:** Prevents competing state authorities and keeps physics semantics in one place.

**See:** [Overview](overview.md), [Snapshots and transport](snapshot-and-transport.md).

### D-002 — Command flow and state flow

**Decision:** Commands and explicit configuration changes flow toward a simulation; completed state and snapshots flow away from it. A simulation instance owns its timeline; the runtime owns its lifecycle.

**Why:** Separates simulation semantics from service orchestration and presentation.

### D-003 — Fixed timestep and independent clocks

**Decision:** Use a fixed simulation timestep and separate scheduler from simulation execution. Simulation, snapshot/network, and browser render clocks are independent.

**Why:** Render or network cadence must not determine simulation time; offline stepping and repeatable tests should share simulation-step semantics.

**Open:** Initial timestep and persistent overload policy.

**See:** [Simulation lifecycle](simulation-lifecycle.md).

### D-004 — Tick and command ordering

**Decision:** Tick N transitions state N−1 to state N. Freeze commands for N before execution, apply them in per-simulation sequence order, and do not insert new commands into a tick after its freeze point. Late commands are retargeted to the next valid tick while retaining requested/applied tick information.

**Why:** Establishes an auditable timeline and removes ambiguous ordering across multiple clients.

**See:** [Commands and configuration](commands.md).

### D-005 — Separate control plane and particle data plane

**Decision:** Lifecycle, authorization, commands, configuration management, acknowledgements, and errors are control-plane concerns. Completed particle snapshots are data-plane concerns.

**Why:** The high-volume state stream must not control simulation execution or command reliability.

### D-006 — Explicit configuration updates and revisions

**Decision:** Configuration updates use a separate management path from ordinary simulation commands. Normalize updates into complete candidate configurations, validate and prepare before activation, and apply atomically between ticks. API-level partial updates are permitted only when applied against a known revision. Stale updates are rejected as conflicts. A revision increments only when effective configuration changes; failed and no-op updates do not increment.

**Why:** Separates client ergonomics from internal consistency and prevents lost updates.

**Open:** Payload schema, field-clearing semantics, conflict response contract, and retention policy.

**See:** [Commands and configuration](commands.md).

### D-007 — Declarative events and effects

**Decision:** Simulation-generated events are produced by observing completed state and do not mutate state themselves. Effects resolve deterministically after event generation. Effects resolved during tick N do not produce additional events during N; resulting behavior is evaluated on N+1. Effects are declarative reusable atomic actions; an event rule may reference multiple effects, but effect-to-effect composition is initially out of scope.

**Why:** Avoids arbitrary callbacks/user code inside a GPU-oriented simulation loop and gives event/effect timing explicit semantics.

**Open:** Concrete schema and deterministic resolution implementation.

**See:** [Commands and configuration](commands.md).

### D-008 — Explicit lifecycle event metadata

**Decision:** Birth/death information must distinguish lifecycle transitions explicitly; lifetime == 0 is not a reliable birth signal. Event generation reads a consistent, read-only completed state and does not compare state N with N−1. Birth/death markers are valid for one tick only.

**Why:** Prevents duplicate or ambiguous lifecycle events.

**Open:** Marker representation and clearing/replacement mechanism.

### D-009 — Versioned definitions, ownership, and dependencies

**Decision:** Particle-system definitions and reusable components are versioned assets, distinct from a simulation's effective configuration revision. Definitions have an individual owner initially. Owners may grant use/edit permissions separately. Shared editing publishes new revisions; copying creates a separately owned definition. References are pinned to specific revisions by default; explicit opt-in live-linking may be supported.

Publishing validates and resolves a stable set of dependency revisions. Later edits, deletion, or sharing-permission changes to source assets must not silently break existing published definitions or simulations using them. Surface warnings for newer revisions or changed/deleted source assets without silently changing pinned behavior.

**Why:** Supports collaboration and experimentation while preserving predictable simulation behavior.

**Open:** Detailed permission roles, live-link semantics, dependency archival/retention, and warning UX.

### D-010 — Stable native ABI and explicit ownership

**Decision:** The native boundary exposes opaque handles and explicit C-compatible types, not C++ classes, STL containers, Vulkan objects, exceptions, or internal GPU layouts. Native exceptions do not cross the ABI; native calls return explicit status/results. .NET wraps native handles in SafeHandle. Native memory exposed across the ABI has explicit owner and lifetime.

**Why:** Allows C++/GPU internals to evolve without leaking implementation layout into .NET contracts.

**See:** [Overview](overview.md), [Snapshots and transport](snapshot-and-transport.md).

### D-011 — Snapshots are immutable versioned observations

**Decision:** A snapshot represents one completed tick, identified by SimulationId, SimulationTick, and SchemaVersion, and includes SimulationTime. Tick order is authoritative, not network arrival order. Snapshot schemas are explicit consumer-facing contracts independent from native ABI/internal state. Extraction is asynchronous and obsolete presentation snapshots may be dropped.

**Why:** Keeps presentation delivery decoupled from simulation correctness and allows internal and consumer representations to evolve independently.

**Open:** Wire format, readback, buffering, schema negotiation, and maximum in-flight work.

**See:** [Snapshots and transport](snapshot-and-transport.md).

## Intentionally open questions

1. Persistent overload behavior after bounded catch-up is exhausted.
2. Initial timestep and multi-simulation scheduling/resource policy.
3. Exact pause/resume classification.
4. Vulkan compute/synchronization and safe snapshot extraction mechanism.
5. Snapshot wire format, schema negotiation, and per-client backpressure policy.
6. Native ABI status values and diagnostic mechanism.
7. Exact command ABI representation and event/effect schema.
8. Lifecycle marker representation.
9. Definition collaboration roles and detailed authorization policy.
10. Definition/effect dependency retention and archival strategy.
11. Live-link behavior, update timing, conflicts/failure UX, and dependency warnings.
12. Configuration payload schema, partial-field clearing semantics, and revision history retention.

## Implementation discipline

Architecture documents describe semantic contracts and accepted design choices. They should not prescribe implementation mechanisms where workload evidence is needed. As the prototype evolves, record new decisions with rationale and consequences, update the relevant detailed document, and distinguish observed implementation behavior from intended target behavior.

## External references

- [Vulkan Guide](https://docs.vulkan.org/guide/latest/): Vulkan device, execution, synchronization, and resource concepts. Use it to evaluate concrete GPU implementation choices; it does not prescribe this application's architecture.
- [.NET native interoperability overview](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/): native boundary design and managed/native interoperability.
- [.NET SafeHandle guidance](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.safehandle): managed ownership of native handles.
- [SignalR overview](https://learn.microsoft.com/en-us/aspnet/core/signalr/introduction): realtime communication capabilities for the eventual service/clients. Transport must remain separate from simulation timing.
