/*
 * Copyright (C) 2011 Linaro
 *
 * Original based on fastboot.c from omapzoom u-boot and usbtty.c
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
 */

#include <common.h>
#include <command.h>
#include <asm/unaligned.h>

#define CONFIG_USBD_MANUFACTURER "linaro"
#define CONFIG_USBD_PRODUCT_NAME "dsuboot"
#define DSUBOOT_PRODUCT_NAME "dsuboot"
#define CONFIG_USBD_VENDORID	0x0525 /* netchip id used for many Linux-USB stuff */
#define CONFIG_USBD_PRODUCTID	0xFFFF
#define CONFIG_DSUBOOT_TRANSFER_BUFFER	0x90000000
#define CONFIG_DSUBOOT_TRANSFER_BUFFER_SIZE 64*1024*1024
//#define CONFIG_USBD_DSUBOOT_BULK_PKTSIZE_HS 512
#define CONFIG_USBD_DSUBOOT_BULK_PKTSIZE_HS 64

#define DSUBOOT_INTERFACE_CLASS     0xff
#define DSUBOOT_INTERFACE_SUB_CLASS 0x42
#define DSUBOOT_INTERFACE_PROTOCOL  0x03

#define	ERR
#define	WARN
/*#define INFO */
/*#define DEBUG*/
#define	SHOW_DOWNLOAD_PROGRESS

