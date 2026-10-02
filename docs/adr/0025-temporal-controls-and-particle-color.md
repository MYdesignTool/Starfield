# ADR 0025: temporal controls and Particle color (development build 22)

Status: implemented in build 22 candidate; AE 2023 owner qualification required.

Owner confirms build 21 Origin birth sampling, and reports current-rate
repopulation. M3-05 owns the shared graph evaluator/simulation/history codec,
native sampling/compiler/parameters/record/schema, CEP inspector/gateway,
focused checks, build fingerprint/version and architecture/checkpoint records.
It extends the temporal contract across existing public controls.

Emission is the integral of nonnegative Particles Per Second. Stable ordinal
births cross integer accumulated emission; changing a later rate never changes
earlier births. A deterministic 120 Hz integration lattice is independent of
requested frame ordering; interpolation/integration precision is documented.
Emitter position, shape/dimensions, seed, speed/direction/random and Particle
life/base size/opacity/color/randomness sample at birth. Static age curves
evaluate normalized age. Auxiliary probability/windows/inheritance sample at
child birth; ancestor systems use the same temporal evaluator.
Force inputs act over time lived, using bounded deterministic integration,
not the current value applied retrospectively to the whole life.
Output population, preview/chance, camera and simulation-clock remapping are
current-frame controls. Identity/topology/layout/curve knots are not animated.

The adapter performs owned PF historical checkouts only in pre-render.
Core receives no host objects. The evaluated particles are encoded in a
transient immutable optional record, never saved to the project. This avoids
serializing every sampled graph and duplicate simulation inside the DLL.
IDs/Render.hpp/C ABI 2 remain unchanged; temporal record/layout changes are
paired AEX/Core builds. Saved graph bytes remain bounded at 64 MiB; the transient
particle transport needs a separate bounded allowance for the 2M output cap.

Color follows the observed reference parameter inventory and official guide:
Particle Color (Solid color / Color over life / Random from gradient /
Loop from grad), Color and an independently authored multi-stop Color Gradient.
Path/source modes are not offered until those sources exist. Color Use is only
implemented where its behavior is supported by evidence. Numerical interpolation
and random distributions are independent, not claims of kernel parity.
Reference: https://superluminal.tv/user-guide (Particle / Start / Particle Color).

Native Particle development schema advances for the gradient controls, with
fresh effects required and no old-layout migration, as the owner requests.
Explicit unique disk IDs are schema-owned. The Emitter native angle type also changes below; other layouts stay
unchanged. No registry/process/host preference changes belong to this task.

Owner additionally requires native AE rotation controls. Emitter Angle X/Y/Z
change from sliders to PF_Param_ANGLE with fresh disk IDs 133/134/135; retired
117/118/119 are reserved. The native UI displays turns plus degrees and a dial.
Stream indices and Core degree units are unchanged; supervised native callbacks
convert the SDK 16.16 angle to degrees, and render dependencies sample ONE_D values.
Emitting is a structural Default/Auxiliary selector and stays constant. Direction
Span remains a cone width, and Spin Frequency remains cycles/second.

Full control matrix, precision, bounds and scoped checks: ../temporal-parameter-audit.md.
