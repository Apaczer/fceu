typedef struct {
        uint8 FP_FASTAPASS(1) (*Read)(int w);
	void FP_FASTAPASS(1) (*Write)(uint8 v);
        void FP_FASTAPASS(1) (*Strobe)(int w);
	void FP_FASTAPASS(3) (*Update)(int w, void *data, int arg);
	void FP_FASTAPASS(3) (*SLHook)(int w, uint8 *buf, int line);
	void FP_FASTAPASS(3) (*Draw)(int w, uint8 *buf, int arg);
} INPUTC;

typedef struct {
	uint8 FP_FASTAPASS(2) (*Read)(int w, uint8 ret);
	void FP_FASTAPASS(1) (*Write)(uint8 v);
	void (*Strobe)(void);
        void FP_FASTAPASS(2) (*Update)(void *data, int arg);
        void FP_FASTAPASS(2) (*SLHook)(uint8 *buf, int line);
        void FP_FASTAPASS(2) (*Draw)(uint8 *buf, int arg);
} INPUTCFC;

void DrawInput(uint8 *buf);
void UpdateInput(void);
void InitializeInput(void);
extern void (*PStrobe[2])(void);
extern void (*InputScanlineHook)(uint8 *buf, int line);
