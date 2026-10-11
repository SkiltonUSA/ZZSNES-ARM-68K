"""Shared bounded ARM ELF unpacker from Amiga-MCP-Debugger (GPL-3.0-or-later)."""
import struct

def unpack_elf(data, image_limit=0x8000):
    """Accept one bounded ARM load segment and only local RELATIVE relocations."""
    if data[:6]!=b"\x7fELF\x01\x01" or struct.unpack_from("<H",data,18)[0]!=40:
        raise ValueError("Expected ELF32 little-endian ARM")
    h=struct.unpack_from("<HHIIIIIHHHHHH",data,16)
    entry,phoff,shoff,phsize,phnum,shsize,shnum=h[3],h[4],h[5],h[8],h[9],h[10],h[11]
    segments=[]
    for i in range(phnum):
        p=struct.unpack_from("<8I",data,phoff+i*phsize)
        if p[0]==1:segments.append(p)
    if len(segments)!=1:raise ValueError("Exactly one owned image segment required")
    _,off,addr,_,filesz,memsz,_,_=segments[0]
    if addr or not 0<memsz<image_limit or filesz>memsz or off+filesz>len(data) or entry>=memsz:
        raise ValueError("Invalid image bounds")
    image=bytearray(data[off:off+filesz])+bytearray(memsz-filesz)
    rel=[]
    for i in range(shnum):
        s=struct.unpack_from("<10I",data,shoff+i*shsize)
        if s[1]==4:raise ValueError("RELA unsupported")
        if s[1]!=9:continue
        if s[9]!=8 or s[5]%8:raise ValueError("Invalid relocation table")
        for pos in range(s[4],s[4]+s[5],8):
            at,info=struct.unpack_from("<II",data,pos)
            if info!=23 or at%4 or at+4>memsz or at in rel:
                raise ValueError(f"Unsupported ARM relocation {info} at {at:x}")
            if struct.unpack_from("<I",image,at)[0]>=memsz:
                raise ValueError("Relocation escapes the application allocation")
            rel.append(at)
    return image,rel,entry

