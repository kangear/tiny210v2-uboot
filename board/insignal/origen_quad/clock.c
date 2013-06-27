/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Copyright (C) 2012 Insignal
 *
 * Clock setup for the Insignal OrigenQUAD board (Exynos4x12)
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
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include "setup.h"

void clock_ctrl_init(void)
{
	struct exynos4412_clock *clk = (struct exynos4412_clock *)EXYNOS4_CLOCK_BASE;
	unsigned int val;

	/* CLK_SRC_CPU */
	writel(0x00000000, &clk->src_cpu);
	sdelay(0x10000);

	/* CLK_DIV_DMC */
	writel(CLK_DIV_DMC0_VAL, &clk->div_dmc0);
	writel(CLK_DIV_DMC1_VAL, &clk->div_dmc1);

	/* CLK_SRC_TOP */
	writel(CLK_SRC_TOP0_VAL, &clk->src_top0);
	writel(CLK_SRC_TOP1_VAL, &clk->src_top1);
	sdelay(0x10000);
	writel(CLK_DIV_TOP_VAL, &clk->div_top);

	/* LEFTBUS */
	writel(CLK_SRC_LEFTBUS_VAL, &clk->src_leftbus);
	sdelay(0x10000);
	writel(CLK_DIV_LEFTBUS_VAL, &clk->div_leftbus);
	/* RIGHTBUS */
	writel(CLK_SRC_RIGHTBUS_VAL, &clk->src_rightbus);
	sdelay(0x10000);
	writel(CLK_DIV_RIGHTBUS_VAL, &clk->div_rightbus);

	/* PLL locktime */
	writel(APLL_LOCK_VAL, &clk->apll_lock);
	writel(MPLL_LOCK_VAL, &clk->mpll_lock);
	writel(EPLL_LOCK_VAL, &clk->epll_lock);
	writel(VPLL_LOCK_VAL, &clk->vpll_lock);

	/* CLK_DIV_CPU0/1 */
	writel(CLK_DIV_CPU0_VAL, &clk->div_cpu0);
	writel(CLK_DIV_CPU1_VAL, &clk->div_cpu1);

	/* APLL */
	writel(APLL_CON1_VAL, &clk->apll_con1);
	writel(APLL_CON0_VAL, &clk->apll_con0);
	/* MPLL */
	writel(MPLL_CON1_VAL, &clk->mpll_con1);
	writel(MPLL_CON0_VAL, &clk->mpll_con0);
	/* EPLL */
	writel(EPLL_CON2_VAL, &clk->epll_con2);
	writel(EPLL_CON1_VAL, &clk->epll_con1);
	writel(EPLL_CON0_VAL, &clk->epll_con0);
	/* VPLL */
	writel(VPLL_CON2_VAL, &clk->vpll_con2);
	writel(VPLL_CON1_VAL, &clk->vpll_con1);
	writel(VPLL_CON0_VAL, &clk->vpll_con0);
	sdelay(0x40000);

	writel(CLK_SRC_CPU_VAL, &clk->src_cpu);
	writel(CLK_SRC_DMC_VAL, &clk->src_dmc);
	writel(CLK_SRC_TOP0_VAL, &clk->src_top0);
	writel(CLK_SRC_TOP1_VAL, &clk->src_top1);
	sdelay(0x10000);

	/* UART */
	writel(CLK_SRC_PERIL0_VAL, &clk->src_peril0);
	writel(CLK_DIV_PERIL0_VAL, &clk->div_peril0);

	/* for IROM */
	/* CLK_DIV_FSYS2 */
	writel(0x0000000F, &clk->div_fsys2); /* MMC2 - SDMMC */
	/* CLK_DIV_FSYS3 */
	writel(0x00000100, &clk->div_fsys3); /* MMC4 - eMMC */

	/* FIMC */
	writel(CLK_SRC_CAM_VAL, &clk->src_cam0);
	writel(CLK_DIV_CAM_VAL, &clk->div_cam0);

	/* MFC */
	writel(CLK_SRC_MFC_VAL, &clk->src_mfc);
	writel(CLK_DIV_MFC_VAL, &clk->div_mfc);

	/* G3D */
	writel(CLK_SRC_G3D_VAL, &clk->src_g3d);
	writel(CLK_DIV_G3D_VAL, &clk->div_g3d);

	/* LCD0 */
	writel(CLK_SRC_LCD0_VAL, &clk->src_lcd0);
	writel(CLK_DIV_LCD0_VAL, &clk->div_lcd);
}

