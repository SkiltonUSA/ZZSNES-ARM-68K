from pathlib import Path
from prepare_emulator_logos import prepare_png
r=Path(__file__).resolve().parent
prepare_png(r/'assets/zzsnes.png','ZZSNES',r/'build-inputs')
