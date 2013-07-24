@******************************************************************************
@ File：start.S
@ 功能：启动代码，设置栈，拷贝代码到CONFIG_SYS_TEXT_BASE处
@******************************************************************************  
#include "s5pv210.h"
.text
.global _start
_start:
	
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr
	bic	r0, r0, #0x1f
	orr	r0, r0, #0xd3
	msr	cpsr,r0
	
	/*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			@ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate BP array
	mcr p15, 0, r0, c7, c10, 4	@ DSB
	mcr p15, 0, r0, c7, c5, 4	@ ISB
	
	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 11 (Z---) BTB
	bic	r0, r0, #0x00001000	@ clear bit 12 (I) I-cache
	mcr	p15, 0, r0, c1, c0, 0
	

	ldr	sp, =0xD0037D80       @ 设置栈，以便调用c函数						
/*	
	adr r0, _start  		  @ 重定位	
                              @ _start当前所位于的地址
	ldr r1, =_start           @ _start的链接地址:CONFIG_SYS_TEXT_BASE 	
	cmp r0, r1
	beq run_on_dram           @ 如果在是链接地址处，则直接运行
*/
	bl  system_clock_init	
	bl  uart_init             @ 初始化串口 
	bl  nand_asm_init         @ NandFlash初始化 
	bl  mem_init              @ dram初始化
	
//判断启动方式
#define PRO_ID_BASE			0xE0000000
#define PRO_ID_OFFSET			0x00
#define OMR_OFFSET			0x04

#define BOOT_ONENAND		0x1
#define BOOT_NAND		0x2
#define BOOT_MMCSD		0x3
#define BOOT_NOR		0x4
#define BOOT_SEC_DEV		0x5

