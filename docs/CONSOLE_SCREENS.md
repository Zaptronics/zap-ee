# TCP console screens

0.13.0 adds optional key, screen_tick and screen_closed callbacks. All callbacks
and screen APIs execute in lwIP context, like the existing command handler.
A TCP command can enter raw mode with zap_pico_network_set_raw(true); HTTP cannot.
Raw mode suppresses the prompt and passes decoded data bytes without Enter.
Only the LF/NUL framing byte following an entry CR is suppressed. Telnet
negotiation remains protocol traffic; an escaped IAC is passed as a data key.

Use an ANSI terminal with local line editing and local echo disabled (PuTTY Raw).
Since 0.13.1 the server echoes accepted characters in the ordinary line console,
visibly erases them on Backspace/Delete, and advances the line on Enter. The
raw-key screen path returns before this echo/editor code, so screen keys are
not printed. These client settings can remain unchanged when switching modes.
The editor supports append and backspace/delete, not cursor/history navigation.
The server does not negotiate character mode. Refresh runs about every 500 ms.
Use output_idle to avoid piling up frames on a slow connection. The application
owns ANSI rendering, ESC handling and process state. Disconnect resets raw mode
and calls screen_closed; reconnect begins at the normal prompt. Leaving raw mode
does not automatically print a prompt. Application policy decides whether to stop.

Validation not run: no builds, tests or rollback application.
