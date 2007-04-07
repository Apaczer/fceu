void ConfigInput(HWND hParent);
int InitDInput(void);
void CreateInputStuff(void);
void InitInputStuff(void);
void DestroyInput(void);
void InputScreenChanged(int fs);
void FixGIGO(void);

extern LPDIRECTINPUT7 lpDI;

#define JOY_A   1
#define JOY_B   2
#define JOY_SELECT      4
#define JOY_START       8
#define JOY_UP  0x10
#define JOY_DOWN        0x20
#define JOY_LEFT        0x40
#define JOY_RIGHT       0x80


extern int InputType[2];
extern int InputTypeFC;
extern int NoFourScore;
extern int UsrInputType[2];
extern int UsrInputTypeFC;
