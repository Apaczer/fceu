typedef struct {
	int turbo_rate_add;	// 8.24 int
	int sound_rate;		// in Hz
	int showfps;
	int frameskip;		// -1 ~ auto, >=0 ~ count
	int sstate_confirm;
	int region_force;	// 0 ~ off, 1 ~ NTSC, 2 ~ PAL
	int gamma;
	int accurate_mode;
	// gp2x specific
	int KeyBinds[32];
	int JoyBinds[4][32];
	int scaling;		// gp2x: unscaled=0, hw_hor, hw_hor_vert, sw_hor
	int cpuclock;
	int mmuhack;
	int ramtimings;
	int perfect_vsync;
	// pandora
	int hw_filter;
	int sw_filter;
} DSETTINGS;

extern DSETTINGS Settings;

