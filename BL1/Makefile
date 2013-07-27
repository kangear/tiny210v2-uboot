
CONFIG_SYS_TEXT_BASE :=0x23E00000
COPY_BL2_SIZE :=0x50000

CFLAGS=-DCONFIG_SYS_TEXT_BASE=$(CONFIG_SYS_TEXT_BASE) \
		-DCOPY_BL2_SIZE=$(COPY_BL2_SIZE)

obj    := $(OBJTREE)/BL1/

COBJS-y  += start.o
COBJS-y  += nand_cp.o
COBJS-y  += mmc_cp.o
COBJS-y  += clock.o
COBJS-y  += uart.o
COBJS-y  += memory.o
COBJS-y  += stdio.o
COBJS-y  += printf.o

OBJS	:= $(addprefix $(obj),$(COBJS-y))

all: $(obj)bl1.bin
	gcc mkv210_image.c -o $(obj)mkv210
	$(obj)mkv210 $(obj)bl1.bin $(obj)tiny210v2-bl1.bin
	cat $(obj)tiny210v2-bl1.bin $(OBJTREE)/u-boot.bin > $(OBJTREE)/tiny210v2-uboot.bin

$(obj)bl1.bin: permake $(OBJS)

	cd $(obj) && arm-linux-ld -Ttext 0xd0020010 -Tstdio.lds -o $(obj)bl1.elf $(COBJS-y)
	@arm-linux-objcopy -O binary $(obj)bl1.elf $(obj)bl1.bin
	@arm-linux-objdump -D $(obj)bl1.elf > $(obj)bl1_elf.dis
	
$(obj)%.o : %.S
	@arm-linux-gcc $(CFLAGS) -o $@ $< -c -fno-builtin

$(obj)%.o : %.c
	@arm-linux-gcc $(CFLAGS) -o $@ $< -c -fno-builtin

clean  distclean:
	@rm *.o *.elf *.dis mkv210 *.bin -f

permake:

ifneq ($(obj), $(wildcard $(obj)))
	mkdir $(obj)
endif

ifneq ($(obj)stdio.lds , $(wildcard $(obj)stdio.lds))
	cp stdio.lds $(obj)
endif

