/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Modified By JhoonKim (jhoon_kim@nate.com), aESOP Embedded Forum(http://www.aesop.or.kr)
 */

#include <common.h>
#include <s5pc110.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc.h>
#include <asm/arch/clk.h>
#include <asm/arch/clock.h>
/*Add by lk for DM9000 driver */
//#include <drivers/net/dm9000x.h>
#include <netdev.h>

/* ------------------------------------------------------------------------- */
#define SMC9115_Tacs	(0x0)	// 0clk		address set-up
#define SMC9115_Tcos	(0x4)	// 4clk		chip selection set-up
#define SMC9115_Tacc	(0xe)	// 14clk	access cycle
#define SMC9115_Tcoh	(0x1)	// 1clk		chip selection hold
#define SMC9115_Tah	(0x4)	// 4clk		address holding time
#define SMC9115_Tacp	(0x6)	// 6clk		page mode access cycle
#define SMC9115_PMC	(0x0)	// normal(1data)page mode configuration

#define SROM_DATA16_WIDTH(x)    (1<<((x*4)+0))
#define SROM_ADDR_MODE_16BIT(x) (1<<((x*4)+1))
#define SROM_WAIT_ENABLE(x)     (1<<((x*4)+2))
#define SROM_BYTE_ENABLE(x)     (1<<((x*4)+3))

/* ------------------------------------------------------------------------- */
#define DM9000_Tacs	(0x0)	// 0clk		address set-up
#define DM9000_Tcos	(0x4)	// 4clk		chip selection set-up
#define DM9000_Tacc	(0xE)	// 14clk	access cycle
#define DM9000_Tcoh	(0x1)	// 1clk		chip selection hold
#define DM9000_Tah	(0x4)	// 4clk		address holding time
#define DM9000_Tacp	(0x6)	// 6clk		page mode access cycle
#define DM9000_PMC	(0x0)	// normal(1data)page mode configuration

DECLARE_GLOBAL_DATA_PTR;

static struct s5pc110_gpio *s5pc110_gpio;

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n" "bne 1b":"=r" (loops):"0"(loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */
static void smc9115_pre_init(void)
{
        unsigned int tmp;
        unsigned char smc_bank_num=4;

        /* gpio configuration */

        tmp = MP01CON_REG;
        tmp &= ~(0xf << smc_bank_num*4);
        tmp |= (0x2 << smc_bank_num*4);
        MP01CON_REG = tmp;

	tmp = SROM_BW_REG;

	tmp &= ~(0xF<<(smc_bank_num * 4));
        tmp |= SROM_DATA16_WIDTH(smc_bank_num);
        tmp |= SROM_ADDR_MODE_16BIT(smc_bank_num);
//      tmp |= SROM_WAIT_ENABLE(smc_bank_num);

        SROM_BW_REG = tmp;
        if(smc_bank_num == 0)
		SROM_BC0_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|
				(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));
        else if(smc_bank_num == 1)
                SROM_BC1_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|
				(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));
        else if(smc_bank_num == 2)
                SROM_BC2_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|
				(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));
        else if(smc_bank_num == 3)
                SROM_BC3_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|
				(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));
        else if(smc_bank_num == 4)
                SROM_BC4_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|
				(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));
        else if(smc_bank_num == 5)
                SROM_BC5_REG = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|
				(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));

}

static void dm9000_pre_init(void)
{
unsigned int tmp;
/******** Modified by lk ************/
#if defined(DM9000_16BIT_DATA)
	//SROM_BW_REG &= ~(0xf << 20);
	//SROM_BW_REG |= (0<<23) | (0<<22) | (0<<21) | (1<<20);
			SROM_BW_REG &= ~(0xf << 4);
				SROM_BW_REG |= (0x1 << 4);
#else
	SROM_BW_REG &= ~(0xf << 20);
	SROM_BW_REG |= (0<<19) | (0<<18) | (0<<16);
#endif
	SROM_BC1_REG = ((0<<28)|(0<<24)|(5<<16)|(0<<12)|(0<<8)|(0<<4)|(0<<0));

	tmp = MP01CON_REG;
	tmp &=~(0xf<<4);								tmp |=(2<<4);
	MP01CON_REG = tmp;
}

static void pwm_pre_init(void)
{
        unsigned int tmp;

        /* Setup GPIO Controller : XpwmTOUT0,1,2
         * Set TOUT0 as output (beeper), low
         * set TOUT1 as output (LCD backlight), high
         */
        tmp = readl(GPD0CON);
        tmp &= ~((0xf << 12) | (0xf << 8) | (0xf << 4) | (0xf << 0));
        tmp |= (0x1 << 12) | (0x1 << 8) | (0x1 << 4) | (0x1 << 0);
        writel(tmp, GPD0CON);

        /* Enable Power */
        writel(((0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x0 << 0)), GPD0DAT);
	delay(100);

        printf("\nPWM Moudle Initialized.\n");
        printf("GPD0CON  : %x, GPD0DAT  : %x\n", readl(GPD0CON), readl(GPD0DAT));
}

int board_init(void)
{
	/* Set Initial global variables */
	s5pc110_gpio = (struct s5pc110_gpio *)S5PC110_GPIO_BASE;

	/***Modified by lk ***/

	//smc9115_pre_init();
        pwm_pre_init();

#ifdef CONFIG_DRIVER_DM9000
	dm9000_pre_init();
#endif

	gd->bd->bi_arch_number = CONFIG_MACH_TYPE;
	gd->bd->bi_boot_params = (PHYS_SDRAM_1+0x100);

	return 0;
}

