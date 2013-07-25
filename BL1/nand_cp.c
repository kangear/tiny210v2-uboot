/*
 * $Id: nand_cp.c,v 1.1 2013/6/6 22:46:46 $
 *
 * (C) kangear
 *
 * 这个程序适用于tiny210v2（NandFlash：K9GAG08U0F）
 * 这个Flash的页大小为8k+512大小；是MLC类型的！
 *
 * 由于是MLC类型的，所以很容易出错，我就在这里边加了ECC 16bit校验。这个校验只
 * 实现了读校验，且还是读与开发板配套的Superboot写到NandFlash中的校验值。由于
 * 到目前并没有发现Superboot对oob区的校验，所以在读的时候，也没有办法进行oob区
 * 校验。
 *
 * 程序完善了oob区的校验会更完整，目前没有做的原因上边已经说明。
 * 由于没有时行oob区进行校验，在读取Main区的校验码时就会有出错的可能，错误的校验
 * 会造成错误的修正，不过根据规律已经解决了除第一位就是错误校验码引起修正外的其它
 * 错误判断。关于如果512Byte中唯一一位就是错误的校验码引起的伪翻转只能通过屏蔽典型
 * Byte的不进行校验来进行减少出现误判断。对于这些问题，如果能进行oob区的校验，都会
 * 迎刃而解了。(由于oob区校验没有办法做实现，所以NF_ReadOob只是一个框架)
 *
 * 2013-6-9添加了nandll_read_n_byte，改善了读的速度。256k只需要3秒钟就可以
 * copy完
 * 2013-6-25进行的优化，去掉无关的注释
 */

#include "stdio.h"
/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0		0x00
#define NAND_CMD_READ1		0x30
#define NAND_CMD_RNDOUT0	0x05
#define NAND_CMD_RNDOUT1	0xE0	
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_RNDIN		0x85
#define NAND_CMD_READID		0x90
#define NAND_CMD_PARAM		0xec
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_RESET		0xff

/* Extended commands for large page devices */
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_RNDOUTSTART	0xE0
#define NAND_CMD_CACHEDPROG	0x15

#define NAND_DISABLE_CE()	(NFCONT_REG |= (1 << 1))
#define NAND_ENABLE_CE()	(NFCONT_REG &= ~(1 << 1))
#define NF_TRANSRnB()		do { while(!(NFSTAT_REG & (1 << 0))); } while(0)


typedef unsigned char		uchar;
typedef volatile unsigned long	vu_long;
typedef volatile unsigned short vu_short;
typedef volatile unsigned char	vu_char;

typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;



#define __REG(x)	(*(vu_long *)(x))
#define __REGl(x)	(*(vu_long *)(x))
#define __REGw(x)	(*(vu_short *)(x))
#define __REGb(x)	(*(vu_char *)(x))
#define __REG2(x,y)	(*(vu_long *)((x) + (y)))


#include "s5pv210.h"
//#define	COPY_BL2_SIZE		882046
//#define CONFIG_SYS_TEXT_BASE 0x22000000
#define DEBUG
#ifdef  DEBUG  
#define debug(fmt,args...)  printf (fmt ,##args)  
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);  
#else  
#define debug(fmt,args...)  
#define debugX(level,fmt,args...) 
#define puthex(args) 
#define put32bits(args)
#endif  /* DEBUG */  

#define NAND_CONTROL_ENABLE()	(NFCONT_REG |= (1 << 0))

/*
 * address format
 *              17 16         9 8            0
 * --------------------------------------------
 * | block(12bit) | page(5bit) | offset(9bit) |
 * --------------------------------------------
 */

#define NFECCCONF_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONF_OFFSET)
#define NFECCCONT_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONT_OFFSET)
#define NFECCSTAT_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCSTAT_OFFSET)
#define NFECCSECSTAT_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCSECSTAT_OFFSET)
#define NFECCCONECC0_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC0_OFFSET)
#define NFECCCONECC1_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC1_OFFSET)
#define NFECCCONECC2_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC2_OFFSET)
#define NFECCCONECC3_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC3_OFFSET)
#define NFECCCONECC4_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC4_OFFSET)
#define NFECCCONECC5_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC5_OFFSET)
#define NFECCCONECC6_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCCONECC6_OFFSET)

