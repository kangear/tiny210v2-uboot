/*
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * Platform dependant code for Fastboot
 *
 * Base code of USB connection part is usbd-otg-hs.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/types.h>
#include <environment.h>
#include <command.h>
#include <fastboot.h>

#include <asm/errno.h>
#include <regs.h>
#include "usbd-otg-hs.h"

#if defined(CONFIG_FASTBOOT)

#define USB_FASTBOOT_TEST

/* S5PC110 Default Partition Table */
fastboot_ptentry ptable_default[] =
{
    #ifdef CONFIG_TINY210   //linux
        {
            .name     = "bootloader",
            .start    = 0x0,
            .length   = 0x100000,
            .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_UBOOT
        },
        {
            .name     = "kernel",
            .start    = 0x100000,
            .length   = 0x500000,
            .flags    = 0
        },
        {
            .name     = "system",
            .start    = 0x600000,
            .length   = 0,
            .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS
        },
    
    #elif defined(CONFIG_ANDROID_FORLINX)
        {
            .name     = "bootloader",
            .start    = 0x0,
            .length   = 0x100000,
            .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_UBOOT
        },
        {
            .name     = "kernel",
            .start    = 0x100000,
            .length   = 0x500000,
            .flags    = 0
        },
        {
            .name     = "system",
            .start    = 0x600000,
            .length   = 0,
            .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS
        },
      /*    {
                    .name     = "bootloader",
                    .start    = 0x0,
                    .length   = 0x100000, //1M
                    .flags    = 0
            },
            {
                    .name     = "recovery",
                    .start    = 0x100000,
                    .length   = 0x500000, //5M
                    .flags    = 0
            },
            {
                    .name     = "kernel",
                    .start    = 0x600000,
                    .length   = 0x500000,  //5M
                    .flags    = 0
            },
            {
                    .name     = "ramdisk",
                    .start    = 0xB00000,
                    .length   = 0x500000, //5M
                    .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS
            },
            {
                    .name     = "system",
                    .start    = 0x1000000,
                    .length   = 0xA000000,  //160M
                    .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS
            },
            {
                    .name     = "cache",
                    .start    = 0xB000000,
                    .length   = 0x2800000, //40M
                    .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS
            },
            {
                    .name     = "userdata",
                    .start    = 0xD800000,
                    .length   = 0,          //34M
                    .flags    = FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS
            }
        *///forlinx
     #else
        {
           #error  "Sorry, You must config linux or android for s5pv210 board\n";
        }

    #endif

};

unsigned int ptable_default_size = sizeof(ptable_default);

#define FBOOT_USBD_IS_CONNECTED() (readl(S5P_OTG_GOTGCTL)&(B_SESSION_VALID|A_SESSION_VALID))
#define FBOOT_USBD_DETECT_IRQ() (readl(S5P_OTG_GINTSTS) & \
					(GINTSTS_WkUpInt|GINTSTS_OEPInt|GINTSTS_IEPInt| \
					 GINTSTS_EnumDone|GINTSTS_USBRst|GINTSTS_USBSusp|GINTSTS_RXFLvl))
#define FBOOT_USBD_CLEAR_IRQ()  do { \
					writel(BIT_ALLMSK, (S5P_OTG_GINTSTS)); \
				} while (0)

#define B_SESSION_VALID		(0x1<<19)
#define A_SESSION_VALID		(0x1<<18)


#define VENDOR_ID 	0x18D1 
#define PRODUCT_ID	0x0002
#define FB_PKT_SZ	64 // full-speed mode
#define OK	0
#define ERROR	-1
#define MIN(x, y)		((x < y) ? x : y)
/* In high speed mode packets are 512
   In full speed mode packets are 64 */
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)//512
#define RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)// 64
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0  (0x0200)
#define TX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1  (0x0040)

#if defined(CONFIG_S5PC100)
 #include <s5pc100.h>
#elif defined(CONFIG_S5PC110)
 #include <s5pc110.h>
#else
#error "* CFG_ERROR : you have to select C100 or C110 for Android Fastboot"
#endif



//#define FASTBOOT_DEBUG
#undef FASTBOOT_DEBUG

#ifdef FASTBOOT_DEBUG
	#define DBG0(fmt, args...) printf("[FB] [%s:%d] " fmt, __FUNCTION__, __LINE__, ##args)
	#define DBG1(fmt, args...) printf("\t" fmt, ##args)
	#define DBG2(fmt, args...) printf("\t\t" fmt, ##args)
#else
	#define DBG0(fmt, args...) do { } while (0)
	#define DBG1(fmt, args...) do { } while (0)
	#define DBG2(fmt, args...) do { } while (0)
#endif

//#undef USB_OTG_DEBUG_SETUP
//#define USB_OTG_DEBUG_SETUP

#ifdef USB_OTG_DEBUG_SETUP
#define DBG_SETUP0(fmt, args...) printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##args)
#define DBG_SETUP1(fmt, args...) printf("\t" fmt, ##args)
#define DBG_SETUP2(fmt, args...) printf(fmt, ##args)
#else
#define DBG_SETUP0(fmt, args...) do { } while (0)
#define DBG_SETUP1(fmt, args...) do { } while (0)
#define DBG_SETUP2(fmt, args...) do { } while (0)
#endif

//#undef USB_OTG_DEBUG_BULK
//#define USB_OTG_DEBUG_BULK

#ifdef USB_OTG_DEBUG_BULK
#define DBG_BULK0(fmt, args...) printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##args)
#define DBG_BULK1(fmt, args...)	printf("\t" fmt, ##args)
#else
#define DBG_BULK0(fmt, args...) do { } while (0)
#define DBG_BULK1(fmt, args...) do { } while (0)
#endif

#define USB_CHECKSUM_EN

/* String 0 is the language id */
#define DEVICE_STRING_MANUFACTURER_INDEX  1
#define DEVICE_STRING_PRODUCT_INDEX       2
#define DEVICE_STRING_SERIAL_NUMBER_INDEX 3
#define DEVICE_STRING_CONFIG_INDEX        4
#define DEVICE_STRING_INTERFACE_INDEX     5
#define DEVICE_STRING_MAX_INDEX           DEVICE_STRING_MANUFACTURER_INDEX
#define DEVICE_STRING_LANGUAGE_ID         0x0409 /* English (United States) */


#define TRUE	1
#define FALSE	0
#define SUSPEND_RESUME_ON FALSE

static char *device_strings[DEVICE_STRING_MANUFACTURER_INDEX+1];
static struct cmd_fastboot_interface *fastboot_interface = NULL;
/* The packet size is dependend of the speed mode
   In high speed mode packets are 512
   In full speed mode packets are 64
   Set to maximum of 512 */

/* Note: The start address must be double word aligned */
static u8 fastboot_bulk_fifo[0x0200+1]; //__attribute__ ((aligned(0x4))); 
const char* reply_msg;
unsigned int transfer_size;
u32 fboot_response_flag=0;

u32 fboot_usbd_dn_addr = 0;
u32 fboot_usbd_dn_cnt = 0;
u32 fboot_remode_wakeup;
u16 fboot_config_value;

int fboot_receive_done = 0;
int fboot_got_header = 0;

USB_OPMODE	fboot_op_mode = USB_CPU;
USB_SPEED	fboot_speed = USB_HIGH;
//USB_SPEED	fboot_speed = USB_FULL;

#ifdef USB_FASTBOOT_TEST
u8* tx_msg_buf = NULL;
#endif
otg_dev_t	fboot_otg;
get_status_t	fboot_get_status;
get_intf_t	fboot_get_intf;

enum EP_INDEX
{
	EP0, EP1, EP2, EP3, EP4
};

/*------------------------------------------------*/
/* EP0 state */
enum EP0_STATE
{
	EP0_STATE_INIT			= 0,
	EP0_STATE_GD_DEV_0		= 11,
	EP0_STATE_GD_DEV_1		= 12,
	EP0_STATE_GD_DEV_2		= 13,
	EP0_STATE_GD_CFG_0		= 21,
	EP0_STATE_GD_CFG_1		= 22,
	EP0_STATE_GD_CFG_2		= 23,
	EP0_STATE_GD_CFG_3		= 24,
	EP0_STATE_GD_CFG_4		= 25,
	EP0_STATE_GD_STR_I0		= 30,
	EP0_STATE_GD_STR_I1		= 31,
	EP0_STATE_GD_STR_I2		= 32,
	EP0_STATE_GD_STR_I3		= 133,
	EP0_STATE_GD_DEV_QUALIFIER	= 33,
	EP0_STATE_INTERFACE_GET		= 34,
	EP0_STATE_GET_STATUS0		= 35,
	EP0_STATE_GET_STATUS1		= 36,
	EP0_STATE_GET_STATUS2		= 37,
	EP0_STATE_GET_STATUS3		= 38,
	EP0_STATE_GET_STATUS4		= 39,
	EP0_STATE_GD_OTHER_SPEED	= 40,
	EP0_STATE_GD_CFG_ONLY_0 	= 41,
	EP0_STATE_GD_CFG_ONLY_1 	= 42,
	EP0_STATE_GD_IF_ONLY_0		= 44,
	EP0_STATE_GD_IF_ONLY_1		= 45,
	EP0_STATE_GD_EP0_ONLY_0 	= 46,
	EP0_STATE_GD_EP1_ONLY_0 	= 47,
	EP0_STATE_GD_EP2_ONLY_0 	= 48,
	EP0_STATE_GD_EP3_ONLY_0 	= 49,
	EP0_STATE_GD_OTHER_SPEED_HIGH_1	= 51,
	EP0_STATE_GD_OTHER_SPEED_HIGH_2	= 52,
	EP0_STATE_GD_OTHER_SPEED_HIGH_3	= 53
};

/*definitions related to CSR setting */


/* S5P_OTG_GAHBCFG*/
#define PTXFE_HALF		(0<<8)
#define PTXFE_ZERO		(1<<8)
#define NPTXFE_HALF		(0<<7)
#define NPTXFE_ZERO		(1<<7)
#define MODE_SLAVE		(0<<5)
#define MODE_DMA		(1<<5)
#define BURST_SINGLE		(0<<1)
#define BURST_INCR		(1<<1)
#define BURST_INCR4		(3<<1)
#define BURST_INCR8		(5<<1)
#define BURST_INCR16		(7<<1)
#define GBL_INT_UNMASK		(1<<0)
#define GBL_INT_MASK		(0<<0)

/* S5P_OTG_GRSTCTL*/
#define AHB_MASTER_IDLE		(1u<<31)
#define CORE_SOFT_RESET		(0x1<<0)

/* S5P_OTG_GINTSTS/S5P_OTG_GINTMSK core interrupt register */
#define INT_RESUME		(1u<<31)
#define INT_DISCONN		(0x1<<29)
#define INT_CONN_ID_STS_CNG	(0x1<<28)
#define INT_HOST_CH		(0x1<<25)
#define INT_OUT_EP		(0x1<<19)
#define INT_IN_EP		(0x1<<18)
#define INT_ENUMDONE		(0x1<<13)
#define INT_RESET		(0x1<<12)
#define INT_SUSPEND		(0x1<<11)
#define INT_TX_FIFO_EMPTY	(0x1<<5)
#define INT_RX_FIFO_NOT_EMPTY	(0x1<<4)
#define INT_SOF			(0x1<<3)
#define INT_DEV_MODE		(0x0<<0)
#define INT_HOST_MODE		(0x1<<1)

/* S5P_OTG_GRXSTSP STATUS*/
#define GLOBAL_OUT_NAK			(0x1<<17)
#define OUT_PKT_RECEIVED		(0x2<<17)
#define OUT_TRNASFER_COMPLETED		(0x3<<17)
#define SETUP_TRANSACTION_COMPLETED	(0x4<<17)
#define SETUP_PKT_RECEIVED		(0x6<<17)

/* S5P_OTG_DCTL device control register */
#define NORMAL_OPERATION		(0x1<<0)
#define SOFT_DISCONNECT			(0x1<<1)
#define	TEST_J_MODE			(TEST_J<<4)
#define	TEST_K_MODE			(TEST_K<<4)
#define	TEST_SE0_NAK_MODE		(TEST_SE0_NAK<<4)
#define	TEST_PACKET_MODE		(TEST_PACKET<<4)
#define	TEST_FORCE_ENABLE_MODE		(TEST_FORCE_ENABLE<<4)
#define TEST_CONTROL_FIELD		(0x7<<4)

/* S5P_OTG_DAINT device all endpoint interrupt register */
#define INT_IN_EP0			(0x1<<0)
#define INT_IN_EP1			(0x1<<1)
#define INT_IN_EP3			(0x1<<3)
#define INT_OUT_EP0			(0x1<<16)
#define INT_OUT_EP2			(0x1<<18)
#define INT_OUT_EP4			(0x1<<20)

