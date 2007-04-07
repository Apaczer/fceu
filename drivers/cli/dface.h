extern CFGSTRUCT DriverConfig[];
extern ARGPSTRUCT DriverArgs[];
extern char *DriverUsage;

void DoDriverArgs(void);
void GetBaseDirectory(char *BaseDirectory);

int InitSound(void);
void WriteSound(int32 *Buffer, int Count, int NoWaiting);
void KillSound(void);
void SilenceSound(int s); /* DOS and SDL */


int InitMouse(void);
void KillMouse(void);
void GetMouseData(uint32 *MouseData);

int InitJoysticks(void);
void KillJoysticks(void);
uint32 *GetJSOr(void);

int InitKeyboard(void);
int UpdateKeyboard(void);
char *GetKeyboard(void);
void KillKeyboard(void);

int InitVideo(void);
void KillVideo(void);
void BlitScreen(uint8 *XBuf);
void LockConsole(void);
void UnlockConsole(void);
void ToggleFS();		/* SDL */
