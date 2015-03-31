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

typedef union fdc_cmd_code {
	struct {
		unsigned char command:5;
		unsigned char sk:1;
		unsigned char mfm:1;
		unsigned char mt:1;
	};
	unsigned char raw;
} fdc_cmd_code_t;

typedef union fdc_cmd_head_sel {
	struct {
		unsigned char ds:2;	/* Drive Select */
		unsigned char hds:1;	/* Head Select */
		unsigned char _unused:5;
	};
	unsigned char raw;
} fdc_cmd_head_sel_t;

struct fdc_cmd_header {
	fdc_cmd_code_t code;
	fdc_cmd_head_sel_t sel;
};

struct fdc_cmd_rw {
	fdc_cmd_code_t code;
	fdc_cmd_head_sel_t sel;
	unsigned char c, h, r, n;
	unsigned char eot, gpl, dtl;
};

struct fdc_cmd_format {
	fdc_cmd_code_t code;
	fdc_cmd_head_sel_t sel;
	unsigned char n, sc, gpl, d;
};

struct fdc_cmd_specify {
	fdc_cmd_code_t code;
	unsigned char hut:4;	/* Head Unload Time */
	unsigned char srt:4;	/* Seek Rate Time */
	unsigned char nd:1;	/* Non-DMA */
	unsigned char hlt:7;	/* Head Load Time */
};

struct fdc_cmd_seek {
	fdc_cmd_code_t code;
	fdc_cmd_head_sel_t sel;
	unsigned char ncn;
};

union fdc_command {
	struct fdc_cmd_header hdr;
	struct fdc_cmd_header read_id;
	struct fdc_cmd_header recalibrate;
	struct fdc_cmd_header sense_drive;
	struct fdc_cmd_rw rw;
	struct fdc_cmd_format format;
	fdc_cmd_code_t sense_interrupt;
	struct fdc_cmd_specify specify;
	struct fdc_cmd_seek seek;
	unsigned char raw[0];
};


#define ST0_IC_NT	0	/* Normal Termination */
#define ST0_IC_AT	1	/* Abnormal Termination */
#define ST0_IC_IC	2	/* Invalid Command */
#define ST0_IC_ATRDY	3	/* Abnormal Termination due to Ready state change */

typedef union fdc_st0 {
	struct {
		unsigned char us:2;	/* 03 Unit Select */
		unsigned char hd:1;	/* 04 Head */
		unsigned char nr:1;	/* 08 Not Ready */
		unsigned char ec:1;	/* 10 Equipment Check */
		unsigned char se:1;	/* 20 Seek End */
		unsigned char ic:2;	/* c0 Interupt Code */
	};
	unsigned char raw;
} fdc_st0_t;

typedef union fdc_st1 {
	struct {
		unsigned char ma:1;	/* 01 Missing Address mark */
		unsigned char nw:1;	/* 02 Not Writeable */
		unsigned char nd:1;	/* 04 No Data */
		unsigned char _unused1:1;
		unsigned char or:1;	/* 10 OverRun */
		unsigned char de:1;	/* 20 Data Error */
		unsigned char _unused2:1;
		unsigned char en:1;	/* 80 End of cylinder */
	};
	unsigned char raw;
} fdc_st1_t;

typedef union fdc_st2 {
	struct {
		unsigned char md:1;	/* 01 Missing address mark in Data field */
		unsigned char bc:1;	/* 02 Bad Cylinder */
		unsigned char sn:1;	/* 04 Scan Not satisfied */
		unsigned char sh:1;	/* 08 Scan equal Hit */
		unsigned char wc:1;	/* 10 Wrong Cylinder */
		unsigned char dd:1;	/* 20 Data error in Data field */
		unsigned char cm:1;	/* 40 Control Mark */
		unsigned char _unused:1;
	};
	unsigned char raw;
} fdc_st2_t;

typedef union fdc_st3 {
	struct {
		unsigned char us:2;	/* 03 Unit Select */
		unsigned char hd:1;	/* 04 Head address */
		unsigned char ts:1;	/* 08 Two Side */
		unsigned char t0:1;	/* 10 Track 0 */
		unsigned char rdy:1;	/* 20 Ready */
		unsigned char wp:1;	/* 40 Write Protected */
		unsigned char ft:1;	/* 80 Fault */
	};
	unsigned char raw;
} fdc_st3_t;

struct fdc_res_rw {
	fdc_st0_t st0;
	fdc_st1_t st1;
	fdc_st2_t st2;
	unsigned char c, h, r, n;
};

struct fdc_res_sense_drive {
	fdc_st3_t st3;
};

struct fdc_res_sense_interrupt {
	fdc_st0_t st0;
	unsigned char pcn;
};

union fdc_result {
	struct fdc_res_rw rw;
	struct fdc_res_rw read_id;
	struct fdc_res_rw format;
	struct fdc_res_sense_drive sense_drive;
	struct fdc_res_sense_interrupt sense_interrupt;
	unsigned char raw[0];
};

#define MAIN_DRIVES_BUSY	0x0f /* any drive */
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
#define CMD_SEEK		0x0f
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

static unsigned char sense_int_status(void)
{
	unsigned char cmd[1];
	unsigned char res[2];

	cmd[0] = CMD_SENSE_INT;
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

static void recalibrate(void)
{
	unsigned char cmd[2];

	cmd[0] = CMD_RECALIBRATE;
	cmd[1] = 0;
	write_cmd(sizeof(cmd), cmd);

	while (fdc_status() & MAIN_DRIVES_BUSY)
		;
}

static void seek(unsigned char cyl)
{
	unsigned char cmd[3];

	cmd[0] = CMD_SEEK;
	cmd[1] = 0;
	cmd[2] = cyl;
	write_cmd(sizeof(cmd), cmd);

	while (fdc_status() & MAIN_DRIVES_BUSY)
		;
}


static void run_test(void)
{
	union fdc_command cmd;
	union fdc_result res;
	long k;

	cmd.rw.code.mt = 1;
	cmd.rw.code.mfm = 1;
	cmd.rw.code.sk = 0;
	cmd.rw.code.command = CMD_READ_DATA;
	cmd.rw.sel.hds = 0;
	cmd.rw.sel.ds = 0;
	cmd.rw.c = 2;
	cmd.rw.h = 0;
	cmd.rw.r = 1;
	cmd.rw.n = 1;
	cmd.rw.eot = 18;
	cmd.rw.gpl = 0x2a;
	cmd.rw.dtl = 0xff;
	write_cmd(sizeof(cmd.rw), cmd.raw);

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
	read_res(sizeof(res.rw), res.raw);

	putstring("bytes read: 0x");
	puthex(k >> 8);
	puthex(k & 0xff);
	putchar('\r');
	print_value("ST0=", res.rw.st0.raw);
	print_value("ST1=", res.rw.st1.raw);
	print_value("ST2=", res.rw.st2.raw);
	print_value("C=", res.rw.c);
	print_value("H=", res.rw.h);
	print_value("R=", res.rw.r);
	print_value("N=", res.rw.n);
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
		recalibrate();
		print_value("ST0=", sense_int_status());
		seek(2);
		print_value("ST0=", sense_int_status());
		run_test();
	}
	fdc_motor_off();
	return 0;
}
