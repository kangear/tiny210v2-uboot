/*
 * Copyright (C) 2010 Samsung Electronics Co. Ltd
 *
 * Many parts of this program were copied from the work of Windriver.
 * Major modifications are as follows:
 * - Adding default partition table.
 * - Supporting OneNAND device.
 * - Supporting SDMMC device.
 * - Adding new command, sdfuse.
 * - Removing Lock scheme.
 * - Removing direct flash operations because they are implemented at others.
 * - Fixing several bugs
 * This program is under the same License with the their work.
 *
 * This is their Copyright:
 *
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
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
 * Part of the rx_handler were copied from the Android project. 
 * Specifically rx command parsing in the  usb_rx_data_complete 
 * function of the file bootable/bootloader/legacy/usbloader/usbloader.c
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_* 
 * They come from bootable/bootloader/legacy/libboot/flash.c
 *
 * This is their Copyright:
 * 
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <fastboot.h>
#if defined(CFG_FASTBOOT_SDMMCBSP)
#include <mmc.h>
#endif

#if (CONFIG_FASTBOOT)

#ifdef	CONFIG_FASTBOOT_DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);
#else
#define debug(fmt,args...)
#define debugX(level,fmt,args...)
#endif	/* DEBUG */

/* Use do_reset for fastboot's 'reboot' command */
extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
/* Use do_fat_fsload for direct image fusing from sd card */
extern int do_fat_fsload (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
/* Use do_setenv and do_saveenv to permenantly save data */
/*TODO kangear */
int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){printf("Call do_saveenv\n");};
int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]){printf("do_setenv\n");};
/* Use do_bootm and do_go for fastboot's 'boot' command */
extern int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

#if defined(CFG_FASTBOOT_ONENANDBSP)
#define CFG_FASTBOOT_FLASHCMD			do_onenand
/* Use do_onenand for fastboot's flash commands */
extern int do_onenand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
#elif defined(CFG_FASTBOOT_NANDBSP)
#define CFG_FASTBOOT_FLASHCMD			do_nand
/* Use do_nand for fastboot's flash commands */
/*TODO sth*/
//extern int do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
static do_nand(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]){printf("Call do_nand\n");};
#elif defined(CFG_FASTBOOT_SDMMCBSP)
int do_movi(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
int do_mmcops(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
int get_mmc_part_info(char *device_name, int part_num, int *start, int *count, unsigned char *pid);
struct mmc *find_mmc_device(int dev_num);
#endif

/*TODO LCD */
void LCD_turnon(void){};
void LCD_setfgcolor(unsigned int color){};
void LCD_setleftcolor(unsigned int color){};
void LCD_setprogress(int percentage){};

/* Forward decl */
static int rx_handler (const unsigned char *buffer, unsigned int buffer_size);
static void reset_handler (void);

static struct cmd_fastboot_interface interface = 
{
	.rx_handler            = rx_handler,
	.reset_handler         = reset_handler,
	.product_name          = NULL,
	.serial_no             = NULL,
	.nand_block_size       = 0,
	.transfer_buffer       = (unsigned char *)0xffffffff,
	.transfer_buffer_size  = 0,
};

static unsigned int download_size;
static unsigned int download_bytes;
//static unsigned int download_bytes_unpadded;
static unsigned int download_error;

/* To support the Android-style naming of flash */
#define MAX_PTN 16
static fastboot_ptentry ptable[MAX_PTN];
static unsigned int pcount;
static int static_pcount = -1;

/* Default partition table (see cpu/.../fastboot.c) */
extern fastboot_ptentry *ptable_default;
extern unsigned int ptable_default_size;

static unsigned char cmd_buf[100] = {0};

static void set_env(char *var, char *val)
{
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };

	setenv[1] = var;
	setenv[2] = val;

	do_setenv(NULL, 0, 3, setenv);
}

static void save_env(struct fastboot_ptentry *ptn,
		     char *var, char *val)
{
#ifndef CFG_FASTBOOT_SDMMCBSP
	char start[32], length[32];
	char ecc_type[32];

	char *lock[5]    = { "nand", "lock",   NULL, NULL, NULL, };
	char *unlock[5]  = { "nand", "unlock", NULL, NULL, NULL, };
	char *ecc[4]     = { "nand", "ecc",    NULL, NULL, };
	char *saveenv[2] = { "setenv", NULL, };

	lock[2] = unlock[2] = start;
	lock[3] = unlock[3] = length;

	set_env (var, val);

	/* Some flashing requires the nand's ecc to be set */
	ecc[2] = ecc_type;
	if ((ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC) &&
	    (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC))	{
		/* Both can not be true */
		printf("Warning can not do hw and sw ecc for partition '%s'\n", ptn->name);
		printf("Ignoring these flags\n");
	} else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC) {
		sprintf(ecc_type, "hw");
		CFG_FASTBOOT_FLASHCMD(NULL, 0, 3, ecc);
	} else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC) {
		sprintf(ecc_type, "sw");
		CFG_FASTBOOT_FLASHCMD(NULL, 0, 3, ecc);
	}
	sprintf(start, "0x%x", ptn->start);
	sprintf(length, "0x%x", ptn->length);

	/* This could be a problem is there is an outstanding lock */
	CFG_FASTBOOT_FLASHCMD(NULL, 0, 4, unlock);
	do_saveenv(NULL, 0, 1, saveenv);
	CFG_FASTBOOT_FLASHCMD(NULL, 0, 4, lock);
#endif
}

