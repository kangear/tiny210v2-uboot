/*
 */



#include <common.h>
#include <command.h>
#include <nand.h>

#ifdef CONFIG_CMD_MENU



extern char console_buffer[];
extern int readline (const char *const prompt);



#define USE_USB_DOWN		1


void main_menu_usage(char menu_type)
{

	printf("\r\n#####	 Boot for S5PV210 Main Menu	#####\r\n");

	if( menu_type == USE_USB_DOWN)
	{
		printf("#####    S5PV210 USB download mode     #####\r\n\n");
	}
	printf("[1] Download Program/Bootloader to Nand Flash\r\n");
	printf("[2] Download Linux Kernel uImage to Nand Flash\r\n");
	printf("[3] Download YAFFS2 image rootfs.img to Nand Flash\r\n");
	printf("[4] Download Program to SDRAM and Run it\r\n");
	printf("[5] Boot the system\r\n");
	printf("[6] Format the Nand Flash\r\n");
	printf("[q] Return main Menu \r\n");
	printf("Enter your selection: ");
}






void menu_shell(void)
{
	char keyselect;
	char cmd_buf[200];

	while (1)
	{
		main_menu_usage(USE_USB_DOWN);
		keyselect = getc();
		printf("%c\n", keyselect);
		switch (keyselect)
		{
			case '1':
			{

				strcpy(cmd_buf, "dnw 0x21000000; nand erase 0x0 0x80000; nand write 0x21000000 0x0 0x80000");

				run_command(cmd_buf, 0);
				break;
			}
			
			

			case '2':
			{

				strcpy(cmd_buf, "dnw 0x21000000; nand erase 0x400000 0x500000; nand write 0x21000000 0x300000 0x500000");

				run_command(cmd_buf, 0);
				break;
			}


			case '3':
			{
//#ifdef CONFIG_MTD_DEVICE
//				strcpy(cmd_buf, "dnw 0x21000000; nand erase root; nand write.yaffs 0x21000000 root $(filesize)");
//#else
				strcpy(cmd_buf, "dnw 0x21000000; nand erase 0xe00000 0xF8D0000; nand write.yaffs 0x21000000 0xe00000 $(filesize)");
//#endif /* CONFIG_MTD_DEVICE */
				run_command(cmd_buf, 0);
				break;
			}

			case '4':
			{
				char addr_buff[12];
				printf("Enter download address:(eg: 0x21000000)\n");
				readline(NULL);
				strcpy(addr_buff,console_buffer);
				sprintf(cmd_buf, "dnw %s;go %s", addr_buff, addr_buff);
				run_command(cmd_buf, 0);
				break;
			}

			case '5':
			{

				printf("Start Linux ...\n");
//#ifdef CONFIG_MTD_DEVICE
//				strcpy(cmd_buf, "nand read 0x21000000 kernel;bootm 0x21000000");
//#else
				strcpy(cmd_buf, "nand read 0x21000000 0x300000 0x500000;bootm 0x21000000");
//#endif /* CONFIG_MTD_DEVICE */
				run_command(cmd_buf, 0);
				break;
			}

			case '6':
			{
				strcpy(cmd_buf, "nand erase 0 ");
				run_command(cmd_buf, 0);
				break;
			}

			
#ifdef CONFIG_TINY210
			case 'Q':
			case 'q':
			{
				return;	
				break;
			}
#endif
		}
				
	}
}

int do_menu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	menu_shell();
	return 0;
}

U_BOOT_CMD(
	menu,	3,	0,	do_menu,
	"display a menu, to select the items to do something",
	"\n"
	"\tdisplay a menu, to select the items to do something"
);

#endif

