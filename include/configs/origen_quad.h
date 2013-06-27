/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Copyright (C) 2012 Insignal
 *
 * Configuration settings for the Insignal OrigenQUAD board (Exynos4x12)
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

#ifndef __CONFIG_ORIGEN_QUAD_H
#define __CONFIG_ORIGEN_QUAD_H

/*
 * High Level Configuration Options
 */
#define CONFIG_CMD_BOOTZ
#define CONFIG_SAMSUNG			/* in a SAMSUNG core */
#define CONFIG_S5P			/* S5P Family */
#define CONFIG_EXYNOS4			/* which is in a Exynos4 series */
#define CONFIG_EXYNOS4412		/* which is in a Exynos4412 */
#define CONFIG_ORIGEN_QUAD		/* which is in a ORIGEN QUAD */
#define CONFIG_BOARDNAME		"OrigenQuad"
#define CONFIG_IDENT_STRING		" for Insignal Origen"
#define CONFIG_MACH_TYPE		4200

#define CONFIG_ARMV7            1       /* This is an ARM V7 CPU core */
#define CONFIG_CPU_EXYNOS4X12   1       /* which is in a Exynos4X12 */
#define CONFIG_S5PC210          1       /* which is in a S5PC210 */
#define CONFIG_S5PC220          1       /* which is in a S5PC220 */
#define CONFIG_SMDKC210         1
#define CONFIG_SMDKC220         1
#define CONFIG_EXYNOS4212       1
#define CONFIG_EXYNOS4412_EVT1  1

#include <asm/arch/cpu.h>		/* get chip and board defs */

/*
 * Clock / PLL
 */
#define CONFIG_SYS_CLK_FREQ		24000000 /* 24MHz input clock */
#define CONFIG_CLK_APLL			1000
#define CONFIG_CLK_MPLL			400
#define CONFIG_SYS_HZ			1000

#define CONFIG_PWM

/*
 * Memory
 */
/* Default address */
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_TEXT_BASE		0x43E00000
#define CONFIG_SYS_LOAD_ADDR		CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_LOAD_ADDR - GENERATED_GBL_DATA_SIZE)
/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 1 << 20UL)
/* Cache */
#define CONFIG_SYS_DCACHE_OFF		/* Keep L2 Cache Disabled */
/* DRAM */
#define CONFIG_NR_DRAM_BANKS		4 /* 256 MB * 4 = 1 GB */
#define SDRAM_BANK_SIZE			(256UL << 20UL)	/* 256 MB */
#define PHYS_SDRAM_1			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_1_SIZE		SDRAM_BANK_SIZE
#define PHYS_SDRAM_2			(PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE)
#define PHYS_SDRAM_2_SIZE		SDRAM_BANK_SIZE
#define PHYS_SDRAM_3			(PHYS_SDRAM_2 + PHYS_SDRAM_2_SIZE)
#define PHYS_SDRAM_3_SIZE		SDRAM_BANK_SIZE
#define PHYS_SDRAM_4			(PHYS_SDRAM_3 + PHYS_SDRAM_3_SIZE)
#define PHYS_SDRAM_4_SIZE		SDRAM_BANK_SIZE
/* Reserved */
#define CONFIG_RESERVED_DRAM		0x100000
/* Mem test */
#define CONFIG_CMD_MEMORY
#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		(PHYS_SDRAM_4 + PHYS_SDRAM_4_SIZE - (8UL << 20UL))

/*
 * Serial
 */
#define CONFIG_SERIAL2			2 /* use SERIAL2 in 0~3 */
#define CONFIG_SERIAL_MULTI		1
#define CONFIG_BAUDRATE			115200

/*
 * PMIC
 */
#define CONFIG_PMIC			1

/*
 * SD/MMC
 */
#define CONFIG_GENERIC_MMC
#define CONFIG_MMC
#define CONFIG_SDHCI
#define CONFIG_S5P_SDHCI
#define CONFIG_CMD_MMC

/*
 * File System
 */
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_CMD_EXT2

/*
 * Flash
 */
#define CONFIG_SYS_NO_FLASH

/*
 * Image map on sdmmc/emmc
 */
#define CONFIG_SECURE_BL1_ONLY
#define CONFIG_SPL
#define	CONFIG_MBR_SIZE			(512)
#define	CONFIG_SBL_SIZE			(8UL << 10)	/* 8KB */
#define	CONFIG_BL1_SIZE			(16UL << 10) /* 16KB */
#define	CONFIG_BL2_SIZE			(512UL << 10) /* 512KB */

/*
 * Console
 */
#define CONFIG_SYS_PROMPT		CONFIG_BOARDNAME " # "
/* Console I/O Buffer Size */
#define CONFIG_SYS_CBSIZE		256
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		384
/* max number of command args */
#define CONFIG_SYS_MAXARGS		16
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/

/*
 * Commands
 */
#define CONFIG_CMDLINE_EDITING
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#include <config_cmd_default.h>
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
/*
 * Environment
 */
#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_ENV_OFFSET		(CONFIG_MBR_SIZE \
					+CONFIG_SBL_SIZE \
					+CONFIG_BL1_SIZE \
					+CONFIG_BL2_SIZE)
#define CONFIG_ENV_SIZE			(16 << 10) /* 16 KB */
#define CONFIG_ENV_OVERWRITE
/* Default */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"loadaddr=0x42000000\0" \
	"console=ttySAC2,115200n8\0" \
	"mmcdev=0:2\0" \
	"mmcroot=/dev/mmcblk0p2 rw\0" \
	"rootwait\0" \
	"mmcargs=setenv bootargs console=${console} " \
	"root=${mmcroot}\0" \
	"loadbootscript=fatload mmc ${mmcdev} ${loadaddr} boot.scr\0" \
	"bootscript=echo Running bootscript from mmc${mmcdev} ...; " \
	"source ${loadaddr}\0" \
	"loaduimage=fatload mmc ${mmcdev} ${loadaddr} uImage\0" \
	"mmcboot=echo Booting from mmc${mmcdev} ...; " \
	"run mmcargs; " \
	"bootm ${loadaddr}\0" \

#define CONFIG_BOOTCOMMAND \
	"if mmc rescan ${mmcdev}; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loaduimage; then " \
				"run mmcboot; " \
			"fi; " \
		"fi; " \
	"fi"

/*
 * Misc
 */
/* Initialize */
#define CONFIG_ARCH_CPU_INIT		1
#define CONFIG_BOARD_EARLY_INIT_F	1
/* display information */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO
/* Delay */
#define CONFIG_BOOTDELAY		3
#define CONFIG_ZERO_BOOTDELAY_CHECK
/* TAGS */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
/* Enable devicetree support */
#define CONFIG_OF_LIBFDT
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
/* USB Options */
#define CONFIG_CMD_USB
#ifdef	CONFIG_CMD_USB
#define CONFIG_USB_OHCI
#define CONFIG_S3C_USBD
#define USBD_DOWN_ADDR			0xC0000000
#endif
#define CONFIG_OF_LIBFDT

#endif
