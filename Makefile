PYTHON ?= python3
MODEL_PYTHON ?= python3
.PHONY: verify test-native test-model
verify:
	$(PYTHON) scripts/verify_backup.py
test-native: verify
	$(PYTHON) scripts/build_pocketsnes.py
	$(PYTHON) scripts/build_mesen_snes.py
	$(PYTHON) scripts/build_mesen_snes_system.py --integer-clock
	$(PYTHON) scripts/test_state_sprites.py

.PHONY: test-state-sprites
test-state-sprites:
	$(PYTHON) scripts/test_state_sprites.py
test-model: verify
	$(MODEL_PYTHON) tests/amiga/pocketsnes/check_arm.py artifacts/pocketsnes/pocket.elf --profile-clock
	$(MODEL_PYTHON) tests/amiga/mesen_snes/check_arm.py artifacts/mesen-snes/snes.elf
	$(MODEL_PYTHON) tests/amiga/mesen_snes/check_system_arm.py artifacts/mesen-snes-system-words/system.elf

.PHONY: package-library prepare-library test-library-host
package-library: verify
	$(PYTHON) scripts/package_pocketsnes_library.py
prepare-library: verify
	$(PYTHON) scripts/prepare_saved_library.py
test-library-host:
	$(PYTHON) tests/amiga/pocketsnes/check_library_host.py

.PHONY: beta verify-beta
beta:
	$(PYTHON) scripts/build_beta.py
verify-beta:
	$(PYTHON) scripts/verify_beta.py

# Current sidebar/display beta
current:
	$(PYTHON) scripts/build_current.py
current-arm:
	$(PYTHON) scripts/build_current.py --arm
verify-current:
	$(PYTHON) scripts/verify_current.py
test-current:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/borders-20261016/source -Idevelopment/beta-20261015/source/amiga/emulator tests/amiga/emulator/model.c -o .context/test-current
	.context/test-current
	$(CC) -O2 -g -fsanitize=address,undefined -Itests/amiga/emulator/mock -Idevelopment/borders-20261016/source tests/amiga/emulator/scan.c -o .context/test-scan
	.context/test-scan
	$(MODEL_PYTHON) tests/amiga/emulator/check_current_arm.py

.PHONY: test-library-large
test-library-large:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/borders-20261016/source tests/amiga/emulator/large_library.c -o .context/test-large
	.context/test-large
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/borders-20261016/source tests/amiga/emulator/legacy_model.c -o .context/test-legacy
	.context/test-legacy

.PHONY: test-zip
test-zip:
	ROM_ZIP_INCLUDE=development/borders-20261016/source/zip $(PYTHON) tests/amiga/emulator/zip/test.py

.PHONY: test-border
test-border:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/borders-20261016/source -Idevelopment/borders-20261016/player -Idevelopment/beta-20261015/build-inputs/player tests/amiga/emulator/border.c -o .context/test-border
	.context/test-border

.PHONY: test-superfx-timing
test-superfx-timing:
	$(PYTHON) scripts/test_superfx_timing.py

.PHONY: test-fx3-warning
test-fx3-warning:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/fx3-20261016 -Idevelopment/borders-20261016/source tests/amiga/pocketsnes/fx3_warning.c -o .context/test-fx3-warning
	.context/test-fx3-warning

.PHONY: current-superfx
current-superfx:
	$(PYTHON) scripts/build_current.py --arm --superfx

.PHONY: test-fx3 current-thin current-thin-superfx
test-fx3:
	$(PYTHON) scripts/test_fx3.py
current-thin:
	$(PYTHON) scripts/build_current.py --arm --thin-scanlines
current-thin-superfx:
	$(PYTHON) scripts/build_current.py --arm --superfx --thin-scanlines

.PHONY: test-thin-geometry
test-thin-geometry:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/thin-scanlines-20261016/player tests/amiga/emulator/thin_geometry.c -o .context/test-thin-geometry
	.context/test-thin-geometry

# Measured development candidate; neither target deploys to hardware.
optimisation:
	$(PYTHON) scripts/build_current.py --arm --optimisation --core-o2
test-optimisation-native:
	mkdir -p .context
	$(CXX) -std=c++14 -O2 -g -fsanitize=address,undefined -Idevelopment/optimisation-20261018/player -Idevelopment/borders-20261016/player -Idevelopment/beta-20261015/build-inputs/player tests/amiga/emulator/optimisation.cpp -o .context/test-optimisation-pixels
	.context/test-optimisation-pixels
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/optimisation-20261018/source -Idevelopment/borders-20261016/source tests/amiga/emulator/optimisation_library.c -o .context/test-optimisation-library
	.context/test-optimisation-library
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/optimisation-20261018/source -Idevelopment/borders-20261016/source tests/amiga/emulator/legacy_model.c -o .context/test-optimisation-legacy
	.context/test-optimisation-legacy

.PHONY: test-optimisation-arm
test-optimisation-arm:
	CURRENT_ARM_DIR=.context/optimisation-o2-build/player TEST_THIN_SCANLINES=1 $(MODEL_PYTHON) tests/amiga/emulator/check_current_arm.py

.PHONY: test-smooth-arm
test-smooth-arm:
	CURRENT_ARM_DIR=.context/optimisation-o2-build/player $(MODEL_PYTHON) tests/amiga/emulator/check_smooth_arm.py

.PHONY: test-scanline-key
test-scanline-key:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/optimisation-20261018/host tests/amiga/emulator/scanline_key.c -o .context/test-scanline-key
	.context/test-scanline-key

.PHONY: test-rom-manager launcher
launcher:
	$(PYTHON) scripts/build_launcher.py
test-rom-manager:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/optimisation-20261018/source -Idevelopment/borders-20261016/source tests/amiga/emulator/rom_manager.c -o .context/test-rom-manager
	.context/test-rom-manager
	$(CC) -O2 -g -fsanitize=address,undefined -Itests/amiga/emulator/mock -Idevelopment/borders-20261016/source tests/amiga/emulator/scan.c -o .context/test-rom-scan
	.context/test-rom-scan

.PHONY: test-region-pacing
test-region-pacing:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/optimisation-20261018/source -Idevelopment/optimisation-20261018/player -Idevelopment/optimisation-20261018/host -Idevelopment/borders-20261016/source -Idevelopment/borders-20261016/build-inputs tests/amiga/emulator/region_pacing.c -o .context/test-region-pacing
	.context/test-region-pacing
