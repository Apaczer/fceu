void FCEUPPU_Init(void);
void FCEUPPU_Reset(void);
void FCEUPPU_Power(void);
void FCEUPPU_Loop(int skip);

void FCEUPPU_LineUpdate098();

void PowerNES098(void);
#define FCEUPPU_LineUpdate() \
	if (PowerNES == PowerNES098) FCEUPPU_LineUpdate098()
