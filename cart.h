extern uint8 *Page[32],*VPage[8],*MMC5SPRVPage[8],*MMC5BGVPage[8];

void ResetCartMapping(void);
void SetupCartPRGMapping(int chip, uint8 *p, uint32 size, int ram);
void SetupCartCHRMapping(int chip, uint8 *p, uint32 size, int ram);
void SetupCartMirroring(int m, int hard, uint8 *extra);

DECLFR(CartBR);
extern uint8 *PRGptr[32];
extern uint8 *CHRptr[32];

extern uint32 PRGsize[32];
extern uint32 CHRsize[32];

extern uint32 PRGmask2[32];
extern uint32 PRGmask4[32];
extern uint32 PRGmask8[32];
extern uint32 PRGmask16[32];
extern uint32 PRGmask32[32];

extern uint32 CHRmask1[32];
extern uint32 CHRmask2[32];
extern uint32 CHRmask4[32];
extern uint32 CHRmask8[32];

void FASTAPASS(2) setprg2(uint32 A, uint32 V);
void FASTAPASS(2) setprg4(uint32 A, uint32 V);
void FASTAPASS(2) setprg8(uint32 A, uint32 V);
void FASTAPASS(2) setprg16(uint32 A, uint32 V);
void FASTAPASS(2) setprg32(uint32 A, uint32 V);

void FASTAPASS(3) setprg2r(int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg4r(int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg8r(int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg16r(int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setprg32r(int r, unsigned int A, unsigned int V);

void FASTAPASS(3) setchr1r(int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setchr2r(int r, unsigned int A, unsigned int V);
void FASTAPASS(3) setchr4r(int r, unsigned int A, unsigned int V);
void FASTAPASS(2) setchr8r(int r, unsigned int V);

void FASTAPASS(2) setchr1(unsigned int A, unsigned int V);
void FASTAPASS(2) setchr2(unsigned int A, unsigned int V);
void FASTAPASS(2) setchr4(unsigned int A, unsigned int V);
void FASTAPASS(2) setchr8(unsigned int V);

void FASTAPASS(2) setvram4(uint32 A, uint8 *p);
void FASTAPASS(1) setvram8(uint8 *p);

void FASTAPASS(3) setvramb1(uint8 *p, uint32 A, uint32 b);
void FASTAPASS(3) setvramb2(uint8 *p, uint32 A, uint32 b);
void FASTAPASS(3) setvramb4(uint8 *p, uint32 A, uint32 b);
void FASTAPASS(2) setvramb8(uint8 *p, uint32 b);

void FASTAPASS(1) setmirror(int t);
void setmirrorw(int a, int b, int c, int d);
void FASTAPASS(3) setntamem(uint8 *p, int ram, uint32 b);

#define MI_H 0
#define MI_V 1
#define MI_0 2
#define MI_1 3

extern int geniestage;

void GeniePower(void);

void OpenGenie(void);
void CloseGenie(void);
