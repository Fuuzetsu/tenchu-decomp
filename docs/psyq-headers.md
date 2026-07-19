# PsyQ header policy

## Default build: clean ABI shims

The files under [`include/psxsdk`](../include/psxsdk) are small,
independently maintained declarations for only the PsyQ interfaces Tenchu uses.
They are not copies of Sony's SDK headers. Keep SDK type names, field order,
signedness, and function signatures aligned with PsyQ 4.5 where the retail
binary agrees. Compiler output and `./Build check` remain the final authority.

Do not add guessed, layout-compatible local structures when a known SDK type
exists. Extend the relevant shim with the minimum declaration instead. Also do
not vendor or automatically download a PsyQ SDK into the normal build.

## PsyQ 4.5 audit

An isolated validation build used a locally supplied PsyQ 4.5 installation,
outside this repository. Its headers were included in their required order:
`SYS/TYPES.H`, `LIBGTE.H`, `LIBGPU.H`, `LIBGS.H`, and `LIBCD.H`. After removing
the declarations superseded by those headers, the project compiled and linked
successfully. Of 708 compiled objects, 707 were byte-identical to the clean-shim
build.

The sole difference was four bytes in `DrawTargetS`. PsyQ 4.5 declares the
`GsSortLine` priority as `unsigned short`, which makes GCC emit an `andi` before
the second call. The retail target has a plain register move. The function
therefore retains its target-proven, full-width local declaration. This is a
documented retail/compiler fact, not a reason to alter the shared SDK shim.
No SDK files from that audit are committed here.

## Why the SDK is not a fetched dependency

The project's pinned nixpkgs has no PsyQ package. The commonly referenced
[FoxdieTeam archive](https://github.com/FoxdieTeam/psyq_sdk) exposes no
repository license metadata, bundles the original tools and libraries, and its
header notices reserve Sony's rights. Making it a mandatory fetched input would
weaken the project's provenance and reproducibility story, so the build does
not fetch it. This is a project-distribution policy, not legal advice.

Modern open SDKs are useful for homebrew and ports, but they are not exact
substitutes for this matching task. The [PSn00bSDK README](https://github.com/Lameguy64/PSn00bSDK)
explicitly says it is not intended as a drop-in replacement. The
[PsyCross README](https://github.com/OpenDriver2/PsyCross) describes partial
PsyQ compatibility, and its current coverage is not an exact `libgs`/GTE ABI
oracle. Neither should silently replace the audited shims in byte-matching
builds.

## Optional external validation

A future audit lane may accept a user-supplied, appropriately licensed PsyQ 4.5
include directory and compile against it locally. Such a lane should never
vendor or download the SDK. It must preserve the header order above, overlay
only target-proven exceptions such as `DrawTargetS`, and finish with
`./Build check`. The default hermetic build should continue to use the clean
minimal shims.