#define NFECCERL0_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL0_OFFSET)
#define NFECCERL1_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL1_OFFSET)
#define NFECCERL2_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL2_OFFSET)
#define NFECCERL3_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL3_OFFSET)
#define NFECCERL4_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL4_OFFSET)
#define NFECCERL5_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL5_OFFSET)
#define NFECCERL6_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL6_OFFSET)
#define NFECCERL7_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERL7_OFFSET)
#define NFECCERP0_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERP0_OFFSET)
#define NFECCERP1_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERP1_OFFSET)
#define NFECCERP2_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERP2_OFFSET)
#define NFECCERP3_REG			__REG(ELFIN_NAND_ECC_BASE+NFECCERP3_OFFSET)

#define NFSTAT_ECCDECDONE (1<<24)

#define NF_RSTECC() {NFECCCONT_REG |= (1<<2);}
#define NF_MECC_UnLock() {NFCONT_REG &= ~(1<<7);}
#define NF_MECC_Lock() {NFCONT_REG |= (1<<7);}
#define NF_SECC_UnLock() {NFCONT_REG &= ~(1<<6);}
#define NF_SECC_Lock() {NFCONT_REG |= (1<<6);}

static void nandll_read_n_byte (uchar* buf, ulong addr, ulong add, ulong size)
{
	int i = 0;
	int j = 0;
	
	NAND_ENABLE_CE();	
	
	NFCMD_REG = NAND_CMD_READ0;
	

	/* Write 5 Address */
	NFADDR_REG = 0;  //col:A0 ~ A7
	NFADDR_REG = 0;  //col:A8 ~ A11
	
	NFADDR_REG = (addr) & 0xff;
	NFADDR_REG = (addr >> 8) & 0xff;
	NFADDR_REG = (addr >> 16) & 0xff;

	NFCMD_REG = NAND_CMD_READ1;

	NF_TRANSRnB();

	/* for compatibility(2460). u32 cannot be used. by scsuh */
	for(i=0; i < size; i++) 
	{
		NFCMD_REG = NAND_CMD_RNDOUT0;
	
		NFADDR_REG = (char)((add+i) & 0xff);       // col:A0~A7
		NFADDR_REG = (char)(((add+i) >> 8) & 0x3f);  // col:A8~A11
		
		NFCMD_REG = NAND_CMD_RNDOUT1;
		*buf++ = NFDATA8_REG; 
		for (j=0; j<1; j++);            
	}
	
	NAND_DISABLE_CE();
	
}


static void nandll_read_16bit_ecc (uchar* buf, ulong addr, uchar num)
{
	int base = 8192+36;
	int ecc_size = 26;
	int off = base + num*28;
	nandll_read_n_byte(buf, addr, off, ecc_size);	
}

static void nandll_read_512byte (uchar *buf, ulong addr, uchar num)
{
	int base = 0;
	int size = 512;
	int off = base + num*size;
	nandll_read_n_byte(buf, addr, off, size);

}

/*
 * 判断一个数组前n项是否为递增
 */
int isInc(uint* a, int len)
{
	if( len > 0 )
	{
		if( len == 1 )
		{
			return 1;
		}
		else
		{
			return  (a[len-2] <= a[len-1]) && isInc(a, len - 1);
		}
	}
	else
	{
	    return -1;
	}
}

/*
 * 修复Main区的反转位
 */