/* S5P_OTG_DIEPCTL0/S5P_OTG_DOEPCTL0 */
#define DEPCTL_EPENA			(0x1<<31)
#define DEPCTL_EPDIS			(0x1<<30)
#define DEPCTL_SNAK			(0x1<<27)
#define DEPCTL_CNAK			(0x1<<26)
#define DEPCTL_CTRL_TYPE		(EP_TYPE_CONTROL<<18)
#define DEPCTL_ISO_TYPE			(EP_TYPE_ISOCHRONOUS<<18)
#define DEPCTL_BULK_TYPE		(EP_TYPE_BULK<<18)
#define DEPCTL_INTR_TYPE		(EP_TYPE_INTERRUPT<<18)
#define DEPCTL_USBACTEP			(0x1<<15)

/*ep0 enable, clear nak, next ep0, max 64byte */
#define EPEN_CNAK_EP0_64 (DEPCTL_EPENA|DEPCTL_CNAK|(CONTROL_EP<<11)|(0<<0))

/*ep0 enable, clear nak, next ep0, 8byte */
#define EPEN_CNAK_EP0_8 (DEPCTL_EPENA|DEPCTL_CNAK|(CONTROL_EP<<11)|(3<<0))

/* DIEPCTLn/DOEPCTLn */
#define BACK2BACK_SETUP_RECEIVED	(0x1<<6)
#define INTKN_TXFEMP			(0x1<<4)
#define NON_ISO_IN_EP_TIMEOUT		(0x1<<3)
#define CTRL_OUT_EP_SETUP_PHASE_DONE	(0x1<<3)
#define AHB_ERROR			(0x1<<2)
#define TRANSFER_DONE			(0x1<<0)


/* codes representing languages */
const u8 fboot_string_desc0[] =
{
	4, STRING_DESCRIPTOR, LANGID_US_L, LANGID_US_H,
};
const u8 fboot_string_desc1[] = /* Manufacturer */
{
#ifdef USB_FASTBOOT_TEST
	(0x16+2), STRING_DESCRIPTOR,
	'G', 0x0, 'o', 0x0, 'o', 0x0, 'g', 0x0, 'l', 0x0,
	'e', 0x0, ',', 0x0, ' ', 0x0, 'I', 0x0, 'n', 0x0,
	'c', 0x0
#else
	(0x14+2), STRING_DESCRIPTOR,
	'S', 0x0, 'y', 0x0, 's', 0x0, 't', 0x0, 'e', 0x0,
	'm', 0x0, ' ', 0x0, 'M', 0x0, 'C', 0x0, 'U', 0x0,
#endif
};

const u8 fboot_string_desc2[] = /* Product */
{
#ifdef USB_FASTBOOT_TEST
	(0x16+2), STRING_DESCRIPTOR,
	'A', 0x0, 'n', 0x0, 'd', 0x0, 'r', 0x0, 'o', 0x0,
	'i', 0x0, 'd', 0x0, ' ', 0x0, '1', 0x0, '.', 0x0,
	'0', 0x0
#else
	(0x2a+2), STRING_DESCRIPTOR,
	'S', 0x0, 'E', 0x0, 'C', 0x0, ' ', 0x0, 'S', 0x0,
	'3', 0x0, 'C', 0x0, '6', 0x0, '4', 0x0, '0', 0x0,
	'0', 0x0, 'X', 0x0, ' ', 0x0, 'T', 0x0, 'e', 0x0,
	's', 0x0, 't', 0x0, ' ', 0x0, 'B', 0x0, '/', 0x0,
	'D', 0x0
#endif
};

const u8 string_desc3[] = /* Test Serial ID */
{
	(0x16+2), STRING_DESCRIPTOR,
	'S', 0x0, 'M', 0x0, 'D', 0x0, 'K', 0x0, 'C', 0x0,
	'1', 0x0, '1', 0x0, '0', 0x0, '-', 0x0, '0', 0x0,
	'1', 0x0
};


/* setting the device qualifier descriptor and a string descriptor */
const u8 fboot_qualifier_desc[] =
{
#ifdef USB_FASTBOOT_TEST
	0x0a,	/*  0 desc size */
	0x06,	/*  1 desc type (DEVICE_QUALIFIER)*/
	0x00,	/*  2 USB release */
	0x02,	/*  3 => 2.00*/
	0xFF,	/*  4 class */
	0x42,	/*  5 subclass */
	0x03,	/*  6 protocol */
	64,	/*  7 max pack size */
	0x01,	/*  8 number of other-speed configuration */
	0x00,	/*  9 reserved */
#else
	0x0a,	/*  0 desc size */
	0x06,	/*  1 desc type (DEVICE_QUALIFIER)*/
	0x00,	/*  2 USB release */
	0x02,	/*  3 => 2.00*/
	0xFF,	/*  4 class */
	0x00,	/*  5 subclass */
	0x00,	/*  6 protocol */
	64,	/*  7 max pack size */
	0x01,	/*  8 number of other-speed configuration */
	0x00,	/*  9 reserved */
#endif
};

const u8 fboot_config_full[] =
{
	0x09,	/*  0 desc size */
	0x07,	/*  1 desc type (other speed)*/
	0x20,	/*  2 Total length of data returned */
	0x00,	/*  3 */
	0x01,	/*  4 Number of interfaces supported by this speed configuration */
	0x01,	/*  5 value to use to select configuration */
	0x00,	/*  6 index of string desc */
		/*  7 same as configuration desc */
	CONF_ATTR_DEFAULT|CONF_ATTR_SELFPOWERED,
	0x19,	/*  8 same as configuration desc */

};

const u8 fboot_config_full_total[] =
{
  0x09, 0x07 ,0x20 ,0x00 ,0x01 ,0x01 ,0x00 ,0xC0 ,0x19,
  0x09 ,0x04 ,0x00 ,0x00 ,0x02 ,0xff ,0x00 ,0x00 ,0x00,
  0x07 ,0x05 ,0x83 ,0x02 ,0x40 ,0x00 ,0x00,
  0x07 ,0x05 ,0x04 ,0x02 ,0x40 ,0x00 ,0x00
};

const u8 fboot_config_high[] =
{
	0x09,	/*  0 desc size */
	0x07,	/*  1 desc type (other speed)*/
	0x20,	/*  2 Total length of data returned */
	0x00,	/*  3 */
	0x01,	/*  4 Number of interfaces supported by this speed configuration */
	0x01,	/*  5 value to use to select configuration */
	0x00,	/*  6 index of string desc */
		/*  7 same as configuration desc */
	CONF_ATTR_DEFAULT|CONF_ATTR_SELFPOWERED,
	0x19,	/*  8 same as configuration desc */

};

const u8 fboot_config_high_total[] =
{
  0x09, 0x07 ,0x20 ,0x00 ,0x01 ,0x01 ,0x00 ,0xC0 ,0x19,
  0x09 ,0x04 ,0x00 ,0x00 ,0x02 ,0xff ,0x00 ,0x00 ,0x00,
  0x07 ,0x05 ,0x81 ,0x02 ,0x00 ,0x02 ,0x00,
  0x07 ,0x05 ,0x02 ,0x02 ,0x00 ,0x02 ,0x00
};

/* Descriptor size */
enum DESCRIPTOR_SIZE
{
	DEVICE_DESC_SIZE	= sizeof(device_desc_t),
	STRING_DESC0_SIZE	= sizeof(fboot_string_desc0),
	STRING_DESC1_SIZE	= sizeof(fboot_string_desc1),
	STRING_DESC2_SIZE	= sizeof(fboot_string_desc2),
	STRING_DESC3_SIZE	= sizeof(string_desc3),
	CONFIG_DESC_SIZE	= sizeof(config_desc_t),
	INTERFACE_DESC_SIZE	= sizeof(intf_desc_t),
	ENDPOINT_DESC_SIZE	= sizeof(ep_desc_t),
	DEVICE_QUALIFIER_SIZE	= sizeof(fboot_qualifier_desc),
	OTHER_SPEED_CFG_SIZE	= 9

};

/*32 <cfg desc>+<if desc>+<endp0 desc>+<endp1 desc>*/
#define CONFIG_DESC_TOTAL_SIZE	\
	(CONFIG_DESC_SIZE+INTERFACE_DESC_SIZE+ENDPOINT_DESC_SIZE*2)
#define FBOOT_TEST_PKT_SIZE 53

u8 fboot_test_pkt [FBOOT_TEST_PKT_SIZE] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	/*JKJKJKJK x 9*/
	0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,	/*JJKKJJKK x 8*/
	0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,	/*JJJJKKKK x 8*/
	0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,	/*JJJJJJJKKKKKKK x8 - '1'*/
	0x7F,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,		/*'1' + JJJJJJJK x 8*/
	0xFC,0x7E,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,0x7E	/*{JKKKKKKK x 10},JK*/
};

void fboot_usb_init_phy(void)
{
	DBG0("\n");
#if defined(CONFIG_S5PC110)
	writel(0xa0, S5P_OTG_PHYPWR);
	writel(0x3, S5P_OTG_PHYCLK);
#elif defined(CONFIG_S5PC100)
	writel(0x0, S5P_OTG_PHYPWR);
	writel(0x22, S5P_OTG_PHYCLK);
#else
#error "* CFG_ERROR : you have to select C100 or C110 for Android Fastboot"
#endif

	writel(0x1, S5P_OTG_RSTCON);
	udelay(10);
	writel(0x0, S5P_OTG_RSTCON);
	udelay(10);
}

/* OTG PHY Power Off */
void fboot_usb_phy_off(void) {
#if defined(CONFIG_S5PC110)
	writel(readl(S5P_OTG_PHYPWR)|(0x18), S5P_OTG_PHYPWR);
	writel(readl(USB_PHY_CONTROL)&~(1<<0), USB_PHY_CONTROL);
#elif defined(CONFIG_S5PC100)
	writel(readl(S5P_OTG_PHYPWR)|(0x3<<3), S5P_OTG_PHYPWR);
	OTHERS_REG &= ~(1<<16);
#else
#error "* CFG_ERROR : you have to select C100 or C110 for Android Fastboot"
#endif
}


void fboot_usb_core_soft_reset(void)
{
	u32 tmp;
	DBG0("\n");

	writel(CORE_SOFT_RESET, S5P_OTG_GRSTCTL);

	do
	{
		tmp = readl(S5P_OTG_GRSTCTL);
	}while(!(tmp & AHB_MASTER_IDLE));

}

void fboot_usb_wait_cable_insert(void)
{
	u32 tmp;
	int ucFirst=1;

	DBG0("\n");
	do {
		udelay(50);

		tmp = readl(S5P_OTG_GOTGCTL);

		if (tmp & (B_SESSION_VALID|A_SESSION_VALID)) {
			DBG0("OTG cable Connected!\n");
			break;
		} else if(ucFirst == 1) {
			printf("Insert a OTG cable into the connector!\n");
			ucFirst = 0;
		}
	} while(1);
}

void fboot_usb_init_core(void)
{
	DBG0("\n");
	writel(PTXFE_HALF|NPTXFE_HALF|MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK,
		S5P_OTG_GAHBCFG);

	writel(  0<<15		/* PHY Low Power Clock sel */
		|1<<14		/* Non-Periodic TxFIFO Rewind Enable */
		|0x5<<10	/* Turnaround time */
		|0<<9		/* 0:HNP disable, 1:HNP enable */
		|0<<8		/* 0:SRP disable, 1:SRP enable */
		|0<<7		/* ULPI DDR sel */
		|0<<6		/* 0: high speed utmi+, 1: full speed serial */
		|0<<4		/* 0: utmi+, 1:ulpi */
		|1<<3		/* phy i/f  0:8bit, 1:16bit */
		|0x7<<0,	/* HS/FS Timeout**/
		S5P_OTG_GUSBCFG );
}

void fboot_usb_check_current_mode(u8 *pucMode)
{
	DBG0("\n");
	u32 tmp;

	tmp = readl(S5P_OTG_GINTSTS);
	*pucMode = tmp & 0x1;
}

void fboot_usb_set_soft_disconnect(void)
{
	DBG0("\n");
	u32 tmp;

	tmp = readl(S5P_OTG_DCTL);
	tmp |= SOFT_DISCONNECT;
	writel(tmp, S5P_OTG_DCTL);
}

void fboot_usb_clear_soft_disconnect(void)
{
	DBG0("\n");
	u32 tmp;

	tmp = readl(S5P_OTG_DCTL);
	tmp &= ~SOFT_DISCONNECT;
	writel(tmp, S5P_OTG_DCTL);
}

void fboot_usb_init_device(void)
{
	DBG0("\n");
	writel(1<<18|fboot_otg.speed<<0, S5P_OTG_DCFG); /* [][1: full speed(30Mhz) 0:high speed]*/

#ifdef USB_FASTBOOT_TEST
	writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|
		INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY| INT_HOST_CH,
		S5P_OTG_GINTMSK);	/*gint unmask */
#else
	writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|
		INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY,
		S5P_OTG_GINTMSK);	/*gint unmask */
#endif
}

