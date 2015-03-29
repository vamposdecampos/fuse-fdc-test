CC	= zcc
CFLAGS	= -Wall -Werror
LDFLAGS	= +zx -lndos -create-app

TARGET	= fdc_test

all:	$(TARGET).tap

%.tap: %.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $<

%.asm: %.c
	$(CC) $(CFLAGS) -a -o $@ $<

clean:
	rm -f *.tap *.o zcc_opt.def
