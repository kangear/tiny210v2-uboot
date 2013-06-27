/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Copyright (C) 2012 Insignal
 *
 * Configuration settings for the Insignal Origen QUAD board (Exynos4412)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __SETUP_ORIGEN_QUAD_H
#define __SETUP_ORIGEN_QUAD_H

#include <config.h>
#include <version.h>
#include <asm/arch/cpu.h>

/* Set PLL */
#define set_pll(mdiv, pdiv, sdiv)	(1<<31 | mdiv<<16 | pdiv<<8 | sdiv)

/* APLL */
#define	set_clk_div_cpu0(core2, apll, pclk_dbg, atb, periph, corem1, corem0, core) \
					((core2 << 28) \
					|(apll << 24) \
					|(pclk_dbg << 20) \
					|(atb << 16) \
					|(periph <<12) \
					|(corem1 << 8) \
					|(corem0 << 4) \
					|(core))
#define set_clk_div_cpu1(cores, hpm, copy) \
					((cores << 8) \
					|(hpm << 4) \
					|(copy))

#if	(CONFIG_CLK_APLL == 800)
#define APLL_CON0_VAL			set_pll(0x64, 0x3, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 1, 1, 3, 7, 5, 2, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(3, 0, 3)
#elif	(CONFIG_CLK_APLL == 1000)
#define APLL_CON0_VAL			set_pll(0x7D, 0x3, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 1, 1, 4, 7, 5, 2, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(3, 0, 4)
#elif	(CONFIG_CLK_APLL == 1100)
#define APLL_CON0_VAL			set_pll(0x113, 0x6, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 2, 1, 4, 7, 6, 3, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(4, 0, 4)
#elif	(CONFIG_CLK_APLL == 1200)
#define APLL_CON0_VAL			set_pll(0x96, 0x3, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 2, 1, 5, 7, 7, 3, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(4, 0, 4)
#elif	(CONFIG_CLK_APLL == 1300)
#define APLL_CON0_VAL			set_pll(0x145, 0x6, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 2, 1, 5, 7, 7, 3, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(5, 0, 5)
#elif	(CONFIG_CLK_APLL == 1400)
#define APLL_CON0_VAL			set_pll(0xAF, 0x3, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 2, 1, 6, 7, 7, 3, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(5, 0, 6)
#elif	(CONFIG_CLK_APLL == 1500)
#define APLL_CON0_VAL			set_pll(0xFA, 0x3, 0x0)
#define CLK_DIV_CPU0_VAL		set_clk_div_cpu0(0, 2, 1, 6, 7, 7, 4, 0)
#define CLK_DIV_CPU1_VAL		set_clk_div_cpu1(5, 0, 6)
#else
#error	Not supported APLL freq
#endif

#define APLL_CON1_VAL			(0x00803800)
#define APLL_LOCK_VAL			(((APLL_CON0_VAL >> 8) & 0x3F) * 270)

/* EPLL */
#define EPLL_CON0_VAL			set_pll(0x40, 0x2, 0x3)
#define EPLL_CON1_VAL			(0x66010000)
#define EPLL_CON2_VAL			(0x00000080)
#define EPLL_LOCK_VAL			(((EPLL_CON0_VAL >> 8) & 0x3F) * 3000)

/* MPLL */
#if	(CONFIG_CLK_MPLL == 200)
#define MPLL_CON0_VAL			set_pll(0x64, 0x3, 0x1)
#elif	(CONFIG_CLK_MPLL == 330)
#define MPLL_CON0_VAL			set_pll(0x116, 0x5, 0x1)
#elif	(CONFIG_CLK_MPLL == 400)
#define MPLL_CON0_VAL			set_pll(0x64, 0x3, 0x0)
#else
#error	Not supported MPLL freq
#endif
#define MPLL_CON1_VAL			(0x00803800)
#define MPLL_LOCK_VAL			(((MPLL_CON0_VAL >> 8) & 0x3F) * 270)

/* VPLL */
#define VPLL_CON0_VAL			set_pll(0x48, 0x2, 0x3)
#define VPLL_CON1_VAL			(0x66010000)
#define VPLL_CON2_VAL			(0x00000080)
#define VPLL_LOCK_VAL			(((EPLL_CON0_VAL >> 8) & 0x3F) * 3000)

