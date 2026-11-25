"""Record completed console builds from their actual output, not snapshot metadata."""
import hashlib
import json
import os
from pathlib import Path


def record_build(root, output, console, version, artifacts, commands):
    root, output = Path(root).resolve(), Path(output).resolve()
    files = {str(Path(name)): hashlib.sha256((output / name).read_bytes()).hexdigest()
             for name in artifacts}
    # Generated/staged source is retained for auditing the actual build inputs.
    suffixes = {'.c', '.cpp', '.h', '.hpp', '.S', '.s', '.inc', '.ld', '.a'}
    sources = {str(p.relative_to(output)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in sorted(output.rglob('*')) if p.is_file() and p.suffix in suffixes}
    record = dict(console=console, version=version, output=str(output.relative_to(root)),
                  artifacts=files, staged_inputs=sources,
                  environment={key: os.environ[key].replace(str(root), '<repo>')
                               for key in ('CLANG', 'ARM_BUILD_COMMAND', 'BETA_CONTAINER_COMMAND',
                                           'ARM_CONTAINER_ROOT', 'CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH')
                               if key in os.environ},
                  commands=[[str(arg).replace(str(root), '<repo>') for arg in cmd]
                            for cmd in commands])
    (output / 'build.json').write_text(json.dumps(record, indent=2) + '\n')
    return record
