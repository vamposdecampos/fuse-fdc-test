#define NULL ((void *) 0)
#define PRINT_AT	0x16

int main(void);

unsigned short saved_iy;

int _start(void)
{
	__asm
	di
	ld	(_saved_iy), iy
	__endasm;

	return main();
}

void putchar(char ch)
{
	(void) ch;
	__asm
	ld	iy, (_saved_iy)
	ld	hl,#2
	add	hl,sp
	ld	a,(hl)
	rst	0x10
	ld	(_saved_iy), iy
	__endasm;
}

void putstring(const char *ch)
{
	ch;
	__asm
	ld	iy, (_saved_iy)
	ld	hl,#2
	add	hl,sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
again:
	ld	a,(hl)
	or	a
	jr	z, done
	push	hl
	rst	0x10
	pop	hl
	inc	hl
	jr	again
done:
	ld	(_saved_iy), iy
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

#define MAIN_BUSY		0x10
#define MAIN_EXEC		0x20
#define MAIN_DIN		0x40 /* CPU reads data */
#define MAIN_RQM		0x80

#define ST2_READY		0x20

/* commands */
#define CMD_INVALID		0
#define CMD_SPECIFY		3
#define CMD_SENSE_DRIVE		4
#define CMD_READ_DATA		6
#define CMD_RECALIBRATE		7
#define CMD_SENSE_INT		8
/* command flags */
#define CMD_MT			0x80
#define CMD_MF			0x40
#define CMD_SK			0x20

static void write_cmd(unsigned char count, unsigned char *cmd)
{
	while (count) {
		while (!(fdc_status() & MAIN_RQM))
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
			if (!(status & MAIN_BUSY))
				return;
		} while (!(status & MAIN_RQM));


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

static unsigned char sense_drive_status(unsigned char unit)
{
	unsigned char cmd[2];
	unsigned char res[1];

	cmd[0] = CMD_SENSE_DRIVE;
	cmd[1] = unit & 3;
	write_cmd(sizeof(cmd), cmd);
	read_res(sizeof(res), res);
	return res[0];
}

static void specify(void)
{
	unsigned char cmd[3];

	cmd[0] = CMD_SPECIFY;
	cmd[1] = 0xaf;
	cmd[2] = 3;
	write_cmd(sizeof(cmd), cmd);
	read_res(0, NULL);
}

/* well, sort of. */
static void reset_fdc(void)
{
	fdc_write(CMD_INVALID);
	while (fdc_status() & MAIN_BUSY)
		fdc_read();
}

static void run_test(void)
{
	unsigned char cmd[9];
	unsigned char res[7];
	long k;

	cmd[0] = CMD_READ_DATA | CMD_MF;
	cmd[1] = 0;
	cmd[2] = 0;	/* C */
	cmd[3] = 0;	/* H */
	cmd[4] = 1;	/* R */
	cmd[5] = 2;	/* N */
	cmd[6] = 18;	/* EOT */
	cmd[7] = 0x2a;	/* GPL */
	cmd[8] = 0xff;	/* DTL */
	write_cmd(sizeof(cmd), cmd);

	k = 0;
	for (;;) {
		unsigned char status = fdc_status();
		if (!(status & MAIN_EXEC))
			break;
		if (status & MAIN_RQM) {
			if (status & MAIN_DIN) {
				fdc_read();
				k++;
			}
		}
	}
	read_res(sizeof(res), res);
	putstring("bytes read: 0x");
	puthex(k >> 8);
	puthex(k & 0xff);
	putchar('\r');
}

int main(void)
{
	unsigned char k;
	unsigned char online = 0;

	putstring("uPD765 / i8272 FDC tester\r");

	fdc_motor_on();

	reset_fdc();
	specify();

	for (k = 0; k < 255; k++) {
		unsigned char st2;

		if (!(k & 3)) {
			if (online)
				break;
			putchar(PRINT_AT);
			putchar(5);
			putchar(0);
		}

		st2 = sense_drive_status(k);
		print_value("drive_status: 0x", st2);
		if (st2 & ST2_READY)
			online = st2;
	}
	if (!online) {
		putstring("no drives online\r");
	} else {
		print_value("drive online after: ", k);
		run_test();
	}
	fdc_motor_off();
	return 0;
}
