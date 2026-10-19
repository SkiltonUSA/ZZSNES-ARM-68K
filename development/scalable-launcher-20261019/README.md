# Scalable MUI launcher overlay

Applied after optimisation-20261018 by build_launcher.py and the
--optimisation build_current.py path. Historical beta sources remain intact.

All seven pages use built-in MUI scroll/virtual groups. The virtual-group class
keeps short pages from limiting window enlargement. App-local fonts, preview
sizes, controller positions and save-card columns follow the screen budget.
The first window opening clamps stale larger MUI dimensions; subsequent user
resizes and game-return openings retain their chosen size.

The diagnostic build uses its own application/port, reads a scratch Library.cfg,
never dispatches player/save actions and exits before the normal settings save.
See docs/scalable-launcher.md for commands and physical test scope.