#if 0
static void save_block_values(struct fastboot_ptentry *ptn,
			      unsigned int offset,
			      unsigned int size)
{
	struct fastboot_ptentry *env_ptn;

	char var[64], val[32];
	char start[32], length[32];
	char ecc_type[32];

	char *lock[5]    = { "nand", "lock",   NULL, NULL, NULL, };
	char *unlock[5]  = { "nand", "unlock", NULL, NULL, NULL, };
	char *ecc[4]     = { "nand", "ecc",    NULL, NULL, };	
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };
	char *saveenv[2] = { "setenv", NULL, };
	
	setenv[1] = var;
	setenv[2] = val;
	lock[2] = unlock[2] = start;
	lock[3] = unlock[3] = length;

	printf ("saving it..\n");

	if (size == 0)
	{
		/* The error case, where the variables are being unset */
		
		sprintf (var, "%s_nand_offset", ptn->name);
		sprintf (val, "");
		do_setenv (NULL, 0, 3, setenv);

		sprintf (var, "%s_nand_size", ptn->name);
		sprintf (val, "");
		do_setenv (NULL, 0, 3, setenv);
	}
	else
	{
		/* Normal case */

		sprintf (var, "%s_nand_offset", ptn->name);
		sprintf (val, "0x%x", offset);

		printf ("%s %s %s\n", setenv[0], setenv[1], setenv[2]);
		
		do_setenv (NULL, 0, 3, setenv);

		sprintf (var, "%s_nand_size", ptn->name);

		sprintf (val, "0x%x", size);

		printf ("%s %s %s\n", setenv[0], setenv[1], setenv[2]);

		do_setenv (NULL, 0, 3, setenv);
	}


	/* Warning : 
	   The environment is assumed to be in a partition named 'enviroment'.
	   It is very possible that your board stores the enviroment 
	   someplace else. */
	env_ptn = fastboot_flash_find_ptn("environment");

	if (env_ptn)
	{
		/* Some flashing requires the nand's ecc to be set */
		ecc[2] = ecc_type;
		if ((env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC) &&
		    (env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC)) 
		{
			/* Both can not be true */
			printf ("Warning can not do hw and sw ecc for partition '%s'\n", ptn->name);
			printf ("Ignoring these flags\n");
		} 
		else if (env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC)
		{
			sprintf (ecc_type, "hw");
			CFG_FASTBOOT_FLASHCMD (NULL, 0, 3, ecc);
		}
		else if (env_ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC)
		{
			sprintf (ecc_type, "sw");
			CFG_FASTBOOT_FLASHCMD (NULL, 0, 3, ecc);
		}
		
		sprintf (start, "0x%x", env_ptn->start);
		sprintf (length, "0x%x", env_ptn->length);			

		/* This could be a problem is there is an outstanding lock */
		CFG_FASTBOOT_FLASHCMD (NULL, 0, 4, unlock);
	}

	do_saveenv (NULL, 0, 1, saveenv);
	
	if (env_ptn)
	{
		CFG_FASTBOOT_FLASHCMD (NULL, 0, 4, lock);
	}
}
#endif

static void reset_handler ()
{
	/* If there was a download going on, bail */
	download_size = 0;
	download_bytes = 0;
	//download_bytes_unpadded = 0;
	download_error = 0;
}

/* When save = 0, just parse.  The input is unchanged
   When save = 1, parse and do the save.  The input is changed */
static int parse_env(void *ptn, char *err_string, int save, int debug)
{
	int ret = 1;
	unsigned int sets = 0;
	unsigned int comment_start = 0;
	char *var = NULL;
	char *var_end = NULL;
	char *val = NULL;
	char *val_end = NULL;
	unsigned int i;

	char *buff = (char *)interface.transfer_buffer;
	//unsigned int size = download_bytes_unpadded;
	unsigned int size = download_bytes;

	/* The input does not have to be null terminated.
	   This will cause a problem in the corner case
	   where the last line does not have a new line.
	   Put a null after the end of the input.

	   WARNING : Input buffer is assumed to be bigger
	   than the size of the input */
	if (save)
		buff[size] = 0;

	for (i = 0; i < size; i++) {

		if (NULL == var) {

			/*
			 * Check for comments, comment ok only on
			 * mostly empty lines
			 */
			if (buff[i] == '#')
				comment_start = 1;

			if (comment_start) {
				if  ((buff[i] == '\r') ||
				     (buff[i] == '\n')) {
					comment_start = 0;
				}
			} else {
				if (!((buff[i] == ' ') ||
				      (buff[i] == '\t') ||
				      (buff[i] == '\r') ||
				      (buff[i] == '\n'))) {
					/*
					 * Normal whitespace before the
					 * variable
					 */
					var = &buff[i];
				}
			}

		} else if (((NULL == var_end) || (NULL == val)) &&
			   ((buff[i] == '\r') || (buff[i] == '\n'))) {

			/* This is the case when a variable
			   is unset. */

			if (save) {
				/* Set the var end to null so the
				   normal string routines will work

				   WARNING : This changes the input */
				buff[i] = '\0';

				save_env(ptn, var, val);

				if (debug)
					printf("Unsetting %s\n", var);
			}

			/* Clear the variable so state is parse is back
			   to initial. */
			var = NULL;
			var_end = NULL;
			sets++;
		} else if (NULL == var_end) {
			if ((buff[i] == ' ') ||
			    (buff[i] == '\t'))
				var_end = &buff[i];
		} else if (NULL == val) {
			if (!((buff[i] == ' ') ||
			      (buff[i] == '\t')))
				val = &buff[i];
		} else if (NULL == val_end) {
			if ((buff[i] == '\r') ||
			    (buff[i] == '\n')) {
				/* look for escaped cr or ln */
				if ('\\' == buff[i - 1]) {
					/* check for dos */
					if ((buff[i] == '\r') &&
					    (buff[i+1] == '\n'))
						buff[i + 1] = ' ';
					buff[i - 1] = buff[i] = ' ';
				} else {
					val_end = &buff[i];
				}
			}
		} else {
			sprintf(err_string, "Internal Error");

			if (debug)
				printf("Internal error at %s %d\n",
				       __FILE__, __LINE__);
			return 1;
		}
		/* Check if a var / val pair is ready */
		if (NULL != val_end) {
			if (save) {
				/* Set the end's with nulls so
				   normal string routines will
				   work.

				   WARNING : This changes the input */
				*var_end = '\0';
				*val_end = '\0';

				save_env(ptn, var, val);

				if (debug)
					printf("Setting %s %s\n", var, val);
			}

			/* Clear the variable so state is parse is back
			   to initial. */
			var = NULL;
			var_end = NULL;
			val = NULL;
			val_end = NULL;

			sets++;
		}
	}

	/* Corner case
	   Check for the case that no newline at end of the input */
	if ((NULL != var) &&
	    (NULL == val_end)) {
		if (save) {
			/* case of val / val pair */
			if (var_end)
				*var_end = '\0';
			/* else case handled by setting 0 past
			   the end of buffer.
			   Similar for val_end being null */
			save_env(ptn, var, val);

			if (debug) {
				if (var_end)
					printf("Trailing Setting %s %s\n", var, val);
				else
					printf("Trailing Unsetting %s\n", var);
			}
		}
		sets++;
	}
	/* Did we set anything ? */
	if (0 == sets)
		sprintf(err_string, "No variables set");
	else
		ret = 0;

	return ret;
}

static int saveenv_to_ptn(struct fastboot_ptentry *ptn, char *err_string)
{
	int ret = 1;
	int save = 0;
	int debug = 0;

	/* err_string is only 32 bytes
	   Initialize with a generic error message. */
	sprintf(err_string, "%s", "Unknown Error");

	/* Parse the input twice.
	   Only save to the enviroment if the entire input if correct */
	save = 0;
	if (0 == parse_env(ptn, err_string, save, debug)) {
		save = 1;
		ret = parse_env(ptn, err_string, save, debug);
	}
	return ret;
}