#ifdef DEBUG
#define DSUBTDBG(fmt,args...)\
        printf("DEBUG: [%s]: %d: \n"fmt, __FUNCTION__, __LINE__,##args)
#else
#define DSUBTDBG(fmt,args...) do{}while(0)
#endif

#ifdef INFO
#define DSUBTINFO(fmt,args...)\
        printf("INFO: [%s]: "fmt, __FUNCTION__, ##args)
#else
#define DSUBTINFO(fmt,args...) do{}while(0)
#endif

#ifdef WARN
#define DSUBTWARN(fmt,args...)\
        printf("WARNING: [%s]: "fmt, __FUNCTION__, ##args)
#else
#define DSUBTWARN(fmt,args...) do{}while(0)
#endif

#ifdef ERR
#define DSUBTERR(fmt,args...)\
        printf("ERROR: [%s]: "fmt, __FUNCTION__, ##args)
#else
#define DSUBTERR(fmt,args...) do{}while(0)
#endif

/* USB specific */

#include <usb_defs.h>

#if defined(CONFIG_PPC)
#include <usb/mpc8xx_udc.h>
#elif defined(CONFIG_OMAP1510)
#include <usb/omap1510_udc.h>
#elif defined(CONFIG_MUSB_UDC)
#include <usb/musb_udc.h>
#elif defined(CONFIG_PXA27X)
#include <usb/pxa27x_udc.h>
#elif defined(CONFIG_SPEAR3XX) || defined(CONFIG_SPEAR600)
#include <usb/spr_udc.h>
#endif

#define STR_LANG		0x00
#define STR_MANUFACTURER	0x01
#define STR_PRODUCT		0x02
#define STR_SERIAL		0x03
#define STR_CONFIGURATION	0x04
#define STR_INTERFACE		0x05
#define STR_COUNT		0x06

#define CONFIG_USBD_CONFIGURATION_STR	"dsuboot configuration"
#define CONFIG_USBD_INTERFACE_STR	"dsuboot interface"

#define USBDSUBT_BCD_DEVICE	0x0
#define	USBDSUBT_MAXPOWER	0x32

#define	NUM_CONFIGS	1
#define	NUM_INTERFACES	1
#define	NUM_ENDPOINTS	2

#define	RX_EP_INDEX	1
#define	TX_EP_INDEX	2

struct _dsubt_config_desc {
	struct usb_configuration_descriptor configuration_desc;
	struct usb_interface_descriptor interface_desc;
	struct usb_endpoint_descriptor endpoint_desc[NUM_ENDPOINTS];
};

static int dsubt_handle_response(void);

/* defined and used by gadget/ep0.c */
extern struct usb_string_descriptor **usb_strings;

/* USB Descriptor Strings */
static char serial_number[28]; /* what should be the length ?, 28 ? */
static u8 wstr_lang[4] = {4,USB_DT_STRING,0x9,0x4};
static u8 wstr_manufacturer[2 + 2*(sizeof(CONFIG_USBD_MANUFACTURER)-1)];
static u8 wstr_product[2 + 2*(sizeof(CONFIG_USBD_PRODUCT_NAME)-1)];
static u8 wstr_serial[2 + 2*(sizeof(serial_number) - 1)];
static u8 wstr_configuration[2 + 2*(sizeof(CONFIG_USBD_CONFIGURATION_STR)-1)];
static u8 wstr_interface[2 + 2*(sizeof(CONFIG_USBD_INTERFACE_STR)-1)];

/* USB descriptors */
static struct usb_device_descriptor device_descriptor = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(USB_BCD_VERSION),
	.bDeviceClass =		0xFF,
	.bDeviceSubClass =	0xFF,
	.bDeviceProtocol =	0xFF,
	.bMaxPacketSize0 =	EP0_MAX_PACKET_SIZE,
	.idVendor =		cpu_to_le16(CONFIG_USBD_VENDORID),
	.idProduct =		cpu_to_le16(CONFIG_USBD_PRODUCTID),
	.bcdDevice =		cpu_to_le16(USBDSUBT_BCD_DEVICE),
	.iManufacturer =	STR_MANUFACTURER,
	.iProduct =		STR_PRODUCT,
	.iSerialNumber =	STR_SERIAL,
	.bNumConfigurations =	NUM_CONFIGS
};

static struct _dsubt_config_desc dsubt_config_desc = {
	.configuration_desc = {
		.bLength = sizeof(struct usb_configuration_descriptor),
		.bDescriptorType = USB_DT_CONFIG,
		.wTotalLength =	cpu_to_le16(sizeof(struct _dsubt_config_desc)),
		.bNumInterfaces = NUM_INTERFACES,
		.bConfigurationValue = 1,
		.iConfiguration = STR_CONFIGURATION,
		.bmAttributes =	BMATTRIBUTE_SELF_POWERED | BMATTRIBUTE_RESERVED,
		.bMaxPower = USBDSUBT_MAXPOWER,
	},
	.interface_desc = {
		.bLength  = sizeof(struct usb_interface_descriptor),
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0x2,
		.bInterfaceClass = DSUBOOT_INTERFACE_CLASS,
		.bInterfaceSubClass = DSUBOOT_INTERFACE_SUB_CLASS,
		.bInterfaceProtocol = DSUBOOT_INTERFACE_PROTOCOL,
		.iInterface = STR_INTERFACE,
	},
	.endpoint_desc = {
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = RX_EP_INDEX | USB_DIR_OUT,
			.bmAttributes =	USB_ENDPOINT_XFER_BULK,
			.bInterval = 0,
		},
		{
			.bLength = sizeof(struct usb_endpoint_descriptor),
			.bDescriptorType = USB_DT_ENDPOINT,
			.bEndpointAddress = TX_EP_INDEX | USB_DIR_IN,
			.bmAttributes = USB_ENDPOINT_XFER_BULK,
			.bInterval = 0,
		},
	},
};

static struct usb_interface_descriptor interface_descriptors[NUM_INTERFACES];
static struct usb_endpoint_descriptor *ep_descriptor_ptrs[NUM_ENDPOINTS];

static struct usb_string_descriptor *dsubt_string_table[STR_COUNT];
static struct usb_device_instance device_instance[1];
static struct usb_bus_instance bus_instance[1];
static struct usb_configuration_instance config_instance[NUM_CONFIGS];
static struct usb_interface_instance interface_instance[NUM_INTERFACES];
static struct usb_alternate_instance alternate_instance[NUM_INTERFACES];
static struct usb_endpoint_instance endpoint_instance[NUM_ENDPOINTS + 1];

/* U-boot version */
extern char version_string[];

struct priv_data {
        void *transfer_buffer;
        int transfer_buffer_size;
	char *serial_no;
	int flag;
	int d_size;
	int d_bytes;
	int bs;
	int exit;
	unsigned char *product_name;
	int configured;
	char response[65];
};

#define FASTBOOT_FLAG_RESPONSE 1

static struct priv_data priv =
{
        .transfer_buffer       = (void *)CONFIG_DSUBOOT_TRANSFER_BUFFER,
        .transfer_buffer_size  = CONFIG_DSUBOOT_TRANSFER_BUFFER_SIZE,
};

static int dsubt_init_endpoints (void);

/* USB specific */

/* utility function for converting char* to wide string used by USB */
static void str2wide (char *str, u16 * wide)
{
	int i;
	for (i = 0; i < strlen (str) && str[i]; i++){
		#if defined(__LITTLE_ENDIAN)
			wide[i] = (u16) str[i];
		#elif defined(__BIG_ENDIAN)
			wide[i] = ((u16)(str[i])<<8);
		#else
			#error "__LITTLE_ENDIAN or __BIG_ENDIAN undefined"
		#endif
	}
}

/* dsuboot_init has to be called before this fn to get correct serial string */
static int dsubt_init_strings(void)
{
	struct usb_string_descriptor *string;

	dsubt_string_table[STR_LANG] =
		(struct usb_string_descriptor*)wstr_lang;

	string = (struct usb_string_descriptor *) wstr_manufacturer;
	string->bLength = sizeof(wstr_manufacturer);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_MANUFACTURER, string->wData);
	dsubt_string_table[STR_MANUFACTURER] = string;

	string = (struct usb_string_descriptor *) wstr_product;
	string->bLength = sizeof(wstr_product);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_PRODUCT_NAME, string->wData);
	dsubt_string_table[STR_PRODUCT] = string;

	string = (struct usb_string_descriptor *) wstr_serial;
	string->bLength = sizeof(wstr_serial);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (serial_number, string->wData);
	dsubt_string_table[STR_SERIAL] = string;

	string = (struct usb_string_descriptor *) wstr_configuration;
	string->bLength = sizeof(wstr_configuration);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_CONFIGURATION_STR, string->wData);
	dsubt_string_table[STR_CONFIGURATION] = string;

	string = (struct usb_string_descriptor *) wstr_interface;
	string->bLength = sizeof(wstr_interface);
	string->bDescriptorType = USB_DT_STRING;
	str2wide (CONFIG_USBD_INTERFACE_STR, string->wData);
	dsubt_string_table[STR_INTERFACE] = string;

	/* Now, initialize the string table for ep0 handling */
	usb_strings = dsubt_string_table;

	return 0;
}