int fboot_usbctl_init(void)
{
	u8 ucMode;
	DBG0("\n");

	DBG_SETUP0("USB Control Init\n");
#if defined(CONFIG_S5PC110)
	writel(readl(USB_PHY_CONTROL)|(1<<0), USB_PHY_CONTROL);	/*USB PHY0 Enable */ // c110
#elif defined(CONFIG_S5PC100)
	OTHERS_REG |= (1<<16);	/*unmask usb signal */
#else
#error "* CFG_ERROR : you have to select C100 or C110 for Android Fastboot"
#endif

	fboot_otg.speed = fboot_speed;
	fboot_otg.set_config = 0;
	fboot_otg.ep0_state = EP0_STATE_INIT;
	fboot_otg.ep0_substate = 0;
	fboot_usb_init_phy();
	fboot_usb_core_soft_reset();
	fboot_usb_wait_cable_insert();
	fboot_usb_init_core();
	fboot_usb_check_current_mode(&ucMode);

	if (ucMode == INT_DEV_MODE) {
		fboot_usb_set_soft_disconnect();
		udelay(10);
		fboot_usb_clear_soft_disconnect();
		fboot_usb_init_device();
		return 0;
	} else {
		printf("Error : Current Mode is Host\n");
		return 0;
	}
}

int fboot_usbc_activate (void)
{
	DBG0("\n");
	/* dont used in usb high speed, but used in common file cmd_usbd.c  */
	return 0;
}

int fboot_usb_stop (void)
{
	DBG0("\n");
	/* dont used in usb high speed, but used in common file cmd_usbd.c  */
	fboot_usb_core_soft_reset();
	fboot_usb_phy_off();
	return 0;
}

void fboot_usb_print_pkt(u8 *pt, u8 count)
{
	int i;
	DBG0("\n");

	for(i=0;i<count;i++)
		printf("%x,", pt[i]);

	printf("]\n");
}

void fboot_usb_verify_checksum(void)
{
	u8 *cs_start, *cs_end;
	u16 dnCS;
	u16 checkSum;

	DBG0("\n");

	/* checksum calculation */
	cs_start = (u8*)fboot_otg.dn_addr;
	cs_end = (u8*)(fboot_otg.dn_addr+fboot_otg.dn_filesize-10);
	checkSum = 0;
	while(cs_start < cs_end) {
		checkSum += *cs_start++;
		if(((u32)cs_start&0xfffff)==0) printf(".");
	}

#if defined(CONFIG_S5PC110)
   #if 1	// fixed alignment fault in case when cs_end is odd.
	dnCS = (u16)((cs_end[1]<<8) + cs_end[0]);
   #else
	dnCS = *(u16 *)cs_end;
   #endif
#elif defined(CONFIG_S5PC100)
	dnCS = *(u16 *)cs_end;
#else
#error "* CFG_ERROR : you have to select C100 or C110 for Android Fastboot"
#endif


	if (checkSum == dnCS)
	{
		printf("\nChecksum O.K.\n");
	}
	else
	{
		printf("\nChecksum Value => MEM:%x DNW:%x\n",checkSum,dnCS);
		printf("Checksum failed.\n\n");
	}
}

void fboot_usb_set_inep_xfersize(EP_TYPE type, u32 pktcnt, u32 xfersize)
{
	DBG0("\n");
	if(type == EP_TYPE_CONTROL)
	{
		writel((pktcnt<<19)|(xfersize<<0), S5P_OTG_DIEPTSIZ0);
	}
	else if(type == EP_TYPE_BULK)
	{
		writel((1<<29)|(pktcnt<<19)|(xfersize<<0), S5P_OTG_DIEPTSIZ_IN);
	}
}

void fboot_usb_set_outep_xfersize(EP_TYPE type, u32 pktcnt, u32 xfersize)
{
	DBG0("\n");
	if(type == EP_TYPE_CONTROL)
	{
		writel((1<<29)|(pktcnt<<19)|(xfersize<<0), S5P_OTG_DOEPTSIZ0);
	}
	else if(type == EP_TYPE_BULK)
	{
		writel((pktcnt<<19)|(xfersize<<0), S5P_OTG_DOEPTSIZ_OUT);
	}
}

void fboot_usb_write_ep0_fifo(u8 *buf, int num)
{
	int i;
	u32 Wr_Data=0;

	DBG0("\n");
	DBG_SETUP1("[fboot_usb_write_ep0_fifo:");

	for(i=0;i<num;i+=4)
	{
		Wr_Data = ((*(buf+3))<<24)|((*(buf+2))<<16)|((*(buf+1))<<8)|*buf;
		DBG_SETUP2(" 0x%08x,", Wr_Data);
		writel(Wr_Data, S5P_OTG_EP0_FIFO);
		buf += 4;
	}

	DBG_SETUP2("]\n");
}


void fboot_usb_write_in_fifo(u8 *buf, int num)
{
	int i;
	u32 data=0;

	DBG0("\n");
	for(i=0;i<num;i+=4)
	{
		data=((*(buf+3))<<24)|((*(buf+2))<<16)|((*(buf+1))<<8)|*buf;
		writel(data, S5P_OTG_IN_FIFO);
		buf += 4;
	}
}

void fboot_usb_read_out_fifo(u8 *buf, int num)
{
	int i;
	u32 data;

	DBG0("\n");
	for (i=0;i<num;i+=4)
	{
		data = readl(S5P_OTG_OUT_FIFO);

		buf[i] = (u8)data;
		buf[i+1] = (u8)(data>>8);
		buf[i+2] = (u8)(data>>16);
		buf[i+3] = (u8)(data>>24);
	}
}

//#define DBG_SETUP1(fmt, args...) printf("\t" fmt, ##args)
void fboot_usb_get_desc(void)
{
	DBG0("\n");
	switch (fboot_otg.dev_req.wValue_H) {
	case DEVICE_DESCRIPTOR:
		fboot_otg.req_length = (u32)((fboot_otg.dev_req.wLength_H << 8) |
			fboot_otg.dev_req.wLength_L);
		DBG_SETUP1("DEVICE_DESCRIPTOR = 0x%x \n",fboot_otg.req_length);
		fboot_otg.ep0_state = EP0_STATE_GD_DEV_0;
		break;

	case CONFIGURATION_DESCRIPTOR:
		fboot_otg.req_length = (u32)((fboot_otg.dev_req.wLength_H << 8) |
			fboot_otg.dev_req.wLength_L);
		DBG_SETUP1("CONFIGURATION_DESCRIPTOR = 0x%x \n",fboot_otg.req_length);

		/* GET_DESCRIPTOR:CONFIGURATION+INTERFACE+ENDPOINT0+ENDPOINT1 */
		if (fboot_otg.req_length > CONFIG_DESC_SIZE){
			fboot_otg.ep0_state = EP0_STATE_GD_CFG_0;
		} else
			fboot_otg.ep0_state = EP0_STATE_GD_CFG_ONLY_0;
		break;

	case STRING_DESCRIPTOR :
		DBG_SETUP1("STRING_DESCRIPTOR \n");

		switch(fboot_otg.dev_req.wValue_L) {
		case 0:
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I0;
			break;
		case 1:
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I1;
			break;
		case 2:
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I2;
			break;
		case 3:
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I3;
			break;
		default:
			break;
		}
		break;

	case ENDPOINT_DESCRIPTOR:
		DBG_SETUP1("ENDPOINT_DESCRIPTOR \n");
		switch(fboot_otg.dev_req.wValue_L&0xf) {
		case 0:
			fboot_otg.ep0_state=EP0_STATE_GD_EP0_ONLY_0;
			break;
		case 1:
			fboot_otg.ep0_state=EP0_STATE_GD_EP1_ONLY_0;
			break;
		default:
			break;
		}
		break;

	case DEVICE_QUALIFIER:
		fboot_otg.req_length = (u32)((fboot_otg.dev_req.wLength_H << 8) |
			fboot_otg.dev_req.wLength_L);
		DBG_SETUP1("DEVICE_QUALIFIER = 0x%x \n",fboot_otg.req_length);
		fboot_otg.ep0_state = EP0_STATE_GD_DEV_QUALIFIER;
		break;

	case OTHER_SPEED_CONFIGURATION :
		DBG_SETUP1("OTHER_SPEED_CONFIGURATION \n");
		fboot_otg.req_length = (u32)((fboot_otg.dev_req.wLength_H << 8) |
			fboot_otg.dev_req.wLength_L);
		fboot_otg.ep0_state = EP0_STATE_GD_OTHER_SPEED;
		break;

	}
}
//#define DBG_SETUP1(fmt, args...) do { } while (0)

void fboot_usb_clear_feature(void)
{
	DBG0("\n");
	switch (fboot_otg.dev_req.bmRequestType) {
	case DEVICE_RECIPIENT:
		DBG_SETUP1("DEVICE_RECIPIENT \n");
		if (fboot_otg.dev_req.wValue_L == 1)
			fboot_remode_wakeup = FALSE;
		break;

	case ENDPOINT_RECIPIENT:
		DBG_SETUP1("ENDPOINT_RECIPIENT \n");
		if (fboot_otg.dev_req.wValue_L == 0) {
			if ((fboot_otg.dev_req.wIndex_L & 0x7f) == CONTROL_EP)
				fboot_get_status.ep_ctrl= 0;

			/* IN	Endpoint */
			if ((fboot_otg.dev_req.wIndex_L & 0x7f) == BULK_IN_EP)
				fboot_get_status.ep_in= 0;

			/* OUT Endpoint */
			if ((fboot_otg.dev_req.wIndex_L & 0x7f) == BULK_OUT_EP)
				fboot_get_status.ep_out= 0;
		}
		break;

	default:
		DBG_SETUP1("\n");
		break;
	}
	fboot_otg.ep0_state = EP0_STATE_INIT;

}

void fboot_usb_set_feature(void)
{
	u32 tmp;
	DBG0("\n");

	switch (fboot_otg.dev_req.bmRequestType) {
	case DEVICE_RECIPIENT:
		DBG_SETUP1("DEVICE_RECIPIENT \n");
		if (fboot_otg.dev_req.wValue_L == 1)
			fboot_remode_wakeup = TRUE;
			break;

	case ENDPOINT_RECIPIENT:
		DBG_SETUP1("ENDPOINT_RECIPIENT \n");
		if (fboot_otg.dev_req.wValue_L == 0) {
			if ((fboot_otg.dev_req.wIndex_L & 0x7f) == CONTROL_EP)
				fboot_get_status.ep_ctrl= 1;

			if ((fboot_otg.dev_req.wIndex_L & 0x7f) == BULK_IN_EP)
				fboot_get_status.ep_in= 1;

			if ((fboot_otg.dev_req.wIndex_L & 0x7f) == BULK_OUT_EP)
				fboot_get_status.ep_out= 1;
		}
		break;

	default:
		DBG_SETUP1("\n");
		break;
	}

	switch (fboot_otg.dev_req.wValue_L) {
	case EP_STALL:
		/* TBD: additional processing if required */
		break;

	case TEST_MODE:
		if ((0 != fboot_otg.dev_req.wIndex_L ) ||(0 != fboot_otg.dev_req.bmRequestType))
			break;

		/* Set TEST MODE*/
		tmp = readl(S5P_OTG_DCTL);
		tmp = (tmp & ~(TEST_CONTROL_FIELD)) | (TEST_FORCE_ENABLE_MODE);
		writel(tmp, S5P_OTG_DCTL);

		switch(fboot_otg.dev_req.wIndex_H) {
		case TEST_J:
			/*Set Test J*/
			tmp = readl(S5P_OTG_DCTL);
			tmp = (tmp & ~(TEST_CONTROL_FIELD)) | (TEST_J_MODE);
			writel(tmp, S5P_OTG_DCTL);
			break;

		case TEST_K:
			/*Set Test K*/
			tmp = readl(S5P_OTG_DCTL);
			tmp = (tmp & ~(TEST_CONTROL_FIELD)) | (TEST_K_MODE);
			writel(tmp, S5P_OTG_DCTL);
			break;

		case TEST_SE0_NAK:
			/*Set Test SE0NAK*/
			tmp = readl(S5P_OTG_DCTL);
			tmp = (tmp & ~(TEST_CONTROL_FIELD)) | (TEST_SE0_NAK_MODE);
			writel(tmp, S5P_OTG_DCTL);
			break;

		case TEST_PACKET:
			DBG_SETUP1 ("Test_packet\n");
			writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, FBOOT_TEST_PKT_SIZE);
			fboot_usb_write_ep0_fifo(fboot_test_pkt, FBOOT_TEST_PKT_SIZE);
			tmp = readl(S5P_OTG_DCTL);
			tmp = (tmp & ~(TEST_CONTROL_FIELD)) | (TEST_PACKET_MODE);
			writel(tmp, S5P_OTG_DCTL);
			DBG_SETUP1 ("S5P_OTG_DCTL=0x%08x\n", tmp);
			break;
		}
		break;

	default:
		break;
	}
	fboot_otg.ep0_state = EP0_STATE_INIT;
}

