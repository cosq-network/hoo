# ISSUE-063: Future Continuation Callback Not Cleaned Up on Destruction

## Status

- **Updated**: 2026-08-05
- **Status**: **RESOLVED**
- **Priority**: P1 in the original audit

## Problem

The original Future held one callback slot without a safe ownership and
destruction model. A pending callback could remain associated with a destroyed
Future, and multiple consumers were not supported.

## Resolution

- Continuations are stored as a linked list of owned nodes.
- Resolution detaches the list under the Future mutex and invokes every
  callback at most once.
- A queued callback retains the Future until it finishes.
- Destruction releases the payload and error, frees pending continuation nodes,
  and prevents stale callback state from being reused.

Callback arguments remain non-owning by API contract; callers must keep them
valid until their callback runs.

## Verification

`HooFutureJitTest.cpp` covers callbacks before and after resolution, multiple
continuations, retention, destruction, and null safety. The complete suite
passes with 2030 tests passed, 2 disabled, and 0 failures.
