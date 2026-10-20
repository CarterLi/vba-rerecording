#ifndef VBA_HASH_TRACE_H
#define VBA_HASH_TRACE_H

// ---------------------------------------------------------------------------
// HashTrace - per-frame state hashing, used to prove that a refactor did not
// change emulation behaviour (regression baseline).
//
// Design notes:
//   1. Disabled by default. When disabled it costs one empty call per frame.
//   2. It does not diff symbol tables and does not rely on "remembering which
//      memory regions matter": it hashes the full 400000-byte blob returned by
//      emulator.emuWriteMemState(), which covers CPU registers, timers, DMA,
//      sound channels and all memory. Any behavioural drift shows up here.
//   3. The buffer is zeroed before every write, so the unused tail is always 0
//      and the hash stays reproducible (emuWriteMemState returns success only,
//      not the number of bytes written).
//   4. This file lives in common/ and uses no MFC or Windows types, so the wx
//      frontend can reuse it unchanged.
//
// WARNING: the two builds being compared must use the *same* savestate layout.
// If a refactor changes the savestate format, do that in a separate, separately
// verified pass - never mixed with a behaviour-preserving refactor.
// ---------------------------------------------------------------------------

// Start tracing into the given CSV path. Repeated calls are ignored.
void hashTraceEnable(const char *path);

// True while a trace file is open.
bool hashTraceEnabled();

// Called once per frame (driven by systemFrameBoundaryWork).
void hashTraceFrameBoundary();

// Flush and close. Also runs automatically via atexit().
void hashTraceClose();

#endif // VBA_HASH_TRACE_H
