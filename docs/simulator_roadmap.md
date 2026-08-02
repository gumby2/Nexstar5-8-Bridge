# Mount simulator roadmap

The current host-side simulator covers basic protocol vectors but does not yet
model the failure patterns seen on the physical mount. The next integration
layer should provide deterministic scenarios for:

- delayed `#` handshake replies;
- truncated and overlong coordinate payloads;
- invalid characters before, inside, and after a payload;
- stale bytes emitted after a slew;
- missing `@` completion markers;
- completion markers arriving after a timeout;
- repeated failures followed by recovery;
- concurrent Web/Telnet/client requests attempting to reach the single-command
  mount transport.

Each scenario should record transmitted bytes, received bytes, discarded bytes,
transaction state, recovery pause, final cached coordinates, minimum heap when
run on hardware, and listener reachability. The simulator must assert that no
new mount command is forwarded while a prior command remains active.

Implementation should wait until the canonical source-tree decision is made so
the simulator vectors target the same transaction code used by the firmware.