int fixEcc(uchar* buf, int num, int flag)
{
	uint subst[16];
	uchar pattern[16];
	int i = 0;

	// 数组赋值为0
	for(i=0; i<16; i++)
	{
		subst[i]=pattern[i]=0;
	}
	{
	subst[0] = (NFECCERL0_REG>>0) & 0x1ff;
	pattern[0] = (NFECCERP0_REG>>0) & 0xff;

	subst[1] = (NFECCERL0_REG>>16) & 0x1ff;
	pattern[1] = (NFECCERP0_REG>>8) & 0xff;

	subst[2] = (NFECCERL1_REG>>0) & 0x1ff;
	pattern[2] = (NFECCERP0_REG>>16) & 0xff;

	subst[3] = (NFECCERL1_REG>>16) & 0x1ff;
	pattern[3] = (NFECCERP0_REG>>24) & 0xff;

	subst[4] = (NFECCERL2_REG>>0) & 0x1ff;
	pattern[4] = (NFECCERP1_REG>>0) & 0xff;

	subst[5] = (NFECCERL2_REG>>16) & 0x1ff;
	pattern[5] = (NFECCERP1_REG>>8) & 0xff;

	subst[6] = (NFECCERL3_REG>>0) & 0x1ff;
	pattern[6] = (NFECCERP1_REG>>16) & 0xff;

	subst[7] = (NFECCERL3_REG>>16) & 0x1ff;
	pattern[7] = (NFECCERP1_REG>>24) & 0xff;

	subst[8] = (NFECCERL4_REG>>0) & 0x1ff;
	pattern[8] = (NFECCERP2_REG>>0) & 0xff;

	subst[9] = (NFECCERL4_REG>>16) & 0x1ff;
	pattern[9] = (NFECCERP2_REG>>8) & 0xff;

	subst[10] = (NFECCERL5_REG>>0) & 0x1ff;
	pattern[10] = (NFECCERP2_REG>>16) & 0xff;

	subst[11] = (NFECCERL5_REG>>16) & 0x1ff;
	pattern[11] = (NFECCERP2_REG>>24) & 0xff;

	subst[12] = (NFECCERL6_REG>>0) & 0x1ff;
	pattern[12] = (NFECCERP3_REG>>0) & 0xff;

	subst[13] = (NFECCERL6_REG>>16) & 0x1ff;
	pattern[13] = (NFECCERP3_REG>>8) & 0xff;

	subst[14] = (NFECCERL7_REG>>0) & 0x1ff;
	pattern[14] = (NFECCERP3_REG>>16) & 0xff;

	subst[15] = (NFECCERL7_REG>>16) & 0x1ff;
	pattern[15] = (NFECCERP3_REG>>24) & 0xff;

	}
	// 解决最后一个误判断
	if(!isInc(subst, num))
		num --;  //如果不是递增说明最后是一个是误判断，所以减少一次修正。

	for(i=0; i<num; i++)
		buf[subst[i]] ^= pattern[i];

	return 0;

}

/*
 * 读512Byte并进行ECC校验
 */
uchar nand_read_512_ecc(uchar *buf, unsigned int addr, uchar num)
{
	uint ret = 0;
	uchar ecctemp[26];
	uchar ECCErrorNo = 0;
	int error_flag = 0;
	
	// 0.初始化16bit_ECC
	NFECCCONF_REG = (511<<16)|(5<<0);

	// 1.复位ECC
	NF_RSTECC();
	//NFCONT_REG |= (1<<5);
	
	// 2.解锁Main ECC
	NF_MECC_UnLock();
	
	// 3.读521Bbyte	 
	nandll_read_512byte(buf, addr, num);
	
	// 4.读26byte的ECC校验码
	nandll_read_16bit_ecc(ecctemp, addr, num);
	
	// 5.等待校验完毕
	while (!(NFECCSTAT_REG & NFSTAT_ECCDECDONE)) {};

	// 6.判断校验结果并解决错误
	ECCErrorNo = NFECCSECSTAT_REG&0x1F;
	//puthex(ECCErrorNo);putc(' ');
	
	if( 0x0 == ECCErrorNo )
	{
#if 0
		// 没有错误
		puthex(0x11);putc(' ');
		putc('\n');putc('\r');
#endif	
			
	}
	else if( ECCErrorNo > 16)
	{
		putc('E');putc('R');putc('R');putc('O');putc('R');putc('!');
		putc('\n');putc('\r');
		putc('>');putc('1');putc('6');putc('b');putc('i');putc('t');
		putc('\n');putc('\r');
		
		while(1);
	}
	else
	{	
		//根据ECC校验码校正反转位	
		fixEcc(buf, ECCErrorNo, error_flag);	
	}

	return ret;
			
}
/*
 * 以16bit ECC校验的方式读一页内容
 */
