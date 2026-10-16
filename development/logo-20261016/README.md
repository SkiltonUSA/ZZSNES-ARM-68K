# Colour logo update

Supplied PNG is the artwork master. With Python 3 and ImageMagick installed,
run python3 regenerate.py in this directory to regenerate embedded 7-plane
MUI data, a 128-colour Workbench icon and the preview. make current rebuilds
the launcher using these inputs; its player/ARM payload is unchanged.

Artwork supplied by the project owner. Transparency is index zero. The icon
helper preserves existing tooltypes, position and launch metadata, then checks
its saved colour pixels by reading the icon back through icon.library v44+.
