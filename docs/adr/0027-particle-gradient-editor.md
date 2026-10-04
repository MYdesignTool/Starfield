# ADR 0027: native Particle gradient editor

Status: candidate implementation/build/scoped checks complete on development
branch; AE 2023 qualification pending.

## Build34 correction after owner feedback - 2026-10-04

Owner build33 testing is blocked by repeated "Unsupported Pixel Format" warnings
when expanding Color Gradient; markers/buttons draw but the bar stays blank.
The build33 image path requested 24RGB without negotiating supplier capabilities.
The previous fake supplier accepted that layout unconditionally and missed this
host restriction. No native bitmap qualification pass is recorded for build33.

Build34 queries Drawbot Supports/PrefersPixelLayoutBGRA/ARGB and creates only an
advertised 32-bit premultiplied layout, with matching byte order and alpha255.
Failed or absent support queries cannot authorize image creation. Missing image
APIs/capabilities use bounded overlapping path bands on an opaque base. Unexpected
image creation/null-image/draw failure disables bitmap attempts for that bounded
UI context until CLOSE_CONTEXT, avoiding repeated calls during dialog repaints.
All image/font/path objects remain callback-local and are released on failure;
fallback drawing retains editing and never changes authored values.
The capability matrix, byte order/opacity, optional APIs, failure repaint guard
and context reset are exercised in the fake host. Actual AE warning-free drawing
and editor interaction remain mandatory host gates. Schema/IDs/flags/CEP/Core
and the renderer remain unchanged from build33; only native UI and build version
change. Development remains on M3-07 with main at accepted build31.

## Build33 correction after owner feedback - 2026-10-04

Owner AE2023 feedback rejects build32 UI qualification: adding a stop reports
node compile / parameter12 / stream-1; the bar has vertical seams; fresh Emitter
and Force effects expose internal metadata. No build32 host pass is recorded.
The strip renderer used separate antialiased fractional rectangles. Build33 uses
one opaque, aligned-row RGB bitmap per bar, with bounded buffers and image release
in the same callback. Ordinary and hidden slider ui_width/ui_height return to
zero; only the actual custom gradient sets those nonstandard fields. This corrects
the SDK contract; the fresh-node visibility result still needs an AE host check.

The fake host reproduces compile rejection when a new count is combined with the
old stop positions. Gradient leaves no longer SUPERVISE intermediate host writes:
the custom event publishes once with native change flags. A retained defensive
supervision path captures the complete 17-field callback bank. Failures report
the offending gradient stream index and preserve the previous graph. Other public
controls retain supervision and keyframes; gradient banks remain constant.
AE dynamic Size Y/gradient visibility uses non-undoable AEGP HIDDEN flags on just
the current Particle effect, because PF_UpdateParamUI does not dynamically toggle
PF_PUI_INVISIBLE in AE. No authored data, receipts or SDK objects are retained by
the UI helper. Optional suite failure cannot reject loading.
Schema5/base81, disk IDs, group IDs, match names, main manifest24 and CoreABI3 stay
unchanged; build32 graphs need no additional layout migration for build33.
SDK reference: https://ae-plugins.docsforadobe.dev/effect-basics/PF_ParamDef/.

The owner accepted build31 for main on 2026-10-04, then requested the Particle
Color Over Life UI shown in their reference screenshot. Task M3-07 implements
an independent visible multi-stop gradient in AE Effect Controls, not a pair of
start/end colors. It uses the same 2..8-stop gradient as CEP and rendering.
Native mouse/keyboard edits change the existing constant stop-bank values during
supported PF event callbacks, with change flags and the existing graph publish
transaction. PF draw/update callbacks never author values. Drawbot owns drawing;
the documented AE color picker owns color selection. UI state/clipboard are
session-only, bounded, and contain no retained SDK objects or authored metadata.

The editor supports add/select/drag/edit/delete, endpoint protection, Flip,
Copy/Paste and independently defined presets. Linear is the currently supported
interpolation and is labelled explicitly; no inert Smooth mode is advertised.
CEP has matching actions and data validation. The Color swatch stays visible as
in the reference; Particle Color chooses solid color or gradient evaluation.
The gradient is shown in non-solid modes and Circle hides its irrelevant Size Y.

Particle schema5 keeps disk IDs but reorganizes stream order into Shape, Life,
Life Random, Particle Properties, Over Life and Rotation. New group disk IDs
are explicit in schema/node-parameters.json. Main manifest/alias bank, graph codec,
C ABI3, temporal sampling and saved gradient encoding remain unchanged. Fresh
Particle effects/graphs are required in development, as authorized by the owner;
there is no old Particle layout migration. Source binding and native readers use
the shared new layout constants. No released disk ID or match name is reused.

The supported Particle controls follow the supplied names/order/defaults. Texture
aspect controls require texture sources; source and model shape families, gradient
alpha stops, new compositing models and other unimplemented reference families
are separate backlog work, not nonfunctional UI switches. Render semantics stay
as independently documented in ADRs0025/0026. Exact reference editor pixel/style
parity is not claimed. Official behavior reference:
https://superluminal.tv/user-guide (Particle / Start / Particle Color / Rotation).

M3-07 owns the native editor/model, NodeEffects/flags/build/version, NodeRecord
layout, NativeNodeGraph/NodeGraphSync binding and fixtures, node schema, CEP
gradient interactions/styles/gateway, the Particle schema registry in Graph.cpp /
GraphConstruction.cpp, and corresponding focused tests/docs. Paired rollback
retains newly added CEP sources and restores their prior absence; old manifests
without an existed field continue to mean a saved source must be restored.
Render.hpp and the simulation/GPU boundaries are unchanged in this UI task.
The installed accepted main build stays separate from candidate construction;
deployment uses ADR0011 standing authorization only with AE absent.
