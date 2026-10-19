# Scalable launcher — 0.6.3-dev

The launcher chooses a compact layout on smaller Workbench screens, including
640×480 and 800×600. Each page scrolls independently. Navigation, Launch,
Save settings, Quit, version and temperature remain outside the page viewport.

Compact mode stacks the ROM list above game details, shows smaller previews,
arranges save cards in two columns and reflows the controller diagram. Static
help labels wrap, and startup/loading cards use the smaller layout. Normal
Workbench resizing works in both layouts; compact/regular window sizes are
remembered separately. The layout style is selected when the launcher starts.

No extra MUI classes are required. The launcher uses MUI 3.8 Scrollgroup and
Virtgroup, local fonts and a Topaz fallback. It never selects or saves a screen mode.

ROM Manager, scanline controls, ROM/state formats and emulation players are retained.
The package includes the existing Xanxi scanline tools and install-missing helper.

## Build

`make launcher` builds the development launcher only. Editable sources are in
`development/scalable-launcher-20261019/source`, layered after the existing
optimisation/ROM Manager sources. `make test-launcher-layout` checks screen
budgets and label wrapping. `scripts/build_launcher.py --probe 640x480` builds
an isolated, read-only MUI layout probe; it does not deploy or launch it.

[Development download](../artifacts/development/scalable-launcher-20261019/ZZSNES-0.6.3-dev-layout.zip).
Published beta release archives remain unchanged.

## Validation

On the physical Amiga, all seven pages, scrolling, resizing and loading cards
passed under 640×480, 800×600 and 1024×768 window budgets. This used the existing
Workbench desktop; no actual screen-mode change was made. A remembered larger
window is clamped on first opening. Game players and saved data formats are unchanged.

[Reports and screenshots](../artifacts/development/scalable-launcher-20261019/).