int dram_init(void)
{
	/* Since we have discontinuous RAM configuration, just put
	 * bank1 here for relocation
	 */
        gd->ram_size    = get_ram_size((long *)PHYS_SDRAM_1, PHYS_SDRAM_1_SIZE);

	return 0;
}

void dram_init_banksize(void)
{
        gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
        gd->bd->bi_dram[0].size = get_ram_size((long *)PHYS_SDRAM_1, \
                                                       PHYS_SDRAM_1_SIZE);

//        gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
  //      gd->bd->bi_dram[1].size = get_ram_size((long *)PHYS_SDRAM_2, \
    //                                                    PHYS_SDRAM_2_SIZE);
}

#ifdef BOARD_LATE_INIT
#if defined(CONFIG_BOOT_NAND)
int board_late_init (void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);
	char boot_cmd[100];

	if ((0x24564236 == magic[0]) && (0x20764316 == magic[1])) {
		sprintf(boot_cmd, "nand erase 0 40000;nand write %08x 0 40000", PHYS_SDRAM_1 + 0x8000);
		magic[0] = 0;
		magic[1] = 0;
		printf("\nready for self-burning U-Boot image\n\n");
		setenv("bootdelay", "0");
		setenv("bootcmd", boot_cmd);
	}

	return 0;
}
#elif defined(CONFIG_BOOT_MOVINAND)
int board_late_init (void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);
	char boot_cmd[100];
	int hc;

	hc = (magic[2] & 0x1) ? 1 : 0;

	if ((0x24564236 == magic[0]) && (0x20764316 == magic[1])) {
		sprintf(boot_cmd, "movi init %d %d;movi write u-boot %08x", magic[3], hc, PHYS_SDRAM_1 + 0x8000);
		magic[0] = 0;
		magic[1] = 0;
		printf("\nready for self-burning U-Boot image\n\n");
		setenv("bootdelay", "0");
		setenv("bootcmd", boot_cmd);
	}

	return 0;
}
#else
int board_late_init (void)
{
	return 0;
}
#endif
#endif
/* Modified by lk for dm9000*/
int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_DRIVER_DM9000
	rc = dm9000_initialize(bis);
#endif
	return rc;
}
/***************************/


#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("\nBoard:   FriendlyLEG-TINY210\n");
	return (0);
}
#endif

#ifdef CONFIG_ENABLE_MMU

#ifdef CONFIG_MCP_SINGLE
ulong virt_to_phy_smdkc110(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x20000000);
	else
		printf("The input address don't need "\
			"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#else
ulong virt_to_phy_smdkc110(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x30000000);
	else if ((0x30000000 <= addr) && (addr < 0x50000000))
		return addr;
	else
		printf("The input address don't need "\
			"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#endif

#endif

#if defined(CONFIG_CMD_NAND) && defined(CFG_NAND_LEGACY)
#include <linux/mtd/nand.h>
extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];
void nand_init(void)
{
	nand_probe(CFG_NAND_BASE);
        if (nand_dev_desc[0].ChipID != NAND_ChipID_UNKNOWN) {
                print_size(nand_dev_desc[0].totlen, "\n");
        }
}
#endif

#ifdef CONFIG_GENERIC_MMC
#define MOUTMMC (50000000) /* 50MHz */
int board_mmc_init(bd_t *bis)
{
	int i;
	struct s5pc110_clock *clk =
		(struct s5pc110_clock *)samsung_get_base_clock();
	unsigned long clk_src, clk_div, mpll, div;

	/*
	 * MMC0 GPIO
	 * GPG0[0]      SD_0_CLK
	 * GPG0[1]      SD_0_CMD
	 * GPG0[2]      SD_0_CDn        -> Not used
	 * GPG0[3:6]    SD_0_DATA[0:3]
	 *
	 * MMC1 GPIO
	 * GPG1[0]      SD_1_CLK
	 * GPG1[1]      SD_1_CMD
	 * GPG1[2]      SD_1_CDn        -> Not used
	 * GPG1[3:6]    SD_1_DATA[0:3]
	 */
	for (i = 0; i < 7; i++) {
		if (i == 2)
			continue;
		/* GPG0[0:6] special function 2 */
		s5p_gpio_cfg_pin(&s5pc110_gpio->g0, i, 0x2);
		/* GPG0[0:6] pull disable */
		s5p_gpio_set_pull(&s5pc110_gpio->g0, i, GPIO_PULL_NONE);
		/* GPG0[0:6] drv 4x */
		s5p_gpio_set_drv(&s5pc110_gpio->g0, i, GPIO_DRV_4X);

		/* GPG1[0:6] special function 2 */
		s5p_gpio_cfg_pin(&s5pc110_gpio->g1, i, 0x2);
		/* GPG1[0:6] pull disable */
		s5p_gpio_set_pull(&s5pc110_gpio->g1, i, GPIO_PULL_NONE);
		/* GPG1[0:6] drv 4x */
		s5p_gpio_set_drv(&s5pc110_gpio->g1, i, GPIO_DRV_4X);
	}

	clk_src = readl(&clk->res9[0]); /* CLK_SRC4 */
	clk_src &= ~((0xf << 4) | 0xf);
	clk_src |= (0x6 << 4) | 0x6; /* Set MMC0/1_SEL to SCLK_MPLL */

	mpll = get_pll_clk(MPLL);
	div = ((mpll + MOUTMMC) / MOUTMMC) - 1;

	clk_div = readl(&clk->div4);
	clk_div &= ~((0xf << 4) | 0xf);
	clk_div |= (div << 4) | div;

	writel(clk_src, &clk->res9[0]);
	writel(clk_div, &clk->div4);

	return (s5p_mmc_init(0, 4) || s5p_mmc_init(1, 4));
}
#endif
