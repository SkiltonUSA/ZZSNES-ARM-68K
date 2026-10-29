Native regression suite: make test-native passed.
Emitter tests: 25,536 cases, three addresses, preserved ABI/state, wrap, flags, live timing and shutdown.
Actual original core versus generated handlers: 32,768 cases, two addresses, all four mode tables.
Linked frames: SMW 600, PAL Chaos Engine 300, equal video/PCM and observed generated-handler execution.
PAL reference initialization exceeded the original 50-million model instruction limit; both complete under a 200-million initialization limit. Per-frame limit remains 50 million. This is a harness budget change, not a core speed change.
Physical: six 300-frame runs, output ca7174c7, all 160,414 PCM frames delivered, MMU/register/witness restoration and exit0/release. Audio-gap observations are retained; audible quality was not verified.
Final default experimental rebuild byte-identical to tested binaries.