void fboot_usb_get_status(void)
{
	DBG0("\n");
	switch(fboot_otg.dev_req.bmRequestType) {
	case  (0x80):	/*device */
		DBG_SETUP1("DEVICE\n");
		fboot_get_status.Device=((u8)fboot_remode_wakeup<<1)|0x1; /* SelfPowered */
		fboot_otg.ep0_state = EP0_STATE_GET_STATUS0;
		break;

	case  (0x81):	/*interface */
		DBG_SETUP1("INTERFACE\n");
		fboot_get_status.Interface=0;
		fboot_otg.ep0_state = EP0_STATE_GET_STATUS1;
		break;

	case  (0x82):	/*endpoint */
		DBG_SETUP1("ENDPOINT\n");
		if ((fboot_otg.dev_req.wIndex_L & 0x7f) == CONTROL_EP)
			fboot_otg.ep0_state = EP0_STATE_GET_STATUS2;

		if ((fboot_otg.dev_req.wIndex_L & 0x7f) == BULK_IN_EP)
			fboot_otg.ep0_state = EP0_STATE_GET_STATUS3;

		if ((fboot_otg.dev_req.wIndex_L & 0x7f) == BULK_OUT_EP)
			fboot_otg.ep0_state = EP0_STATE_GET_STATUS4;
		break;

	default:
		DBG_SETUP1("\n");
		break;
	}
}

void fboot_usb_ep0_int_hndlr(void)
{
	DBG0("\n");
	u16 i;
	u32 buf[2]={0x0000, };
	u16 addr;

	DBG_SETUP0("Event EP0\n");

	if (fboot_otg.ep0_state == EP0_STATE_INIT) {

		for(i=0;i<2;i++)
			buf[i] = readl(S5P_OTG_EP0_FIFO);

		fboot_otg.dev_req.bmRequestType = buf[0];
		fboot_otg.dev_req.bRequest	= buf[0]>>8;
		fboot_otg.dev_req.wValue_L	= buf[0]>>16;
		fboot_otg.dev_req.wValue_H	= buf[0]>>24;
		fboot_otg.dev_req.wIndex_L	= buf[1];
		fboot_otg.dev_req.wIndex_H	= buf[1]>>8;
		fboot_otg.dev_req.wLength_L	= buf[1]>>16;
		fboot_otg.dev_req.wLength_H	= buf[1]>>24;

#ifdef USB_OTG_DEBUG_SETUP
		fboot_usb_print_pkt((u8 *)&fboot_otg.dev_req, 8);
#endif

		switch (fboot_otg.dev_req.bRequest) {
		case STANDARD_SET_ADDRESS:
			/* Set Address Update bit */
			addr = (fboot_otg.dev_req.wValue_L);
			writel(1<<18|addr<<4|fboot_otg.speed<<0, S5P_OTG_DCFG);
			DBG_SETUP1("S5P_OTG_DCFG : %x, STANDARD_SET_ADDRESS : %d\n",
					readl(S5P_OTG_DCFG), addr);
			fboot_otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_SET_DESCRIPTOR:
			DBG_SETUP1("STANDARD_SET_DESCRIPTOR \n");
			break;

		case STANDARD_SET_CONFIGURATION:
			DBG_SETUP1("STANDARD_SET_CONFIGURATION \n");
			/* Configuration value in configuration descriptor */
			fboot_config_value = fboot_otg.dev_req.wValue_L;
			fboot_otg.set_config = 1;
			fboot_otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_GET_CONFIGURATION:
			DBG_SETUP1("STANDARD_GET_CONFIGURATION \n");
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);

			/*ep0 enable, clear nak, next ep0, 8byte */
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			writel(fboot_config_value, S5P_OTG_EP0_FIFO);
			fboot_otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_GET_DESCRIPTOR:
			DBG_SETUP1("STANDARD_GET_DESCRIPTOR :");
			fboot_usb_get_desc();
			break;

		case STANDARD_CLEAR_FEATURE:
			DBG_SETUP1("STANDARD_CLEAR_FEATURE :");
			fboot_usb_clear_feature();
			break;

		case STANDARD_SET_FEATURE:
			DBG_SETUP1("STANDARD_SET_FEATURE :");
			fboot_usb_set_feature();
			break;

		case STANDARD_GET_STATUS:
			DBG_SETUP1("STANDARD_GET_STATUS :");
			fboot_usb_get_status();
			break;

		case STANDARD_GET_INTERFACE:
			DBG_SETUP1("STANDARD_GET_INTERFACE \n");
			fboot_otg.ep0_state = EP0_STATE_INTERFACE_GET;
			break;

		case STANDARD_SET_INTERFACE:
			DBG_SETUP1("STANDARD_SET_INTERFACE \n");
			fboot_get_intf.AlternateSetting= fboot_otg.dev_req.wValue_L;
			fboot_otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_SYNCH_FRAME:
			DBG_SETUP1("STANDARD_SYNCH_FRAME \n");
			fboot_otg.ep0_state = EP0_STATE_INIT;
			break;

		default:
			break;
		}
	}

	fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.ctrl_max_pktsize);

	if(fboot_otg.speed == USB_HIGH) {
		/*clear nak, next ep0, 64byte */
		writel(((1<<26)|(CONTROL_EP<<11)|(0<<0)), S5P_OTG_DIEPCTL0);
	}
	else {
		/*clear nak, next ep0, 8byte */
		writel(((1<<26)|(CONTROL_EP<<11)|(3<<0)), S5P_OTG_DIEPCTL0);
	}
}

void fboot_usb_set_otherspeed_conf_desc(u32 length)
{
	DBG0("\n");

	/* Standard device descriptor */
	if (fboot_otg.speed == USB_HIGH)
	{
	    if (length ==9)
	    {
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 9);
			writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_full)+0, 9);
		}
	    else if(length ==32)
		{
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 32);
			writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_full_total)+0, 32);

	    }
		fboot_otg.ep0_state = EP0_STATE_INIT;
	}
	else
	{
	    if (length ==9)
	    {
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_high)+0, 8);
		}
	    else if(length ==32)
		{
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
			writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_high_total)+0, 8);
	    }
		fboot_otg.ep0_state = EP0_STATE_GD_OTHER_SPEED_HIGH_1;
	}
}


