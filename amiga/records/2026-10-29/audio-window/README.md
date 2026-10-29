# Audio and window checks

See [the report](../../../../docs/audio-window.md). `ref/new` records isolate
initial scheduling changes in the original 1× window. `normal` records exercise
the enlarged window implementation; these are core-frame timings, not displayed
FPS or proof of audibly continuous output. The 3× run preceded the final 2×
budget adjustment (its own scheduling is identical). A subsequent host fix
normalizes high-resolution snapshots once, instead of twice. See the SNES
hires capture check for that path. ARM pixel code is unchanged by those host fixes.

`verify` uses expensive 68k pixel comparisons, unsuitable for performance
measurement. The 3× SNES verifier exceeded the harness's 100-second wait, then
completed normally; its final log was retrieved after Core1 had released.
`lifecycle` covers pause, screenshot, save/load, 3×/fullscreen/3× and clean quit.
MUI checks exercise all four display choices and compact layouts.

All completed hardware runs check PCM accounting, device errors, register/MMU/
cache restoration and owned-memory release. This is not listening acceptance.
Private ROMs and save states are excluded.
