# Published source reconciliation

The 11 files listed in [the audit](release-source-audit.json) were
present in the published numbered release source archive but had not been
committed to the source branch. Each local file was compared byte-for-byte with
that archive before committing it. This records the existing clock controls,
build/package integration and applicable host timer acceptance.

No player, launcher, installer or firmware binary is changed by this audit.
It does not add a performance claim or replace the recorded hardware checks.
Separate experimental edits, where present, are excluded from this commit.
