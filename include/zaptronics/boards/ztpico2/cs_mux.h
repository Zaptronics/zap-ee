// cs_mux.h
#pragma once

// Public "no line" indicator
#define CS_INIT_STATE (-2)
#define CS_NO_LINE (-1)

#ifdef __cplusplus
extern "C" {
#endif

// Initialise all CS GPIOs and put both decoders into their idle state.
void cs_init(void);

// Select a decoded CS line by integer:
//
//   line 0..7   -> first 74HC138, Y0..Y7  (second chip idle)
//   line 8..15  -> second 74HC138, Y0..Y7 (first chip idle)
//   line <0 or >15 -> both chips idle (no device selected)
//
// You can safely pass CS_NO_LINE or 16 to mean "none".
void cs_select_line(int line);

// Convenience wrapper: deselect all devices.
static inline void cs_deselect(void) {
    cs_select_line(CS_NO_LINE);
}

// Optional: get the currently selected line, or CS_NO_LINE.
int cs_get_current_line(void);

#ifdef __cplusplus
}
#endif
