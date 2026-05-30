# Service-Recovery-Scheduler
C++ library that manages recovery actions for a set of monitored services. Each service has a name and an ordered list of recovery actions (e.g. `RESTART → RESTART → STOP → DISABLE`). When a service reports a failure, the scheduler selects and executes the next action in its sequence. Consecutive failures escalate through the list.


Assumptions and Design Decisions

The exercise intentionally leaves several aspects unspecified. The following assumptions were made to keep the implementation simple, deterministic, and aligned with the stated requirements.

Service Registration
- Services are registered once during application startup.
- Service names must be unique.
- Registering a service with an existing name is considered a programming error and results in an exception.
- Runtime modification of a service's recovery sequence is not supported in the current implementation.

Recovery Escalation
- Each service owns an ordered list of recovery actions.
- Every reported failure advances the service to the next recovery level.
- Once the final recovery action is reached, subsequent failures continue executing the final action.
- Recovery actions are dummy implementations and do not perform real operating-system or service-management operations.

Example:

RESTART → RESTART → STOP → DISABLE

Failure progression:

Failure #1 → RESTART
Failure #2 → RESTART
Failure #3 → STOP
Failure #4 → DISABLE
Failure #5 → DISABLE
...

Service Recovery
- Use PIMPL IDIOM to implement the class to keep the public API stable.
- The scheduler does not perform health checks.
- The scheduler relies on external components to report service health.
- Service recovery is reported explicitly through "notify_healthy()".
- When a service is reported healthy, its escalation level is reset to the first recovery action.

Concurrency Model
The implementation is designed to support concurrent failure notifications while preserving correctness.

The following guarantees are provided:
- State transitions for a single service are serialized.
- Recovery actions for different services may execute in parallel.
-  Set worker threads model based on cpu capability (Receieve work -> Queue work -> worker thread pick work -> Execute work)
- Failures reported for the same service are processed in order.
- Shared state is protected using synchronization primitives.

This approach prevents race conditions while allowing better scalability when multiple services fail simultaneously.

Error Handling
The scheduler throws exceptions for invalid usage, including:
- Empty service names.
- Empty recovery sequences.
- Null recovery actions.
- Registration of duplicate services.
- Operations on unknown services.

Querying State
The scheduler maintains service state that can be queried at any time.

The exposed state includes:
- Current escalation level.
- Last recovery action executed.
- Failure statistics required by the scheduler.

The implementation does not maintain a full historical event log.

Extensibility
Recovery actions are modeled using an abstraction ("IRecoveryAction").

This allows:
- Addition of new recovery actions without modifying scheduler logic.
- Easy unit testing through mocks, stubs, or test doubles.
- Separation of recovery policy from recovery execution.


Out of Scope

The following capabilities were intentionally excluded because they were not required by the exercise:
- Runtime updates to recovery sequences.
- Service unregistration.
- Persistent state storage.
- Distributed scheduling.
- Automatic health monitoring.
- Configuration file loading.
- Real service management operations.


