# ADR 0002: rational host time and deterministic evaluation

- Status: accepted for M0/M1.
- Date: 2026-09-27

Represent host time as a signed 64-bit numerator and positive 64-bit denominator in AE's supplied time units. Normalize with checked arithmetic at the core boundary. Never reduce time to a frame counter or floating-point seconds before parameter sampling. Frame duration, subframe shutter samples, and negative times retain their rational form.

Particle random streams are pure functions of effect seed, persistent particle ID, and stream-purpose ID. Evaluate each requested frame from absolute time; never advance shared simulation state from the prior render call. Integrators added later must define stable step size, ordering, and rounding before shipping.

The SDK adapter converts PF rational types with checked 128-bit intermediates where available and rejects overflow with a controlled render error. Core code receives validated rationals and is independent of AE structs.
