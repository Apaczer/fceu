void GetFileBase(char *f);
#define MSG_ERRAM       marray[0]
extern char *marray[];
extern uint32 uppow2(uint32 n);

char *FCEU_MakeFName(int type, int id1, char *cd1);

#define FCEUMKF_STATE	1
#define FCEUMKF_SNAP	2
#define FCEUMKF_SAV	3
#define FCEUMKF_CHEAT	4
#define FCEUMKF_FDSROM	5
#define FCEUMKF_PALETTE	6
#define FCEUMKF_GGROM	7