void fboot_usb_transfer_ep0(void)
{
	DBG0("\n");

	DBG_SETUP0("fboot_otg.ep0_state = %d\n", fboot_otg.ep0_state);

	switch (fboot_otg.ep0_state) {
	case EP0_STATE_INIT:
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 0);

		/*ep0 enable, clear nak, next ep0, 8byte */
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		DBG_SETUP1("EP0_STATE_INIT\n");
		break;

	/* GET_DESCRIPTOR:DEVICE */
	case EP0_STATE_GD_DEV_0:
		DBG_SETUP1("EP0_STATE_GD_DEV_0 :");
		if (fboot_otg.speed == USB_HIGH) {
			DBG_SETUP1("High Speed\n");

			/*ep0 enable, clear nak, next ep0, max 64byte */
			writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			if (fboot_otg.req_length < DEVICE_DESC_SIZE) {
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+0, fboot_otg.req_length);
			} else {
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, DEVICE_DESC_SIZE);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+0, DEVICE_DESC_SIZE);
			}
			fboot_otg.ep0_state = EP0_STATE_INIT;
		} else {
			DBG_SETUP1("Full Speed\n");
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			if(fboot_otg.req_length<DEVICE_DESC_SIZE) {
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
			} else {
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, DEVICE_DESC_SIZE);
			}

			if(fboot_otg.req_length<FS_CTRL_PKT_SIZE) {
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+0, fboot_otg.req_length);
				fboot_otg.ep0_state = EP0_STATE_INIT;
			} else {
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+0, FS_CTRL_PKT_SIZE);
				fboot_otg.ep0_state = EP0_STATE_GD_DEV_1;
			}
		}
		break;

	case EP0_STATE_GD_DEV_1:
		DBG_SETUP1("EP0_STATE_GD_DEV_1\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		if(fboot_otg.req_length<(2*FS_CTRL_PKT_SIZE)) {
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+FS_CTRL_PKT_SIZE,
						(fboot_otg.req_length-FS_CTRL_PKT_SIZE));
			fboot_otg.ep0_state = EP0_STATE_INIT;
		} else {
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+FS_CTRL_PKT_SIZE,
						FS_CTRL_PKT_SIZE);
			fboot_otg.ep0_state = EP0_STATE_GD_DEV_2;
		}
		break;

	case EP0_STATE_GD_DEV_2:
		DBG_SETUP1("EP0_STATE_GD_DEV_2\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		if(fboot_otg.req_length<DEVICE_DESC_SIZE) {
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+(2*FS_CTRL_PKT_SIZE),
						(fboot_otg.req_length-2*FS_CTRL_PKT_SIZE));
		} else {
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.dev))+(2*FS_CTRL_PKT_SIZE),
						(DEVICE_DESC_SIZE-2*FS_CTRL_PKT_SIZE));
		}
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	/* GET_DESCRIPTOR:CONFIGURATION+INTERFACE+ENDPOINT0+ENDPOINT1 */
	case EP0_STATE_GD_CFG_0:
		DBG_SETUP1("EP0_STATE_GD_CFG_0 :");
		if (fboot_otg.speed == USB_HIGH)
		{
			DBG_SETUP1("High Speed\n");
			writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			if(fboot_otg.req_length<CONFIG_DESC_TOTAL_SIZE)
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, fboot_otg.req_length);
			}
			else
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, CONFIG_DESC_TOTAL_SIZE);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, CONFIG_DESC_TOTAL_SIZE);
			}
			fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			DBG_SETUP1("Full Speed\n");
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			if(fboot_otg.req_length<CONFIG_DESC_TOTAL_SIZE)
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
			}
			else
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, CONFIG_DESC_TOTAL_SIZE);
			}
			if(fboot_otg.req_length<FS_CTRL_PKT_SIZE)
			{
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, fboot_otg.req_length);
				fboot_otg.ep0_state = EP0_STATE_INIT;
			}
			else
			{
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, FS_CTRL_PKT_SIZE);
				fboot_otg.ep0_state = EP0_STATE_GD_CFG_1;
			}
		}
		break;

	case EP0_STATE_GD_CFG_1:
		DBG_SETUP1("EP0_STATE_GD_CFG_1\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		if(fboot_otg.req_length<(2*FS_CTRL_PKT_SIZE))
		{
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+FS_CTRL_PKT_SIZE,
						(fboot_otg.req_length-FS_CTRL_PKT_SIZE));
			fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+FS_CTRL_PKT_SIZE,
						FS_CTRL_PKT_SIZE);
			fboot_otg.ep0_state = EP0_STATE_GD_CFG_2;
		}
		break;

	case EP0_STATE_GD_CFG_2:
		DBG_SETUP1("EP0_STATE_GD_CFG_2\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		if(fboot_otg.req_length<(3*FS_CTRL_PKT_SIZE))
		{
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+(2*FS_CTRL_PKT_SIZE),
						(fboot_otg.req_length-2*FS_CTRL_PKT_SIZE));
			fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+(2*FS_CTRL_PKT_SIZE),
						FS_CTRL_PKT_SIZE);
			fboot_otg.ep0_state = EP0_STATE_GD_CFG_3;
		}
		break;

	case EP0_STATE_GD_CFG_3:
		DBG_SETUP1("EP0_STATE_GD_CFG_3\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		if(fboot_otg.req_length<(4*FS_CTRL_PKT_SIZE))
		{
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+(3*FS_CTRL_PKT_SIZE),
						(fboot_otg.req_length-3*FS_CTRL_PKT_SIZE));
			fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+(3*FS_CTRL_PKT_SIZE),
						FS_CTRL_PKT_SIZE);
			fboot_otg.ep0_state = EP0_STATE_GD_CFG_4;
		}
		break;

	case EP0_STATE_GD_CFG_4:
		DBG_SETUP1("EP0_STATE_GD_CFG_4\n");
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_DEV_QUALIFIER:	/*only supported in USB 2.0*/
		DBG_SETUP1("EP0_STATE_GD_DEV_QUALIFIER\n");
		writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
		if(fboot_otg.req_length<10)
		{
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
			fboot_usb_write_ep0_fifo((u8 *)fboot_qualifier_desc+0, fboot_otg.req_length);
		}
		else
		{
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 10);
			fboot_usb_write_ep0_fifo((u8 *)fboot_qualifier_desc+0, 10);
		}
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_OTHER_SPEED:
			DBG_SETUP1("EP0_STATE_GD_OTHER_SPEED\n");
			fboot_usb_set_otherspeed_conf_desc(fboot_otg.req_length);
			break;

	case EP0_STATE_GD_OTHER_SPEED_HIGH_1:
		DBG_SETUP1("EP0_STATE_GD_OTHER_SPEED_HIGH_1\n");
		if(fboot_otg.req_length==9)
		{
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
				fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_high)+8, 1);
				fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
				fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_high)+8, 8);
				fboot_otg.ep0_state = EP0_STATE_GD_OTHER_SPEED_HIGH_2;
		}
			break;

	case EP0_STATE_GD_OTHER_SPEED_HIGH_2:
		DBG_SETUP1("EP0_STATE_GD_OTHER_SPEED_HIGH_2\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_high)+16, 8);
			fboot_otg.ep0_state = EP0_STATE_GD_OTHER_SPEED_HIGH_3;
			break;

	case EP0_STATE_GD_OTHER_SPEED_HIGH_3:
		DBG_SETUP1("EP0_STATE_GD_OTHER_SPEED_HIGH_3\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo(((u8 *)&fboot_config_high)+24, 8);
			fboot_otg.ep0_state = EP0_STATE_INIT;
			break;

	/* GET_DESCRIPTOR:CONFIGURATION ONLY*/
	case EP0_STATE_GD_CFG_ONLY_0:
		DBG_SETUP1("EP0_STATE_GD_CFG_ONLY_0:");
		if (fboot_otg.speed == USB_HIGH)
		{
			DBG_SETUP1("High Speed\n");
			if(fboot_otg.req_length<CONFIG_DESC_SIZE)
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, fboot_otg.req_length);
			}
			else
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, CONFIG_DESC_SIZE);
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0,
							CONFIG_DESC_SIZE);
			}
			fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			DBG_SETUP1("Full Speed\n");
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			if(fboot_otg.req_length<CONFIG_DESC_SIZE)
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
			}
			else
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, CONFIG_DESC_SIZE);
			}
			if(fboot_otg.req_length<FS_CTRL_PKT_SIZE)
			{
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, fboot_otg.req_length);
				fboot_otg.ep0_state = EP0_STATE_INIT;
			}
			else
			{
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+0, FS_CTRL_PKT_SIZE);
				fboot_otg.ep0_state = EP0_STATE_GD_CFG_ONLY_1;
			}
		}
		break;

	case EP0_STATE_GD_CFG_ONLY_1:
		DBG_SETUP1("EP0_STATE_GD_CFG_ONLY_1\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.config))+FS_CTRL_PKT_SIZE,
					(CONFIG_DESC_SIZE-FS_CTRL_PKT_SIZE));
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	/* GET_DESCRIPTOR:INTERFACE ONLY */

	case EP0_STATE_GD_IF_ONLY_0:
		DBG_SETUP1("EP0_STATE_GD_IF_ONLY_0 :");
		if (fboot_otg.speed == USB_HIGH)
		{
			DBG_SETUP1("High Speed\n");
			if(fboot_otg.req_length<INTERFACE_DESC_SIZE)
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.intf))+0, fboot_otg.req_length);
			}
			else
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, INTERFACE_DESC_SIZE);
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.intf))+0, INTERFACE_DESC_SIZE);
			}
			fboot_otg.ep0_state = EP0_STATE_INIT;
		}
		else
		{
			DBG_SETUP1("Full Speed\n");
			writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			if(fboot_otg.req_length<INTERFACE_DESC_SIZE)
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, fboot_otg.req_length);
			}
			else
			{
				fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, INTERFACE_DESC_SIZE);
			}
			if(fboot_otg.req_length<FS_CTRL_PKT_SIZE)
			{
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.intf))+0, fboot_otg.req_length);
				fboot_otg.ep0_state = EP0_STATE_INIT;
			}
			else
			{
				fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.intf))+0, FS_CTRL_PKT_SIZE);
				fboot_otg.ep0_state = EP0_STATE_GD_IF_ONLY_1;
			}
		}
		break;

	case EP0_STATE_GD_IF_ONLY_1:
		DBG_SETUP1("EP0_STATE_GD_IF_ONLY_1\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.intf))+FS_CTRL_PKT_SIZE,
					(INTERFACE_DESC_SIZE-FS_CTRL_PKT_SIZE));
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;


	/* GET_DESCRIPTOR:ENDPOINT 1 ONLY */
	case EP0_STATE_GD_EP0_ONLY_0:
		DBG_SETUP1("EP0_STATE_GD_EP0_ONLY_0\n");
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, ENDPOINT_DESC_SIZE);
		fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.ep1))+0, ENDPOINT_DESC_SIZE);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	/* GET_DESCRIPTOR:ENDPOINT 2 ONLY */
	case EP0_STATE_GD_EP1_ONLY_0:
		DBG_SETUP1("EP0_STATE_GD_EP1_ONLY_0\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, ENDPOINT_DESC_SIZE);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo(((u8 *)&(fboot_otg.desc.ep2))+0, ENDPOINT_DESC_SIZE);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	/* GET_DESCRIPTOR:STRING  */
	case EP0_STATE_GD_STR_I0:
		DBG_SETUP1("EP0_STATE_GD_STR_I0\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, STRING_DESC0_SIZE);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)fboot_string_desc0, STRING_DESC0_SIZE);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_STR_I1:
		DBG_SETUP1("EP0_STATE_GD_STR_I1 %d\n", fboot_otg.ep0_substate);
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, sizeof(fboot_string_desc1));
		if ((fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize+fboot_otg.ctrl_max_pktsize)
			< sizeof(fboot_string_desc1)) {

			if (fboot_otg.speed == USB_HIGH)
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			else
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo((u8 *)fboot_string_desc1+(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize),
						fboot_otg.ctrl_max_pktsize);
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I1;
			fboot_otg.ep0_substate++;
		} else {
			if (fboot_otg.speed == USB_HIGH)
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			else
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo((u8 *)fboot_string_desc1+(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize),
						sizeof(fboot_string_desc1)-(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize));
			fboot_otg.ep0_state = EP0_STATE_INIT;
			fboot_otg.ep0_substate = 0;
		}
		break;

	case EP0_STATE_GD_STR_I2:
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, sizeof(fboot_string_desc2));
		if ((fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize+fboot_otg.ctrl_max_pktsize)
			< sizeof(fboot_string_desc2)) {

			if (fboot_otg.speed == USB_HIGH)
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			else
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo((u8 *)fboot_string_desc2+(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize),
						fboot_otg.ctrl_max_pktsize);
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I2;
			fboot_otg.ep0_substate++;
		} else {
			if (fboot_otg.speed == USB_HIGH)
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			else
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo((u8 *)fboot_string_desc2+(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize),
						sizeof(fboot_string_desc2)-(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize));
			fboot_otg.ep0_state = EP0_STATE_INIT;
			fboot_otg.ep0_substate = 0;
		}
		DBG_SETUP1("EP0_STATE_GD_STR_I2 %d", fboot_otg.ep0_substate);
		break;

	case EP0_STATE_GD_STR_I3:
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, sizeof(string_desc3));
		if ((fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize+fboot_otg.ctrl_max_pktsize)
			< sizeof(string_desc3)) {

			if (fboot_otg.speed == USB_HIGH)
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			else
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo((u8 *)string_desc3+(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize),
						fboot_otg.ctrl_max_pktsize);
			fboot_otg.ep0_state = EP0_STATE_GD_STR_I1;
			fboot_otg.ep0_substate++;
		} else {
			if (fboot_otg.speed == USB_HIGH)
				writel(EPEN_CNAK_EP0_64, S5P_OTG_DIEPCTL0);
			else
				writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
			fboot_usb_write_ep0_fifo((u8 *)string_desc3+(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize),
						sizeof(string_desc3)-(fboot_otg.ep0_substate*fboot_otg.ctrl_max_pktsize));
			fboot_otg.ep0_state = EP0_STATE_INIT;
			fboot_otg.ep0_substate = 0;
		}
		DBG_SETUP1("EP0_STATE_GD_STR_I3 %d", fboot_otg.ep0_substate);
		break;

	case EP0_STATE_INTERFACE_GET:
		DBG_SETUP1("EP0_STATE_INTERFACE_GET\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)&fboot_get_intf+0, 1);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;


	case EP0_STATE_GET_STATUS0:
		DBG_SETUP1("EP0_STATE_GET_STATUS0\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)&fboot_get_status+0, 1);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS1:
		DBG_SETUP1("EP0_STATE_GET_STATUS1\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)&fboot_get_status+1, 1);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS2:
		DBG_SETUP1("EP0_STATE_GET_STATUS2\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)&fboot_get_status+2, 1);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS3:
		DBG_SETUP1("EP0_STATE_GET_STATUS3\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)&fboot_get_status+3, 1);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS4:
		DBG_SETUP1("EP0_STATE_GET_STATUS4\n");
		fboot_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		writel(EPEN_CNAK_EP0_8, S5P_OTG_DIEPCTL0);
		fboot_usb_write_ep0_fifo((u8 *)&fboot_get_status+4, 1);
		fboot_otg.ep0_state = EP0_STATE_INIT;
		break;

	default:
		break;
	}
}

		//void fboot_usb_clear_dnfile_info();
		//void fboot_usb_clear_upfile_info();

void fboot_usb_clear_all_outep_nak(void);
void fboot_usb_int_bulkin(void)
{
	DBG0("\n");
	u8* bulkin_buf;
	u32 remain_cnt;
	u32 DIEPCTL_IN=readl(S5P_OTG_DIEPCTL_IN);
	u32 tmp;

	DBG_BULK0("~~~~~~~~~~~~~~~~ bulkin Function ~~~~~~~~~~~~\n");


#ifdef USB_FASTBOOT_TEST
	if ((fboot_response_flag==1)&&(reply_msg)) {

		DBG1("reply_msg=%s\n",reply_msg);
		fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1, strlen(reply_msg));

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0, S5P_OTG_DIEPCTL_IN);

		fboot_usb_write_in_fifo(reply_msg,strlen(reply_msg)); 

		fboot_response_flag=0;
	}
	else if (fboot_response_flag==0)
	{
		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		//writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0, S5P_OTG_DIEPCTL_IN);
		//writel((DEPCTL_SNAK|DEPCTL_BULK_TYPE), S5P_OTG_DIEPCTL_IN);
		// NAK should be cleared. why??
		tmp = readl(S5P_OTG_DOEPCTL0+0x20*BULK_OUT_EP);
		tmp |= (DEPCTL_CNAK);
		writel(tmp, S5P_OTG_DOEPCTL0+0x20*BULK_OUT_EP);
	}
	//DBG_BULK1("fboot_response_flag=%d S5P_OTG_DIEPCTL_IN=0x%x\n",fboot_response_flag,DIEPCTL_IN);

	return;
#else


	if (remain_cnt > fboot_otg.bulkin_max_pktsize) {
		fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1, fboot_otg.bulkin_max_pktsize);

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0,
			S5P_OTG_DIEPCTL_IN);

		fboot_usb_write_in_fifo(bulkin_buf, fboot_otg.bulkin_max_pktsize);

		fboot_otg.up_ptr += fboot_otg.bulkin_max_pktsize;

	} else if(remain_cnt > 0) {
		fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1, remain_cnt);

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0,
			S5P_OTG_DIEPCTL_IN);

		fboot_usb_write_in_fifo(bulkin_buf, remain_cnt);

		fboot_otg.up_ptr += remain_cnt;

	} else { /*remain_cnt = 0*/
		writel((DEPCTL_SNAK|DEPCTL_BULK_TYPE), S5P_OTG_DIEPCTL_IN);
	}
	
#endif
}

void fboot_usb_upload_start(void)
{
	DBG0("\n");
	u8 tmp_buf[12];
	u32 check;

	fboot_usb_read_out_fifo((u8 *)tmp_buf, 10);
	check = *((u8 *)(tmp_buf+8)) + (*((u8 *)(tmp_buf+9))<<8);

	if (check==0x1) {
		fboot_otg.up_addr =
			*((u8 *)(tmp_buf+0))+
			(*((u8 *)(tmp_buf+1))<<8)+
			(*((u8 *)(tmp_buf+2))<<16)+
			(*((u8 *)(tmp_buf+3))<<24);

		fboot_otg.up_size =
			*((u8 *)(tmp_buf+4))+
			(*((u8 *)(tmp_buf+5))<<8)+
			(*((u8 *)(tmp_buf+6))<<16)+
			(*((u8 *)(tmp_buf+7))<<24);

		fboot_otg.up_ptr=(u8 *)fboot_otg.up_addr;
		DBG_BULK1("UploadAddress : 0x%x, UploadSize: %d\n",
			fboot_otg.up_addr, fboot_otg.up_size);

		if (fboot_otg.op_mode == USB_CPU) {
			if (fboot_otg.up_size > fboot_otg.bulkin_max_pktsize) {
				fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1,
					fboot_otg.bulkin_max_pktsize);
			} else {
				fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1,
					fboot_otg.up_size);
			}

			/*ep1 enable, clear nak, bulk, usb active, max pkt 64*/
			writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0,
				S5P_OTG_DIEPCTL_IN);
		} else if ((fboot_otg.op_mode == USB_DMA) && (fboot_otg.up_size > 0)) {
			u32 pktcnt, remainder;

			DBG_BULK1("Dma Start for IN PKT \n");

			writel(MODE_DMA|BURST_INCR4|GBL_INT_UNMASK,
				S5P_OTG_GAHBCFG);
			writel(INT_RESUME|INT_OUT_EP|INT_IN_EP| INT_ENUMDONE|
				INT_RESET|INT_SUSPEND, S5P_OTG_GINTMSK);

			writel((u32)fboot_otg.up_ptr, S5P_OTG_DIEPDMA_IN);

			pktcnt = (u32)(fboot_otg.up_size/fboot_otg.bulkin_max_pktsize);
			remainder = (u32)(fboot_otg.up_size%fboot_otg.bulkin_max_pktsize);
			if(remainder != 0) {
				pktcnt += 1;
			}

			if (pktcnt > 1023) {
				fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1023,
					(fboot_otg.bulkin_max_pktsize*1023));
			} else {
				fboot_usb_set_inep_xfersize(EP_TYPE_BULK, pktcnt,
					fboot_otg.up_size);
			}

			/*ep1 enable, clear nak, bulk, usb active, next ep1, max pkt */
			writel(1u<<31|1<<26|2<<18|1<<15|BULK_IN_EP<<11|
				fboot_otg.bulkin_max_pktsize<<0,
				S5P_OTG_DIEPCTL_IN);
		}
	}
	fboot_otg.dn_filesize=0;
}

