# ADR 0001: independent product identity

- Status: accepted for the internal M0/M1 shell; revisit before first shared release.
- Date: 2026-09-27

Use project codename **Starfield Particle**, display category **Starfield FX**, match name `org.starfieldfx.particle`, and internal package ID `org.starfieldfx.aftereffects`. This gives the new effect its own stable host identity. Do not register under the old effect's match name or vendor identity.

The effect match name and parameter numeric IDs are project-file compatibility keys. Once a project is shared, published, or used in production, changing them is a breaking migration. PiPL and runtime metadata must be generated from or checked against the same manifest.
