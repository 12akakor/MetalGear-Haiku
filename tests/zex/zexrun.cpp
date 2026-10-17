/*
 * Runs a CP/M .COM Z80 test (zexdoc / zexall) on the interpreter.
 * BDOS functions 2 and 9 are handled when PC reaches 0x0005.
 * Usage: zexrun file.com
 */
#include "../../src/z80/z80.h"

#include <stdio.h>
#include <string.h>


class Ram : public Z80Bus {
public:
	uint8_t mem[65536];
	uint8_t Read(uint16_t a) { return mem[a]; }
	void Write(uint16_t a, uint8_t v) { mem[a] = v; }
	uint8_t In(uint16_t) { return 0xff; }
	void Out(uint16_t, uint8_t) {}
};


int
main(int argc, char** argv)
{
	if (argc < 2)
		return 2;
	static Ram ram;
	memset(ram.mem, 0, sizeof(ram.mem));
	FILE* fp = fopen(argv[1], "rb");
	if (fp == NULL)
		return 2;
	size_t n = fread(ram.mem + 0x100, 1, 0xfe00, fp);
	fclose(fp);
	(void)n;
	ram.mem[0x0005] = 0xc9;		/* RET */
	ram.mem[0x0006] = 0x00;		/* BDOS top of memory (0xf000 below) */
	ram.mem[0x0007] = 0xf0;

	Z80 cpu(&ram);
	cpu.r.pc = 0x100;
	cpu.r.sp = 0xf000;
	unsigned long long cycles = 0;
	bool failed = false;
	char line[256];
	int pos = 0;
	for (;;) {
		if (cpu.r.pc == 0x0000)
			break;
		if (cpu.r.pc == 0x0005) {
			if (cpu.r.c == 2) {
				putchar(cpu.r.e);
				line[pos < 255 ? pos++ : pos] = (char)cpu.r.e;
			} else if (cpu.r.c == 9) {
				for (uint16_t a = cpu.r.de; ram.mem[a] != '$'; a++) {
					putchar(ram.mem[a]);
					char c = (char)ram.mem[a];
					if (c == '\n') {
						line[pos] = 0;
						if (strstr(line, "ERROR"))
							failed = true;
						pos = 0;
					} else if (pos < 255)
						line[pos++] = c;
				}
			}
			fflush(stdout);
		}
		cycles += cpu.Step();
	}
	line[pos] = 0;
	if (strstr(line, "ERROR"))
		failed = true;
	printf("\n%llu cycles\n", cycles);
	return failed ? 1 : 0;
}
