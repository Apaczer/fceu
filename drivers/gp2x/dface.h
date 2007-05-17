extern CFGSTRUCT DriverConfig[];
extern ARGPSTRUCT DriverArgs[];
extern char *DriverUsage;

void DoDriverArgs(void);
void GetBaseDirectory(char *BaseDirectory);

int InitSound(void);
void WriteSound(int16 *Buffer, int Count);

void KillSound(void);
void SilenceSound(int s); /* DOS and SDL */


int InitJoysticks(void);
void KillJoysticks(void);
uint32 *GetJSOr(void);

int InitVideo(void);
void KillVideo(void);
void BlitScreen(uint8 *buf);
void LockConsole(void);
void UnlockConsole(void);
void ToggleFS();		/* SDL */