void fboot_usb_download_start(u32 fifo_cnt_byte)
{
	DBG0("\n");
	u8 tmp_buf[8];

	fboot_usb_read_out_fifo((u8 *)tmp_buf, 8);
	DBG_BULK1("downloadFileSize==0, 1'st BYTE_READ_CNT_REG : %x\n",
		fifo_cnt_byte);

	fboot_otg.dn_addr=fboot_usbd_dn_addr;
	fboot_otg.dn_filesize=
		*((u8 *)(tmp_buf+4))+
		(*((u8 *)(tmp_buf+5))<<8)+
		(*((u8 *)(tmp_buf+6))<<16)+
		(*((u8 *)(tmp_buf+7))<<24);

	fboot_otg.dn_ptr=(u8 *)fboot_otg.dn_addr;
	DBG_BULK1("downloadAddress : 0x%x, downloadFileSize: %x\n",
		fboot_otg.dn_addr, fboot_otg.dn_filesize);

	/* The first 8-bytes are deleted.*/
	fboot_usb_read_out_fifo((u8 *)fboot_otg.dn_ptr, fifo_cnt_byte-8);
	fboot_otg.dn_ptr += fifo_cnt_byte-8;

	if (fboot_otg.op_mode == USB_CPU) {
		fboot_usb_set_outep_xfersize(EP_TYPE_BULK, 1,
			fboot_otg.bulkout_max_pktsize);

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkout_max_pktsize<<0,
		S5P_OTG_DOEPCTL_OUT);
	} else if (fboot_otg.dn_filesize>fboot_otg.bulkout_max_pktsize) {
		u32 pkt_cnt, remain_cnt;

		DBG_BULK1("downloadFileSize!=0, Dma Start for 2nd OUT PKT \n");
		writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|
			INT_RESET|INT_SUSPEND, S5P_OTG_GINTMSK); /*gint unmask */
		writel(MODE_DMA|BURST_INCR4|GBL_INT_UNMASK,
			S5P_OTG_GAHBCFG);
		writel((u32)fboot_otg.dn_ptr, S5P_OTG_DOEPDMA_OUT);
		pkt_cnt = (u32)(fboot_otg.dn_filesize-fboot_otg.bulkout_max_pktsize)/fboot_otg.bulkout_max_pktsize;
		remain_cnt = (u32)((fboot_otg.dn_filesize-fboot_otg.bulkout_max_pktsize)%fboot_otg.bulkout_max_pktsize);
		if(remain_cnt != 0) {
			pkt_cnt += 1;
		}

		if (pkt_cnt > 1023) {
			fboot_usb_set_outep_xfersize(EP_TYPE_BULK, 1023,
				(fboot_otg.bulkout_max_pktsize*1023));
		} else {
			fboot_usb_set_outep_xfersize(EP_TYPE_BULK, pkt_cnt,
				(fboot_otg.dn_filesize-fboot_otg.bulkout_max_pktsize));
		}

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkout_max_pktsize<<0,
			S5P_OTG_DOEPCTL_OUT);
	}
}

void fboot_usb_download_continue(u32 fifo_cnt_byte)
{
	DBG0("\n");
	if (fboot_otg.op_mode == USB_CPU) {
		fboot_usb_read_out_fifo((u8 *)fboot_otg.dn_ptr, fifo_cnt_byte);
		fboot_otg.dn_ptr += fifo_cnt_byte;
		DBG_BULK1("downloadFileSize!=0, 2nd BYTE_READ_CNT_REG = 0x%x, m_pDownPt = 0x%x\n",
				fifo_cnt_byte, fboot_otg.dn_ptr);

		fboot_usb_set_outep_xfersize(EP_TYPE_BULK, 1, fboot_otg.bulkout_max_pktsize);

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkout_max_pktsize<<0,
			S5P_OTG_DOEPCTL_OUT);

		/* USB format : addr(4)+size(4)+data(n)+cs(2) */
		if (((u32)fboot_otg.dn_ptr - fboot_otg.dn_addr) >= (fboot_otg.dn_filesize - 8)) {
			printf("Download Done!! Download Address: 0x%x, Download Filesize:0x%x\n",
				fboot_otg.dn_addr, (fboot_otg.dn_filesize-10));
				
			fboot_usbd_dn_cnt 	= fboot_otg.dn_filesize-10;
			fboot_usbd_dn_addr	= fboot_otg.dn_addr;

#ifdef USB_CHECKSUM_EN
			fboot_usb_verify_checksum();
#endif
			fboot_receive_done = 1;
		}

	}\
}


		unsigned int total_dn_size =0;
void fboot_usb_int_bulkout(u32 fifo_cnt_byte)
{
	DBG0("\n");
	DBG_BULK0("@@\n Bulk Out Function : fboot_otg.dn_filesize=0x%x\n", fboot_otg.dn_filesize);
#ifdef USB_FASTBOOT_TEST
	fboot_usb_read_out_fifo((u8 *)fastboot_bulk_fifo, fifo_cnt_byte);
	if (fifo_cnt_byte<64) {
		sprintf(fastboot_bulk_fifo+fifo_cnt_byte, "\0"); // append null
		printf("Received %d bytes: %s\n",fifo_cnt_byte, fastboot_bulk_fifo);
	}
	DBG0(" Received %d bytes\n",fifo_cnt_byte);

	/*
	// maximize transfer size when it is possible
	total_dn_size += fifo_cnt_byte;
	if (fboot_otg.dn_filesize > 1023) {
		fboot_usb_set_outep_xfersize(EP_TYPE_BULK, 1023,
			(fboot_otg.bulkout_max_pktsize*1023));
	} else {
		fboot_usb_set_outep_xfersize(EP_TYPE_BULK, pkt_cnt,
			(fboot_otg.dn_filesize-fboot_otg.bulkout_max_pktsize));
	}
	*/


	/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
	writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkout_max_pktsize<<0,
			S5P_OTG_DOEPCTL_OUT);

	/* Pass this up to the interface's handler */
	if (fastboot_interface && fastboot_interface->rx_handler) {
		/* Call rx_handler at common/cmd_fastboot.c */
		if (!fastboot_interface->rx_handler(&fastboot_bulk_fifo[0], fifo_cnt_byte))
			;//OK 
	}

	/* Since the buffer is not null terminated,
	 * poison the buffer */
	//memset(&fastboot_bulk_fifo[0], 0, fifo_size);
	//memset(&fastboot_bulk_fifo[0], 0, fastboot_fifo_size);
		
	/*ep1 enable, clear nak, bulk, usb active, max pkt 64*/
	//writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0, S5P_OTG_DIEPCTL_IN);
#else
	if (fboot_otg.dn_filesize==0) {
		if (fifo_cnt_byte == 10) {
			fboot_usb_upload_start();
		} else {
			fboot_usb_download_start(fifo_cnt_byte);
		}
	} else {
		fboot_usb_download_continue(fifo_cnt_byte);
	}
#endif
}

void fboot_usb_dma_in_done(void)
{
	s32 remain_cnt;

	DBG0("\n");
	DBG_BULK0("DMA IN : Transfer Done\n");

	fboot_otg.up_ptr = (u8 *)readl(S5P_OTG_DIEPDMA_IN);
	remain_cnt = fboot_otg.up_size- ((u32)fboot_otg.up_ptr - fboot_otg.up_addr);

	if (remain_cnt>0) {
		u32 pktcnt, remainder;
		pktcnt = (u32)(remain_cnt/fboot_otg.bulkin_max_pktsize);
		remainder = (u32)(remain_cnt%fboot_otg.bulkin_max_pktsize);
		if(remainder != 0) {
			pktcnt += 1;
		}
		DBG_SETUP1("remain_cnt : %d \n", remain_cnt);
		if (pktcnt> 1023) {
			fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1023,
				(fboot_otg.bulkin_max_pktsize*1023));
		} else {
			fboot_usb_set_inep_xfersize(EP_TYPE_BULK, pktcnt,
				remain_cnt);
		}

		/*ep1 enable, clear nak, bulk, usb active, next ep1, max pkt */
		writel(1u<<31|1<<26|2<<18|1<<15|BULK_IN_EP<<11|fboot_otg.bulkin_max_pktsize<<0,
			S5P_OTG_DIEPCTL_IN);
	} else
		DBG_SETUP1("DMA IN : Transfer Complete\n");
}

void fboot_usb_dma_out_done(void)
{
	s32 remain_cnt;
	DBG0("\n");

	DBG_BULK1("DMA OUT : Transfer Done\n");
	fboot_otg.dn_ptr = (u8 *)readl(S5P_OTG_DOEPDMA_OUT);

	remain_cnt = fboot_otg.dn_filesize - ((u32)fboot_otg.dn_ptr - fboot_otg.dn_addr + 8);

	if (remain_cnt>0) {
		u32 pktcnt, remainder;
		pktcnt = (u32)(remain_cnt/fboot_otg.bulkout_max_pktsize);
		remainder = (u32)(remain_cnt%fboot_otg.bulkout_max_pktsize);
		if(remainder != 0) {
			pktcnt += 1;
		}
		DBG_BULK1("remain_cnt : %d \n", remain_cnt);
		if (pktcnt> 1023) {
			fboot_usb_set_outep_xfersize(EP_TYPE_BULK, 1023,
				(fboot_otg.bulkout_max_pktsize*1023));
		} else {
			fboot_usb_set_outep_xfersize(EP_TYPE_BULK, pktcnt,
				remain_cnt);
		}

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64*/
		writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkout_max_pktsize<<0,
			S5P_OTG_DOEPCTL_OUT);
	} else {
		DBG_BULK1("DMA OUT : Transfer Complete\n");
		udelay(500);		/*for FPGA ???*/
	}
}

void fboot_usb_set_all_outep_nak(void)
{
	u8 i;
	u32 tmp;

	DBG0("\n");
	for(i=0;i<16;i++)
	{
		tmp = readl(S5P_OTG_DOEPCTL0+0x20*i);
		tmp |= DEPCTL_SNAK;
		writel(tmp, S5P_OTG_DOEPCTL0+0x20*i);
	}
}

void fboot_usb_clear_all_outep_nak(void)
{
	u8 i;
	u32 tmp;

	DBG0("\n");
	for(i=0;i<16;i++)
	{
		tmp = readl(S5P_OTG_DOEPCTL0+0x20*i);
		tmp |= (DEPCTL_EPENA|DEPCTL_CNAK);
		writel(tmp, S5P_OTG_DOEPCTL0+0x20*i);
	}
}

void fboot_usb_set_max_pktsize(USB_SPEED speed)
{
	DBG0("\n");
	if (speed == USB_HIGH)
	{
		fboot_otg.speed = USB_HIGH;
		fboot_otg.ctrl_max_pktsize = HS_CTRL_PKT_SIZE;
		fboot_otg.bulkin_max_pktsize = HS_BULK_PKT_SIZE;
		fboot_otg.bulkout_max_pktsize = HS_BULK_PKT_SIZE;
	}
	else
	{
		fboot_otg.speed = USB_FULL;
		fboot_otg.ctrl_max_pktsize = FS_CTRL_PKT_SIZE;
		fboot_otg.bulkin_max_pktsize = FS_BULK_PKT_SIZE;
		fboot_otg.bulkout_max_pktsize = FS_BULK_PKT_SIZE;
	}
}

void fboot_usb_set_endpoint(void)
{
	DBG0("\n");
	/* Unmask S5P_OTG_DAINT source */
	writel(0xff, S5P_OTG_DIEPINT0);
	writel(0xff, S5P_OTG_DOEPINT0);
	writel(0xff, S5P_OTG_DIEPINT_IN);
	writel(0xff, S5P_OTG_DOEPINT_OUT);

	/* Init For Ep0*/
	if(fboot_otg.speed == USB_HIGH)
	{
		/*MPS:64bytes */
		writel(((1<<26)|(CONTROL_EP<<11)|(0<<0)), S5P_OTG_DIEPCTL0);
		/*ep0 enable, clear nak */
		writel((1u<<31)|(1<<26)|(0<<0), S5P_OTG_DOEPCTL0);
	}
	else
	{
		/*MPS:8bytes */
		writel(((1<<26)|(CONTROL_EP<<11)|(3<<0)), S5P_OTG_DIEPCTL0);
		/*ep0 enable, clear nak */
		writel((1u<<31)|(1<<26)|(3<<0), S5P_OTG_DOEPCTL0);
	}
}

