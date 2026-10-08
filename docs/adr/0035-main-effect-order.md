# ADR 0035 — Main effect names, order and launcher

Status: implementation built; AE migration qualification open, 2026-10-08. Task M3-14.

## Scope

The owner goal requests reference main-effect naming and order. Use the owner
AE23.5 inventory and newer Motion Blur screenshots as behavioral data. Align
the implemented subset; do not add inactive shading/volume/physics controls.
M3-13 Source textures are deployed as native53/CEP53/ABI6; their actual AE gates
and Masks/Effects implementation remain open while this independent UI work
proceeds. The complete goal remains active.

Implemented main order: image and Panel/Presets actions, owner-requested Max
Particles cap, Time Remapping, Render Settings, Motion Blur, Simulation Settings,
GPU Rendering. Time Remapping's checkbox becomes `On / Off`. The current
reference Motion Blur labels/defaults/modes stay unchanged; PTF remains deferred.

## Identity and migration

The public SDK describes saved parameters by PF_ParamDef.uu.id and permits
reordering registration when disk IDs remain unchanged:
https://ae-plugins.docsforadobe.dev/effect-details/changing-parameter-orders/
The local May2023 SDK is the build authority. No ID, default, type, graph key,
sequence format, Core ABI, node schema or effect match name changes here.

Only the existing sixteen renderer UI streams610..625 are permuted:

| Group | Previous streams | New streams | Fixed disk IDs |
| --- | --- | --- | --- |
| Motion Blur | 616..625 | 610..619 | 1640..1649 |
| Simulation Settings | 610..612 | 620..622 | 1600..1602 |
| GPU Rendering | 613..615 | 623..625 | 1610..1612 |

Main streams1..609 and626..754 remain fixed, including the graph carrier,
commit/revision/checksum controls, 512 native numeric aliases and128 texture
resource slots. Schema parameter `id` remains its permanent published logical
ID; an explicit `index` describes the new registration position for moved rows.
Headers and every native checkout/callback use the new physical positions.
CEP globals resolve fixed disk IDs and require no numeric-index migration.
AE should restore values, keyframes and expressions by fixed disk IDs. Saved
projects and user-written numeric-index expressions remain explicit AE2023
migration gates; compilation/fake-host evidence cannot close them.

The existing custom launcher stream1/disk1631 gains two painted actions below
the image, `Panel: Click To Open` and `Presets: Browse`. Its click hit areas select
the appropriate existing extension menu command. This changes no host preference,
registry key, process, renderer parameter or graph value. Image clicking keeps
the existing Presets action. Narrow panels scale the image/actions together.

Native54/CEP54 is a full paired build with unchanged ABI6. Preserve exact
native53/CEP53 rollback and require a fresh closed-AE check before publication.

## Minimal checks and remaining gates

Check the full registered layout against schema, unique/fixed disk IDs, the
sixteen-stream permutation, untouched alias/resource bounds, native callbacks
and read/write types. Exercise Motion Blur UI enabled states and both launcher
hit targets with bounded fake SDK events. Run only related existing gateway/
preset/texture checks and build May2023 /MT. AE23.5 saved projects, animation,
undo/reopen, actual group order and menu focus remain owner gates.

## Candidate evidence and live-panel correction, 2026-10-08

The sixteen-stream permutation is implemented with unchanged disk IDs. All
native global checkouts, commit readers and UI callbacks use one Motion Blur
index table. CEP resolves Time Remapping by disk0921 first, then the previous
and new display labels; missing controls produce a named error.

The owner reported a fresh-effect `host_error: TypeError: null is not an object`
while native53 was installed. The live CEP Junction had also exposed the in-flight
CEP54 source edit. A fake-host regression reproduced an empty main-switch lookup:
the initial fallback incorrectly treated a display label as a match name and
used an unpadded disk suffix. That path is fixed, but the owner's exact host
failure cause remains a hypothesis pending their retry. A fresh no-AE check
allowed restoring all eleven managed live files to exact deployed CEP53 hashes.
The corrected CEP54 draft now stays under artifacts/prepared; no further draft
edit is made in the live CEP Junction. Native53/Core/selector were not changed
during this recovery.

Evidence:

- `tests/RunMainParameterTests.ps1 -Run`: 1523 checks, 0 failures, using actual
  main registration, Motion Blur UI and SmartFX global checkout code.
  Log: artifacts/m3-14-main-parameter-tests.log.
- `tests/RunMainLauncherTests.ps1 -Run`: 103 checks, 0 failures, including actual
  click events and both menu scripts in a fake Utility suite; no AE script runs.
  Log: artifacts/m3-14-main-launcher-tests.log.
- `tests/main_order_tests.js` with STARFIELD_PANEL_ROOT set to the isolated CEP54:
  30 checks passed, including fixed disk/logical IDs, new stream indices and
  both old/new Time Remapping labels.
  Log: artifacts/m3-14-main-order-tests.log.
- Isolated CEP54 Texture/preset-resource checks: 36 passed.
  Log: artifacts/m3-14-texture-panel-regression.log.
- Restored CEP53 fresh-layer gateway and native Particle/Emitter/Force gateway
  passed; Texture checks36 passed.
  Logs: artifacts/m3-13-restored-fresh-layer-gateway-tests.log,
  artifacts/m3-13-restored-gateway-tests.log,
  artifacts/m3-13-restored-texture-panel-tests.log.
- Complete native/Core May2023 /MT build passed without publishing dist/runtime.
  Log: artifacts/m3-14-main-order-build.log.

These checks do not qualify AE saved-project migration or Source texture behavior.

## Paired deployment, 2026-10-08

At 17:08 +08:00, a fresh no-AfterFX/AfterFX_64 check allowed publication under
the standing owner authorization. Source4d64765a8e7ceceeaa5ff5f7229e77c9d1a66a25
is installed as native54/CEP54/ABI6 through Deploy-TestBuild.ps1 and the existing
two Junctions. Seven native/Core and eleven CEP hashes were verified; the Core
selector remains StarfieldCore-037D48F4411A16E8.dll and its selected DLL hash
matches StarfieldCore.dll. The verified backup contains exact native53/CEP53.
The read-only paired rollback report passed. No process/registry change occurred.

Receipts: artifacts/m3-14-native54-deploy-before.json,
artifacts/m3-14-native54-deploy-after.json,
artifacts/m3-14-native54-native-deploy.log,
artifacts/m3-14-native54-deploy-wrapper.log,
artifacts/m3-14-native54-rollback-report.log.

One-step rollback, with AE closed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm3-14-native54-panel54-main-order-20261008' -Restore
```

The owner's fresh-effect error retry, actual group/menu display and saved-project
migration are pending. Use native54/CEP54 for the next host feedback. M3-13 Source
time/footage/undo gates, Masks/Effects and the remaining Particle families stay
open under the complete goal.
