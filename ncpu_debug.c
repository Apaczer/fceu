#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"

// asm core state
extern uint32 nes_registers[0x10];
extern uint32 pc_base;
uint32 PC_prev = 0, g_cnt = 0;

int cpu_repeat;
int cpu_lastval;

static void leave(void)
{
	printf("\nA: %02x, X: %02x, Y: %02x, S: %02x\n", X.A, X.X, X.Y, X.S);
	printf("PC = %04lx, OP=%02X\n", PC_prev, (PC_prev < 0x2000) ? RAM[PC_prev&0x7ff] : ARead[PC_prev](PC_prev));
	exit(1);
}

static void compare_state(void)
{
	uint8 nes_flags;
	int fail = 0;

	if ((nes_registers[0] >> 24) != X.A) {
		printf("A: %02lx vs %02x\n", nes_registers[0] >> 24, X.A);
		fail = 1;
	}

	if ((nes_registers[1] & 0xff) != X.X) {
		printf("X: %02lx vs %02x\n", nes_registers[1] & 0xff, X.X);
		fail = 1;
	}

	if ((nes_registers[2] & 0xff) != X.Y) {
		printf("Y: %02lx vs %02x\n", nes_registers[2] & 0xff, X.Y);
		fail = 1;
	}

	if (nes_registers[3] - pc_base != X.PC) {
		printf("PC: %04lx vs %04x\n", nes_registers[3] - pc_base, X.PC);
		fail = 1;
	}

	if ((nes_registers[4] >> 24) != X.S) {
		printf("S: %02lx vs %02x\n", nes_registers[4] >> 24, X.S);
		fail = 1;
	}

	if (((nes_registers[4]>>8)&0xff) != X.IRQlow) {
		printf("IRQlow: %02lx vs %02x\n", ((nes_registers[4]>>8)&0xff), X.IRQlow);
		fail = 1;
	}

	// NVUB DIZC
	nes_flags = nes_registers[4] & 0x5d;
	if (  nes_registers[5]&0x80000000)  nes_flags |= 0x80; // N
	if (!(nes_registers[5]&0x000000ff)) nes_flags |= 0x02; // Z
	// nes_flags |= 0x20; // U, not set in C core (set only when pushing)

	if (nes_flags != (X.P&~0x20)) {
		printf("flags: %02x vs %02x\n", nes_flags, (X.P&~0x20));
		fail = 1;
	}

	if ((int32)nes_registers[7] != X.count) {
		printf("cycles: %li vs %li\n", (int32)nes_registers[7], X.count);
		fail = 1;
	}

	if (fail) leave();
}


void TriggerIRQ_d(void)
{
	printf("-- irq\n");
	TriggerIRQ_c();
	TriggerIRQ_a();
	compare_state();
}

void TriggerNMI_d(void)
{
	printf("-- nmi\n");
	TriggerNMI_c();
	TriggerNMI_a();
	compare_state();
}

void TriggerNMINSF_d(void)
{
}

void X6502_Run_d(int32 c)
{
	int32 cycles = c << 4; /* *16 */ \
	if (PAL) cycles -= c;  /* *15 */ \

	//printf("-- %06i: run(%i)\n", (int)g_cnt, (int)c);
	g_cnt += c;

	while (cycles > 0)
	{
		PC_prev = X.PC;
		nes_registers[7]=1;
		X.count=1;

		cpu_repeat = 0;
		X6502_Run_c();

		cpu_repeat = 1;
		X6502_Run_a();

		compare_state();
		cycles -= 1 - X.count;
	}

	//printf("-- run_end\n");
}

void X6502_Reset_d(void)
{
	printf("-- reset\n");

	X6502_Reset_c();
	X6502_Reset_a();
	compare_state();
}

void X6502_Power_d(void)
{
	printf("-- power\n");

	X6502_Power_c();
	X6502_Power_a();
	compare_state();
}

void X6502_AddCycles_d(int x)
{
	printf("-- AddCycles(%i|%i)\n", x, x*48);

	printf("can't use this in debug\n");
	exit(1);
	//X6502_AddCycles_c(x);
	//X6502_AddCycles_a(x);
	//compare_state();
}

void X6502_IRQBegin_d(int w)
{
	printf("-- IRQBegin(%02x)\n", w);

	X6502_IRQBegin_c(w);
	X6502_IRQBegin_a(w);
}

void X6502_IRQEnd_d(int w)
{
	printf("-- IRQEnd(%02x)\n", w);

	X6502_IRQEnd_c(w);
	X6502_IRQEnd_a(w);
}



