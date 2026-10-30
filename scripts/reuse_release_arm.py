"""Preserve the recorded 0.7.8 ARM payload for a host-only rebuild.

Offsets come from the release Hunk's memcpy/relocation loop disassembly.
The whole-executable digest pins those offsets; arbitrary binaries are rejected.
This is not a substitute for compiling changed ARM sources.
"""
import hashlib
import struct

SHA256 = '4fd4cbf9e800f3f2db3baaff2adcc29e9b8bfdf05ef61a68b6111c328fb53664'


def header(binary):
    if len(binary) != 1099224 or hashlib.sha256(binary).hexdigest() != SHA256:
        raise ValueError('Expected the recorded ZZSNES 0.7.8 release player')
    image = binary[256:256 + 742424]
    mapping = binary[40 + 0xb54f0:40 + 0xb5628]
    relocations = struct.unpack_from('>3665I', binary, 40 + 0xb5628)
    if len(set(relocations)) != len(relocations):
        raise ValueError('Duplicate relocation')
    for offset in relocations:
        if offset % 4 or offset + 4 > len(image):
            raise ValueError('Relocation outside the preserved image')
        if struct.pack('<II', offset, 23) not in image:
            raise ValueError('Relocation missing from embedded ARM ELF table')
    def array(name, kind, values):
        return f'static const {kind} {name}[]={{' + ','.join(map(str, values)) + '};\n'
    return ('#define ZZ_ENTRY 0u\n' + array('zz_image', 'unsigned char', image)
            + array('zz_mapping_probe', 'unsigned char', mapping)
            + array('zz_relocations', 'unsigned long', relocations))
