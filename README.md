## FCE Ultra for MiyooCFW

FCEU is a cross platform, NTSC and PAL Famicom/NES emulator. This is a downstream fork of [GPFCE](https://notaz.gp2x.de/git/fceu.git), which comes with asm CPU core for old ARM optimization. 

### Build steps:
- cross-compile (MiyooCFW)  
`make -j$(nproc) MIYOO=1`

- native (Linux via SDL)  
`make -j$(nproc)`

For debug define `DEBUG=1` before/in Makefile
