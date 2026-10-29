# Configured audio output

The new per-application AX default redirected audio away from the connected
Paula RCA output. Normal builds now open configured AHI unit 0. AX remains
available through the explicit `--ax-audio-test` development build. Global
AHI preferences are unchanged.

The rebuilt ARM payload is byte-for-byte identical to the previous release.
Native tests run the generated AHI worker with fake Exec/AHI and check unit 0,
PCM byte ordering, short flush, state drain, failures and cleanup.
Run `python3 scripts/test_configured_audio.py` after `--release-ax`.

The 180-frame Workbench-window hardware check completed with matching queued,
submitted and completed sample counts, zero audio errors, restored ARM registers,
MMU/cache witnesses and released runtime memory. Logged late submissions remain;
this is routing/cleanup validation, not a performance or audible-quality claim.
Listening confirmation is pending.

Actions: inspected settings/logs; confirmed speakers use Paula; disabled automatic
AX selection for normal builds; rebuilt matching players/launcher; ran native and
physical checks; backed up and replaced installed executables. ROMs, saves and
configuration files were preserved.
