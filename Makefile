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
