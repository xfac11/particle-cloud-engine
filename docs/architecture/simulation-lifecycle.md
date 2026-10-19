# Simulation Lifecycle and Scheduling

**Status:** Target architecture. The realtime overload policy is not fully finalized.

## Responsibilities

The scheduler and simulation engine are separate abstractions.

- **Scheduler:** tracks elapsed wall-clock time, the configured fixed timestep, and when simulation work should run. It does not know physics, particles, forces, shaders, or Vulkan.
- **Simulation engine:** performs one simulation step for a supplied tick, consumes the frozen tick inputs, executes the required computation, and exposes the state only when the work required for that state is complete.
- **Runtime/service:** owns simulation lifecycle and resource policy. A simulation instance owns its timeline; the runtime owns its lifecycle.

This separation permits realtime scheduling, deterministic offline stepping, and testing to use the same simulation-step semantics.

## Three independent clocks

1. **Simulation clock:** advances by the fixed timestep when a simulation tick completes.
2. **Snapshot/network clock:** independently selects completed states for publication and delivery.
3. **Browser render clock:** controls presentation cadence.

For example, a simulation may step at 120 Hz, publish snapshots at 30 Hz, and render in the browser at 60 Hz. These are illustrative values, not required defaults. Browser rendering and network delivery must not define simulation time.

## Fixed-timestep model

The target simulation uses a fixed timestep, denoted dt. Tick N represents the transition from state N−1 to state N.

```text
State N-1
   |
   v
Freeze commands assigned to tick N
   |
   v
Apply frozen commands in sequence order
   |
   v
Execute one fixed simulation step
   |
   v
Complete all work required for authoritative state N
   |
   v
State N is complete and observable
```

A command assigned to tick N affects the calculation that produces state N. Once the tick begins, the command set is immutable; later arrivals are candidates for a later valid tick.

A tick is complete only after all simulation work needed to establish its authoritative state has completed. In the target GPU path, that includes required GPU execution and synchronization. Snapshot availability is a separate milestone.

A native step represents one fixed logical tick and does not accept arbitrary wall-clock delta time; scheduling belongs outside the engine. Native calls return explicit status values, and exceptions must never cross the C ABI. Validation failures before execution starts leave state unchanged. If a failure occurs after execution begins, the instance is marked `Failed` unless the implementation can prove the authoritative state remains valid. An incomplete tick must never be reported as complete or exposed as a snapshot.

## Realtime and deterministic/offline execution

The same simulation-step semantics should support both:

- **Realtime mode:** a scheduler decides when the next tick should execute based on elapsed wall-clock time.
- **Deterministic/offline mode:** a caller explicitly advances ticks, for example from tick 0 through tick 10,000, without relying on wall-clock timing.

This enables repeatable tests, replay/debugging, and benchmarks. Determinism still depends on the simulation implementation and event/effect ordering; a fixed timestep alone does not guarantee bit-for-bit determinism across all hardware.

## Catch-up and overload

A realtime scheduler may accumulate elapsed time while the simulation is behind. Running catch-up ticks helps preserve simulation time but can create a “spiral of death” if each catch-up tick takes long enough to increase the backlog further.

The proposed policy is:

- Use the fixed timestep as the simulation-time authority.
- Bound the number of catch-up ticks performed in one scheduler iteration.
- Expose backlog/behind status so overload is observable.
- Do not silently drop authoritative simulation ticks when doing so would violate simulation timeline or physics semantics.
- Permit obsolete presentation snapshots to be dropped independently.
- Do not convert paused wall-clock time into a burst of catch-up ticks on resume.

The final policy for persistent overload after the per-iteration catch-up limit is exhausted remains open. Options include continuing to run behind wall clock, reducing service load, pausing/degrading a simulation explicitly, or applying a documented policy. It must not be an accidental side effect of the scheduler.

## Pause and resume

While paused, simulation time and the simulation tick do not advance. Simulation-affecting commands may continue to be received, validated, and queued in the normal per-simulation ordered command queue.

On resume, queued commands follow normal target-tick and sequence-order semantics. Commands targeting a tick that has passed are retargeted according to the late-command policy. Resume continues from the current simulation time; elapsed pause duration does not create physics ticks.

Whether pause/resume are represented as simulation commands or scheduler/runtime controls remains open. Either way, their semantics must be explicit and auditable.

## Snapshot independence

Snapshot extraction, serialization, and network delivery must not become an implicit per-tick synchronization requirement for the simulation timeline. A completed state may be handed to asynchronous snapshot work if the implementation can safely preserve a consistent view of that state.

Snapshot extraction may have bounded in-flight work and may supersede obsolete presentation snapshots. The maximum concurrency and buffering strategy should be selected after the GPU readback and snapshot workload can be measured.

## Current implementation status

The current C++ prototype creates a Vulkan instance and selects a physical device, but logical-device creation and ParticleEngine::Update are empty. It does not yet implement a simulation clock, scheduler, GPU compute pipeline, or snapshot extraction. The rules above describe the target architecture, not existing behavior.

## Related documents

- [Architecture overview](overview.md)
- [Commands and configuration](commands.md)
- [Snapshots and transport](snapshot-and-transport.md)
- [Architecture decisions](decisions.md)

## Open questions

- What happens when a simulation remains overloaded after bounded catch-up?
- How are scheduling resources allocated across multiple simulations?
- What is the initial fixed timestep and how is it configured?
- How are pause/resume represented at the service and simulation boundaries?
- What maximum number of snapshot extractions may be in flight?
