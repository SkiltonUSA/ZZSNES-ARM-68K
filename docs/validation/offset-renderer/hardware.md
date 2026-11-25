# Physical offset-renderer comparison

ARM 800 MHz KEEP; Normal CPU; fullscreen; scanlines off; Paula AHI `0002000e`,
22,030 Hz output. Core0 AGA service inactive. Star Fox 2 starts fresh; Stunt Race
FX restores the same race checkpoint before each run. No user input/listening test.

| Run | Frames | Core time (s) | Reported elapsed (s) | Starvations | Silence queued |
|---|---:|---:|---:|---:|---:|
| offset-renderer-0-reference | 900 | 16.487228 | 23.478280 | 222 | 312 |
| offset-renderer-1-candidate | 900 | 16.208602 | 23.232018 | 219 | 298 |
| offset-renderer-2-candidate | 900 | 16.209849 | 23.156223 | 215 | 298 |
| offset-renderer-3-reference | 900 | 16.488666 | 23.524306 | 226 | 314 |
| offset-race-0-reference | 600 | 12.079736 | 17.367143 | 185 | 253 |
| offset-race-1-candidate | 600 | 12.141058 | 16.513697 | 194 | 212 |
| offset-race-2-candidate | 600 | 12.142378 | 16.552202 | 192 | 212 |
| offset-race-3-reference | 600 | 12.077276 | 16.510356 | 192 | 211 |
| offset-race-repeat-0-reference | 600 | 12.078130 | 16.696138 | 194 | 211 |

Star Fox 2 uses reference/candidate/candidate/reference order. Mean core execution
time falls **1.69%**; reported elapsed falls 1.31%. Audio starvation remains high
(222/226 reference events versus 219/215 candidate events). This does not establish
audibly continuous play or a general FPS gain.

**Exclude `offset-race-0-reference` from timing comparisons:** it automatically
refreshes the thumbnail after switching games. Its raw log is retained, including
`SNAPSHOT saved`. The two candidate runs and subsequent two reference runs have
no automatic capture. This valid race comparison is candidate/candidate/reference/
reference order, not ABBA. Mean core execution time rises **0.53%**; elapsed times
overlap and starvation averages 193 in both groups. No consistent race improvement.

All nine physical runs restore/exit cleanly with zero audio/display errors,
register/cache restoration and released memory. Guest core hashes at sampled
frames 1/600 agree in each game. Exact video/PCM/GSU comparison was performed in
native/ARM replay; sampled hardware hashes alone are not full-frame proof.

Decision: retain an opt-in experiment, **do not promote it to production**.
Production remains the recorded 0.7.13 player. Further work needs a larger,
repeatable benefit across demanding gameplay and listening validation. Do not add
this result to the separately measured GSU-loop percentage. No combined build
has been measured.