#define init_wMaxPacketSize(x)	le16_to_cpu(get_unaligned(\
			&ep_descriptor_ptrs[(x) - 1]->wMaxPacketSize));

static void dsubt_event_handler (struct usb_device_instance *device,
				  usb_device_event_t event, int data)
{
	switch (event) {
	case DEVICE_RESET:
	case DEVICE_BUS_INACTIVE:
		priv.configured = 0;
		DSUBTINFO("dsuboot configured = 0\n");
		break;
	case DEVICE_CONFIGURED:
		priv.configured = 1;
		DSUBTINFO("dsuboot configured = 1\n");
		break;

	case DEVICE_ADDRESS_ASSIGNED:
		dsubt_init_endpoints ();
		DSUBTINFO("dsuboot endpoints init'd\n");

	default:
		break;
	}
}

/* dsuboot_init has to be called before this fn to get correct serial string */
static int dsubt_init_instances(void)
{
	int i;

	/* initialize device instance */
	memset (device_instance, 0, sizeof (struct usb_device_instance));
	device_instance->device_state = STATE_INIT;
	device_instance->device_descriptor = &device_descriptor;
	device_instance->event = dsubt_event_handler;
	device_instance->cdc_recv_setup = NULL;
	device_instance->bus = bus_instance;
	device_instance->configurations = NUM_CONFIGS;
	device_instance->configuration_instance_array = config_instance;

	/* XXX: what is this bus instance for ?, can't it be removed by moving
	    endpoint_array and serial_number_str is moved to device instance */
	/* initialize bus instance */
	memset (bus_instance, 0, sizeof (struct usb_bus_instance));
	bus_instance->device = device_instance;
	bus_instance->endpoint_array = endpoint_instance;
	/* XXX: what is the relevance of max_endpoints & maxpacketsize ? */
	bus_instance->max_endpoints = 1;
	bus_instance->maxpacketsize = 64;
	bus_instance->serial_number_str = serial_number;

	/* configuration instance */
	memset (config_instance, 0,
		sizeof (struct usb_configuration_instance));
	config_instance->interfaces = NUM_INTERFACES;
	config_instance->configuration_descriptor =
		(struct usb_configuration_descriptor *)&dsubt_config_desc;
	config_instance->interface_instance_array = interface_instance;

	/* XXX: is alternate instance required in case of no alternate ? */
	/* interface instance */
	memset (interface_instance, 0,
		sizeof (struct usb_interface_instance));
	interface_instance->alternates = 1;
	interface_instance->alternates_instance_array = alternate_instance;

	/* alternates instance */
	memset (alternate_instance, 0,
		sizeof (struct usb_alternate_instance));
	alternate_instance->interface_descriptor = interface_descriptors;
	alternate_instance->endpoints = NUM_ENDPOINTS;
	alternate_instance->endpoints_descriptor_array = ep_descriptor_ptrs;

	/* endpoint instances */
	memset (&endpoint_instance[0], 0,
		sizeof (struct usb_endpoint_instance));
	endpoint_instance[0].endpoint_address = 0;
	endpoint_instance[0].rcv_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].rcv_attributes = USB_ENDPOINT_XFER_CONTROL;
	endpoint_instance[0].tx_packetSize = EP0_MAX_PACKET_SIZE;
	endpoint_instance[0].tx_attributes = USB_ENDPOINT_XFER_CONTROL;
	/* XXX: following statement to done along with other endpoints
		at another place ? */
	udc_setup_ep (device_instance, 0, &endpoint_instance[0]);

	for (i = 1; i <= NUM_ENDPOINTS; i++) {
		memset (&endpoint_instance[i], 0,
			sizeof (struct usb_endpoint_instance));

		endpoint_instance[i].endpoint_address =
			ep_descriptor_ptrs[i - 1]->bEndpointAddress;

		endpoint_instance[i].rcv_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].rcv_packetSize = init_wMaxPacketSize(i);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		endpoint_instance[i].tx_packetSize = init_wMaxPacketSize(i);

		endpoint_instance[i].tx_attributes =
			ep_descriptor_ptrs[i - 1]->bmAttributes;

		urb_link_init (&endpoint_instance[i].rcv);
		urb_link_init (&endpoint_instance[i].rdy);
		urb_link_init (&endpoint_instance[i].tx);
		urb_link_init (&endpoint_instance[i].done);

		if (endpoint_instance[i].endpoint_address & USB_DIR_IN)
			endpoint_instance[i].tx_urb =
				usbd_alloc_urb (device_instance,
						&endpoint_instance[i]);
		else
			endpoint_instance[i].rcv_urb =
				usbd_alloc_urb (device_instance,
						&endpoint_instance[i]);
	}

	return 0;
}