static int write_to_ptn(struct fastboot_ptentry *ptn, unsigned int addr, unsigned int size)
#if defined(CFG_FASTBOOT_ONENANDBSP) || defined(CFG_FASTBOOT_NANDBSP)
{
	int ret = 1;
	char start[32], length[32];
	char wstart[32], wlength[32], wbuffer[32];
	char write_type[32];

	/* do_nand and do_onenand do not check argv[0] */
	char *argv_erase[5]  = { NULL, "erase",  NULL, NULL, NULL, };
	char *argv_write[6]  = { NULL, NULL,  NULL, NULL, NULL, NULL, };

	argv_erase[2] = start;
	argv_erase[3] = length;

	argv_write[1] = write_type;
	argv_write[2] = wbuffer;
	argv_write[3] = wstart;
	argv_write[4] = wlength;

	printf("flashing '%s'\n", ptn->name);

	sprintf(start, "0x%x", ptn->start);
	printf("ptn->length = %d\n", ptn->length);
	if (ptn->length == 0)
	{
		sprintf(cmd_buf, "nand erase %x ", ptn->start, ptn->length);
		run_command(cmd_buf, 0);
	}
	else
	{
		sprintf(cmd_buf, "nand erase %x %x", ptn->start, ptn->length);
		printf("cmd = %s\n", cmd_buf);
		run_command(cmd_buf, 0);
		sprintf(length, "0x%x", ptn->length);
	}

	/* Which flavor of write to use */
	if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS)
	{
		sprintf(write_type, "write.yaffs");
		sprintf(wlength, "0x%x", size);
	}
#if 0
	else if(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_UBOOT) // forlinx add for write.uboot
	{
		sprintf(write_type, "write.uboot");
		sprintf(wlength, "0x%x", size);
	}
#endif
	else
	{
		sprintf(write_type, "write");
		if (interface.nand_block_size &&
			(size % interface.nand_block_size))
		{
			size = (size + interface.nand_block_size - 1)
					/ interface.nand_block_size * interface.nand_block_size;
		}
		sprintf(wlength, "0x%x", size);
	}

	sprintf(wbuffer, "0x%x", addr);
	sprintf(wstart, "0x%x", ptn->start);

	sprintf(cmd_buf, "nand %s %s %s %s", write_type, argv_write[2], argv_write[3], argv_write[4]);
	printf("cmd = %s\n", cmd_buf);
	ret = run_command(cmd_buf, 0);
	//ret = CFG_FASTBOOT_FLASHCMD(NULL, 0, 5, argv_write);

#if 0
	if (0 == repeat) {
		if (ret) /* failed */
			save_block_values(ptn, 0, 0);
		else     /* success */
			save_block_values(ptn, ptn->start, download_bytes);
	}
#endif

	return ret;
}
#elif defined(CFG_FASTBOOT_SDMMCBSP)
{
	int ret = 1;
	char device[32], part[32];
	char start[32], length[32], buffer[32];

	char *argv[6]  = { NULL, "write", NULL, NULL, NULL, NULL, };
	int argc = 0;

	if ((ptn->length != 0) && (size > ptn->length))
	{
		printf("Error: Image size is larger than partition size!\n");
		return 1;
	}

	printf("flashing '%s'\n", ptn->name);

	if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_USE_MMC_CMD)
	{
		argv[2] = device;
		argv[3] = buffer;
		argv[4] = start;
		argv[5] = length;

		sprintf(device, "mmc %d", 1);
		sprintf(buffer, "0x%x", addr);
		sprintf(start, "0x%x", (ptn->start / CFG_FASTBOOT_SDMMC_BLOCKSIZE));
		sprintf(length, "0x%x", (ptn->length / CFG_FASTBOOT_SDMMC_BLOCKSIZE));

		ret = do_mmcops(NULL, 0, 6, argv);
	}
	else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_USE_MOVI_CMD)
	{
		argv[2] = part;
		argv[3] = buffer;

		argc = 4;

		/* use the partition name that can be understood by a command, movi */
		if (!strcmp(ptn->name, "bootloader"))
		{
			strncpy(part, "u-boot", 7);
		}
		else if (!strcmp(ptn->name, "ramdisk"))
		{
			strncpy(part, "rootfs", 7);
			argv[4] = length;
			sprintf(length, "0x%x",
				((size + CFG_FASTBOOT_SDMMC_BLOCKSIZE - 1)
				/ CFG_FASTBOOT_SDMMC_BLOCKSIZE ) * CFG_FASTBOOT_SDMMC_BLOCKSIZE);
			argc++;
		}
		else	/* kernel, fwbl1 */
		{
			argv[2] = ptn->name;
		}
		sprintf(buffer, "0x%x", addr);

		ret = do_movi(NULL, 0, argc, argv);

		/* the return value of do_movi is different from usual commands. Hence the followings. */
		ret = 1 - ret;
	}

	return ret;
}
#endif