/* CLK_SRC_CPU */
#define MUX_MPLL_USER_SEL		1
#define MUX_HPM_SEL			0
#define MUX_CORE_SEL			0
#define MUX_APLL_SEL			1
#define CLK_SRC_CPU_VAL			((MUX_MPLL_USER_SEL << 24) \
					|(MUX_HPM_SEL << 20) \
					|(MUX_CORE_SEL << 16) \
					|(MUX_APLL_SEL))

/* CLK_SRC_TOP0	*/
#define MUX_ONENAND_SEL			0x0 /* 0 = DOUT133, 1 = DOUT166 */
#define MUX_ACLK_133_SEL		0x0 /* 0 = SCLKMPLL, 1 = SCLKAPLL	*/
#define MUX_ACLK_160_SEL		0x0
#define MUX_ACLK_100_SEL		0x0
#define MUX_ACLK_200_SEL		0x0
#define MUX_VPLL_SEL			0x1
#define MUX_EPLL_SEL			0x1
#define CLK_SRC_TOP0_VAL		((MUX_ONENAND_SEL << 28) \
					|(MUX_ACLK_133_SEL << 24) \
					|(MUX_ACLK_160_SEL << 20) \
					|(MUX_ACLK_100_SEL << 16) \
					|(MUX_ACLK_200_SEL << 12) \
					|(MUX_VPLL_SEL << 8) \
					|(MUX_EPLL_SEL << 4))

/* CLK_SRC_TOP1	*/
#define VPLLSRC_SEL			0x0 /* 0 = FINPLL, 1 = SCLKHDMI27M */
#define CLK_SRC_TOP1_VAL		(0x01111000)

/* CLK_DIV_TOP	*/
#define ACLK_400_MCUISP_RATIO		0x1
#define ACLK_266_GPS_RATIO		0x2
#define ONENAND_RATIO			0x1
#define ACLK_133_RATIO			0x5
#define ACLK_160_RATIO			0x4
#define ACLK_100_RATIO			0x7
#define ACLK_200_RATIO			0x4
#define CLK_DIV_TOP_VAL			((ACLK_400_MCUISP_RATIO << 24) \
					|(ACLK_266_GPS_RATIO << 20) \
					|(ONENAND_RATIO << 16) \
					|(ACLK_133_RATIO << 12) \
					|(ACLK_160_RATIO << 8) \
					|(ACLK_100_RATIO << 4) \
					|(ACLK_200_RATIO))

/* CLK_SRC_LEFTBUS */
#define CLK_SRC_LEFTBUS_VAL		(0x10)

/* CLK_DIV_LEFRBUS */
#define GPL_RATIO			0x1
#define GDL_RATIO			0x3
#define CLK_DIV_LEFTBUS_VAL		((GPL_RATIO << 4)|(GDL_RATIO))

/* CLK_SRC_RIGHTBUS */
#define CLK_SRC_RIGHTBUS_VAL		(0x10)

/* CLK_DIV_RIGHTBUS */
#define GPR_RATIO			0x1
#define GDR_RATIO			0x3
#define CLK_DIV_RIGHTBUS_VAL		((GPR_RATIO << 4)|(GDR_RATIO))

/* CLK_SRC_DMC	*/
#define MUX_PWI_SEL			0x1
#define MUX_CORE_TIMERS_SEL		0x1
#define MUX_DPHY_SEL			0x0
#define MUX_DMC_BUS_SEL			0x0
#define CLK_SRC_DMC_VAL			((MUX_PWI_SEL << 16) \
					|(MUX_CORE_TIMERS_SEL << 12) \
					|(MUX_DPHY_SEL << 8) \
					|(MUX_DMC_BUS_SEL << 4))

/* CLK_DIV_DMC0	*/
#define CORE_TIMERS_RATIO		0x0
#define COPY2_RATIO			0x0
#define DMCP_RATIO			0x1
#define DMCD_RATIO			0x1
#if	(CONFIG_CLK_MPLL == 200)
#define DMC_RATIO			0x3
#else
#define DMC_RATIO			0x1
#endif
#define DPHY_RATIO			0x1
#define ACP_PCLK_RATIO			0x1
#define ACP_RATIO			0x3

#define CLK_DIV_DMC0_VAL		((CORE_TIMERS_RATIO << 28) \
					|(COPY2_RATIO << 24) \
					|(DMCP_RATIO << 20) \
					|(DMCD_RATIO << 16) \
					|(DMC_RATIO << 12) \
					|(DPHY_RATIO << 8) \
					|(ACP_PCLK_RATIO << 4) \
					|(ACP_RATIO))