/* XXX: ep_descriptor_ptrs can be removed by making better use of
	dsubt_config_desc.endpoint_desc */
static int dsubt_init_endpoint_ptrs(void)
{
	ep_descriptor_ptrs[0] = &dsubt_config_desc.endpoint_desc[0];
	ep_descriptor_ptrs[1] = &dsubt_config_desc.endpoint_desc[1];

	return 0;
}

static int dsubt_init_endpoints(void)
{
	int i;

	/* XXX: should it be moved to some other function ? */
	bus_instance->max_endpoints = NUM_ENDPOINTS + 1;

	/* XXX: is this for loop required ?, yes for MUSB it is */
	for (i = 1; i <= NUM_ENDPOINTS; i++) {

		/* configure packetsize based on HS negotiation status */
		DSUBTINFO("setting up HS USB device ep%x\n",
			endpoint_instance[i].endpoint_address);
		ep_descriptor_ptrs[i - 1]->wMaxPacketSize =
			CONFIG_USBD_DSUBOOT_BULK_PKTSIZE_HS;

		endpoint_instance[i].tx_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);
		endpoint_instance[i].rcv_packetSize =
			le16_to_cpu(ep_descriptor_ptrs[i - 1]->wMaxPacketSize);

		udc_setup_ep (device_instance, i, &endpoint_instance[i]);

	}

	return 0;
}

static struct urb *next_urb (struct usb_device_instance *device,
			     struct usb_endpoint_instance *endpoint)
{
	struct urb *current_urb = NULL;
	int space;

	/* If there's a queue, then we should add to the last urb */
	if (!endpoint->tx_queue) {
		current_urb = endpoint->tx_urb;
	} else {
		/* Last urb from tx chain */
		current_urb =
			p2surround (struct urb, link, endpoint->tx.prev);
	}

	/* Make sure this one has enough room */
	space = current_urb->buffer_length - current_urb->actual_length;
	if (space > 0) {
		return current_urb;
	} else {		/* No space here */
		/* First look at done list */
		current_urb = first_urb_detached (&endpoint->done);
		if (!current_urb) {
			current_urb = usbd_alloc_urb (device, endpoint);
		}

		urb_append (&endpoint->tx, current_urb);
		endpoint->tx_queue++;
	}
	return current_urb;
}

static int dsubt_dsuboot_init(void)
{
	priv.flag = 0;
	priv.d_size = 0;
	priv.exit = 0;
	priv.serial_no = (char *)"00123";
	priv.product_name = (unsigned char *)DSUBOOT_PRODUCT_NAME;
	priv.bs = 1024;
	return 0;
}