static int rx_handler (const unsigned char *buffer, unsigned int buffer_size)
{
	int ret = 1;

	/* Use 65 instead of 64
	   null gets dropped  
	   strcpy's need the extra byte */
	char response[65];

	if (download_size) 
	{
		/* Something to download */

		if (buffer_size)
		{
			/* Handle possible overflow */
			unsigned int transfer_size = download_size - download_bytes;

			if (buffer_size < transfer_size)
				transfer_size = buffer_size;
			
			/* Save the data to the transfer buffer */
			memcpy (interface.transfer_buffer + download_bytes, 
				buffer, transfer_size);

			download_bytes += transfer_size;

			/* Check if transfer is done */
			if (download_bytes >= download_size)
			{
				/* Reset global transfer variable,
				   Keep download_bytes because it will be
				   used in the next possible flashing command */
				download_size = 0;

				if (download_error)
				{
					/* There was an earlier error */
					sprintf(response, "ERROR");
				}
				else
				{
					/* Everything has transferred,
					   send the OK response */
					sprintf(response, "OKAY");
				}
				fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

				printf ("\ndownloading of %d bytes finished\n", download_bytes);
				LCD_setprogress(0);
			}

			/* Provide some feedback */
			if (download_bytes && download_size &&
			    0 == (download_bytes & (0x100000 - 1)))
			{
				/* Some feeback that the download is happening */
				if (download_error)
					printf("X");
				else
					printf(".");
				if (0 == (download_bytes %
					  (80 * 0x100000)))
					printf("\n");

				LCD_setfgcolor(0x2E8B57);
				LCD_setprogress(download_bytes / (download_size/100));
			}
		}
		else
		{
			/* Ignore empty buffers */
			printf ("Warning empty download buffer\n");
			printf ("Ignoring\n");
		}
		ret = 0;
	}
	else
	{
		/* A command */

		/* Cast to make compiler happy with string functions */
		const char *cmdbuf = (char *) buffer;

		/* Generic failed response */
		sprintf(response, "FAIL");

		/* reboot 
		   Reboot the board. */
		if (memcmp(cmdbuf, "reboot", 6) == 0)
		{
			if (!strcmp(cmdbuf + 6, "-bootloader"))
			{
				strcpy((char *)interface.transfer_buffer, (char *)FASTBOOT_REBOOT_MAGIC);
			}
			else
			{
				memset(interface.transfer_buffer, 0x0, FASTBOOT_REBOOT_MAGIC_SIZE);
			}

			sprintf(response,"OKAY");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_SYNC);
			//udelay (1000000); /* 1 sec */

			do_reset (NULL, 0, 0, NULL);
			
			/* This code is unreachable,
			   leave it to make the compiler happy */
			return 0;
		}
		
		/* getvar
		   Get common fastboot variables
		   Board has a chance to handle other variables */
		if (memcmp(cmdbuf, "getvar:", 7) == 0)
		{
			strcpy(response,"OKAY");
        
			if (!strcmp(cmdbuf + 7, "version"))
			{
				strcpy(response + 4, FASTBOOT_VERSION);
			} 
			else if (!strcmp(cmdbuf + 7, "product"))
			{
				if (interface.product_name) 
					strcpy(response + 4, interface.product_name);
			}
			else if (!strcmp(cmdbuf + 7, "serialno"))
			{
				if (interface.serial_no) 
					strcpy(response + 4, interface.serial_no);
			}
			else if (!strcmp(cmdbuf + 7, "downloadsize"))
			{
				if (interface.transfer_buffer_size) 
					sprintf(response + 4, "%08x", interface.transfer_buffer_size);
			} 
			else 
			{
				fastboot_getvar(cmdbuf + 7, response + 4);
			}
			ret = 0;
			goto send_tx_status;
		}

		/* erase
		   Erase a register flash partition
		   Board has to set up flash partitions */
		if (memcmp(cmdbuf, "erase:", 6) == 0)
		{
			struct fastboot_ptentry *ptn;

			ptn = fastboot_flash_find_ptn(cmdbuf + 6);
			if (ptn == 0)
			{
				sprintf(response, "FAILpartition does not exist");
				ret = 0;
				goto send_tx_status;
			}

			char start[32], length[32];
			int status;

#if defined(CFG_FASTBOOT_SDMMCBSP)
			printf("erasing(formatting) '%s'\n", ptn->name);
#else
			printf("erasing '%s'\n", ptn->name);
#endif
			LCD_setfgcolor(0x7FFFD4);
			LCD_setprogress(100);

#if defined(CFG_FASTBOOT_SDMMCBSP)
			// Temporary (but, simplest) implementation
			char run_cmd[80];
			status = 1;
			if (!strcmp(ptn->name, "userdata"))
			{
				sprintf(run_cmd, "ext3format mmc 0:3");
				status = run_command(run_cmd, 0);
			}
			else if (!strcmp(ptn->name, "cache"))
			{
				sprintf(run_cmd, "ext3format mmc 0:4");
				status = run_command(run_cmd, 0);
			}
			else if (!strcmp(ptn->name, "fat"))
			{
				sprintf(run_cmd, "fatformat mmc 0:1");
				status = run_command(run_cmd, 0);
			}
#else
			int argc_erase = 4;
			/* do_nand and do_onenand do not check argv[0] */
			char *argv_erase[5]  = { NULL, "erase",  NULL, NULL, NULL, };
		
			argv_erase[2] = start;
			argv_erase[3] = length;

			sprintf(start, "0x%x", ptn->start);
			sprintf(length, "0x%x", ptn->length);

			if (ptn->length == 0)
				argc_erase = 3;

			sprintf(cmd_buf, "nand erase 0x%x 0x%x ", ptn->start, ptn->length);
			status = run_command(cmd_buf, 0);
			//kangear
#endif

			if (status)
			{
				sprintf(response,"FAILfailed to erase partition");
			}
			else
			{
				printf("partition '%s' erased\n", ptn->name);
				sprintf(response, "OKAY");
			}
			ret = 0;
			goto send_tx_status;
		}

		/* download
		   download something .. 
		   What happens to it depends on the next command after data */
		if (memcmp(cmdbuf, "download:", 9) == 0)
		{
			/* save the size */
			download_size = simple_strtoul (cmdbuf + 9, NULL, 16);
			/* Reset the bytes count, now it is safe */
			download_bytes = 0;
			/* Reset error */
			download_error = 0;

			printf ("Starting download of %d bytes\n", download_size);

			if (0 == download_size)
			{
				/* bad user input */
				sprintf(response, "FAILdata invalid size");
			}
			else if (download_size > interface.transfer_buffer_size)
			{
				/* set download_size to 0 because this is an error */
				download_size = 0;
				sprintf(response, "FAILdata too large");
			}
			else
			{
				/* The default case, the transfer fits
				   completely in the interface buffer */
				sprintf(response, "DATA%08x", download_size);
			}
			ret = 0;
			goto send_tx_status;
		}

		/* boot
		   boot what was downloaded

		   WARNING WARNING WARNING

		   This is not what you expect.
		   The fastboot client does its own packaging of the
		   kernel.  The layout is defined in the android header
		   file bootimage.h.  This layeout is copiedlooks like this,

		   **
		   ** +-----------------+
		   ** | boot header     | 1 page
		   ** +-----------------+
		   ** | kernel          | n pages
		   ** +-----------------+
		   ** | ramdisk         | m pages
		   ** +-----------------+
		   ** | second stage    | o pages
		   ** +-----------------+
		   **

		   What is a page size ?
		   The fastboot client uses 2048

		   The is the default value of CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE
		*/
		if (memcmp(cmdbuf, "boot", 4) == 0)
		{
			if ((download_bytes) &&
			    (CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE < download_bytes))
			{
				/* Note: We store zImage and ramdisk at different partitions */
				char addr_kernel[32];
				char addr_ramdisk[32];
				int pageoffset_ramdisk;

				char *bootm[3] = { "bootm", NULL, NULL, };
				//char *go[3]    = { "go",    NULL, NULL, };

				/*
				 * Use this later to determine if a command line was passed
				 * for the kernel.
				 */
				struct fastboot_boot_img_hdr *fb_hdr =
					(struct fastboot_boot_img_hdr *) interface.transfer_buffer;

				/* Skip the mkbootimage header */
				image_header_t *hdr =
					(image_header_t *)
					&interface.transfer_buffer[CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE];

				printf("Kernel size: %08x\n", fb_hdr->kernel_size);
				printf("Ramdisk size: %08x\n", fb_hdr->ramdisk_size);

				pageoffset_ramdisk = 1 + (fb_hdr->kernel_size + CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE - 1) / CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE;

				bootm[1] = addr_kernel;
				sprintf (addr_kernel, "0x%x", CFG_FASTBOOT_ADDR_KERNEL);
				memcpy((void *)CFG_FASTBOOT_ADDR_KERNEL,
					interface.transfer_buffer + CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE,
					fb_hdr->kernel_size);
				bootm[2] = addr_ramdisk;
				sprintf (addr_ramdisk, "0x%x", CFG_FASTBOOT_ADDR_RAMDISK);
				memcpy((void *)CFG_FASTBOOT_ADDR_RAMDISK, interface.transfer_buffer +
					(pageoffset_ramdisk * CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE),
					fb_hdr->ramdisk_size);

				/* Execution should jump to kernel so send the response
				   now and wait a bit.  */
				sprintf(response, "OKAY");
				fastboot_tx_status(response, strlen(response), FASTBOOT_TX_SYNC);
				udelay (1000000); /* 1 sec */

				if (ntohl(hdr->ih_magic) == IH_MAGIC) {
					/* Looks like a kernel.. */
					printf ("Booting kernel..\n");

					/*
					 * Check if the user sent a bootargs down.
					 * If not, do not override what is already there
					 */
					if (strlen ((char *) &fb_hdr->cmdline[0]))
						set_env ("bootargs", (char *) &fb_hdr->cmdline[0]);

					do_bootm (NULL, 0, 2, bootm);
				} else {
					/* Raw image, maybe another uboot */
					printf ("Booting raw image..\n");

					//do_go (NULL, 0, 2, go);
					do_bootm (NULL, 0, 3, bootm);
				}
				printf ("ERROR : bootting failed\n");
				printf ("You should reset the board\n");
			}
			sprintf(response, "FAILinvalid boot image");
			ret = 0;
		}

		/* flash
		   Flash what was downloaded */
		if (memcmp(cmdbuf, "flash:", 6) == 0)
		{
			if (download_bytes == 0)
			{
				sprintf(response, "FAILno image downloaded");
				ret = 0;
				goto send_tx_status;
			}
			
			struct fastboot_ptentry *ptn;

			LCD_setfgcolor(0x8B4500);
			LCD_setprogress(100);

			/* Special case: boot.img */
			if (!strcmp("boot", cmdbuf + 6))
			{
				int pageoffset_ramdisk;

				struct fastboot_boot_img_hdr *fb_hdr =
					(struct fastboot_boot_img_hdr *) interface.transfer_buffer;
				image_header_t *hdr =
					(image_header_t *)
					&interface.transfer_buffer[CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE];

				printf("Kernel size: %08x\n", fb_hdr->kernel_size);
				printf("Ramdisk size: %08x\n", fb_hdr->ramdisk_size);

				ptn = fastboot_flash_find_ptn("kernel");
				if (ptn->length && fb_hdr->kernel_size > ptn->length)
				{
					sprintf(response, "FAILimage too large for partition");
					goto send_tx_status;
				}
				ret = write_to_ptn(ptn,
					(unsigned int)interface.transfer_buffer + CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE,
					fb_hdr->kernel_size);

				pageoffset_ramdisk =
					1 + (fb_hdr->kernel_size + CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE - 1)
						/ CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE;
				ptn = fastboot_flash_find_ptn("ramdisk");
				if (ptn->length && fb_hdr->ramdisk_size > ptn->length)
				{
					sprintf(response, "FAILimage too large for partition");
					goto send_tx_status;
				}
				ret |= write_to_ptn(ptn,
					(unsigned int)interface.transfer_buffer + (pageoffset_ramdisk * CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE),
					fb_hdr->ramdisk_size);

				if (ret)
				{
					printf("flashing '%s' failed\n", "kernel+ramdisk");
					sprintf(response, "FAILfailed to flash partition");
				}
				else
				{
					printf("partition '%s' flashed\n", "kernel+ramdisk");
					sprintf(response, "OKAY");
				}

				ret = 0;
				goto send_tx_status;
			}

			ptn = fastboot_flash_find_ptn(cmdbuf + 6);
			if (ptn == 0)
			{
				sprintf(response, "FAILpartition does not exist");
			}
			else if ((download_bytes > ptn->length) && (ptn->length != 0) &&	
				   !(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV))
			{
				sprintf(response, "FAILimage too large for partition");
				/* TODO : Improve check for yaffs write */
			}
			else
			{
				/* Check if this is not really a flash write
				   but rather a saveenv */
				if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)
				{
					/* Since the response can only be 64 bytes,
					   there is no point in having a large error message. */
					char err_string[32];
					if (saveenv_to_ptn(ptn, &err_string[0]))
					{
						printf("savenv '%s' failed : %s\n", ptn->name, err_string);
						sprintf(response, "FAIL%s", err_string);
					}
					else
					{
						printf("partition '%s' saveenv-ed\n", ptn->name);
						sprintf(response, "OKAY");
					}
				}
				else
				{
					/* Normal case */
					if (write_to_ptn(ptn, (unsigned int)interface.transfer_buffer, download_bytes))
					{
						printf("flashing '%s' failed\n", ptn->name);
						sprintf(response, "FAILfailed to flash partition");
					}
					else
					{
						printf("partition '%s' flashed\n", ptn->name);
						sprintf(response, "OKAY");
					}
				}
			}
			ret = 0;
			goto send_tx_status;
		}

		/* verify */
		/* continue */
		/* powerdown */

		/* oem
		   oem command. */
		if (memcmp(cmdbuf, "oem", 3) == 0)
		{
			sprintf(response,"INFOunknown OEM command");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

			sprintf(response,"OKAY");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

			return 0;
		}

