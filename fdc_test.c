#include <stdio.h>

/* Spectrum +3 */

static void __FASTCALL__ fdc_write(unsigned char val)
{
	#asm
	ld a, l
	ld bc, 0x3ffd
	out (c), a
	#endasm
}

static unsigned char __FASTCALL__ fdc_read(void)
{
	#asm
	xor a
	ld h, a
	ld bc, 0x3ffd
	in a, (c)
	ld l, a
	#endasm
}

static unsigned char __FASTCALL__ fdc_status(void)
{
	#asm
	xor a
	ld h, a
	ld bc, 0x2ffd
	in a, (c)
	ld l, a
	#endasm
}

static void __FASTCALL__ fdc_motor_on(void)
{
	#asm
	ld bc, 0x1ffd
	ld a, 0x0c
	out (c), a
	#endasm
}

static void __FASTCALL__ fdc_motor_off(void)
{
	#asm
	ld bc, 0x1ffd
	ld a, 0x04
	out (c), a
	#endasm
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
	printf("%cuPD765 / i8272 FDC tester\n\n",12);

	fdc_motor_on();
	sense_drive_status();
	fdc_motor_off();
	return 0;
}
