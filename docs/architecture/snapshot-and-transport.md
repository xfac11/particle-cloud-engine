# Snapshots and Transport

**Status:** Target contract. Wire format and physical GPU readback strategy remain open.

## Purpose and core rule

A snapshot is an immutable, consumer-facing representation of one completed simulation state. It is not a command and is not necessarily a serialization of all internal simulation state.

> Simulation execution must not depend on snapshot production or network delivery completing for every tick.

The GPU/native simulation may maintain physics data that clients do not need: forces, accelerations, spatial structures, solver state, temporary buffers, and implementation-specific GPU layouts. These are not transport-contract requirements.

## Conceptual pipeline

```text
C++ simulation
    |
    v
Completed authoritative state N
    |
    +-------------------------> simulation continues
    |
    v
Asynchronous snapshot extraction
    |
    v
Immutable consumer snapshot N
    |
    v
Snapshot buffer / latest-state store
    |
    v
.NET transport scheduler
    |
    v
SignalR clients / browser
```

This diagram describes responsibilities, not a chosen buffer or threading implementation.

## Snapshot identity and consistency

A snapshot is identified by:

- SimulationId: the application-level simulation instance.
- SimulationTick: the completed tick represented by the snapshot and the authoritative ordering key for that simulation.
- SchemaVersion: the version of the consumer-facing representation.

It also contains SimulationTime and a consumer-facing State payload.

For a given simulation, tick order—not network arrival order—is authoritative. Clients may receive snapshots out of order and identify older snapshots using SimulationTick. A separate transport sequence is not required initially; it remains open for diagnostics if needed.

Every particle in a snapshot belongs to the same completed tick. A snapshot must never mix state from different ticks or expose partially completed GPU state.

## Completion and extraction

Simulation completion and snapshot availability are distinct milestones.

- A tick becomes complete only after all computation required for authoritative state N has completed.
- Snapshot extraction may read only a completed, consistent state.
- Snapshot extraction is asynchronous with respect to simulation execution, subject to safe retention/access to the completed state.
- Multiple completed states may have snapshot extraction in flight; maximum concurrency is not specified yet.
- Snapshot extraction work may be superseded or discarded if its presentation snapshot is obsolete and the semantics permit it.
- Authoritative simulation work is never discarded because a snapshot became obsolete.

The specific Vulkan synchronization/readback mechanism—fences, semaphores, barriers, timeline synchronization, staging buffers, persistent mapping, or another strategy—must be selected after the actual GPU and snapshot workload is understood and measured.

## State versus reliable messages

Different message categories have different delivery semantics.

| Data | Semantics |
|---|---|
| Particle positions/velocities in state snapshots | Latest-state; obsolete intermediate snapshots may be dropped |
| Configuration state | Latest effective revision and/or explicit acknowledgement |
| Command acknowledgement | Reliable enough to distinguish accepted, rejected, queued, and applied outcomes |
| Simulation error | Reliable diagnostic/status delivery |
| Domain event | Reliable unless a specific event contract explicitly says otherwise |
| Simulation command | Must not be silently dropped because of presentation/network backpressure |

State snapshots are observations. Commands are inputs. Do not use snapshot delivery semantics for commands or reliable events.

## Backpressure and slow clients

Snapshot production, buffering, and network delivery have independent scheduling/backpressure. A slow SignalR client must not block the simulation.

If a consumer falls behind, obsolete state snapshots may be dropped and replaced by a newer completed state. The system should avoid an unbounded queue of full snapshots for each slow client. Commands, acknowledgements, errors, and domain events require their own reliable delivery/retention policy and must not be discarded under the same “latest state wins” rule.

The exact buffering strategy, maximum in-flight extractions, per-client limits, and transport backpressure policy remain open until measured.

## Snapshot representation and schema versioning

Snapshot representations are explicit, versioned consumer-facing contracts independent of:

- internal C++ particle representation,
- GPU buffer layouts,
- native ABI versioning,
- and other consumer-specific representations.

Schema versions describe semantics as well as representation, not just byte layout. Additive changes may be compatible where the contract explicitly permits them; breaking semantic or representation changes require an explicit schema version change. Native ABI version and snapshot schema version are separate concerns.

A universal snapshot schema is not required. Different consumers may receive different representations when their requirements differ. The browser should receive only the state it needs for presentation/use.

The wire format (binary, JSON, or another representation), schema selection/negotiation, and version compatibility mechanism remain open until API/transport design and performance evidence are available.

## Memory ownership across native boundaries

Any memory exposed across the native ABI has an explicit owner and lifetime. Native simulation memory is never implicitly borrowed by .NET or another consumer.

The architecture does not yet require native-owned snapshot handles, caller-provided buffers, copying, or another physical strategy. The choice should consider GPU readback, snapshot size, allocation behavior, lifetime, and measured performance. Snapshot lifetime must be independent of authoritative simulation state lifetime and network delivery.

## Presentation

The browser may render the latest snapshot or interpolate between snapshots. Interpolation is presentation-only; it does not change authoritative state or simulation time. The render clock remains independent from simulation and snapshot/network clocks.

## Current implementation status

The current C++ prototype does not yet implement GPU compute simulation or snapshot/readback. The current .NET project is an interop console prototype and does not yet implement SignalR transport. This document describes target semantics, not existing functionality.

## Open questions

- Snapshot wire format and per-consumer schema negotiation.
- GPU-to-CPU extraction/readback implementation and synchronization.
- Maximum in-flight extraction count and buffer ownership.
- Per-client buffering/backpressure and reliable event retention.
- Whether transport diagnostics justify a separate snapshot sequence number.