send_tx_status:
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

		LCD_setprogress(0);
	} /* End of command */
	
	return ret;
}

static int check_against_static_partition(struct fastboot_ptentry *ptn)
{
	int ret = 0;
	struct fastboot_ptentry *c;
	int i;

	for (i = 0; i < static_pcount; i++) {
		c = fastboot_flash_get_ptn((unsigned int) i);

		if (0 == ptn->length)
			break;

		if ((ptn->start >= c->start) &&
		    (ptn->start < c->start + c->length))
			break;

		if ((ptn->start + ptn->length > c->start) &&
		    (ptn->start + ptn->length <= c->start + c->length))
			break;

		if ((0 == strcmp(ptn->name, c->name)) &&
		    (0 == strcmp(c->name, ptn->name)))
			break;
	}

	if (i >= static_pcount)
		ret = 1;
	return ret;
}

static unsigned long long memparse(char *ptr, char **retptr)
{
	char *endptr;	/* local pointer to end of parsed string */

	unsigned long ret = simple_strtoul(ptr, &endptr, 0);

	switch (*endptr) {
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		endptr++;
	default:
		break;
	}

	if (retptr)
		*retptr = endptr;

	return ret;
}

static int add_partition_from_environment(char *s, char **retptr)
{
	unsigned long size;
	unsigned long offset = 0;
	char *name;
	int name_len;
	int delim;
	unsigned int flags;
	struct fastboot_ptentry part;

	size = memparse(s, &s);
	if (0 == size) {
		printf("Error:FASTBOOT size of parition is 0\n");
		return 1;
	}

	/* fetch partition name and flags */
	flags = 0; /* this is going to be a regular partition */
	delim = 0;
	/* check for offset */
	if (*s == '@') {
		s++;
		offset = memparse(s, &s);
	} else {
		printf("Error:FASTBOOT offset of parition is not given\n");
		return 1;
	}

	/* now look for name */
	if (*s == '(')
		delim = ')';

	if (delim) {
		char *p;

		name = ++s;
		p = strchr((const char *)name, delim);
		if (!p) {
			printf("Error:FASTBOOT no closing %c found in partition name\n", delim);
			return 1;
		}
		name_len = p - name;
		s = p + 1;
	} else {
		printf("Error:FASTBOOT no partition name for \'%s\'\n", s);
		return 1;
	}

	/* test for options */
	while (1) {
		if (strncmp(s, "i", 1) == 0) {
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_I;
			s += 1;
		} else if (strncmp(s, "yaffs", 5) == 0) {
			/* yaffs */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS;
			s += 5;
		} else if (strncmp(s, "swecc", 5) == 0) {
			/* swecc */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC;
			s += 5;
		} else if (strncmp(s, "hwecc", 5) == 0) {
			/* hwecc */
			flags |= FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC;
			s += 5;
		} else {
			break;
		}
		if (strncmp(s, "|", 1) == 0)
			s += 1;
	}

	/* enter this partition (offset will be calculated later if it is zero at this point) */
	part.length = size;
	part.start = offset;
	part.flags = flags;

	if (name) {
		if (name_len >= sizeof(part.name)) {
			printf("Error:FASTBOOT partition name is too long\n");
			return 1;
		}
		strncpy(&part.name[0], name, name_len);
		/* name is not null terminated */
		part.name[name_len] = '\0';
	} else {
		printf("Error:FASTBOOT no name\n");
		return 1;
	}


	/* Check if this overlaps a static partition */
	if (check_against_static_partition(&part)) {
		printf("Adding: %s, offset 0x%8.8x, size 0x%8.8x, flags 0x%8.8x\n",
		       part.name, part.start, part.length, part.flags);
		fastboot_flash_add_ptn(&part);
	}

	/* return (updated) pointer command line string */
	*retptr = s;

	/* return partition table */
	return 0;
}

