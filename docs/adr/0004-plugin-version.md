# ADR 0004: one plug-in version contract

- Status: accepted for the M1 shell.
- Date: 2026-09-27

The plug-in code version is `0.1.0`, stage `DEVELOP`, build `1`. Its packed `PF_VERSION` value is `0x8001` (32769). `ae_plugin/PluginVersion.h` is shared by `PF_Cmd_GLOBAL_SETUP` and the PiPL resource; a compile-time assertion checks the PiPL integer against the Adobe SDK's `PF_VERSION` encoding.

The AE load check caught why this contract matters: the earlier PiPL value `33793` encoded stage `BETA` even though AE's short diagnostic displayed both versions as `0.1`. Keep stage and build fields synchronized along with the displayed major/minor values.