#define CLK_DIV_DMC1_VAL		(0x07071713)

/* CLK_SRC_PERIL0 */
#define UART4_SEL			1
#define UART3_SEL			1
#define UART2_SEL			1
#define UART1_SEL			1
#define UART0_SEL			1
#define CLK_SRC_PERIL0_VAL		((UART4_SEL << 16) \
					|(UART3_SEL << 12) \
					|(UART2_SEL<< 8) \
					|(UART1_SEL << 4) \
					|(UART0_SEL))

/* CLK_DIV_PERIL0	*/
#define UART4_RATIO			3
#define UART3_RATIO			3
#define UART2_RATIO			3
#define UART1_RATIO			3
#define UART0_RATIO			3
#define CLK_DIV_PERIL0_VAL		((UART4_RATIO << 16) \
					|(UART3_RATIO << 12) \
					|(UART2_RATIO << 8) \
					|(UART1_RATIO << 4) \
					|(UART0_RATIO))

/* Clock Source CAM/FIMC */
/* CLK_SRC_CAM */
#define CAM0_SEL_XUSBXTI	1
#define CAM1_SEL_XUSBXTI	1
#define CSIS0_SEL_XUSBXTI	1
#define CSIS1_SEL_XUSBXTI	1

#define FIMC_SEL_SCLKMPLL	6
#define FIMC0_LCLK_SEL		FIMC_SEL_SCLKMPLL
#define FIMC1_LCLK_SEL		FIMC_SEL_SCLKMPLL
#define FIMC2_LCLK_SEL		FIMC_SEL_SCLKMPLL
#define FIMC3_LCLK_SEL		FIMC_SEL_SCLKMPLL

#define CLK_SRC_CAM_VAL		((CSIS1_SEL_XUSBXTI << 28) \
				| (CSIS0_SEL_XUSBXTI << 24) \
				| (CAM1_SEL_XUSBXTI << 20) \
				| (CAM0_SEL_XUSBXTI << 16) \
				| (FIMC3_LCLK_SEL << 12) \
				| (FIMC2_LCLK_SEL << 8) \
				| (FIMC1_LCLK_SEL << 4) \
				| (FIMC0_LCLK_SEL << 0))

/* SCLK CAM */
/* CLK_DIV_CAM */
#define FIMC0_LCLK_RATIO	4
#define FIMC1_LCLK_RATIO	4
#define FIMC2_LCLK_RATIO	4
#define FIMC3_LCLK_RATIO	4
#define CLK_DIV_CAM_VAL		((FIMC3_LCLK_RATIO << 12) \
				| (FIMC2_LCLK_RATIO << 8) \
				| (FIMC1_LCLK_RATIO << 4) \
				| (FIMC0_LCLK_RATIO << 0))

/* SCLK MFC */
/* CLK_SRC_MFC */
#define MFC_SEL_MPLL		0
#define MOUTMFC_0		0
#define MFC_SEL			MOUTMFC_0
#define MFC_0_SEL		MFC_SEL_MPLL
#define CLK_SRC_MFC_VAL		((MFC_SEL << 8) | (MFC_0_SEL))


/* CLK_DIV_MFC */
#define MFC_RATIO		3
#define CLK_DIV_MFC_VAL		(MFC_RATIO)

/* SCLK G3D */
/* CLK_SRC_G3D */
#define G3D_SEL_MPLL		0
#define MOUTG3D_0		0
#define G3D_SEL			MOUTG3D_0
#define G3D_0_SEL		G3D_SEL_MPLL
#define CLK_SRC_G3D_VAL		((G3D_SEL << 8) | (G3D_0_SEL))

/* CLK_DIV_G3D */
#define G3D_RATIO		1
#define CLK_DIV_G3D_VAL		(G3D_RATIO)

/* SCLK LCD0 */
/* CLK_SRC_LCD0 */
#define FIMD_SEL_SCLKMPLL	6
#define MDNIE0_SEL_XUSBXTI	1
#define MDNIE_PWM0_SEL_XUSBXTI	1
#define MIPI0_SEL_XUSBXTI	1
#define CLK_SRC_LCD0_VAL	((MIPI0_SEL_XUSBXTI << 12) \
				| (MDNIE_PWM0_SEL_XUSBXTI << 8) \
				| (MDNIE0_SEL_XUSBXTI << 4) \
				| (FIMD_SEL_SCLKMPLL << 0))

