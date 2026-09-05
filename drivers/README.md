# Drivers

One directory per silicon family. Drivers implement chip protocol, conversion,
identity/status and chip-level configuration only. Public headers belong under
`include/zaptronics/drivers/`.

Drivers must not contain module connector mapping, Hub motherboard pin numbers,
application sequencing, CLI formatting, Pico SDK types, Zephyr types, or other
vendor-SDK types in their public API.
