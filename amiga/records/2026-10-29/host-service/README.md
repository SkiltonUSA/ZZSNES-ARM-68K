# Host-service evidence

[Report](../../../../docs/host-service-profiling.md). These private harnesses
record the exact workspace build/run parameters; adapt transport and ROM paths
locally. The repository build flags provide isolated reproducible test builds.
No ROMs, executable binaries or raw system preference file are included.

- results-0: four accepted profiling runs. SNES 1/2 missed the stereo copy call
  and were superseded by 3/4; those incomplete runs are not in the analysis.
- batch-results-0: eight ABBA timing runs with no additional cost histograms.
- lifecycle: pause/resume, window/fullscreen round trips and clean exit.
- modes.txt / preferences.json: read-only output configuration discovery.
- Per-build directories: source/binary hashes and generated batch patches.
- batch-red.log: expected old-worker failure of the 150-submission requirement.
- build-batch.log: native real-worker tests for references and both candidates.

All final output-hash fields in no-audit runs are unsuitable as a per-frame
pixel comparison. Only the initial readback is used for that purpose here.
Elapsed call costs can overlap across tasks and are not a CPU utilization sum.
