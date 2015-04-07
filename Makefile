CC	= sdcc
CFLAGS	= -mz80 --std-sdcc99 --Werror --opt-code-size
LDFLAGS	= -mz80 --out-fmt-ihx --no-std-crt0 --code-loc 0x8000 --data-loc 0xf000
PASMO	= pasmo
OBJCOPY	= objcopy


TARGET	= fdc_test

all:	$(TARGET).tap

%.tap: %_loader.asm %.bin
	$(PASMO) --tapbas $< $@

%.bin: %.hex
	$(OBJCOPY) -I ihex -O binary $< $@

%.hex: %.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $<

%.asm: %.c
	$(CC) $(CFLAGS) -S -o $@ $<

clean:
	rm -f $(TARGET).tap $(TARGET).o $(TARGET).hex $(TARGET).bin $(TARGET).rel $(TARGET).lst $(TARGET).lk $(TARGET).noi $(TARGET).map $(TARGET).sym