/* CLK_DIV_LCD0 */
#define MIPI0_PRE_RATIO		7
#define FIMD0_RATIO		4
#define CLK_DIV_LCD0_VAL	((MIPI0_PRE_RATIO << 20) \
				| (FIMD0_RATIO))
/* Power Down Modes */
#define S5P_CHECK_SLEEP			0x00000BAD
#define S5P_CHECK_DIDLE			0xBAD00000
#define S5P_CHECK_LPA			0xABAD0000

/* TZPC */
#define	TZPC_BASE			0x10110000
#define	TZPC_OFFSET			0x10000
#define TZPC0_BASE			(TZPC_BASE)
#define TZPC1_BASE			(TZPC0_BASE + TZPC_OFFSET)
#define TZPC2_BASE			(TZPC1_BASE + TZPC_OFFSET)
#define TZPC3_BASE			(TZPC2_BASE + TZPC_OFFSET)
#define TZPC4_BASE			(TZPC3_BASE + TZPC_OFFSET)
#define TZPC5_BASE			(TZPC4_BASE + TZPC_OFFSET)
#define TZPC_DECPROT0SET_OFFSET		0x804
#define TZPC_DECPROT1SET_OFFSET		0x810
#define TZPC_DECPROT2SET_OFFSET		0x81C
#define TZPC_DECPROT3SET_OFFSET		0x828

/* IMAGE SIZE (BYTE) */
#define	MBR_BYTE_COUNT			CONFIG_MBR_SIZE
#define	SBL_BYTE_COUNT			CONFIG_SBL_SIZE
#define	BL1_BYTE_COUNT			CONFIG_BL1_SIZE
#define	BL2_BYTE_COUNT			CONFIG_BL2_SIZE
#define	ENV_BYTE_COUNT			CONFIG_ENV_SIZE

/* IMAGE OFFSET (BYTE) */
#define	MBR_BYTE_OFFSET			(0)
#define	SBL_BYTE_OFFSET			(MBR_BYTE_OFFSET + MBR_BYTE_COUNT)
#define	BL1_BYTE_OFFSET			(SBL_BYTE_OFFSET + SBL_BYTE_COUNT)
#define	BL2_BYTE_OFFSET			(BL1_BYTE_OFFSET + BL1_BYTE_COUNT)
#define	ENV_BYTE_OFFSET			(Bl2_BYTE_OFFSET + BL2_BYTE_COUNT)

#define SDMMC_BLK_SIZE			(512)

/* IMAGE SIZE (BLOCK) */
#define	SBL_BLK_COUNT			(SBL_BYTE_COUNT / SDMMC_BLK_SIZE)
#define	BL1_BLK_COUNT			(BL1_BYTE_COUNT / SDMMC_BLK_SIZE)
#define	BL2_BLK_COUNT			(BL2_BYTE_COUNT / SDMMC_BLK_SIZE)
#define	ENV_BLK_COUNT			(ENV_BYTE_COUNT / SDMMC_BLK_SIZE)

/* IMAGE OFFSET (BLOCK) */
#define	SBL_BLK_OFFSET			(SBL_BYTE_OFFSET / SDMMC_BLK_SIZE)
#define	BL1_BLK_OFFSET			(BL1_BYTE_OFFSET / SDMMC_BLK_SIZE)
#define	BL2_BLK_OFFSET			(BL2_BYTE_OFFSET / SDMMC_BLK_SIZE)
#define	ENV_BLK_OFFSET			(ENV_BYTE_OFFSET / SDMMC_BLK_SIZE)

/* UART */
#if	defined(CONFIG_SERIAL0)
#define PERIPH_ID_UART			PERIPH_ID_UART0
#elif	defined(CONFIG_SERIAL1)
#define PERIPH_ID_UART			PERIPH_ID_UART1
#elif	defined(CONFIG_SERIAL2)
#define PERIPH_ID_UART			PERIPH_ID_UART2
#elif	defined(CONFIG_SERIAL3)
#define PERIPH_ID_UART			PERIPH_ID_UART3
#endif

#ifndef __ASSEMBLY__
void sdelay(unsigned long);
void clock_ctrl_init(void);
void mem_ctrl_init(void);
void tzpc_init(void);
#endif

#endif /* __SETUP_ORIGEN_QUAD_H */