static int set_partition_table()
#if defined(CFG_FASTBOOT_ONENANDBSP) || defined(CFG_FASTBOOT_NANDBSP)
{
	char fbparts[4096], *env;

	/*
	 * Place the runtime partitions at the end of the
	 * static paritions.  First save the start off so
	 * it can be saved from run to run.
	 */
	if (static_pcount >= 0)
	{
		/* Reset */
		pcount = static_pcount;
	}
	else
	{
		/* Save */
		static_pcount = pcount;
	}
	env = getenv("fbparts");
	if (env)
	{
		unsigned int len;
		len = strlen(env);
		if (len && len < 4096)
		{
			char *s, *e;

			memcpy(&fbparts[0], env, len + 1);
			printf("Fastboot: Adding partitions from environment\n");
			s = &fbparts[0];
			e = s + len;
			while (s < e)
			{
				if (add_partition_from_environment(s, &s))
				{
					printf("Error:Fastboot: Abort adding partitions\n");
					/* reset back to static */
					pcount = static_pcount;
					break;
				}
				/* Skip a bunch of delimiters */
				while (s < e)
				{
					if ((' ' == *s) ||
					    ('\t' == *s) ||
					    ('\n' == *s) ||
					    ('\r' == *s) ||
					    (',' == *s)) {
						s++;
					}
					else
					{
						break;
					}
				}
			}
		}
	}
	else if (ptable_default_size >= sizeof(fastboot_ptentry))
	{
		printf("Fastboot: employ default partition information\n");
		//memcpy(ptable, ptable_default, ptable_default_size);
		memcpy((void*)ptable, (void*)&ptable_default, ptable_default_size);
		pcount = ptable_default_size / sizeof(fastboot_ptentry);
	}
	else
	{
		printf("No partition informations!");
		return 0;
	}

#if 1 // Debug
	fastboot_flash_dump_ptn();
#endif

	LCD_setleftcolor(0x1024C0);

	return 0;
}
#elif defined(CFG_FASTBOOT_SDMMCBSP)
{
	int start, count;
	unsigned char pid;

	pcount = 0;

#if defined(CONFIG_FUSED)
	/* FW BL1 for fused chip */
	strcpy(ptable[pcount].name, "fwbl1");
	ptable[pcount].start = 0;
	ptable[pcount].length = 0;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MOVI_CMD;
	pcount++;
#endif

	/* Bootloader */
	strcpy(ptable[pcount].name, "bootloader");
	ptable[pcount].start = 0;
	ptable[pcount].length = 0;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MOVI_CMD;
	pcount++;

	/* Kernel */
	strcpy(ptable[pcount].name, "kernel");
	ptable[pcount].start = 0;
	ptable[pcount].length = 0;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MOVI_CMD;
	pcount++;

	/* Ramdisk */
	strcpy(ptable[pcount].name, "ramdisk");
	ptable[pcount].start = 0;
	ptable[pcount].length = 0x300000;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MOVI_CMD;
	pcount++;

	/* System */
	get_mmc_part_info("0", 2, &start, &count, &pid);
	if (pid != 0x83)
		goto part_type_error;
	strcpy(ptable[pcount].name, "system");
	ptable[pcount].start = start * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].length = count * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MMC_CMD;
	pcount++;

	/* Data */
	get_mmc_part_info("0", 3, &start, &count, &pid);
	if (pid != 0x83)
		goto part_type_error;
	strcpy(ptable[pcount].name, "userdata");
	ptable[pcount].start = start * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].length = count * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MMC_CMD;
	pcount++;

	/* Cache */
	get_mmc_part_info("0", 4, &start, &count, &pid);
	if (pid != 0x83)
		goto part_type_error;
	strcpy(ptable[pcount].name, "cache");
	ptable[pcount].start = start * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].length = count * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MMC_CMD;
	pcount++;

	/* fat */
	get_mmc_part_info("0", 1, &start, &count, &pid);
	if (pid != 0xc)
		goto part_type_error;
	strcpy(ptable[pcount].name, "fat");
	ptable[pcount].start = start * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].length = count * CFG_FASTBOOT_SDMMC_BLOCKSIZE;
	ptable[pcount].flags = FASTBOOT_PTENTRY_FLAGS_USE_MMC_CMD;
	pcount++;

