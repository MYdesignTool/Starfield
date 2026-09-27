# ADR 0010: reference-product boundary, module registry, and panel carrier

- Status: accepted (owner direction, 2026-09-27).
- Depends on ADR 0001 (identity), 0006 (graph model), 0009 (panel bridge).

## Context

The reference product ships as one effect plus sixteen `Stardust_controls_*` modules and a
panel, all as `.aex` files, and the owner has already reverse engineered it. The owner asked
whether its nodes can be reused directly, whether modifying that plug-in is a viable path, and
whether our panel can avoid the manual CEP install the same way theirs does.

## Decision

### 1. Binaries are never reused

The reference product's `.aex` files are copyrighted commercial binaries. They are **never**
copied, modified, rebranded, bundled, or redistributed by this project. This is not a style
preference:

- Our repository is MIT licensed and published; distributing or shipping a derivative of a
  commercial plug-in binary would be public infringement, and the MIT grant cannot cover it.
- Their end-user licence governs modification and reverse engineering; nothing here relies on
  a permission we do not have.
- The project's own rule (see `docs/agent-backlog.md`) is *independent implementation of
  observed behavior*: static analysis is a feature-discovery aid, never a source specification.
- ADR 0001 forbids reusing the old plug-in identity, match names, or versioning.

Recorded behavior (parameter names, defaults, visible output) and public SDK contracts stay
legitimate inputs. Decompiled code, private symbols, and binary layouts are not.

### 2. Architecture-level inspiration is welcome and planned

What is worth taking from the reference is its **decomposition**, not its code: emitters,
particles, physics/forces, transforms, materials, models, lights, volumes, oversampling,
post-effects and UI grouping are separate subsystems, each with its own parameter set. Our
equivalent is already the graph's typed node registry (`core::graph_keys` +
`make_particle_node_registry`).

Planned refactor (implementation task, not a contract change): turn the registry into a
**module table** where every subsystem declares
{node types, parameter descriptors with ranges/defaults/units, ECW topic, palette entry}.
The Effect Controls grouping (manifest revision 6 topics) and the panel's node palette are
then generated from that one table instead of being maintained in three places. New families
are added by adding a module, not by editing registration, gateway and schema separately.

### 3. Panel carrier stays CEP for the Alpha, and stays replaceable

Findings from the local May 2023 SDK:

- The public headers expose **no supported AEGP dockable-panel API**. Documented custom-UI
  routes are the effect's own UI area (`PF_PUI_CONTROL` + `PF_EffectCustomUISuite2`, Drawbot)
  and comp-window overlays. A second `.aex` can create its own OS window, but it is not a
  documented AE panel dock, and reverse engineering how the reference docks is out of bounds
  (§1).
- The panel **carrier does not change the bridge**: AEGP cannot write arbitrary-data streams
  (ADR 0009), so any carrier must use the supervised ordinary-parameter surface and the
  effect's `PF_Cmd_USER_CHANGED_PARAM` rewrite. Swapping CEP for another carrier saves the
  install step and nothing else.

Therefore: keep CEP for the Alpha, and make the install one command
(`cep_panel/Install.ps1`, which writes the CSXS debug key and links the extension folder).
Revisit the carrier only when one of these is true:

1. the supervised-parameter bridge passes its AE 2023 host gate, **and**
2. the install step is a real adoption blocker, **and**
3. one of the documented alternatives is enough: an in-effect custom-UI chain editor (Drawbot;
   no widgets, no text layout, hand-rolled hit-testing), an on-canvas gizmo (already planned as
   P-03), or a signed ZXP package (removes the debug-key step entirely).

Adobe's UXP direction for AE is a further reason not to invest in a bespoke `.aex` UI now:
the protocol in ADR 0009 is carrier independent, so a later UXP panel is a port, not a rewrite.

## Amendment (owner follow-up, 2026-09-27)

The repository may be made private while the work matures. That lowers *distribution* risk but
does not move the derivative-work boundary: code translated out of a decompiler is still a
derivative of the original, so this project stays an independent implementation of observed
behavior, and no reference-product binary enters the tree, private or public. Two consequences
worth writing down:

- **Parameter discovery does not need decompilation.** AE exposes every effect parameter,
  including hidden and non-animated ones, to the public scripting DOM.
  `tools/dump_effect_parameters.jsx` dumps names, match names, values, keyframe state and nesting
  for every effect on the selected layers in seconds. Compared with analysing a binary it is also
  more accurate: it reports the installed version's real behavior, not an older build's metadata.
- The reference panel shipping as a `.aex` remains an observation about its architecture, not a
  route to copy: how it attaches itself to AE is an implementation detail we would have to
  reverse, and §3 documents the supported alternatives we can build ourselves.
