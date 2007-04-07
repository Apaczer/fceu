void FDSControl(int what);

#define FDS_IDISK  1
#define FDS_EJECT  2
#define FDS_SELECT 3

int FDSLoad(char *name, int fp);
void FDSSoundReset(void);
