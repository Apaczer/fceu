typedef struct {
	int sound;
	int joy[4];
	int joyAMap[4][2];
	int joyBMap[4][4];
	// gp2x specific
	int showfps;
	int scaling;		// unscaled=0, hw_hor, hw_hor_vert, sw_hor
	int frameskip;		// -1 ~ auto, >=0 ~ count
	int sstate_confirm;
	int region_force;	// 0 ~ off, 1 ~ PAL, 2 ~ NTSC
	int cpuclock;
	int mmuhack;
	int ramtimings;
} DSETTINGS;

extern DSETTINGS Settings;

void gp2x_opt_setup(void);

