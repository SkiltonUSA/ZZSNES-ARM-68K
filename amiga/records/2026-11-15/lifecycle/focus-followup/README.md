# Focus follow-up: isolated rerun required

Private, bounded 900-frame tests switched fullscreen at frame 300 and back to a
window at frame 600, with activation samples taken inside the player. Both NES
and SNES reference tests finished cleanly but lost focus shortly after returning
to Workbench. A delayed-activation NES test also finished cleanly and lost focus.
The delayed SNES test encountered a bridge timeout; its completion log was not
retrieved and it is **not counted as a pass**.

A separate window-scaling acceptance test from another workspace was then
observed on the same Amiga. This means the focus/timing tests were not isolated;
it does not establish the cause of the focus loss, bridge timeout or earlier
failures. No focus workaround was promoted. A built explicit-screen-front
variant was not run.

The user chose to leave that other workspace testing and continue emulator work
offline. A final process listing showed no private NES/SNES test process. Do not
resume hardware tests until exclusive access is available. No installed player,
firmware, Workbench or AHI preference was changed by this follow-up.
