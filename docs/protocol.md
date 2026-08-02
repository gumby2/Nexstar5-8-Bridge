# NexStar protocol constraints

- The mount accepts only one active command at a time.
- The command path must remain blocked until the current command completes.
- `?` initialization/handshake returns `#`.
- Completed movement commands return `@`.
- Do not issue E/Z position queries during an active GOTO.
- AbortSlew is not supported by the original mount.
- During GOTO, report slewing and use cached or estimated position only.

## Transaction recovery

The mount can emit delayed or unexpected bytes after a command. The firmware
therefore treats each transaction as a bounded state machine:

1. Acquire the single-command lock.
2. Send `?` and accept `#` as the handshake terminator; other bytes are stale
   input and are recorded/ignored within the handshake timeout.
3. Send exactly one command and its payload, then read the expected response
   length or wait for `@` for a movement command.
4. After receiving `@`, drain input for a short bounded quiet interval before
   allowing the next E/Z poll.
5. On timeout, mark the transaction failed, defer polling using recovery
   backoff, and release the lock only after the transaction is fully bounded.

This drain protects the next transaction from trailing bytes; it does not treat
invalid mount bytes as valid data or hide a genuine mount communication fault.
