int main(void);
int _start(void)
{
	return main();
}

void putchar(char ch)
{
	(void) ch;
	__asm
	ld	hl,#2
	add	hl,sp
	ld	a,(hl)
	rst	0x10
	__endasm;
}

void putstring(const char *ch)
{
	ch;
	__asm
	ld	hl,#2
	add	hl,sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
again:
	ld	a,(hl)
	or	a
	ret	z
	push	hl
	rst	0x10
	pop	hl
	inc	hl
	jr	again
	__endasm;
}

const char hex_str[] = "0123456789abcdef";

void puthex(unsigned char val)
{
	putchar(hex_str[val >> 4]);
	putchar(hex_str[val & 0xf]);
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

/* generic */

static void write_cmd(unsigned char count, unsigned char *cmd)
{
	while (count) {
		while (!(fdc_status() & 0x80))
			;
		fdc_write(*cmd);
		cmd++;
		count--;
	}
}

static void read_res(unsigned char count, unsigned char *res)
{
	unsigned char status;
	unsigned char data;

	while (1) {
		do {
			status = fdc_status();
			if (!(status & 0x10))
				return;
		} while (!(status & 0x80));


		data = fdc_read();
		if (count) {
			*res = data;
			res++;
			count--;
		}
	}
}

static void print_value(const char *what, unsigned char val)
{
	putstring(what);
	puthex(val);
	putchar('\r');
}

static void sense_drive_status(void)
{
	unsigned char cmd[2];
	unsigned char res[1];

	cmd[0] = 4;
	cmd[1] = 0;
	write_cmd(sizeof(cmd), cmd);
	read_res(sizeof(res), res);

	print_value("status: 0x", fdc_status());
	print_value("result: 0x", res[0]);
}


int main(void)
{
	putstring("uPD765 / i8272 FDC tester\r");

	fdc_motor_on();
	sense_drive_status();
	fdc_motor_off();
	return 0;
}
