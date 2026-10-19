# System Architecture Overview

**Status:** Target architecture under design. This document separates agreed architectural rules from implementation details that remain open.

## Purpose

Cloud Particle System is intended to let users create and share reusable particle-system definitions and run them in simulations. The target uses a headless C++/Vulkan simulation engine, a .NET service for orchestration and API/transport responsibilities, and a browser client for interaction and presentation.

The repository is currently a prototype. This document describes the target design; it must not be read as a claim that every component or behavior is already implemented.

## Core architectural rule

> Commands and configuration changes flow toward a simulation; completed simulation state flows away from it. Only simulation execution mutates authoritative simulation state.

The browser, API, transport, and orchestration layers may validate requests, manage lifecycle, select data for consumers, and present results. They do not independently calculate authoritative physics.

## Target components and responsibilities

### Particle-system definition and collaboration service

The product layer manages reusable, versioned particle-system definitions and reusable component definitions such as effects.

- A definition has an individual owner initially.
- Owners may grant collaborators explicit permissions. Permission to use a definition and permission to edit it are distinct.
- Shared editing creates a new definition revision. A user may also copy a definition to create an independently owned asset.
- Published definitions resolve their dependencies to a well-defined set of component revisions. Existing published revisions remain usable when a source asset is later edited, deleted, or its sharing permissions change.
- The product should warn when newer component revisions exist or source assets have changed, without silently changing the published dependency set.
- Definition revisions are pinned by default. Following the latest revision is an explicit opt-in.
- Definition ownership does not automatically grant control over every simulation that uses the definition.

Authorization and collaboration are service/product concerns, not C++ simulation-engine concerns. Detailed roles, retention rules, and dependency archival mechanics remain open.

### .NET service / runtime

The .NET layer is responsible for application-level orchestration, including:

- Creating, identifying, starting, pausing, stopping, and disposing simulation instances.
- Resolving application-level `SimulationId` values to native simulation handles.
- Authorizing access to definitions and running simulations.
- Validating external request shape and converting API DTOs into internal simulation commands.
- Managing per-simulation command ingress, sequencing, target-tick assignment, configuration revisions, and observability.
- Coordinating snapshot production and transport without making simulation progress depend on client delivery.
- Translating native status results into managed/application semantics.

The repository's `src/Engine.API/Engine.API` project is currently a .NET 8 console executable and an interop prototype, not a completed ASP.NET Core/SignalR service.

### C++ simulation engine and Vulkan

The simulation engine owns and advances authoritative state for its simulation instance.

Target responsibilities include:

- Executing a fixed-timestep simulation step.
- Applying the immutable set of commands selected for that tick in sequence order.
- Running simulation computation, eventually on the GPU through Vulkan.
- Generating simulation events from a completed state and resolving configured effects deterministically.
- Completing all work required for an authoritative state before exposing that state as completed.
- Providing a deliberate snapshot/readback boundary rather than exposing internal GPU or C++ storage.

The C++ engine must not depend on HTTP, JSON, SignalR, browser APIs, authentication, or user identity.

The current prototype has a CPU-side `std::vector<Particle>`, Vulkan instance creation and physical-device selection, an empty logical-device creation function, and an empty update method. It does not yet implement GPU compute simulation, a compute pipeline, or snapshot readback.

### Native ABI / interop boundary

The C ABI is a stable boundary between C++ and .NET.

- Expose opaque native handles and explicitly defined C-compatible types only.
- Do not expose C++ classes, STL containers, exceptions, Vulkan objects, or internal GPU layouts.
- Native exceptions must not cross the ABI. Native calls should return explicit status/results, with diagnostics handled separately.
- The .NET interop layer should wrap native handles in a dedicated `SafeHandle`; raw `IntPtr` values should not escape the interop layer.
- Native simulation memory is never implicitly borrowed. Any memory exposed across the ABI must have explicit ownership and lifetime.
- External API DTOs are converted to simulation-native command representations before crossing the ABI.

The current exports use `__declspec(dllexport)`, return raw pointers, and do not yet provide the intended explicit error contract. These are prototype details, not the target contract.

### Snapshot producer and transport

Snapshot production converts a completed simulation state into an immutable consumer-facing representation.

- A snapshot represents one completed simulation tick and never combines data from multiple ticks.
- A snapshot is identified by `SimulationId`, `SimulationTick`, and `SchemaVersion`; it also includes `SimulationTime`.
- Snapshot schemas are explicit, versioned consumer contracts, independent of native ABI versioning and internal particle/GPU representations.
- Different consumers may receive different snapshot representations; the browser receives only the state it needs.
- Snapshot extraction is asynchronous with respect to simulation execution, subject to safe access to a completed state.
- Snapshot production and transport have their own scheduling and backpressure.
- Obsolete state snapshots may be dropped when superseded. Commands, command acknowledgements, simulation errors, and domain events require semantics appropriate to reliable delivery.
- A slow SignalR client must not block simulation progress.

The concrete wire format, readback mechanism, buffering strategy, and maximum in-flight extraction count remain open until the workload can be measured.

### Browser client

The browser is responsible for user interaction and presentation, including editing/sharing definitions, issuing authorized requests, receiving snapshots, and rendering them (potentially using Three.js).

