#include <common.h>
#include <s5pc110.h>

/* #include <movi.h> */
#include <asm/io.h>
#if 0
#include <regs.h>
#include <mmc.h>
#endif

#define MAGIC_NUMBER_MOVI       (0x24564236)
#if defined(CONFIG_SECURE) || defined(CONFIG_FUSED)
#define FWBL1_SIZE              (4* 1024)
#endif

#define SS_SIZE                 (8 * 1024)

#if defined(CONFIG_EVT1)
#define eFUSE_SIZE              (1 * 512)       // 512 Byte eFuse, 512 Byte reserved
#else
#define eFUSE_SIZE              (1 * 1024)      // 1 kB eFuse, 1 KB reserved
#endif /* CONFIG_EVT1 */


#define MOVI_BLKSIZE            (1<<9) /* 512 bytes */

/* partition information */
#define PART_SIZE_BL            (512 * 1024)
#define PART_SIZE_KERNEL        (4 * 1024 * 1024)
#define PART_SIZE_ROOTFS        (26 * 1024 * 1024)

//#define MOVI_LAST_BLKPOS        (MOVI_TOTAL_BLKCNT - (eFUSE_SIZE / MOVI_BLKSIZE))

/* Add block count at fused chip */
#if defined(CONFIG_SECURE) || defined(CONFIG_FUSED)
#define MOVI_FWBL1_BLKCNT       (FWBL1_SIZE / MOVI_BLKSIZE)     /* 4KB */
#endif

#define MOVI_BL1_BLKCNT         (SS_SIZE / MOVI_BLKSIZE)        /* 8KB */
#define MOVI_ENV_BLKCNT         (CONFIG_ENV_SIZE / MOVI_BLKSIZE)   /* 16KB */
#define MOVI_BL2_BLKCNT         (PART_SIZE_BL / MOVI_BLKSIZE)   /* 512KB */
#define MOVI_ZIMAGE_BLKCNT      (PART_SIZE_KERNEL / MOVI_BLKSIZE)       /* 4MB */

/* Change writing block position at fused chip */
#if defined(CONFIG_EVT1)
        #if defined(CONFIG_SECURE) || defined(CONFIG_FUSED)
#define MOVI_BL2_POS            ((eFUSE_SIZE / MOVI_BLKSIZE) + (FWBL1_SIZE / MOVI_BLKSIZE) + MOVI_BL1_BLKCNT + MOVI_ENV_BLKCNT)
        #else
#define MOVI_BL2_POS            ((eFUSE_SIZE / MOVI_BLKSIZE) + MOVI_BL1_BLKCNT + MOVI_ENV_BLKCNT)
        #endif
#else
//#define MOVI_BL2_POS            (MOVI_LAST_BLKPOS - MOVI_BL1_BLKCNT - MOVI_BL2_BLKCNT - MOVI_ENV_BLKCNT)
#endif

#if 0
#if defined(CONFIG_SECURE) || defined(CONFIG_FUSED)
#include <secure.h>
#define	SECURE_KEY_ADDRESS	(0xD0037580)

extern int Check_IntegrityOfImage (
	SecureBoot_CTX	*sbContext,
	unsigned char	*osImage,
	int		osImageLen,
	unsigned char	*osSignedData,
	int		osSignedDataLen );
#endif


extern raw_area_t raw_area_control;
#endif

typedef u32(*copy_sd_mmc_to_mem)
(u32 channel, u32 start_block, u16 block_size, u32 *trg, u32 init);

