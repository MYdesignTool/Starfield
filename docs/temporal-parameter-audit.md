# Build 22 parameter time audit (M3-05)

The owner confirms build 21 fixes Origin birth positions. Build 22 extends that
contract to all currently implemented public controls. This is an independent
implementation; numerical/reference parity and AE performance remain owner gates.

| Controls | Sampling and effect |
| --- | --- |
| Particles Per Second | Integrate the historical rate; ordinal births cross accumulated emission. A later rate, including zero, cannot erase or reposition older births. |
| Emitter Type, Origin XY/Z, Size X/Y/Z, Disc Size | Actual AE values at each particle birth. Shape and offsets stay attached to that birth. |
| Random Seed, Speed, Speed Random, Angle X/Y/Z, Direction, Direction Span | Birth velocity and random identity. Later keys affect later births. Native Angle controls show turns/degrees and remain keyframeable. |
| Hidden per-axis velocity / amplitude bootstrap controls | Same birth sampling; separate random purposes retain deterministic identity. |
| Particle Life, Size, Size Random, Opacity, Opacity Random, Color, Particle Color mode | Actual AE values at birth. Later Life keys do not retroactively kill older particles. |
| Size / Opacity Over Life | Birth-sampled endpoint multipliers combined with independent constant 0–100% age curves. |
| Color Gradient | Independent saved 2–8-stop ordered gradient; Solid / Color over life / Random from gradient / Loop from grad. Random starts/colors stay attached to particle identity. |
| Gravity, Gravity Random, Wind X/Y/Z, Air Density, Spin/Frequency/resist/delay | Integrate force values over the lived interval. Later forces act from their own times, rather than replaying the whole life with current values. Instantaneous physical plus Spin field velocity feeds Auxiliary inheritance. |
| Wind and Spin Over Life | Constant percentage curve applied to normalized age during each force interval. |
| Auxiliary Chance / Emit Life Start/End / Inherit Velocity/Size/Opacity/Color | Child-birth sampling; evaluate the parent's actual historical state at that time. |
| Output Max Particles, Preview / Particle chance, Camera | Current-frame rendering/population controls; camera moves project existing 3D particles. |
| Time Remapping | Current-frame simulation clock; deterministic historical evaluation at the remapped time. |
| Emitting Default/Auxiliary, graph topology, layout, identity, curve/gradient knot banks | Constant structure/data. Emitting changes are graph transactions, not animated topology. |

## Precision and bounds

Emission and force integration use a deterministic 120 Hz lattice anchored at
simulation zero, including partial birth/final intervals. Linear emission is
integrated analytically per interval; nonlinear interpolation is approximated.
Holds exactly on lattice boundaries are preserved. Short sub-lattice pulses can
be missed; this is not an assertion of exact Stardust interpolation/kernel parity.
Birth attributes themselves use actual AE checkout at the solved birth time,
with up to 1,000,000 ticks/s, reduced to fit AE's 32-bit numerator.

Cancellation is checked throughout. Each evaluation is bounded to 20M work units
and 2M clock intervals (about 16,666 seconds); expensive graphs return a specific
work-limit error. The 2M particle cap is a population limit, not a performance
promise. No cross-frame cache or global history is retained.

The AEX performs owned PF checkouts in pre-render; no sibling-effect AEGP queries
occur during rendering. Transient record 0x8004/version 1 stores immutable final
particles (128 bytes each) once and never enters project data. Saved graphs stay
bounded to 64 MiB; bounded transient graph transport allows 512 MiB. C ABI 2 and
Render.hpp remain unchanged. Shared algorithm inputs require paired AEX/Core builds.

## Native layout / reference scope

Fresh Emitter and Particle effects are required for this development layout.
Emitter rotation sliders retire IDs 117/118/119 and use PF_Param_ANGLE IDs
133/134/135 (same stream indices, degree-valued Core contract). Native 16.16
angles support -32768..32767.99998 degrees; CEP scrubs at 0.1 degree.
Particle schema 3/base 60 adds mode ID 211 and gradient count/positions/colors
930, 940–947, 950–957. No released schema or migration is claimed.

The official [Stardust guide](https://superluminal.tv/user-guide) and the local
observed parameter inventory support birth size/opacity and the implemented
color modes. Path/source color modes, Color Use behaviors, Life Random,
Speed Over Life, Inertia, Orient and other unimplemented reference modules remain
separate work. Gradient editing is in CEP; the native effect owns its saved banks.

## Checks / host gate

1,420 focused C++ checks pass: current-node 276, native sync 784 + camera 12,
renderer controls 39, native Emitter/Particle/Appearance/Force 81/74/74/80.
Actual generated-expression, gateway startup and gradient native-bank JavaScript
suites pass. Tests cover 24 birth-parameter changes, rate ramps/stop/delayed start,
later Life keys, integrated Gravity/Air Density, Spin inheritance, Auxiliary-only
Output, reverse/repeated frames, transient codec/time/persistence guards,
independent curves, gradients and native Angle registration.

All five May 2023 SDK Release /MT AEXs and paired Core DLL build. The agent has
not started or operated AE. AE 2023.5.0 Build 52 needs owner checks of keyframed
controls, prior-key cache invalidation, gradient editing/undo/reopen and performance.
