# Simulation Commands, Events, Effects, and Configuration

**Status:** Target semantic contract. Concrete API schemas and runtime representations remain open.

## Core rule

> Commands flow toward the simulation. Simulation execution alone mutates authoritative state. Completed state and simulation-generated events flow away from the simulation.

The external API and transport DTOs are not the internal simulation command model. The .NET layer validates external requests and converts them into internal command representations before crossing the native ABI. C++ understands simulation semantics, not HTTP, JSON, SignalR, authentication, or user identity.

## Command envelope and ordering

A command should carry enough identity and timing information to audit when it was requested and when it took effect. The conceptual envelope includes:

- SimulationId
- CommandId
- per-simulation Sequence
- RequestedTick
- AppliedTick when applied
- CommandType
- typed payload

Exact types and serialization are not yet prescribed.

Commands from multiple clients enter one logically ordered ingress for the target simulation. The sequence is assigned by that ingress, not inferred from client timestamps. Ordering is per simulation, not global across every simulation.

The native boundary accepts individual commands semantically. Batching may later be introduced as a transparent optimization if measurements justify it, but must preserve per-command identity, sequence, and failure/observability semantics.

## Tick semantics

Tick N is the transition from state N−1 to state N:

1. Freeze/select commands assigned to tick N.
2. Apply the frozen commands in per-simulation sequence order.
3. Execute the fixed-timestep simulation step.
4. Complete required computation.
5. Expose the resulting completed state N.

A command targeting tick N affects the calculation that produces state N. Once tick N begins, its command set is immutable; new arrivals cannot be inserted into it. Multiple commands may apply to the same tick in sequence order.

The simulation timeline determines when a command takes effect. Command receipt or queueing is not the same as application.

## Late commands and pause

A late command is not silently rejected by default. If its requested tick has passed, retarget it to the next valid tick and retain both requested and applied tick information.

While paused, simulation time does not advance. Simulation-affecting commands continue to be validated and queued in the same per-simulation ordered queue. On resume, normal ordering applies; resume does not convert elapsed pause duration into a burst of physics ticks.

The exact classification of pause/resume as runtime controls versus simulation commands remains open.

## Command lifecycle and reliability

Command lifecycle states should distinguish:

- **Received**
- **Validated**
- **Queued**
- **Applied**
- **Visible** (optional, when the effect can be observed in a later snapshot)

Lifecycle state and timestamps support debugging and latency/performance analysis. Acceptance and queueing do not promise that the command has already affected state.

Presentation/network backpressure must not silently drop simulation commands. State snapshots may be dropped when obsolete; commands require explicit acceptance/rejection and lifecycle semantics. Command acknowledgement and simulation error delivery should be reliable according to the service contract.

## Categories of changes

Do not classify every setting as a simulation command merely because it can affect behavior.

1. **Simulation inputs/events:** time-dependent inputs such as forces, impulses, gravity changes represented as tick inputs, and particle spawning.
2. **Simulation topology/resource management:** underlying allocation/capacity changes. Dynamic spawn/removal can be simulation commands, while resizing buffers or changing resource capacity is a separate lifecycle/resource concern.
3. **Execution configuration:** fixed timestep, scheduler policy, and catch-up limits.
4. **Presentation/runtime configuration:** snapshot rate, transport limits, interpolation, and render rate.
5. **Configuration management:** explicit updates to the rules/parameters that govern future ticks, through a separate management path rather than the ordinary command stream.

## Simulation-generated events and effects

Distinguish three concepts:

- **Command:** an external request entering the simulation.
- **Event:** something generated or observed by the simulation.
- **Effect:** a state-changing consequence of resolving an event.

For tick N:

1. Freeze external commands for N.
2. Execute simulation work to produce completed state N.
3. Generate semantic events by observing completed state N.
4. Resolve the generated event set deterministically.
5. Apply the resulting effects to produce the final observable state N.
6. Effects resolved during N do not generate additional events during N. Behavior resulting from those effects is evaluated during N+1.

Event generation is conceptually after the simulation computation and does not mutate authoritative state. Event generation may eventually run on CPU or GPU; that mechanism is open. A snapshot is only published after the state it represents is complete.

Particles created by an external SpawnParticles command are tick inputs and can participate in the physics calculation for that tick. Particles created as a result of a simulation-generated event/effect appear in the resulting state but first participate in physics on the following tick.

