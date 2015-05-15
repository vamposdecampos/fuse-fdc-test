#define NULL ((void *) 0)
#define PRINT_AT	0x16

int main(void);

unsigned short saved_iy;

int _start(void)
{
	__asm
	di
	ld	bc, #l__INITIALIZER
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir

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

void puthex16(unsigned short val)
{
	puthex(val >> 8);
	puthex(val & 0xff);
}


#if defined(FLAVOR_plus3) || defined(FLAVOR_plus3tc)

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

#ifdef FLAVOR_plus3tc

static const char *flavor = "Spectrum +3 w/ TC hack";
static const int tc_support = 1;

/* this is hacked in fuse, there is no TC output on the real +3 */
static void fdc_tc(unsigned char tc)
{
	plus3_memory_port = 4 | 8 | (tc ? 0x80 : 0);
}

#else

const char *flavor = "Spectrum +3";
static const int tc_support = 0;

static void fdc_tc(unsigned char tc)
{
	(void) tc;
}

#endif

#endif


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
#define CMD_READ_ID		0x0a
#define CMD_FORMAT		0x0d
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
	union fdc_command cmd;

	cmd.specify.code.raw = CMD_SPECIFY;
	cmd.specify.hut = 0xf;
	cmd.specify.srt = 0xa;
	cmd.specify.nd = 1;
	cmd.specify.hlt = 1;

	write_cmd(sizeof(cmd.specify), cmd.raw);
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


static void test_read(void)
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
				k++;
				if (k == 3)
					fdc_tc(1);
				fdc_read();
			}
		}
	}
	fdc_tc(0);
	read_res(sizeof(res.rw), res.raw);

	putstring("bytes read: 0x");
	puthex16(k);
	putchar('\r');
	print_value("ST0=", res.rw.st0.raw);
	print_value("ST1=", res.rw.st1.raw);
	print_value("ST2=", res.rw.st2.raw);
	print_value("C=", res.rw.c);
	print_value("H=", res.rw.h);
	print_value("R=", res.rw.r);
	print_value("N=", res.rw.n);
}

static void test_format(void)
{
	union fdc_command cmd;
	union fdc_result res;
	long k;

	cmd.format.code.mt = 0;
	cmd.format.code.mfm = 1;
	cmd.format.code.sk = 0;
	cmd.format.code.command = CMD_FORMAT;
	cmd.format.sel.hds = 0;
	cmd.format.sel.ds = 0;
	cmd.format.n = 1;
	cmd.format.sc = 18;
	cmd.format.gpl = 0x2a;
	cmd.format.d = 0xe5;
	write_cmd(sizeof(cmd.format), cmd.raw);

	k = 0;
	for (;;) {
		unsigned char status = fdc_status();
		if (!(status & MAIN_EXEC))
			break;
		if (status & MAIN_RQM) {
			if (!(status & MAIN_DIN)) {
				k++;
				fdc_write(k);
			}
		}
	}
	read_res(sizeof(res.format), res.raw);

	putstring("bytes written: 0x");
	puthex16(k);
	putchar('\r');
	print_value("ST0=", res.format.st0.raw);
	print_value("ST1=", res.format.st1.raw);
	print_value("ST2=", res.format.st2.raw);
	print_value("C=", res.format.c);
	print_value("H=", res.format.h);
	print_value("R=", res.format.r);
	print_value("N=", res.format.n);
}

static const char *rw_res_names[] = {
	"ST0",
	"ST1",
	"ST2",
	"C",
	"H",
	"R",
	"N",
};

struct test {
	union fdc_command cmd;
	unsigned char cmd_len;
	union fdc_result res; /* expected result */
	unsigned char res_len;
	unsigned short data_len;
	char **res_names;
	unsigned short tc:1;
};