#if 1 // Debug
	fastboot_flash_dump_ptn();
#endif

	LCD_setleftcolor(0x8a2be2);

	return 0;

part_type_error:
	printf("Error: No MBR is found at SD/MMC.\n");
	printf("Hint: use fdisk command to make partitions.\n");

	return -1;
}
#else
#error one of CFG_FASTBOOT_ONENANDBSP, CFG_FASTBOOT_NANDBSP or CFG_FASTBOOT_SDMMCBSP should is defined.
#endif


int do_fastboot (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1;
	int check_timeout = 0;
	uint64_t timeout_endtime = 0;
	uint64_t timeout_ticks = 0;
	long timeout_seconds = -1;
	int continue_from_disconnect = 0;

	if (set_partition_table())
		return 1;


	/* Time out */
	if (2 == argc)
	{
		long try_seconds;
		char *try_seconds_end;
		/* Check for timeout */
		try_seconds = simple_strtol(argv[1], &try_seconds_end, 10);
		if ((try_seconds_end != argv[1]) && (try_seconds >= 0))
		{
			check_timeout = 1;
			timeout_seconds = try_seconds;
			printf("Fastboot inactivity timeout %ld seconds\n", timeout_seconds);
		}
	}

	if (1 == check_timeout)
	{
		timeout_ticks = (uint64_t) (timeout_seconds * get_tbclk());
	}


	do
	{
		continue_from_disconnect = 0;

		/* Initialize the board specific support */
		if (0 == fastboot_init(&interface))
		{
			int poll_status;

			/* If we got this far, we are a success */
			ret = 0;

			timeout_endtime = get_ticks();
			timeout_endtime += timeout_ticks;

			LCD_turnon();

			while (1)
			{
				uint64_t current_time = 0;
				poll_status = fastboot_poll();

				if (1 == check_timeout)
					current_time = get_ticks();

				/* Check if the user wanted to terminate with ^C */
				if ((poll_status != FASTBOOT_OK) && (ctrlc()))
				{
					printf("Fastboot ended by user\n");
					continue_from_disconnect = 0;
					break;
				}

				if (FASTBOOT_ERROR == poll_status)
				{
					/* Error */
					printf("Fastboot error \n");
					break;
				}
				else if (FASTBOOT_DISCONNECT == poll_status)
				{
					/* break, cleanup and re-init */
					printf("Fastboot disconnect detected\n");
					continue_from_disconnect = 1;
					break;
				}
				else if ((1 == check_timeout) &&
					   (FASTBOOT_INACTIVE == poll_status))
				{
					/* No activity */
					if (current_time >= timeout_endtime)
					{
						printf("Fastboot inactivity detected\n");
						break;
					}
				}
				else
				{
					/* Something happened */
					/* Actual works of parsing are done by rx_handler */
					if (1 == check_timeout)
					{
						/* Update the timeout endtime */
						timeout_endtime = current_time;
						timeout_endtime += timeout_ticks;
					}
				}
			} /* while (1) */
		}

		/* Reset the board specific support */
		fastboot_shutdown();

		LCD_setfgcolor(0x000010);
		LCD_setleftcolor(0x000010);
		LCD_setprogress(100);

		/* restart the loop if a disconnect was detected */
	} while (continue_from_disconnect);

	return ret;
}

U_BOOT_CMD(
	fastboot,	2,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	"[inactive timeout]\n"
	"    - Run as a fastboot usb device.\n"
	"    - The optional inactive timeout is the decimal seconds before\n"
	"    - the normal console resumes\n"
);


#define CONFIG_FASTBOOT_SDFUSE
#ifdef CONFIG_FASTBOOT_SDFUSE

#include <part.h>
#include <fat.h>
#define CFG_FASTBOOT_SDFUSE_DIR		"/sdfuse"
#ifdef CFG_FASTBOOT_SDMMCBSP
#define CFG_FASTBOOT_SDFUSE_MMCDEV	1
#else
#define CFG_FASTBOOT_SDFUSE_MMCDEV	1//bs change 0 to 1
#endif
#define CFG_FASTBOOT_SDFUSE_MMCPART	1
/*
 * part : partition name (This should be a defined name at ptable)
 * file : file to read
 */
static int update_from_sd (char *part, char *file)
{
	int ret = 1;

	/* Read file */
	if (file != NULL)
	{
		long size;
		unsigned long offset;
		unsigned long count;
		char filename[32];
		block_dev_desc_t *dev_desc=NULL;

		printf("Partition: %s, File: %s/%s\n", part, CFG_FASTBOOT_SDFUSE_DIR, file);
		LCD_setfgcolor(0x2E8B57);
		LCD_setprogress(100);
		dev_desc = get_dev("mmc", CFG_FASTBOOT_SDFUSE_MMCDEV);
		printf("bs dev_desc->dev=%d",dev_desc->dev);
		if (dev_desc == NULL) {
			printf ("** Invalid boot device **\n");
			return 1;
		}
		if (fat_register_device(dev_desc, CFG_FASTBOOT_SDFUSE_MMCPART) != 0) {
			printf ("** Invalid partition **\n");
			return 1;
		}
		sprintf(filename, "%s/%s", CFG_FASTBOOT_SDFUSE_DIR, file);
		offset = CFG_FASTBOOT_TRANSFER_BUFFER;
		count = 0;
		size = file_fat_read (filename, (unsigned char *) offset, count);

		if (size == -1) {
			printf("Failed to read %s\n", filename);
			return 1;
		}

		download_size = 0;	// should be 0
		download_bytes = size;

		printf ("%ld (0x%08x) bytes read\n", size, size);
                if(strcmp(file,"zImage-210")==0)//forlinx.bs
                {
                     download_bytes=(0x500000);//forlinx.bs
                     printf ("%ld (0x%08x) bytes reset read\n", size, size);
                }//forlinx.bs
	}
	else {
		printf("Partition: %s\n", part);

		download_size = 0;	// should be 0
		download_bytes = 0;
	}

	/* Write image into partition */
	/* If file is empty or NULL, just erase the part. */
	{
		char command[32];

		if (download_bytes == 0)
			sprintf(command, "%s:%s", "erase", part);
		else
			sprintf(command, "%s:%s", "flash", part);

		ret = rx_handler(command, sizeof(command));
	}

	return ret;
}

