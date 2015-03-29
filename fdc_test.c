#include <stdio.h>

int main(void);
int _start(void)
{
	return main();
}

void putchar(char ch)
{
	ch;
	__asm
	ld	hl,#2
	add	hl,sp
	ld	a,(hl)
	rst	0x10
	__endasm;
}

/* Spectrum +3 */

__sfr __banked __at(0x3ffd) fdc_data_port;
__sfr __banked __at(0x2ffd) fdc_status_port;
__sfr __banked __at(0x1ffd) plus3_memory_port;

static void fdc_write(unsigned char val)
{
	fdc_data_port = val;
}

static unsigned char fdc_read(void)
{
	return fdc_data_port;
}

static unsigned char fdc_status(void)
{
	return fdc_status_port;
}

static void fdc_motor_on(void)
{
	plus3_memory_port = 4 | 8;
}

static void fdc_motor_off(void)
{
	plus3_memory_port = 4;
}


static void sense_drive_status(void)
{
	fdc_write(4);
	fdc_write(0);
	printf("status: 0x%02x\n", fdc_status());
	printf("result: 0x%02x\n", fdc_read());
}


int main(void)
{
	printf("uPD765 / i8272 FDC tester\r");

	fdc_motor_on();
	sense_drive_status();
	fdc_motor_off();
	return 0;
}
