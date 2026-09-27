# ADR 0004: one plug-in version contract

- Status: accepted for the M1 shell.
- Date: 2026-09-27

The plug-in code version is `0.1.0`, stage `DEVELOP`, build `2` (G-04). Its packed `PF_VERSION` value is `0x8002` (32770). `ae_plugin/PluginVersion.h` is shared by `PF_Cmd_GLOBAL_SETUP` and the PiPL resource; a compile-time assertion checks the PiPL integer against the Adobe SDK's `PF_VERSION` encoding.

G-04 increments build 1 to build 2 to identify the appended graph/source/capture
parameters (manifest revision 4). Existing IDs and the effect match name are
unchanged; old controls remain in AE Controls mode through the documented
`USE_VALUE_FOR_OLD_PROJECTS` default. Graph byte schema remains 1. See ADR 0008.

The AE load check caught why this contract matters: the earlier PiPL value `33793` encoded stage `BETA` even though AE's short diagnostic displayed both versions as `0.1`. Keep stage and build fields synchronized along with the displayed major/minor values.