void copy_uboot_to_ram(void)
{
	ulong ch;
#if defined(CONFIG_EVT1)
	ch = *(volatile u32 *)(0xD0037488);
	copy_sd_mmc_to_mem copy_bl2 =
	    (copy_sd_mmc_to_mem) (*(u32 *) (0xD0037F98));
#if 0
	#if defined(CONFIG_SECURE)
	volatile u32 * pub_key;
	int secure_booting;
	int i;
	ulong rv;
	#endif
#endif
#else
	ch = *(volatile u32 *)(0xD003A508);
	copy_sd_mmc_to_mem copy_bl2 =
	    (copy_sd_mmc_to_mem) (*(u32 *) (0xD003E008));
#endif
	u32 ret;
	if (ch == 0xEB000000) {
		ret = copy_bl2(0, MOVI_BL2_POS, MOVI_BL2_BLKCNT,
			CONFIG_SYS_TEXT_BASE, 0);
#if 0
#if defined(CONFIG_SECURE)
		pub_key = (volatile u32 *)SECURE_KEY_ADDRESS;
			secure_booting = 0;

				for(i=0;i<33;i++){
						if( *(pub_key+i) != 0x0) secure_booting = 1;
				}

		if (secure_booting == 1) {
			/* do security check */
			rv = Check_IntegrityOfImage( (SecureBoot_CTX *)SECURE_KEY_ADDRESS, (unsigned char*)CFG_PHY_UBOOT_BASE,
			(1024*512-128), (unsigned char*)(CFG_PHY_UBOOT_BASE+(1024*512-128)), 128 );

			if (rv != 0){
				while(1);
			}
		}
#endif
#endif
	}
	else if (ch == 0xEB200000) {
		ret = copy_bl2(2, MOVI_BL2_POS, MOVI_BL2_BLKCNT,
			CONFIG_SYS_TEXT_BASE, 0);
#if 0
#if defined(CONFIG_SECURE)
		pub_key = (volatile unsigned long *)SECURE_KEY_ADDRESS;
			secure_booting = 0;

				for(i=0;i<33;i++){
						if( *(pub_key+i) != 0x0) secure_booting = 1;
				}

		if (secure_booting == 1) {
			/* do security check */
			rv = Check_IntegrityOfImage( (SecureBoot_CTX *)SECURE_KEY_ADDRESS, (unsigned char*)CFG_PHY_UBOOT_BASE,
			(1024*512-128), (unsigned char*)(CFG_PHY_UBOOT_BASE+(1024*512-128)), 128 );

			if (rv != 0){
				while(1);

			}
		}
#endif
#endif
	}
	else
		return;

	if (ret == 0)
		while (1)
			;
	else
		return;
}

void board_init_f(unsigned long bootflag)
{
        __attribute__((noreturn)) void (*uboot)(void);
        copy_uboot_to_ram();

        /* Jump to U-Boot image */
        uboot = (void *)CONFIG_SYS_TEXT_BASE;
        (*uboot)();
        /* Never returns Here */
}

/* Place Holders */
void board_init_r(gd_t *id, ulong dest_addr)
{
        /* Function attribute is no-return */
        /* This Function never executes */
        while (1)
                ;
}

void save_boot_params(u32 r0, u32 r1, u32 r2, u32 r3) {}

#if 0
/*
 * Copy zImage from SD/MMC to mem
 */
#ifdef CONFIG_MCP_SINGLE
void movi_zImage_copy(void)
{
	copy_sd_mmc_to_mem copy_zImage =
	    (copy_sd_mmc_to_mem) (*(u32 *) COPY_SDMMC_TO_MEM);
	u32 ret;

	/*
	 * 0x3C6FCE is total size of 2GB SD/MMC card
	 * TODO : eMMC will be used as boot device on HP proto2 board
	 *        So, total size of eMMC will be re-defined next board.
	 */
	ret =
	    copy_zImage(0, 0x3C6FCE, MOVI_ZIMAGE_BLKCNT, CFG_PHY_KERNEL_BASE,
			1);

	if (ret == 0)
		while (1)
			;
	else
		return;
}
#endif

void print_movi_bl2_info(void)
{
	printf("%d, %d, %d\n", MOVI_BL2_POS, MOVI_BL2_BLKCNT, MOVI_ENV_BLKCNT);
}

void movi_write_env(ulong addr)
{
	movi_write(raw_area_control.image[2].start_blk,
		   raw_area_control.image[2].used_blk, addr);
}

void movi_read_env(ulong addr)
{
	movi_read(raw_area_control.image[2].start_blk,
		  raw_area_control.image[2].used_blk, addr);
}

void movi_write_bl1(ulong addr)
{
	int i;
	ulong checksum;
	ulong src;
	ulong tmp;

	src = addr;
#if defined(CONFIG_EVT1)
	addr += 16;
	for (i = 16, checksum = 0; i < SS_SIZE; i++) {
		checksum += *(u8 *) addr++;
	}
	printf("checksum : 0x%x\n", checksum);
	*(volatile u32 *)(src + 0x8) = checksum;
	movi_write(raw_area_control.image[1].start_blk,
		   raw_area_control.image[1].used_blk, src);
#else
	for (i = 0, checksum = 0; i < SS_SIZE - 4; i++) {
		checksum += *(u8 *) addr++;
	}

	tmp = *(ulong *) addr;
	*(ulong *) addr = checksum;

	movi_write(raw_area_control.image[0].start_blk,
		   raw_area_control.image[0].used_blk, src);

	*(ulong *) addr = tmp;
#endif
}

#if defined(CONFIG_VOGUES)
int movi_boot_src()
{
	ulong reg;
	ulong src;

	reg = (*(volatile u32 *)(INF_REG_BASE + INF_REG3_OFFSET));

	if (reg == BOOT_MMCSD)
		/* boot device is SDMMC */
		src = 0;
	else if (reg == BOOT_NOR)
		/* boot device is NOR */
		src = 1;

	return src;
}
#endif
#endif
