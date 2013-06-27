/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Copyright (C) 2012 Insignal
 *
 * Memory setup for the Insignal OrigenQUAD board (Exynos4x12)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <asm/io.h>
#include <asm/arch/dmc.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include "setup.h"

void mem_ctrl_init(void)
{
	struct exynos4_clock *clk = (struct exynos4_clock *)EXYNOS4_CLOCK_BASE;
	struct exynos4_dmc *dmc0 = (struct exynos4_dmc *)(EXYNOS4412_DMC0_BASE);
	struct exynos4_dmc *dmc1 = (struct exynos4_dmc *)(EXYNOS4412_DMC1_BASE);

	/* Set DMC to 50MHz for IROM */
	writel(0x00117713, &clk->div_dmc0);

	/*
	 * DMC 0
	 */
	writel(0xE3855403, &dmc0->phyzqcontrol);
	writel(0x71101008, &dmc0->phycontrol0);
	writel(0x7110100A, &dmc0->phycontrol0);
	writel(0x00000084, &dmc0->phycontrol1);
	writel(0x71101008, &dmc0->phycontrol0);
	writel(0x0000008C, &dmc0->phycontrol1);
	writel(0x00000084, &dmc0->phycontrol1);
	writel(0x0000008C, &dmc0->phycontrol1);
	writel(0x00000084, &dmc0->phycontrol1);
	writel(0x0FFF30CA, &dmc0->concontrol);
	writel(0x00202500, &dmc0->memcontrol);
	writel(0x40C01323, &dmc0->memconfig0);
	writel(0x80000007, &dmc0->ivcontrol);

	writel(0x64000000, &dmc0->prechconfig);
	writel(0x9C4000FF, &dmc0->phycontrol0);
	writel(0x0000005D, &dmc0->timingref);

#if (CONFIG_CLK_MPLL == 330)
	writel(0x2b47654e, &dmc0->timingrow);
	writel(0x35330306, &dmc0->timingdata);
	writel(0x442f0365, &dmc0->timingpower);
#else
	writel(0x34498691, &dmc0->timingrow);
	writel(0x36330306, &dmc0->timingdata);
	writel(0x50380365, &dmc0->timingpower);
#endif
	sdelay(0x100000);

	writel(0x07000000, &dmc0->directcmd);
	sdelay(0x100000);

	writel(0x00071C00, &dmc0->directcmd);
	sdelay(0x100000);

	writel(0x00010BFC, &dmc0->directcmd);
	sdelay(0x100000);

	writel(0x00000608, &dmc0->directcmd);
	writel(0x00000810, &dmc0->directcmd);
	writel(0x00000C08, &dmc0->directcmd);

	/*
	 * DMC 1
	 */
	writel(0xE3855403, &dmc1->phyzqcontrol);
	writel(0x71101008, &dmc1->phycontrol0);
	writel(0x7110100A, &dmc1->phycontrol0);
	writel(0x00000084, &dmc1->phycontrol1);
	writel(0x71101008, &dmc1->phycontrol0);
	writel(0x0000008C, &dmc1->phycontrol1);
	writel(0x00000084, &dmc1->phycontrol1);
	writel(0x0000008C, &dmc1->phycontrol1);
	writel(0x00000084, &dmc1->phycontrol1);
	writel(0x0FFF30CA, &dmc1->concontrol);
	writel(0x00202500, &dmc1->memcontrol);
	writel(0x40C01323, &dmc1->memconfig0);
	writel(0x80000007, &dmc1->ivcontrol);
	writel(0x64000000, &dmc1->prechconfig);
	writel(0x9C4000FF, &dmc1->phycontrol0);
	writel(0x0000005D, &dmc1->timingref);

#if (CONFIG_CLK_MPLL == 330)
	writel(0x2b47654e, &dmc1->timingrow);
	writel(0x35330306, &dmc1->timingdata);
	writel(0x442f0365, &dmc1->timingpower);
#else
	writel(0x34498691, &dmc1->timingrow);
	writel(0x36330306, &dmc1->timingdata);
	writel(0x50380365, &dmc1->timingpower);
#endif
	sdelay(0x100000);

	writel(0x07000000, &dmc1->directcmd);
	sdelay(0x100000);

	writel(0x00071C00, &dmc1->directcmd);
	sdelay(0x100000);

	writel(0x00010BFC, &dmc1->directcmd);
	sdelay(0x100000);

	writel(0x00000608, &dmc1->directcmd);
	writel(0x00000810, &dmc1->directcmd);
	writel(0x00000C08, &dmc1->directcmd);
}

void mem_ctrl_init_2nd(void)
{
	struct exynos4_dmc *dmc0 = (struct exynos4_dmc *)(EXYNOS4_MIU_BASE);
	struct exynos4_dmc *dmc1 = (struct exynos4_dmc *)(EXYNOS4_MIU_BASE + 0x10000);

	writel(0x7F10100A, &dmc0->phycontrol0);
	writel(0xE0000084, &dmc0->phycontrol1);
	writel(0x7F10100B, &dmc0->phycontrol0);
	sdelay(20000);
	writel(0x0000008C, &dmc0->phycontrol1);
	writel(0x00000084, &dmc0->phycontrol1);
	sdelay(20000);

	writel(0x7F10100A, &dmc1->phycontrol0);
	writel(0xE0000084, &dmc1->phycontrol1);
	writel(0x7F10100B, &dmc1->phycontrol0);
	sdelay(20000);
	writel(0x0000008C, &dmc1->phycontrol1);
	writel(0x00000084, &dmc1->phycontrol1);
	sdelay(20000);

	writel(0x0FFF30FA, &dmc0->concontrol);
	writel(0x0FFF30FA, &dmc1->concontrol);
	writel(0x00202533, &dmc0->memcontrol);
	writel(0x00202533, &dmc1->memcontrol);
}
