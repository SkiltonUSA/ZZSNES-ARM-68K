# Compact launcher validation

- Both native screen-budget and 20,000-entry library/legacy persistence checks pass.
- Warnings-as-errors m68k launcher builds pass.
- Real AmigaOS/MUI: seven pages, 640×480 budget, 480/608/768-pixel resize,
  Sound selector visibility, five audio outputs, window-size choices, Current settings
  row count and a 200-entry bounded activity history pass.
- Games fits its 640×480 viewport without horizontal or vertical content overflow.
- Normal launch opens at 1100×500 on the 1920×800 Workbench screen.
- Installed launcher bytes verified by MD5; configuration files and player binaries
  match their pre-install checksums. Earlier launcher retained for rollback.

Only launchers were replaced. These checks do not establish new controller
protocol support, gameplay performance or a new audio fix. Published release
archives have not been replaced.

The read-only compact probe preceded the final summary-row ordering and normal
window-ID change; the resulting production launch was checked separately.

Follow-up: default width reduced to900; both rebuilt launchers installed and
opened at900×500 on hardware. See narrow-launcher.json for current hashes.
