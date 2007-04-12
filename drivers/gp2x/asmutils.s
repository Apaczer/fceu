@ vim:filetype=armasm

@ test
.global flushcache @ beginning_addr, end_addr, flags

flushcache:
    swi #0x9f0002
    mov pc, lr