void fboot_usb_set_descriptors(void)
{
	DBG0("\n");

	/* Standard device descriptor */
	fboot_otg.desc.dev.bLength=DEVICE_DESC_SIZE;	/*0x12*/
	fboot_otg.desc.dev.bDescriptorType=DEVICE_DESCRIPTOR;
	fboot_otg.desc.dev.bMaxPacketSize0=fboot_otg.ctrl_max_pktsize;
#ifdef USB_FASTBOOT_TEST
	fboot_otg.desc.dev.bDeviceClass=0x0; /* 0x0*/
	fboot_otg.desc.dev.bDeviceSubClass=0x0;
	fboot_otg.desc.dev.bDeviceProtocol=0x0;
	fboot_otg.desc.dev.idVendorL=VENDOR_ID&0xff;//0xB4;	/**/
	fboot_otg.desc.dev.idVendorH=VENDOR_ID>>8;//0x0B;	/**/
	fboot_otg.desc.dev.idProductL=PRODUCT_ID&0xff;//0xFF; /**/
	fboot_otg.desc.dev.idProductH=PRODUCT_ID>>8;//0x0F; /**/
#else
	fboot_otg.desc.dev.bDeviceClass=0xFF; /* 0x0*/
	fboot_otg.desc.dev.bDeviceSubClass=0x0;
	fboot_otg.desc.dev.bDeviceProtocol=0x0;
	fboot_otg.desc.dev.idVendorL=0xE8;	/*0x45;*/
	fboot_otg.desc.dev.idVendorH=0x04;	/*0x53;*/
	fboot_otg.desc.dev.idProductL=0x34; /*0x00*/
	fboot_otg.desc.dev.idProductH=0x12; /*0x64*/
#endif
	fboot_otg.desc.dev.bcdDeviceL=0x00;
	fboot_otg.desc.dev.bcdDeviceH=0x01;
	fboot_otg.desc.dev.iManufacturer=DEVICE_STRING_MANUFACTURER_INDEX; /* index of string descriptor */
	fboot_otg.desc.dev.iProduct=DEVICE_STRING_PRODUCT_INDEX;	/* index of string descriptor */
	fboot_otg.desc.dev.iSerialNumber=DEVICE_STRING_SERIAL_NUMBER_INDEX;
	fboot_otg.desc.dev.bNumConfigurations=0x1;
	if (fboot_otg.speed == USB_FULL) {
		fboot_otg.desc.dev.bcdUSBL=0x10;
		fboot_otg.desc.dev.bcdUSBH=0x01;	/* Ver 1.10*/
	}
	else {
		fboot_otg.desc.dev.bcdUSBL=0x00;
		//fboot_otg.desc.dev.bcdUSBL=0x10;
		fboot_otg.desc.dev.bcdUSBH=0x02;	/* Ver 2.0*/
	}

	/* Standard configuration descriptor */
	fboot_otg.desc.config.bLength=CONFIG_DESC_SIZE; /* 0x9 bytes */
	fboot_otg.desc.config.bDescriptorType=CONFIGURATION_DESCRIPTOR;
	fboot_otg.desc.config.wTotalLengthL=CONFIG_DESC_TOTAL_SIZE;
	fboot_otg.desc.config.wTotalLengthH=0;
	fboot_otg.desc.config.bNumInterfaces=1;
/* dbg	  descConf.bConfigurationValue=2; // why 2? There's no reason.*/
	fboot_otg.desc.config.bConfigurationValue=1;
	fboot_otg.desc.config.iConfiguration=0;
	fboot_otg.desc.config.bmAttributes=CONF_ATTR_DEFAULT|CONF_ATTR_SELFPOWERED; /* bus powered only.*/
	fboot_otg.desc.config.maxPower=25; /* draws 50mA current from the USB bus.*/

	/* Standard interface descriptor */
	fboot_otg.desc.intf.bLength=INTERFACE_DESC_SIZE; /* 9*/
	fboot_otg.desc.intf.bDescriptorType=INTERFACE_DESCRIPTOR;
	fboot_otg.desc.intf.bInterfaceNumber=0x0;
	fboot_otg.desc.intf.bAlternateSetting=0x0; /* ?*/
	fboot_otg.desc.intf.bNumEndpoints = 2;	/* # of endpoints except EP0*/
#ifdef USB_FASTBOOT_TEST
	fboot_otg.desc.intf.bInterfaceClass=	FASTBOOT_INTERFACE_CLASS;// 0xff; /* 0x0 ?*/
	fboot_otg.desc.intf.bInterfaceSubClass=	FASTBOOT_INTERFACE_SUB_CLASS;// 0x42;
	fboot_otg.desc.intf.bInterfaceProtocol=	FASTBOOT_INTERFACE_PROTOCOL;//0x03;
#else
	fboot_otg.desc.intf.bInterfaceClass=0xff; /* 0x0 ?*/
	fboot_otg.desc.intf.bInterfaceSubClass=0x0;
	fboot_otg.desc.intf.bInterfaceProtocol=0x0;
#endif
	fboot_otg.desc.intf.iInterface=0x0;

	/* Standard endpoint0 descriptor */
	fboot_otg.desc.ep1.bLength=ENDPOINT_DESC_SIZE;
	fboot_otg.desc.ep1.bDescriptorType=ENDPOINT_DESCRIPTOR;
	fboot_otg.desc.ep1.bEndpointAddress=BULK_IN_EP|EP_ADDR_IN;
	fboot_otg.desc.ep1.bmAttributes=EP_ATTR_BULK;
	fboot_otg.desc.ep1.wMaxPacketSizeL=(u8)fboot_otg.bulkin_max_pktsize; /* 64*/
	fboot_otg.desc.ep1.wMaxPacketSizeH=(u8)(fboot_otg.bulkin_max_pktsize>>8);
	fboot_otg.desc.ep1.bInterval=0x0; /* not used */

	/* Standard endpoint1 descriptor */
	fboot_otg.desc.ep2.bLength=ENDPOINT_DESC_SIZE;
	fboot_otg.desc.ep2.bDescriptorType=ENDPOINT_DESCRIPTOR;
	fboot_otg.desc.ep2.bEndpointAddress=BULK_OUT_EP|EP_ADDR_OUT;
	fboot_otg.desc.ep2.bmAttributes=EP_ATTR_BULK;
	fboot_otg.desc.ep2.wMaxPacketSizeL=(u8)fboot_otg.bulkout_max_pktsize; /* 64*/
	fboot_otg.desc.ep2.wMaxPacketSizeH=(u8)(fboot_otg.bulkout_max_pktsize>>8);
	fboot_otg.desc.ep2.bInterval=0x0; /* not used */
}

void fboot_usb_check_speed(USB_SPEED *speed)
{
	u32 status;

	status = readl(S5P_OTG_DSTS); /* System status read */

	*speed = (USB_SPEED)((status&0x6) >>1);
}

void fboot_usb_clear_dnfile_info(void)
{
	fboot_otg.dn_addr = 0;
	fboot_otg.dn_filesize = 0;
	fboot_otg.dn_ptr = 0;
}

void fboot_usb_clear_upfile_info(void)
{
	fboot_otg.up_addr= 0;
	fboot_otg.up_size= 0;
	fboot_otg.up_ptr = 0;
}


int fboot_usb_check_setconf(void)
{
	if (fboot_otg.set_config == 0)
		return FALSE;
	else
		return TRUE;
}

void fboot_usb_set_opmode(USB_OPMODE mode)
{
	fboot_otg.op_mode = mode;

	writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|
		INT_RESET|INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY,
		S5P_OTG_GINTMSK); /*gint unmask */

	writel(MODE_SLAVE|BURST_SINGLE|GBL_INT_UNMASK, S5P_OTG_GAHBCFG);

	fboot_usb_set_outep_xfersize(EP_TYPE_BULK, 1, fboot_otg.bulkout_max_pktsize);
	fboot_usb_set_inep_xfersize(EP_TYPE_BULK, 1, 0);

	/*bulk out ep enable, clear nak, bulk, usb active, next ep3, max pkt */
	writel(1u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkout_max_pktsize<<0,
		S5P_OTG_DOEPCTL_OUT);

	/*bulk in ep enable, clear nak, bulk, usb active, next ep1, max pkt */
	writel(0u<<31|1<<26|2<<18|1<<15|fboot_otg.bulkin_max_pktsize<<0,
		S5P_OTG_DIEPCTL_IN);
}

void fboot_usb_reset(void)
{
	fboot_usb_set_all_outep_nak();

	fboot_otg.ep0_state = EP0_STATE_INIT;
	writel(((1<<BULK_OUT_EP)|(1<<CONTROL_EP))<<16|((1<<BULK_IN_EP)|(1<<CONTROL_EP)),
		S5P_OTG_DAINTMSK);
	writel(CTRL_OUT_EP_SETUP_PHASE_DONE|AHB_ERROR|TRANSFER_DONE,
		S5P_OTG_DOEPMSK);
	writel(INTKN_TXFEMP|NON_ISO_IN_EP_TIMEOUT|AHB_ERROR|TRANSFER_DONE,
		S5P_OTG_DIEPMSK);

	/* Rx FIFO Size */
	writel(RX_FIFO_SIZE, S5P_OTG_GRXFSIZ);

	/* Non Periodic Tx FIFO Size */
	writel(NPTX_FIFO_SIZE<<16| NPTX_FIFO_START_ADDR<<0, S5P_OTG_GNPTXFSIZ);

	fboot_usb_clear_all_outep_nak();

	/*clear device address */
	writel(readl(S5P_OTG_DCFG)&~(0x7f<<4), S5P_OTG_DCFG);

	if(SUSPEND_RESUME_ON) {
		writel(readl(S5P_OTG_PCGCCTL)&~(1<<0), S5P_OTG_PCGCCTL);
	}
}
int fboot_usb_set_init(void)
{
	u32 status;

	status = readl(S5P_OTG_DSTS); /* System status read */

	/* Set if Device is High speed or Full speed */
	if (((status&0x6) >>1) == USB_HIGH) {
		DBG_SETUP1("High Speed Detection\n");
		fboot_usb_set_max_pktsize(USB_HIGH);
	}
	else if(((status&0x6) >>1) == USB_FULL) {
		DBG_SETUP1("Full Speed Detec tion\n");
		fboot_usb_set_max_pktsize(USB_FULL);
	}
	else {
		printf("**** Error:Neither High_Speed nor Full_Speed\n");
		return FALSE;
	}

	fboot_usb_set_endpoint();
	fboot_usb_set_descriptors();
	fboot_usb_clear_dnfile_info();
	fboot_usb_set_opmode(fboot_op_mode);

	return TRUE;
}

void fboot_usb_pkt_receive(void)
{
	u32 rx_status;
	u32 fifo_cnt_byte;

	rx_status = readl(S5P_OTG_GRXSTSP);
	DBG_SETUP0("S5P_OTG_GRXSTSP = 0x%x\n", rx_status);

	if ((rx_status & (0xf<<17)) == SETUP_PKT_RECEIVED) {
		DBG_SETUP1("SETUP_PKT_RECEIVED\n");
		fboot_usb_ep0_int_hndlr();

	} else if ((rx_status & (0xf<<17)) == OUT_PKT_RECEIVED) {
		fifo_cnt_byte = (rx_status & 0x7ff0)>>4;
		DBG_SETUP1("OUT_PKT_RECEIVED\n");

		if((rx_status & BULK_OUT_EP)&&(fifo_cnt_byte)) {
			fboot_usb_int_bulkout(fifo_cnt_byte);
			if( fboot_otg.op_mode == USB_CPU )
				writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|
					INT_ENUMDONE|INT_RESET|INT_SUSPEND|
					INT_RX_FIFO_NOT_EMPTY,
					S5P_OTG_GINTMSK);
			return;
		}

	} else if ((rx_status & (0xf<<17)) == GLOBAL_OUT_NAK) {
		DBG_SETUP1("GLOBAL_OUT_NAK\n");

	} else if ((rx_status & (0xf<<17)) == OUT_TRNASFER_COMPLETED) {
		DBG_SETUP1("OUT_TRNASFER_COMPLETED\n");

	} else if ((rx_status & (0xf<<17)) == SETUP_TRANSACTION_COMPLETED) {
		DBG_SETUP1("SETUP_TRANSACTION_COMPLETED\n");

	} else {
		DBG_SETUP1("Reserved\n");
	}
}

