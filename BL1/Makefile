
CONFIG_SYS_TEXT_BASE :=0x23E00000
COPY_BL2_SIZE :=0x50000

CFLAGS=-DCONFIG_SYS_TEXT_BASE=$(CONFIG_SYS_TEXT_BASE) \
		-DCOPY_BL2_SIZE=$(COPY_BL2_SIZE)

all:bl1.bin
	gcc mkv210_image.c -o mkv210
	./mkv210 bl1.bin tiny210v2-bl1.bin
	cat ./tiny210v2-bl1.bin ../u-boot.bin > ../tiny210v2-uboot.bin

bl1.bin: start.o nand_cp.o mmc_cp.o clock.o uart.o memory.o stdio.o printf.o
	@arm-linux-ld -Ttext 0xd0020010 -Tstdio.lds -o bl1.elf $^
	@arm-linux-objcopy -O binary bl1.elf bl1.bin
	@arm-linux-objdump -D bl1.elf > bl1_elf.dis
	
%.o : %.S
	@arm-linux-gcc $(CFLAGS) -o $@ $< -c -fno-builtin

%.o : %.c
	@arm-linux-gcc $(CFLAGS) -o $@ $< -c -fno-builtin

clean  distclean:
	@rm *.o *.elf *.dis mkv210 *.bin -f
