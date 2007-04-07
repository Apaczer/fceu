void KeyboardClose(void);
int KeyboardInitialize(void);
void KeyboardUpdate(void);
uint32 KeyboardDodo(void);
uint32 UpdatePPadData(int w);
void UpdateFKB(void);


void ConfigFKB(HWND hParent);
void ConfigKeyboardie(HWND hParent, int port);
void ConfigKeyboardiePowerpad(HWND hParent, int port);

extern int cidisabled;

/* Config stuff: */
extern int keyBMap[4][8];
extern int keybEnable;
extern int powerpadside;
extern int powerpadsc[2][12];

extern int fkbmap[0x48];
extern uint8 fkbkeys[0x48];