### Lifecycle events

Birth/death information must be explicit enough to distinguish a newly created or activated particle from a dead particle. lifetime == 0 is not a reliable birth signal because it may also represent death or another lifecycle condition.

Event generation reads a consistent, read-only completed state; it does not compare state N against state N−1 and does not mutate the authoritative state. Birth/death markers are valid for one tick only, so an event is not repeatedly emitted for the same transition. The marker representation and how it is cleared/replaced remain implementation details.

Event payloads contain the data needed to resolve configured effects, such as a death position. They should not require later lookup of the source particle from storage that may have been reused. Payloads should contain only the fields required by the event/effect contract, not the full internal particle by default.

### Declarative reusable effects

Effects are declarative configuration, not arbitrary user code or callbacks.

- Reusable named effect definitions represent one atomic supported action.
- An event rule may reference multiple reusable effects.
- Initial effects do not reference or compose other effects; nested effect composition is out of scope.
- Example: an On Death rule can reference both SparkBurst and SmokePuff.
- Effect resolution is deterministic and does not execute arbitrary custom code inside the physics loop.

Illustrative configuration only:

```yaml
effects:
  SparkBurst:
    type: SpawnParticles
    particleType: Spark
    count: 5
  SmokePuff:
    type: SpawnParticles
    particleType: Smoke
    count: 2
particleSystems:
  Firework:
    events:
      onDeath:
        effects: [SparkBurst, SmokePuff]
```

This is an example of the model, not a committed schema.

## Configuration updates

Running simulations may receive explicit configuration updates; the configuration is not frozen forever at creation. Configuration updates use a separate management path from ordinary simulation commands.

- Normalize an update into a complete candidate configuration for the affected scope.
- API-level partial updates may be accepted for convenience, but must be applied against a known configuration revision to build a complete candidate.
- Validate and prepare the complete candidate before activation.
- Activate it atomically at a defined boundary between ticks; no tick may observe a partially updated configuration.
- If validation/preparation fails, the active configuration remains unchanged.
- A valid no-op update does not increment the version.
- Increment the per-simulation effective configuration version only when the effective configuration changes.
- Each tick should be attributable to the active configuration version.
- A stale base revision is rejected as a conflict rather than silently merged; the client refreshes and retries.
- Retain configuration history for debugging subject to a retention policy that remains open.

Per-reference effect parameter overrides are deferred. Initially, references use their configured reusable effect definition; users needing different parameters can define another effect asset.

## Definition and dependency versioning

Particle system definitions and reusable effects are versioned assets, separate from a running simulation's effective configuration.

- Each definition has an individual owner initially. Owners may grant explicit collaborator permissions; permission to use and permission to edit are distinct.
- Shared editing publishes a new definition revision. Users may also copy a definition to create a separately owned asset.
- Existing simulations pin the definition revision by default. Explicit opt-in live-linking may be supported later.
- Reusable component references such as effects are pinned to specific revisions by default. Following latest is an explicit opt-in.
- A published particle system definition validates and resolves a stable set of dependency revisions.
- Later edits, deletion, or sharing-permission changes to a source asset must not silently break an already published dependency or a simulation using it.
- Surface warnings when newer component revisions are available or a source asset was deleted/changed, without silently altering pinned behavior.
- Exact dependency storage, archival/retention mechanics, live-link update behavior, and warning presentation remain open.

Definition ownership does not automatically grant control over a simulation that uses the definition. Authorization is a service/product responsibility, not a C++ engine responsibility.

## Native boundary

The C ABI should expose only opaque handles and explicit C-compatible boundary types. C++ classes, STL containers, exceptions, Vulkan objects, and internal GPU layouts must not cross it. Native exceptions do not cross the ABI; native calls return explicit status/results. The .NET interop layer owns native handle lifetime through SafeHandle.

## Open questions

- Exact command payload/ABI representation and diagnostic mechanism.
- Pause/resume classification.
- API DTOs and partial-update field-clearing semantics.
- Exact event/effect schema and deterministic ordering implementation.
- How one-tick lifecycle metadata is represented and cleared.
- Definition/effect storage and retention strategy.
- Detailed collaboration roles and authorization rules.
- Live-link update timing, conflicts, and failure presentation.
- Configuration history retention and snapshot/config-version exposure.