void fboot_usb_transfer(void)
{
	u32 ep_int;
	u32 check_dma;
	u32 ep_int_status;
	u32 DIEPCTL_IN=readl(S5P_OTG_DIEPCTL_IN);
	u32 DIEPINT_IN=readl(S5P_OTG_DIEPINT_IN);
	u32 tmp;
	//u32 DIEPMSK=readl(S5P_OTG_DIEPMSK);

	ep_int = readl(S5P_OTG_DAINT);
		DBG_SETUP0("\n");
	//DBG_SETUP0("----------------------------------------------\n\tS5P_OTG_DAINT = 0x%x  ", ep_int);
	DBG_SETUP0("S5P_OTG_DAINT = 0x%x\n", ep_int);
		DBG_BULK1("rxf %d S5P_OTG_DIEPCTL_IN=0x%x ", fboot_response_flag,DIEPCTL_IN);
		//DBG_BULK1(" S5P_OTG_DIEPCTL_IN=0x%x\n", DIEPCTL_IN);
		//DBG_BULK1(" S5P_OTG_DIEPMSK=0x%x\n", DIEPMSK);
		DBG_BULK1("S5P_OTG_DIEPINT_IN=0x%x\n", DIEPINT_IN);
	if (ep_int & (1<<CONTROL_EP))	// Checking InEpInt
	{
		ep_int_status = readl(S5P_OTG_DIEPINT0);
		DBG_SETUP1("S5P_OTG_DIEPINT0 : %x \n", ep_int_status);

		if (ep_int_status & INTKN_TXFEMP) {
			u32 uNTxFifoSpace;
			do {
				uNTxFifoSpace=readl(S5P_OTG_GNPTXSTS)&0xffff;
			}while(uNTxFifoSpace<fboot_otg.ctrl_max_pktsize);

			fboot_usb_transfer_ep0();
		}

		writel(ep_int_status, S5P_OTG_DIEPINT0); /* Interrupt Clear */
	}

	else if (ep_int & ((1<<CONTROL_EP)<<16))	// Checking OutEPInt
	{
		ep_int_status = readl(S5P_OTG_DOEPINT0);
		DBG_SETUP1("S5P_OTG_DOEPINT0 : %x \n", ep_int_status);

		fboot_usb_set_outep_xfersize(EP_TYPE_CONTROL, 1, 8);
		writel(1u<<31|1<<26, S5P_OTG_DOEPCTL0); /*ep0 enable, clear nak */

		writel(ep_int_status, S5P_OTG_DOEPINT0); /* Interrupt Clear */
	}

	else if(ep_int & (1<<BULK_IN_EP)) {
#ifdef USB_FASTBOOT_TEST
		DBG_BULK1(" <BULK_IN_EP>\n");
#endif
		ep_int_status = readl(S5P_OTG_DIEPINT_IN);
		DBG_BULK1("S5P_OTG_DIEPINT_IN : %x \n", ep_int_status);
		writel(ep_int_status, S5P_OTG_DIEPINT_IN); /* Interrupt Clear */

		if ( (ep_int_status&INTKN_TXFEMP) && fboot_otg.op_mode == USB_CPU)
		{
#ifdef USB_FASTBOOT_TEST
			fboot_usb_int_bulkin();
#else
			fboot_usb_int_bulkin();
#endif
		}

		check_dma = readl(S5P_OTG_GAHBCFG);
		if ((check_dma&MODE_DMA)&&(ep_int_status&TRANSFER_DONE))
			fboot_usb_dma_in_done();
	}

	else if (ep_int & ((1<<BULK_OUT_EP)<<16)) {
#ifdef USB_FASTBOOT_TEST
		DBG_BULK1(" <<{BULK_OUT_EP>>>>>>\n");
#endif
		ep_int_status = readl(S5P_OTG_DOEPINT_OUT);
		DBG_BULK1("S5P_OTG_DOEPINT_OUT : 0x%x\n", ep_int_status);
		writel(ep_int_status, S5P_OTG_DOEPINT_OUT); /* Interrupt Clear */

		check_dma = readl(S5P_OTG_GAHBCFG);
		if ((check_dma&MODE_DMA)&&(ep_int_status&TRANSFER_DONE)) {
			fboot_usb_dma_out_done();
		}
	}
}

int  fboot_usb_int_hndlr(void)
{
	u32 int_status;
	int tmp;
	u32 val;

	int ret = ERROR;
	int_status = readl(S5P_OTG_GINTSTS); /* Core Interrupt Register */
	writel(int_status, S5P_OTG_GINTSTS); /* Interrupt Clear */
	DBG_SETUP0("*** USB OTG Interrupt(S5P_OTG_GINTSTS: 0x%08x) ****\n",
		int_status);

	if (int_status & INT_RESET) {
		DBG_SETUP1("INT_RESET\n");
		writel(INT_RESET, S5P_OTG_GINTSTS); /* Interrupt Clear */

		fboot_usb_reset();
		ret= OK;
	}

	if (int_status & INT_ENUMDONE) {
		DBG_SETUP1("INT_ENUMDONE :");
		writel(INT_ENUMDONE, S5P_OTG_GINTSTS); /* Interrupt Clear */

		tmp = fboot_usb_set_init();
		ret= OK;
		if (tmp == FALSE)
			return ret;

	}

	if (int_status & INT_RESUME) {
		DBG_SETUP1("INT_RESUME\n");
		writel(INT_RESUME, S5P_OTG_GINTSTS); /* Interrupt Clear */

		if(SUSPEND_RESUME_ON) {
			writel(readl(S5P_OTG_PCGCCTL)&~(1<<0), S5P_OTG_PCGCCTL);
			DBG_SETUP1("INT_RESUME\n");
		}
		ret= OK;
	}

	if (int_status & INT_SUSPEND) {
		DBG_SETUP1("INT_SUSPEND\n");
		writel(INT_SUSPEND, S5P_OTG_GINTSTS); /* Interrupt Clear */

		if(SUSPEND_RESUME_ON) {
			writel(readl(S5P_OTG_PCGCCTL)|(1<<0), S5P_OTG_PCGCCTL);
		}
		ret= OK;
	}

	if(int_status & INT_RX_FIFO_NOT_EMPTY) {
		DBG_SETUP1("INT_RX_FIFO_NOT_EMPTY\n");
		/* Read only register field */

		writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|
			INT_ENUMDONE|INT_RESET|INT_SUSPEND,
			S5P_OTG_GINTMSK);
		DBG_SETUP1("\t");
		fboot_usb_pkt_receive();
		writel(INT_RESUME|INT_OUT_EP|INT_IN_EP|INT_ENUMDONE|
			INT_RESET |INT_SUSPEND|INT_RX_FIFO_NOT_EMPTY,
			S5P_OTG_GINTMSK); /*gint unmask */
		ret= OK;
	}

	if ((int_status & INT_IN_EP) || (int_status & INT_OUT_EP)) {
		if (int_status&INT_IN_EP)
		DBG_SETUP1("INT_IN__EP\n");
		else
		DBG_SETUP1("INT_OUT_EP\n");
		/* Read only register field */

		fboot_usb_transfer();
		ret= OK;
	}
	/*
	else if(int_status & INT_HOST_CH) {
		DBG_SETUP1("--------------- INT_HOST_CH: ignored now\n");
		val= readl(S5P_OTG_GINTMSK);
		if (val & INT_HOST_CH)
			DBG_SETUP1("weird!!! it should have been masked.");
		else
			writel(val|INT_HOST_CH, S5P_OTG_GINTMSK);
		ret= OK;
	}
	*/
	DBG_SETUP1("\n");
	return ret;
}

//-----------------------------------------------------------------------------------
// FASTBOOT codes
//-----------------------------------------------------------------------------------

/* A board specific test if u-boot should go into the fastboot command
   ahead of the bootcmd
   Returns 0 to continue with normal u-boot flow
   Returns 1 to execute fastboot */
int fastboot_preboot(void)
{
	if (memcmp((const char *)CFG_FASTBOOT_TRANSFER_BUFFER,
		FASTBOOT_REBOOT_MAGIC, FASTBOOT_REBOOT_MAGIC_SIZE) == 0)
	{
		return 1;
	}

	/* Test code */
	writel(0xF, 0xE1600000);
	writel(readl(0xE1600010) | 0x1, 0xE1600010);

	writel(0x33333333&0xFFFF0000|(0x1111), 0xE0200C40);//forlinx.bs four leds
	writel(0x0, 0xE0200C48);
	writel(0x33333333, 0xE0200C60);
	writel(0xAAAA, 0xE0200C68);

	writel(readl(0xE1600008) & ~0xffff, 0xE1600008);

	/* It seems that we require a little time before reading keypad */
	printf("checking mode for fastboot ...\n");

	//if (!(readl(0xE160000C) & 0x82))
	if (!(readl(0xE160000C) & 0x80))
	{
		return 1;
	}

	return 0;
}

static void set_serial_number(void)
{
	char *dieid = getenv("dieid#");
	if (dieid == NULL) {
		device_strings[DEVICE_STRING_SERIAL_NUMBER_INDEX] = "SLSI0123";
	} else {
		static char serial_number[32];
		int len;

		memset(&serial_number[0], 0, 32);
		len = strlen(dieid);
		if (len > 30)
			len = 30;

		strncpy(&serial_number[0], dieid, len);

		device_strings[DEVICE_STRING_SERIAL_NUMBER_INDEX] =
			&serial_number[0];
	}
}

/* Initizes the board specific fastboot
   Returns 0 on success
   Returns 1 on failure */
int fastboot_init(struct cmd_fastboot_interface *interface)
{
	int ret = 1;
	u8 devctl;

	DBG0("\n");

	// usbd init
	fboot_usbctl_init();
	// usbd activate
	//fboot_usbc_activate();

	device_strings[DEVICE_STRING_MANUFACTURER_INDEX]  = "Samsung S.LSI";
#if defined(CONFIG_SMDKC100)
	device_strings[DEVICE_STRING_PRODUCT_INDEX]       = "smdkc100";
#elif defined(CONFIG_SMDKC110)
#if !defined(CONFIG_MCP_SINGLE)
	device_strings[DEVICE_STRING_PRODUCT_INDEX]       = "smdkc110";
#else
	device_strings[DEVICE_STRING_PRODUCT_INDEX]       = "smdkv210";
#endif
#elif defined(CONFIG_TINY210)
        device_strings[DEVICE_STRING_PRODUCT_INDEX]       = "tiny100";
#else
	/* Default, An error message to prompt user */
#error "Need a product name for fastboot"

#endif
	set_serial_number();
	/* These are just made up */
	device_strings[DEVICE_STRING_CONFIG_INDEX]        = "Android Fastboot";
	device_strings[DEVICE_STRING_INTERFACE_INDEX]     = "Android Fastboot";

	/* The interface structure */
	fastboot_interface = interface;
	fastboot_interface->product_name                  = device_strings[DEVICE_STRING_PRODUCT_INDEX];
	fastboot_interface->serial_no                     = device_strings[DEVICE_STRING_SERIAL_NUMBER_INDEX];
	fastboot_interface->nand_block_size               = CFG_FASTBOOT_PAGESIZE * 64;
	fastboot_interface->transfer_buffer               = (unsigned char *) CFG_FASTBOOT_TRANSFER_BUFFER;
	fastboot_interface->transfer_buffer_size          = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE;

	memset((unsigned char *) CFG_FASTBOOT_TRANSFER_BUFFER, 0x0, FASTBOOT_REBOOT_MAGIC_SIZE);

	ret = 0;	// This is a fake return value, because we do not initialize USB yet!

	return ret;
}

/* Cleans up the board specific fastboot */
void fastboot_shutdown(void)
{
	DBG0("\n");
	/* when operation is done, usbd must be stopped */
	fboot_usb_stop();
}
int fastboot_fifo_size(void)
{
	return (fboot_otg.speed== USB_HIGH) ? RX_ENDPOINT_MAXIMUM_PACKET_SIZE_2_0 : RX_ENDPOINT_MAXIMUM_PACKET_SIZE_1_1;
}

/*
 * Handles board specific usb protocol exchanges
 * Returns 0 on success
 * Returns 1 on disconnects, break out of loop
 * Returns 2 if no USB activity detected
 * Returns -1 on failure, unhandled usb requests and other error conditions
*/
int fastboot_poll(void)
{
	//printf("DEBUG: %s is called.\n", __FUNCTION__);
	/* No activity */
	int ret = FASTBOOT_INACTIVE;

	u32 intrusb;
	u16 intrtx;
	u16 intrrx;

	/* Look at the interrupt registers */
	intrusb =  readl(S5P_OTG_GINTSTS);

	/* A disconnect happended, this signals that the cable
	   has been disconnected, return immediately */
	if (!FBOOT_USBD_IS_CONNECTED())
		return FASTBOOT_DISCONNECT;

	else if (FBOOT_USBD_DETECT_IRQ()) {
		if (!fboot_usb_int_hndlr())
			ret = FASTBOOT_OK;
		else
			ret = FASTBOOT_ERROR;
		FBOOT_USBD_CLEAR_IRQ();
	}

	return ret;
}


/* Send a status reply to the client app
   buffer does not have to be null terminated.
   buffer_size must be not be larger than what is returned by
   fastboot_fifo_size
   Returns 0 on success
   Returns 1 on failure */
int fastboot_tx_status(const char *buffer, unsigned int buffer_size, const u32 need_sync_flag)
{
	int ret = 1;
	unsigned int i;
	/* fastboot client only reads back at most 64 */
	transfer_size = MIN(64, buffer_size);

	DBG0("    Response - \"%s\" (%d bytes)\n", buffer, buffer_size);
//------------------------------ kdj
	//printf("    Response - \"%s\" (%d bytes)\n", buffer, buffer_size);
	reply_msg = buffer;
	fboot_response_flag=1;
	if (need_sync_flag)
	{
		while(fboot_response_flag)
			fastboot_poll();
	}
	return 1;
}

/* A board specific variable handler.
   The size of the buffers is governed by the fastboot spec.
   rx_buffer is at most 57 bytes
   tx_buffer is at most 60 bytes
   Returns 0 on success
   Returns 1 on failure */
int fastboot_getvar(const char *rx_buffer, char *tx_buffer)
{
	DBG0("\n");
	/* Place board specific variables here */
	return 1;
}

#endif /* CONFIG_FASTBOOT */