static void nandll_read_page_ecc (uchar *buf, unsigned int addr, int large_block)
{
	int i = 0;
	int page_size = 8192;	
	int num = page_size/512;
	for (i=0; i<num;i++,buf+=512)
	{
		nand_read_512_ecc(buf, addr, i);
	}
}

/*
 * 读一页内容(无ECC校验)
 */
static int nandll_read_page (uchar *buf, ulong addr, int large_block)
{
	int i;
	int page_size = 512;
	if (1 == large_block)
		page_size = 2048;
		
	else if (2 == large_block)
		page_size = 4096;
	
	else if (3 == large_block)
		page_size = 8192;

	NAND_ENABLE_CE();

	NFCMD_REG = NAND_CMD_READ0;

	/* Write Address */
	NFADDR_REG = 0;

	if (large_block)
		NFADDR_REG = 0;

	NFADDR_REG = (addr) & 0xff;
	NFADDR_REG = (addr >> 8) & 0xff;
	NFADDR_REG = (addr >> 16) & 0xff;

	if (large_block)
		NFCMD_REG = NAND_CMD_READSTART;

	NF_TRANSRnB();

	/* for compatibility(2460). u32 cannot be used. by scsuh */
	for(i=0; i < page_size; i++) 
	{
		*buf++  = NFDATA8_REG;              
	}

	NAND_DISABLE_CE();
	return 0;
}
/*
 * Read data from NAND.
 */
static int nandll_read_blocks (ulong dst_addr, ulong size, int large_block)
{
	uchar *buf = (uchar *)dst_addr;
	int i;
	uint page_shift = 9;

	if (1 == large_block)
	{
		page_shift = 11;
	
		/* Read pages */
		for (i = (0x6000>>page_shift); i < (size>>page_shift); i++, buf+=(1<<page_shift)) 
		{
			nandll_read_page(buf, i, large_block);
		}
	}
	else if(3 == large_block)
	{
		page_shift = 13;
		for (i = 4; i < (4 + (size>>page_shift)); i++, buf+=(1<<(page_shift))) 
		{
			// 不进行ECC校验拷贝
			//nandll_read_page(buf, i, large_block);
			
			// 进行ECC校验拷贝
			nandll_read_page_ecc(buf, i, large_block);			
		}
	}
        
	return 0;
}

int copy_uboot_to_ram_nand (void)
{
	int large_block = 0;
	int i;
	vu_char tmp_id;
	vu_long id = 0;

	NAND_CONTROL_ENABLE();
	NAND_ENABLE_CE();
	NFCMD_REG = NAND_CMD_READID;
	NFADDR_REG =  0x00;

	/* wait for a while */
	for (i=0; i<200; i++);
	for (i=0; i<4; i++)
	{
		tmp_id = NFDATA8_REG;
		id |= tmp_id << (3-i)*8;
	}

	if (tmp_id > 0x80)
		large_block = 1;
	
	printf("NAND: 2GB(MLC2) ID:%x\n\r", id);
	
	if(id == 0xECD59476)
		large_block = 3;

	/* read NAND Block.
	 * 128KB ->240KB because of U-Boot size increase. by scsuh
	 * So, read 0x3c000 bytes not 0x20000(128KB).
	 */
	return nandll_read_blocks(CONFIG_SYS_TEXT_BASE, COPY_BL2_SIZE, large_block);
}
void cleanup_before_uboot(void)
{
	//disable_interrupts();
	//icache_disable();
	//dcache_disable();
	/*flush I/D-cache*/
	//cache_flush();	
}

void board_init_f_nand(unsigned long bootflag)
{
	__attribute__((noreturn)) void (*uboot)(void);
	
	printf("\n\rBL1 Ver:1307020\n\r");
	printf("Start cp \n\r");
	
	copy_uboot_to_ram_nand();
	//跳转之前要进行cleanup_before_uboot
	cleanup_before_uboot();
	/* Jump to U-Boot image */
	uboot = (void *)CONFIG_SYS_TEXT_BASE;
	
	printf("Running! \n\r");

	(*uboot)();
	/* Never returns Here */
}

// BL1 main()
void main(void) __attribute__((weak, alias("__main")));
void __main(void) {};
