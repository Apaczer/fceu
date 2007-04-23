#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"

// asm core state
extern uint32 nes_registers[0x10];
extern uint32 pc_base;
extern uint8  nes_internal_ram[0x800];
uint32 PC_prev = 0xcccccc, OP_prev = 0xcccccc;
int32  g_cnt = 0;

static int pending_add_cycles = 0, pending_rebase = 0;

uint8  dreads[4];
uint32 dwrites_c[2], dwrites_a[2];
int dread_count_c, dread_count_a, dwrite_count_c, dwrite_count_a;

static void leave(void)
{
	printf("\nA: %02x, X: %02x, Y: %02x, S: %02x\n", X.A, X.X, X.Y, X.S);
	printf("PC = %04lx, OP=%02lX\n", PC_prev, OP_prev);
	exit(1);
}

static void compare_state(void)
{
	uint8 nes_flags;
	int i, fail = 0;

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

	if (dread_count_a != dread_count_c) {
		printf("dread_count: %i vs %i\n", dread_count_a, dread_count_c);
		fail = 1;
	}

	if (dwrite_count_a != dwrite_count_c) {
		printf("dwrite_count: %i vs %i\n", dwrite_count_a, dwrite_count_c);
		fail = 1;
	}

	for (i = dwrite_count_a - 1; !fail && i >= 0; i--)
		if (dwrites_a[i] != dwrites_c[i]) {
			printf("dwrites[%i]: %06lx vs %06lx\n", dwrite_count_a, dwrites_a[i], dwrites_c[i]);
			fail = 1;
		}

	if (fail) leave();
}

#if 1
static void compare_ram(void)
{
	int i, fail = 0;
	for (i = 0; i < 0x800/4; i++)
	{
		if (((int *)nes_internal_ram)[i] != ((int32 *)RAM)[i]) {
			int u;
			fail = 1;
			for (u = i*4; u < i*4+4; u++)
				if (nes_internal_ram[u] != RAM[u])
					printf("RAM[%03x]: %02x vs %02x\n", u, nes_internal_ram[u], RAM[u]);
		}
	}

	if (fail) leave();
}
#endif

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
	int32 cycles = c << 4; /* *16 */
	if (PAL) cycles -= c;  /* *15 */

	//printf("-- %06i: run(%i)\n", (int)g_cnt, (int)c);
	g_cnt += cycles;

	if (c > 200)
		compare_ram();

	while (g_cnt > 0)
	{
		nes_registers[7]=1;
		X.count=1;

		dread_count_c = dread_count_a = dwrite_count_c = dwrite_count_a = 0;
		X6502_Run_c();

		X6502_Run_a();

		compare_state();
		g_cnt -= 1 - X.count;
		if (pending_add_cycles) {
			//X6502_AddCycles_c(pending_add_cycles);
			//X6502_AddCycles_a(pending_add_cycles);
			g_cnt -= pending_add_cycles*48;
			pending_add_cycles = 0;
		}
		if (pending_rebase) {
			X6502_Rebase_a();
			pending_rebase = 0;
		}
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
	if (nes_internal_ram == RAM) printf("nes_internal_ram == RAM!!\n");
	dread_count_c = dread_count_a = dwrite_count_c = dwrite_count_a = 0;

	X6502_Power_c();
	X6502_Power_a();
	compare_state();
}

void X6502_AddCycles_d(int x)
{
	printf("-- AddCycles(%i|%i)\n", x, x*48);

	pending_add_cycles = x; // *48;
//	printf("can't use this in debug\n");
//	exit(1);
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


void X6502_Rebase_d(void)
{
	pending_rebase = 1;
}


