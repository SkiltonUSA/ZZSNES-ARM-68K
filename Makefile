PYTHON ?= python3
MODEL_PYTHON ?= python3
.PHONY: verify test-native test-model
verify:
	$(PYTHON) scripts/verify_backup.py
test-native: verify
	$(PYTHON) scripts/build_pocketsnes.py
	$(PYTHON) scripts/build_mesen_snes.py
	$(PYTHON) scripts/build_mesen_snes_system.py --integer-clock
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
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/library-20261016/source -Idevelopment/beta-20261015/source/amiga/emulator tests/amiga/emulator/model.c -o .context/test-current
	.context/test-current
	$(CC) -O2 -g -fsanitize=address,undefined -Itests/amiga/emulator/mock -Idevelopment/library-20261016/source tests/amiga/emulator/scan.c -o .context/test-scan
	.context/test-scan
	$(MODEL_PYTHON) tests/amiga/emulator/check_current_arm.py

.PHONY: test-library-large
test-library-large:
	mkdir -p .context
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/library-20261016/source tests/amiga/emulator/large_library.c -o .context/test-large
	.context/test-large
	$(CC) -O2 -g -fsanitize=address,undefined -Idevelopment/library-20261016/source tests/amiga/emulator/legacy_model.c -o .context/test-legacy
	.context/test-legacy
