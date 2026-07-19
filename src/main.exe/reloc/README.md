# Normal-link extension sources

Place new C translation units in this directory when using `./Build relink`.
The normal linker collects their text, read-only data, initialized data,
small BSS, and regular BSS instead of discarding sections that are absent from
Splat's retail input list.

This directory is not a trampoline or a fixed-address patch area. GNU `ld`
places these sections after the retail initialized inputs and before the
linker-owned BSS. Existing decompiled functions can be edited in place; this
directory is only needed for brand-new translation units or helpers.

The fixed pool ceiling/current RAM budget limits added initialized/BSS storage
to the available RAM gap, and the remaining relocation audits still determine
whether a changed executable is runnable. See `docs/relocatable-build.md`.
