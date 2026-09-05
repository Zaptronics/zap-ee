# Modules

One directory per Zaptronics module family. Public headers belong under
`include/zaptronics/modules/`.

A module owns board topology, fitted devices, logical channel numbering,
capability reporting and module-intrinsic safe behaviour. It consumes generic
chip-driver targets rather than copying driver source files.