struct test tests[] = {
	/* tests using Terminal Count */
	{	/* 0 */
		.cmd.rw = {
			.code.raw	= CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.hds = 0,
			.c = 2, .h = 0, .r = 1, .n = 1,
		},
		.data_len = 42,
		.tc = 1,
		.res.rw = {
			.c = 2, .h = 0, .r = 2, .n = 1,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 1 */
		.cmd.rw = {
			.code.raw	= CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.hds = 0,
			.c = 2, .h = 0, .r = 18, .n = 1,
		},
		.data_len = 5,
		.tc = 1,
		.res.rw = {
			.c = 3, .h = 0, .r =  1, .n = 1,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 2 */
		.cmd.rw = {
			.code.raw	= CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.raw = 1 << 2,
			.c = 2, .h = 1, .r = 18, .n = 1,
		},
		.data_len = 5,
		.tc = 1,
		.res.rw = {
			.c = 3, .h = 1, .r =  1, .n = 1,
			.st0.raw = 1 << 2,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 3 */
		.cmd.rw = {
			.code.raw	= CMD_MT | CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.hds = 0,
			.c = 2, .h = 0, .r = 17, .n = 1,
		},
		.data_len = 2 * 256 + 1,
		.tc = 1,
		.res.rw = {
			.c = 2, .h = 1, .r = 2, .n = 1,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 4 */
		.cmd.rw = {
			.code.raw	= CMD_MT | CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.raw = 1 << 2,
			.c = 2, .h = 1, .r = 18, .n = 1,
		},
		.data_len = 6,
		.tc = 1,
		.res.rw = {
			.c = 3, .h = 0, .r = 1, .n = 1,
			.st0.raw = 1 << 2,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},

	/* tests without Terminal Count (thus longer transfers) */
	{	/* 5 */
		.cmd.rw = {
			.code.raw	= CMD_MF | CMD_READ_DATA,
			.eot		= 4,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.hds = 0,
			.c = 2, .h = 0, .r = 3, .n = 1,
		},
		.data_len = 2 * 256,
		.tc = 0,
		.res.rw = {
			.c = 2, .h = 0, .r = 4, .n = 1,
			.st0.raw = 0x40,
			.st1.raw = 0x80,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 6 */
		.cmd.rw = {
			.code.raw	= CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.hds = 0,
			.c = 2, .h = 0, .r =  1, .n = 1,
		},
		.data_len = 18 * 256,
		.tc = 0,
		.res.rw = {
			.c = 2, .h = 0, .r = 18, .n = 1, /* ? */
			.st0.raw = 0x40,
			.st1.raw = 0x80,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 7 */
		.cmd.rw = {
			.code.raw	= CMD_MF | CMD_READ_DATA,
			.eot		= 2,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.raw = 0,
			.c = 2, .h = 0, .r =  1, .n = 1,
		},
		.data_len = 2 * 256,
		.tc = 0,
		.res.rw = {
			.c = 2, .h = 0, .r =  2, .n = 1,
			.st0.raw = 0x40,
			.st1.raw = 0x80,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 8 */
		.cmd.rw = {
			.code.raw	= CMD_MT | CMD_MF | CMD_READ_DATA,
			.eot		= 2,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.raw = 1 << 2,
			.c = 2, .h = 1, .r =  1, .n = 1,
		},
		.data_len = 2 * 256,
		.tc = 0,
		.res.rw = {
			.c = 3, .h = 0, .r =  1, .n = 1,
			.st0.raw = 1 << 2,
			/* CHECK: apparently EOT causes Normal Termination, even without TC? */
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* 9 */
		.cmd.rw = {
			.code.raw	= CMD_MT | CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.hds = 0,
			.c = 2, .h = 0, .r =  1, .n = 1,
		},
		.data_len = 2 * 18 * 256,
		.tc = 0,
		.res.rw = {
			.c = 3, .h = 0, .r = 1, .n = 1,
			.st0.raw = 0x40,
			.st1.raw = 0x80,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
	{	/* a */
		.cmd.rw = {
			.code.raw	= CMD_MT | CMD_MF | CMD_READ_DATA,
			.eot		= 18,
			.gpl		= 0x2a,
			.dtl		= 0xff,
			.sel.raw = 1 << 2,
			.c = 2, .h = 1, .r =  1, .n = 1,
		},
		.data_len = 18 * 256,
		.tc = 0,
		.res.rw = {
			.c = 3, .h = 0, .r = 1, .n = 1,
			.st0.raw = 1 << 2,
		},
		.cmd_len = sizeof(struct fdc_cmd_rw),
		.res_len = sizeof(struct fdc_res_rw),
		.res_names = rw_res_names,
	},
};
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))


static void assert_fail_eq(const char *what, unsigned short expected, unsigned short actual)
{
	putchar('\r');
	putstring(what);
	putstring(" FAIL ");
	puthex16(expected);
	putstring(" != ");
	puthex16(actual);
	putchar('\r');
}
#define ASSERT_EQ(what, expected, actual) do { \
	if ((expected) != (actual)) { \
		assert_fail_eq(what, expected, actual); \
		goto fail; \
	} \
} while (0)

static void run_one_test(struct test *test)
{
	union fdc_result res;
	unsigned short k;

	/* TODO: set drive */
	write_cmd(test->cmd_len, test->cmd.raw);

	k = 0;
	for (;;) {
		unsigned char status = fdc_status();
		if (!(status & MAIN_EXEC))
			break;
		if (status & MAIN_RQM) {
			if (status & MAIN_DIN) {
				k++;
				if (test->tc && k == test->data_len)
					fdc_tc(1);
				fdc_read();
			}
		}
	}
	fdc_tc(0);
	read_res(sizeof(res.rw), res.raw);

	ASSERT_EQ("bytes_read", test->data_len, k);
	for (k = 0; k < test->res_len; k++)
		ASSERT_EQ(test->res_names[k], test->res.raw[k], res.raw[k]);
fail:
}

static void run_test(void)
{
	unsigned char n;
	struct test *test;
	unsigned short sec_len;

	for (test = tests, n = 0; n < ARRAY_SIZE(tests); test++, n++) {
		if (!tc_support && test->tc)
			continue;
		putchar('t');
		puthex(n);
		putchar(' ');
		run_one_test(test);
	}
	for (test = tests, n = 0; n < ARRAY_SIZE(tests); test++, n++) {
		if (!test->tc || !tc_support)
			continue;
		/* try again with sector-boundary transfer size */
		sec_len = 128 << test->cmd.rw.n;
		test->data_len = (test->data_len + sec_len - 1) & ~(sec_len - 1);
		putchar('l');
		puthex(n);
		putchar(' ');
		run_one_test(test);
	}
}

int main(void)
{
	unsigned char k;
	unsigned char online = 0;

	putstring("uPD765 / i8272 FDC tester\r");
	putstring(flavor);
	putchar('\r');

	fdc_motor_on();

	reset_fdc();
	specify();

	for (k = 0; k < 255; k++) {
		fdc_st3_t st3;

		if (!(k & 3)) {
			if (online)
				break;
			putchar(PRINT_AT);
			putchar(4);
			putchar(0);
		}

		st3.raw = sense_drive_status(k);
		print_value("drive_status: 0x", st3.raw);
		if (st3.rdy)
			online = st3.raw;
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
	putstring("\rdone.");
	return 0;
}