#define INF_REG_BASE			0xE010F000
#define INF_REG3_OFFSET			0x0c

	ldr	r0, =PRO_ID_BASE
	ldr	r1, [r0,#OMR_OFFSET]
	bic	r2, r1, #0xffffffc1

	/* NAND BOOT */
	cmp	r2, #0x0		@ 512B 4-cycle
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x2		@ 2KB 5-cycle
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x4		@ 4KB 5-cycle	8-bit ECC
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x6		@ 4KB 5-cycle	16-bit ECC
	moveq	r3, #BOOT_NAND

	cmp	r2, #0x8		@ OneNAND Mux
	moveq	r3, #BOOT_ONENAND

	/* SD/MMC BOOT */
	cmp     r2, #0xc
	moveq   r3, #BOOT_MMCSD	

	/* NOR BOOT */
	cmp     r2, #0x14
	moveq   r3, #BOOT_NOR	

	/* Uart BOOTONG failed */
	cmp     r2, #(0x1<<4)
	moveq   r3, #BOOT_SEC_DEV

	ldr	r0, =INF_REG_BASE
	str	r3, [r0, #INF_REG3_OFFSET]
	
	ldr	r1, [r0, #INF_REG3_OFFSET]
	cmp	r1, #BOOT_NAND		/* 0x0 => boot device is nand */
	beq	nand_boot_210
	cmp     r1, #BOOT_MMCSD
	beq     mmcsd_boot_210
	
nand_boot_210:
	bl  board_init_f_nand     @ 拷贝bl2代码到dram中，并运行

mmcsd_boot_210:
	bl  board_init_f_mmc

run_on_dram:			 
	bl  main                  @ 跳转		

halt:
	b halt


led_debug:     
	ldr     R0,=0xE0200280      @ R0设为GPJ2CON寄存器。此寄存器
                              @ 用于选择端口J各引脚的功能：
                              @ 是输出、是输入、还是其他
	mov     R1,#0x00000001        
	str     R1,[R0]             @ 设置GPJ2_0为输出口, 位[10:9]=0b01
	ldr     R0,=0xE0200284      @ R0设为GPJ2DAT寄存器。此寄存器
                              @ 用于读/写端口GPJ2各引脚的数据
	mov     R1,#0x00000000      @ 此值改为0x00000001,
                              @ 可让LED1熄灭
	str     R1,[R0]             @ GPJ2_0输出0，LED1点亮
	mov pc, lr



	/* Setting GPIO for NAND */
	/* This setting is NAND initialze code at booting time in iROM. */
	
nand_asm_init:
	
/*
 * Nand Interface Init for SMDKC110
 */
 
#define ELFIN_GPIO_BASE			0xE0200000 
#define ELFIN_NAND_BASE			0xB0E00000
#define NFCONF_VAL	    (2<<23)|(7<<12)|(7<<8)|(7<<4)|(1<<3)|(0<<2)|(1<<1)|(0<<0)
#define NFCONT_VAL      (0x1<<23)|(0x1<<22)|(0<<18)|(0<<17)|(0<<16)|(0<<10)|(0<<9)|(0<<8)|(0<<7)|(0<<6)|(0x2<<1)|(1<<0)

//#define  NFCONF_VAL     (0<<25)|(0x3<<23)|(7<<12)|(7<<8)|(7<<4)|(1<<3)|(0<<2)|(1<<1)|(0<<0)
//#define NFCONT_VAL      (0x1<<23)|(0x1<<22)|(0<<18)|(0<<17)|(0<<16)|(0<<10)|(0<<9)|(0<<8)|(0<<7)|(0<<6)|(1<<5)|(0x2<<1)|(1<<0)

#define MP01CON_OFFSET 			0x2E0
#define MP01PUD_OFFSET 			0x2E8
#define MP03CON_OFFSET      0x320
#define MP03PUD_OFFSET      0x328
#define NFCONF_OFFSET       0x00
#define NFCONT_OFFSET       0x04
 

	ldr	r0, =ELFIN_GPIO_BASE

	ldr	r1, [r0, #MP01CON_OFFSET]
	bic	r1, r1, #(0xf<<8)
	orr	r1, r1, #(0x3<<8)
	str	r1, [r0, #MP01CON_OFFSET]

	ldr	r1, [r0, #MP01PUD_OFFSET]
	bic	r1, r1, #(0x3<<4)
	str	r1, [r0, #MP01PUD_OFFSET]

	ldr	r1, [r0, #MP03CON_OFFSET]
	bic	r1, r1, #0xFFFFFF
	ldr	r2, =0x22222222
	orr	r1, r1, r2
	str	r1, [r0, #MP03CON_OFFSET]

	ldr	r1, [r0, #MP03PUD_OFFSET]
	ldr	r2, =0x3fff
	bic	r1, r1, r2
	str	r1, [r0, #MP03PUD_OFFSET]

	ldr	r0, =ELFIN_NAND_BASE

	ldr	r1, [r0, #NFCONF_OFFSET]
	ldr	r2, =0x777F
	bic	r1, r1, r2
	ldr	r2, =NFCONF_VAL
	orr	r1, r1, r2
	str	r1, [r0, #NFCONF_OFFSET]

	ldr	r1, [r0, #NFCONT_OFFSET]
	ldr	r2, =0x707C7
	bic	r1, r1, r2
	ldr	r2, =NFCONT_VAL
	orr	r1, r1, r2
	str	r1, [r0, #NFCONT_OFFSET]

	ldr	r1, [r0, #NFCONF_OFFSET]
	orr	r1, r1, #0x70
	orr	r1, r1, #0x7700
	str     r1, [r0, #NFCONF_OFFSET]

	ldr	r1, [r0, #NFCONT_OFFSET]
	orr	r1, r1, #0x03
	str     r1, [r0, #NFCONT_OFFSET]

	mov	pc, lr
	
/*
 * system_clock_init: Initialize core clock and bus clock.
 * void system_clock_init(void)
 */
system_clock_init:

#define CONFIG_MCP_SINGLE	1
#define CONFIG_EVT1		1		/* EVT1 */

#define APLL_LOCKTIME_VAL	0x2cf

#define APLL_MDIV       0x7d
#define APLL_PDIV       0x3
#define APLL_SDIV       0x1

#if defined(CONFIG_EVT1)
/* Set AFC value */
#define AFC_ON		0x00000000
#define AFC_OFF		0x10000010
#endif

#define MPLL_MDIV	0x29b
#define MPLL_PDIV	0xc
#define MPLL_SDIV	0x1

#define EPLL_MDIV	0x60
#define EPLL_PDIV	0x6
#define EPLL_SDIV	0x2

#define VPLL_MDIV	0x6c
#define VPLL_PDIV	0x6
#define VPLL_SDIV	0x3

/* CLK_DIV0 */
#define APLL_RATIO	0
#define A2M_RATIO	4
#define HCLK_MSYS_RATIO	8
#define PCLK_MSYS_RATIO	12
#define HCLK_DSYS_RATIO	16
#define PCLK_DSYS_RATIO 20
#define HCLK_PSYS_RATIO	24
#define PCLK_PSYS_RATIO 28

#define CLK_DIV0_MASK	0x7fffffff

#define set_pll(mdiv, pdiv, sdiv)	(1<<31 | mdiv<<16 | pdiv<<8 | sdiv)

#define APLL_VAL	set_pll(APLL_MDIV,APLL_PDIV,APLL_SDIV)
#define MPLL_VAL	set_pll(MPLL_MDIV,MPLL_PDIV,MPLL_SDIV)
#define EPLL_VAL	set_pll(EPLL_MDIV,EPLL_PDIV,EPLL_SDIV)
#define VPLL_VAL	set_pll(VPLL_MDIV,VPLL_PDIV,VPLL_SDIV)
#define CLK_DIV0_VAL    ((0<<APLL_RATIO)|(4<<A2M_RATIO)|(4<<HCLK_MSYS_RATIO)|(1<<PCLK_MSYS_RATIO)\
			|(3<<HCLK_DSYS_RATIO)|(1<<PCLK_DSYS_RATIO)|(4<<HCLK_PSYS_RATIO)|(1<<PCLK_PSYS_RATIO))

	ldr	r0, =ELFIN_CLOCK_POWER_BASE	@0xe0100000

	/* Set Mux to FIN */
	ldr	r1, =0x0
	str	r1, [r0, #CLK_SRC0_OFFSET]

	ldr	r1,	=APLL_LOCKTIME_VAL
	str	r1,	[r0, #APLL_LOCK_OFFSET]

	/* Disable PLL */
	ldr	r1, =0x0
	str	r1, [r0, #APLL_CON0_OFFSET]
	ldr	r1, =0x0
	str	r1, [r0, #MPLL_CON_OFFSET]

	ldr	r1, =0x0
	str	r1, [r0, #MPLL_CON_OFFSET]

	ldr   	r1, [r0, #CLK_DIV0_OFFSET]
	ldr	r2, =CLK_DIV0_MASK
	bic	r1, r1, r2

	ldr	r2, =CLK_DIV0_VAL
	orr	r1, r1, r2
	str	r1, [r0, #CLK_DIV0_OFFSET]

	ldr	r1, =APLL_VAL
	str	r1, [r0, #APLL_CON0_OFFSET]

	ldr	r1, =MPLL_VAL
	str	r1, [r0, #MPLL_CON_OFFSET]

	ldr	r1, =VPLL_VAL
	str	r1, [r0, #VPLL_CON_OFFSET]
#if defined(CONFIG_EVT1)
	ldr	r1, =AFC_ON
	str	r1, [r0, #APLL_CON1_OFFSET]
#endif
	mov	r1, #0x10000
1:	subs	r1, r1, #1
	bne	1b

	ldr	r1, [r0, #CLK_SRC0_OFFSET]
	ldr	r2, =0x10001111
	orr	r1, r1, r2
	str	r1, [r0, #CLK_SRC0_OFFSET]

#if defined(CONFIG_MCP_AC)

	/* CLK_SRC6[25:24] -> OneDRAM clock sel = MPLL */
	ldr	r1, [r0, #CLK_SRC6_OFFSET]
	bic	r1, r1, #(0x3<<24)
	orr	r1, r1, #0x01000000
	str	r1, [r0, #CLK_SRC6_OFFSET]

	/* CLK_DIV6[31:28] -> 4=1/5, 3=1/4(166MHZ@667MHz), 2=1/3 */
	ldr	r1, [r0, #CLK_DIV6_OFFSET]
	bic	r1, r1, #(0xF<<28)
	bic	r1, r1, #(0x7<<12)	@; ONENAND_RATIO: 0
	orr	r1, r1, #0x30000000
	str	r1, [r0, #CLK_DIV6_OFFSET]

#elif defined (CONFIG_MCP_H)

	/* CLK_SRC6[25:24] -> OneDRAM clock sel = 00:SCLKA2M, 01:SCLKMPLL */
	ldr	r1, [r0, #CLK_SRC6_OFFSET]
	bic	r1, r1, #(0x3<<24)
	orr	r1, r1, #0x00000000
	str	r1, [r0, #CLK_SRC6_OFFSET]

	/* CLK_DIV6[31:28] -> 4=1/5, 3=1/4(166MHZ@667MHz), 2=1/3 */
	ldr	r1, [r0, #CLK_DIV6_OFFSET]
	bic	r1, r1, #(0xF<<28)
	bic	r1, r1, #(0x7<<12)	@; ONENAND_RATIO: 0
	orr	r1, r1, #0x00000000
	str	r1, [r0, #CLK_DIV6_OFFSET]

#elif defined (CONFIG_MCP_B) || defined (CONFIG_MCP_D)

	/* CLK_SRC6[25:24] -> OneDRAM clock sel = 00:SCLKA2M, 01:SCLKMPLL */
	ldr	r1, [r0, #CLK_SRC6_OFFSET]
	bic	r1, r1, #(0x3<<24)
	orr	r1, r1, #0x01000000
	str	r1, [r0, #CLK_SRC6_OFFSET]

	/* CLK_DIV6[31:28] -> 4=1/5, 3=1/4(166MHZ@667MHz), 2=1/3 */
	ldr	r1, [r0, #CLK_DIV6_OFFSET]
	bic	r1, r1, #(0xF<<28)
	bic	r1, r1, #(0x7<<12)	@; ONENAND_RATIO: 0
	orr	r1, r1, #0x30000000
	str	r1, [r0, #CLK_DIV6_OFFSET]

#elif defined (CONFIG_MCP_SINGLE)

	/* CLK_DIV6 */
	ldr	r1, [r0, #CLK_DIV6_OFFSET]
	bic	r1, r1, #(0x7<<12)	@; ONENAND_RATIO: 0
	str	r1, [r0, #CLK_DIV6_OFFSET]

#endif

	mov	pc, lr

