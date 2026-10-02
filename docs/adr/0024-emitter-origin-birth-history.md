# ADR 0024: emitter Origin sampled at birth

Status: development implementation; owner AE 2023 qualification required.

Owner confirms build 20 renders, but all survivors follow animated Origin XY.
Origin is a birth position. The current-frame graph Origin must not translate
particles born earlier. Position keys are sampled through the existing main
effect's owned numeric dependencies at each selected particle's birth time.

Pre-render evaluates the graph with a host-independent origin sampler, caching
unique (emitter UUID, birth seconds) requests. PF checkout/checkin is confined to
the adapter and pre-render. Its immutable optional record 0x8003/version 1 stores
requested birth times/returned world positions (48 bytes per sample, at most 1,000,000).
This record exists only in the pre-render snapshot, never in saved project data.
The DLL uses that record, including recursive Auxiliary parent evaluations, and
strictly rejects a missing requested sample. No AE pointers enter the graph/Core.
PiPL/runtime share WIDE_TIME_INPUT plus AUTOMATIC_WIDE_TIME_INPUT so AE tracks
historical numeric checkouts and invalidates dependent frames after key edits.
There is no history cache across frames requiring time-span cache validation.
Graph bytes keep C ABI 2 and Render.hpp stable; no public parameter IDs change.

AE callback rational time uses up to 1,000,000 ticks/s, reduced if necessary to
fit its signed 32-bit numerator. This queries AE's actual interpolation at each
birth, rather than approximating a path from frame samples. Repeated birth queries
reuse their result. Cancellation and history/codec bounds reject excess work.
Constant graphs without native bindings retain the existing pure evaluation.

All primary and Auxiliary Origin XY/Z offsets use birth values; existing seeded
shape offsets, velocity and force motion remain relative to that position. Other
controls keep build-20 sampling semantics. Animated emission-rate/lifetime clocks
and integrated animated Force history are separate work. Each frame is evaluated
independently; frame order and process-global caches never define authored history.

Candidate evidence: native sync 792/camera 12, portable current nodes 78 and
renderer controls 39 checks pass (921). Nonlinear subframe native PF sampling,
frozen codec/pixels, old-particle positions, Auxiliary parents, reverse frames,
saved graph preservation, failed checkout and cancellation are covered. All five
May 2023 SDK Release /MT AEXs and paired Core DLL build without warnings/errors.
No AE session operated. Owner trajectory, key interpolation, past-key cache
invalidation and reopen qualification remain open.
