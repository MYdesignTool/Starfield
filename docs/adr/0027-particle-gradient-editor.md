# ADR 0027: native Particle gradient editor

Status: candidate implementation/build/scoped checks complete on development
branch; AE 2023 qualification pending.

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