- The render clock is independent of the simulation clock and network/snapshot clock.
- The browser may render the latest available snapshot or interpolate between snapshots.
- Interpolation is presentation-only and does not alter authoritative simulation state.
- Simulation-affecting changes enter through the service's command/configuration paths; the browser does not directly mutate authoritative particle state.

## Control plane and particle data plane

Keep management traffic separate from the high-volume state stream.

**Control plane** includes lifecycle requests, authorization, definition operations, simulation commands, configuration updates, command acknowledgements, status, and errors.

**Particle data plane** includes completed-state snapshots and presentation-oriented particle data.

Configuration updates are not ordinary simulation commands. They use a separate management path, are normalized into a complete candidate configuration, validated and prepared before activation, and applied atomically between ticks. Updates are conditional on the revision they were based on; stale updates are rejected as conflicts rather than silently merged.

## Independent clocks

The architecture has three independent rates:

- **Simulation clock:** fixed timestep and monotonically advancing simulation tick.
- **Snapshot/network clock:** selects completed states for publication and delivery.
- **Render clock:** browser presentation cadence.

A browser rendering at 60–144 Hz does not determine simulation frequency. Snapshot production can be slower than simulation and may skip obsolete intermediate states.

## Simulation timeline and configuration

A simulation instance owns its timeline; the runtime owns its lifecycle.

- Tick N is the transition from state N−1 to state N.
- The commands selected for tick N are frozen before execution and applied in sequence order; they affect the calculation that produces state N.
- Commands arriving after the tick's freeze point cannot be inserted into that tick. Late commands are retargeted to the next valid tick, preserving requested and applied tick information.
- Paused simulations do not advance simulation time. Commands can continue to be validated and queued; resume does not turn paused wall-clock time into a burst of physics ticks.
- Simulation commands, execution configuration, resource/topology management, and presentation configuration are distinct categories.
- Event generation observes completed state N and does not mutate it. Effects are resolved deterministically after event generation; effects resolved for tick N do not generate further events in tick N. Particles created by such effects first participate in physics on tick N+1.
- Birth/death event data must be explicit enough to distinguish lifecycle transitions; `lifetime == 0` is not a reliable birth signal.
- Running simulations may receive explicit configuration updates. A complete candidate is validated and prepared before atomic activation between ticks. Failed and no-op updates do not increment the effective configuration revision.
- API-level partial configuration updates are allowed in principle, but they must be applied against a known revision to build a complete candidate. Stale revisions produce a conflict.
- Per-simulation configuration revisions are distinct from particle-system definition revisions and reusable-component revisions.

See [Simulation lifecycle](simulation-lifecycle.md), [Commands and configuration](commands.md), [Snapshots and transport](snapshot-and-transport.md), and [Architecture decisions](decisions.md) for detailed contracts and open questions.

## Scheduler and overload

The scheduler determines when simulation work should run; it does not know physics, particles, shaders, or Vulkan internals. The simulation engine performs the actual tick.

Realtime scheduling and deterministic/offline stepping should share the same simulation-step semantics. The scheduler should bound catch-up work per iteration and make overload observable. The final persistent-overload policy remains open. Simulation ticks must not be silently dropped where that would violate timeline/physics semantics; obsolete presentation snapshots may be dropped independently.

## Current prototype versus target

| Area | Current repository evidence | Target direction |
|---|---|---|
| C++ simulation | CPU `std::vector<Particle>`; `Update` is empty | Authoritative fixed-step simulation, eventually GPU compute |
| Vulkan | Instance creation and first physical-device selection; logical-device creation is empty | Device/queue/resources/compute pipeline designed and implemented incrementally |
| Native ABI | Raw pointer exports; Windows-specific export macro; update error printed to stderr | Stable C-compatible ABI, opaque handles, explicit statuses, SafeHandle in .NET |
| .NET | .NET 8 console project with early interop wrapper | Service/runtime, API, authorization, command/config orchestration, snapshot transport |
| SignalR / browser | Not implemented in the current API project | Separate state transport and browser presentation |
| Definitions/collaboration | Not implemented | Versioned assets, ownership, permissions, shared editing and copying |

The prototype's current shape is evidence about what exists today, not a requirement to preserve its design. In particular, internal `Particle` layout is not a native ABI or snapshot contract.

## Visual reference

The repository's [`schema.excalidraw.svg`](../../schema.excalidraw.svg) is the existing visual architecture reference. Keep it aligned with these written contracts as the architecture evolves.

## Open questions

- Persistent realtime-overload behavior after bounded catch-up is exhausted.
- Exact scheduler/threading model and policy for multiple simulations.
- GPU compute, synchronization, and safe snapshot readback mechanisms.
- Snapshot wire format, per-consumer schema selection, and transport version negotiation.
- Maximum in-flight snapshot extractions and buffer sizing.
- Native ABI status codes and diagnostic mechanism.
- Exact command representation and pause/resume classification.
- Effect schema, dependency storage/retention, and warning presentation.
- Definition collaboration roles, detailed authorization policy, and live-link update behavior.
- Configuration payload schema, field-clearing semantics, and conflict response contract.
