# Service-Recovery-Scheduler
C++ library that manages recovery actions for a set of monitored services. Each service has a name and an ordered list of recovery actions (e.g. `RESTART → RESTART → STOP → DISABLE`). When a service reports a failure, the scheduler selects and executes the next action in its sequence. Consecutive failures escalate through the list.