static int dsubt_rx_process(unsigned char *buffer, int length)
{
	/*
	 * Rather than roll our own protocol we just implement the fastboot
	 * download command.
	 * If a dfu or fastboot implementation ever goes mainline then try to share
	 * code then.
	 */
	/* Generic failed response */
	strcpy(priv.response, "FAIL");

	if (!priv.d_size) {
		/* command */
		char *cmdbuf = (char *) buffer;

		if(memcmp(cmdbuf, "download:", 9) == 0) {
			DSUBTDBG("download\n");

			/* XXX: need any check for size & bytes ? */
			priv.d_size =
				simple_strtoul (cmdbuf + 9, NULL, 16);
			priv.d_bytes = 0;

#ifdef SHOW_DOWNLOAD_PROGRESS
			printf("starting download of %d bytes\n",
				priv.d_size);
#endif

			if (priv.d_size == 0) {
				strcpy(priv.response, "FAILdata invalid size");
			} else if (priv.d_size >
					priv.transfer_buffer_size) {
				priv.d_size = 0;
				strcpy(priv.response, "FAILdata too large");
			} else {
				sprintf(priv.response, "DATA%08x", priv.d_size);
			}
		}

		priv.flag |= FASTBOOT_FLAG_RESPONSE;
	} else {
		if (length) {
			unsigned int xfr_size;

			xfr_size = priv.d_size - priv.d_bytes;
			if (xfr_size > length)
				xfr_size = length;
			memcpy(priv.transfer_buffer + priv.d_bytes,
				buffer, xfr_size);
			priv.d_bytes += xfr_size;

#ifdef SHOW_DOWNLOAD_PROGRESS
			/* Inform via prompt that download is happening */
			if (! (priv.d_bytes % (16 * priv.bs)))
				printf(".");
			if (! (priv.d_bytes % (80 * 16 * priv.bs)))
				printf("\n");
#endif
			if (priv.d_bytes >= priv.d_size) {
				priv.d_size = 0;
				strcpy(priv.response, "OKAY");
				priv.flag |= FASTBOOT_FLAG_RESPONSE;
#ifdef SHOW_DOWNLOAD_PROGRESS
				printf(".\n");
#endif
				printf("downloaded %d bytes\n", priv.d_bytes);
				priv.exit |= 1;
			}
		} else
			DSUBTWARN("empty buffer download\n");
	}

	return 0;
}

static int dsubt_handle_rx(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[RX_EP_INDEX];

	if (ep->rcv_urb->actual_length) {
		DSUBTDBG("rx length: %u\n", ep->rcv_urb->actual_length);
		dsubt_rx_process(ep->rcv_urb->buffer, ep->rcv_urb->actual_length);
		/* Required to poison rx urb buffer as in omapzoom ?,
		    yes, as dsuboot command are sent w/o NULL termination.
		    Attempt is made here to reduce poison length, may be safer
		    to posion the whole buffer, also it is assumed that at
		    the time of creation of urb it is poisoned 	*/
		memset(ep->rcv_urb->buffer, 0, ep->rcv_urb->actual_length);
		ep->rcv_urb->actual_length = 0;
	}

	return 0;
}

static int dsubt_response_process(void)
{
	struct usb_endpoint_instance *ep = &endpoint_instance[TX_EP_INDEX];
	struct urb *current_urb = NULL;
	unsigned char *dest = NULL;
	int n, ret = 0;

	current_urb = next_urb (device_instance, ep);
	if (!current_urb) {
		DSUBTERR("%s: current_urb NULL", __func__);
		return -1;
	}

	dest = current_urb->buffer + current_urb->actual_length;
	n = MIN (64, strlen(priv.response));
	memcpy(dest, priv.response, n);
	current_urb->actual_length += n;
	DSUBTDBG("response urb length: %u\n", current_urb->actual_length);
	if (ep->last == 0) {
		ret = udc_endpoint_write (ep);
		return ret;
	}

	return ret;
}

static int dsubt_handle_response(void)
{
	if (priv.flag & FASTBOOT_FLAG_RESPONSE) {
		dsubt_response_process();
		priv.flag &= ~FASTBOOT_FLAG_RESPONSE;
	}

	return 0;
}

int dsudownload(u32 *loadaddr, u32 *loadsize)
{
	printf("booting from usb peripheral port\n");
	dsubt_dsuboot_init();
	dsubt_init_endpoint_ptrs();
	if (udc_init() < 0) {
		DSUBTERR("%s: MUSB UDC init failure\n", __func__);
		return -1;
	}
	dsubt_init_strings();
	dsubt_init_instances();

	udc_startup_events (device_instance);
	udc_connect();

	DSUBTINFO("dsuboot initialized\n");

	while(1) {
		udc_irq();
		if (priv.configured) {
			dsubt_handle_rx();
			dsubt_handle_response();
		}
		priv.exit |= ctrlc();
		if (priv.exit) {
			printf("dsudownload end\n");
			break;
		}
	}

	*loadaddr = (u32)priv.transfer_buffer;
	*loadsize = priv.d_bytes;
	return 0;
}