/* SD Fusing : read images from FAT partition of SD Card, and write it to boot device.
 * 
 * NOTE
 * - sdfuse is not a original code of fastboot
 * - Fusing image from SD Card is not a original part of Fastboot protocol.
 * - This command implemented at this file to re-use an existing code of fastboot */
int do_sdfuse (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1;
	int enable_reset = 0;
	struct mmc *mmc = find_mmc_device(CFG_FASTBOOT_SDFUSE_MMCDEV);

	if (mmc_init(mmc)) {
		printf("bs debug sdmmc init is failed.\n");
	}

	interface.nand_block_size	= CFG_FASTBOOT_PAGESIZE * 64;
	interface.transfer_buffer	= (unsigned char *) CFG_FASTBOOT_TRANSFER_BUFFER;
	interface.transfer_buffer_size	= CFG_FASTBOOT_TRANSFER_BUFFER_SIZE;

	printf("[Fusing Image from SD Card.]\n");

	if (set_partition_table())
		return 1;

	if ((argc == 2) && !strcmp(argv[1], "info"))
	{
		printf("sdfuse will read images from the followings:\n");
		printf(" sd/mmc device  : mmc %d:%d\n",
			CFG_FASTBOOT_SDFUSE_MMCDEV, CFG_FASTBOOT_SDFUSE_MMCPART);
		printf(" directory      : %s\n", CFG_FASTBOOT_SDFUSE_DIR);
		printf(" booting device : %s\n",
#if defined(CFG_FASTBOOT_ONENANDBSP)
			"OneNAND"
#elif defined(CFG_FASTBOOT_NANDBSP)
			"NAND"
#elif defined(CFG_FASTBOOT_SDMMCBSP)
			"MoviNAND"
#else
#error "Unknown booting device!"
#endif
#if defined(CONFIG_FUSED)
			" (on eFused Chip)"
#endif
		);
		return 0;
	}
	else if ((argc == 2) && !strcmp(argv[1], "flashall"))
	{
		LCD_turnon();

               if (update_from_sd("bootloader", "u-boot-210.bin"))
			goto err_sdfuse;
               if (update_from_sd("kernel", "zImage-210"))
			goto err_sdfuse;
		//if (update_from_sd("ramdisk", "ramdisk.img"))
	       //	goto err_sdfuse;
		if (update_from_sd("system", "rootfs-210.yaffs2"))
			goto err_sdfuse;
		//if (update_from_sd("userdata", "userdata.img"))
		//	goto err_sdfuse;
		//if (update_from_sd("cache", NULL))
		//	goto err_sdfuse;

		enable_reset = 1;
		ret = 0;
	}
        else if ((argc == 2) && !strcmp(argv[1], "batchwrite"))
	{
	       LCD_turnon();

               if (update_from_sd("bootloader", "u-boot-210.bin"))
			goto err_sdfuse;
               if (update_from_sd("kernel", "zImage-210"))
			goto err_sdfuse;
		//if (update_from_sd("ramdisk", "ramdisk.img"))
	       //	goto err_sdfuse;
		if (update_from_sd("system", "rootfs-210.yaffs2"))
			goto err_sdfuse;
		//if (update_from_sd("userdata", "userdata.img"))
		//	goto err_sdfuse;
		//if (update_from_sd("cache", NULL))
		//	goto err_sdfuse;

		enable_reset = 0;//forlinx.bs
		ret = 0;
	} 
	else if ((argc == 4) && !strcmp(argv[1], "flash"))
	{
		LCD_turnon();

		if (update_from_sd(argv[2], argv[3]))
			goto err_sdfuse;

		ret = 0;
	}
	else if ((argc == 3) && !strcmp(argv[1], "erase"))
	{
		LCD_turnon();

		if (update_from_sd(argv[2], NULL))
			goto err_sdfuse;

		ret = 0;
	}
	else
	{
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}



err_sdfuse:
	LCD_setfgcolor(0x000010);
	LCD_setleftcolor(0x000010);
	LCD_setprogress(100);

	if (enable_reset)
		do_reset (NULL, 0, 0, NULL);

	return ret;
}

U_BOOT_CMD(
	sdfuse,		4,	1,	do_sdfuse,
	"sdfuse  - read images from FAT partition of SD card and write them to booting device.\n",
	"info                             - print primitive infomation.\n"
	"sdfuse flashall                         - flash boot.img, system.img,\n"
	"                                          erase userdata, cache, and reboot.\n"
	"sdfuse flash <partition> [ <filename> ] - write a file to a partition.\n"
	"sdfuse erase <partition>                - erase (format) a partition.\n"
);
#endif	// CONFIG_FASTBOOT_SDFUSE


/*
 * Android style flash utilties */
void fastboot_flash_add_ptn(fastboot_ptentry *ptn)
{
	if (pcount < MAX_PTN)
	{
		memcpy(ptable + pcount, ptn, sizeof(*ptn));
		pcount++;
	}
}

void fastboot_flash_dump_ptn(void)
{
	unsigned int n;

	printf("[Partition table on "
#if defined(CFG_FASTBOOT_ONENANDBSP)
		"OneNAND"
#elif defined(CFG_FASTBOOT_NANDBSP)
		"NAND"
#elif defined(CFG_FASTBOOT_SDMMCBSP)
		"MoviNAND"
#endif
		"]\n");

	for (n = 0; n < pcount; n++)
	{
		fastboot_ptentry *ptn = ptable + n;
#if 0	/* old format - decimal */
		printf("ptn %d name='%s' start=%d len=%d\n",
				n, ptn->name, ptn->start, ptn->length);
#else
		printf("ptn %d name='%s' ", n, ptn->name);
		if (n == 0 || ptn->start)
			printf("start=0x%X ", ptn->start);
		else
			printf("start=N/A ");
		if (ptn->length)
			printf("len=0x%X(~%dKB) ", ptn->length, ptn->length>>10);
		else
			printf("len=N/A ");
		if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS)
			printf("(Yaffs)\n");
        else if(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_UBOOT)
            printf("(Uboot)\n");
        else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_USE_MOVI_CMD)
			printf("(use hard-coded info. (cmd: movi))\n");
		else
			printf("\n");
#endif
	}
}


fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
	unsigned int n;
    
	for (n = 0; n < pcount; n++)
	{
		/* Make sure a substring is not accepted */
		if (strlen(name) == strlen(ptable[n].name))
		{
			if (0 == strcmp(ptable[n].name, name))
				return ptable + n;
		}
	}
	return 0;
}

fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
	if (n < pcount) {
		return ptable + n;
	} else {
		return 0;
	}
}

unsigned int fastboot_flash_get_ptn_count(void)
{
	return pcount;
}



#endif	/* CONFIG_FASTBOOT */
