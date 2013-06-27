/*
 * SAMSUNG EXYNOS USB HOST EHCI Controller
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *	Vivek Gautam <gautam.vivek@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <common.h>
#include <usb.h>
#include <asm/arch/cpu.h>
#include <asm/arch/ehci.h>
#include <asm/arch/system.h>
#include <asm/arch/power.h>
#include <asm/arch/gpio.h>
#include "ehci.h"

/* Setup the EHCI host controller. */
static void setup_usb_phy(struct exynos_usb_phy *usb)
{
	u32 hsic_ctrl;

	set_usbhost_mode(USB20_PHY_CFG_HOST_LINK_EN);

	set_usbhost_phy_ctrl(POWER_USB_HOST_PHY_CTRL_EN);

	clrbits_le32(&usb->usbphyctrl0,
			HOST_CTRL0_FSEL_MASK |
			HOST_CTRL0_COMMONON_N |
			/* HOST Phy setting */
			HOST_CTRL0_PHYSWRST |
			HOST_CTRL0_PHYSWRSTALL |
			HOST_CTRL0_SIDDQ |
			HOST_CTRL0_FORCESUSPEND |
			HOST_CTRL0_FORCESLEEP);

	setbits_le32(&usb->usbphyctrl0,
			/* Setting up the ref freq */
			(CLK_24MHZ << 16) |
			/* HOST Phy setting */
			HOST_CTRL0_LINKSWRST |
			HOST_CTRL0_UTMISWRST);
	udelay(10);
	clrbits_le32(&usb->usbphyctrl0,
			HOST_CTRL0_LINKSWRST |
			HOST_CTRL0_UTMISWRST);
	udelay(20);

	/* HSIC phy reset */
	clrbits_le32(&usb->hsicphyctrl1,
			HSIC_CTRL_FORCESUSPEND |
			HSIC_CTRL_FORCESLEEP |
			HSIC_CTRL_SIDDQ);

	clrbits_le32(&usb->hsicphyctrl2,
			HSIC_CTRL_FORCESUSPEND |
			HSIC_CTRL_FORCESLEEP |
			HSIC_CTRL_SIDDQ);

	hsic_ctrl = (((HSIC_CTRL_REFCLKDIV_12 & HSIC_CTRL_REFCLKDIV_MASK)
				<< HSIC_CTRL_REFCLKDIV_SHIFT)
			| ((HSIC_CTRL_REFCLKSEL & HSIC_CTRL_REFCLKSEL_MASK)
				<< HSIC_CTRL_REFCLKSEL_SHIFT)
			| HSIC_CTRL_PHYSWRST);

	setbits_le32(&usb->hsicphyctrl1, hsic_ctrl);
	setbits_le32(&usb->hsicphyctrl2, hsic_ctrl);

	udelay(10);

	clrbits_le32(&usb->hsicphyctrl1, HSIC_CTRL_PHYSWRST);
	clrbits_le32(&usb->hsicphyctrl2, HSIC_CTRL_PHYSWRST);

	udelay(80);

	/* EHCI Ctrl setting */
	setbits_le32(&usb->ehcictrl,
			EHCICTRL_ENAINCRXALIGN |
			EHCICTRL_ENAINCR4 |
			EHCICTRL_ENAINCR8 |
			EHCICTRL_ENAINCR16);
}

/* Reset the EHCI host controller. */
static void reset_usb_phy(struct exynos_usb_phy *usb)
{
	/* HOST_PHY reset */
	setbits_le32(&usb->usbphyctrl0,
			HOST_CTRL0_PHYSWRST |
			HOST_CTRL0_PHYSWRSTALL |
			HOST_CTRL0_SIDDQ |
			HOST_CTRL0_FORCESUSPEND |
			HOST_CTRL0_FORCESLEEP);

	set_usbhost_phy_ctrl(POWER_USB_HOST_PHY_CTRL_DISABLE);
}

struct exynos5_gpio_part1 *gpio;
/*
 * EHCI-initialization
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	struct exynos_usb_phy *usb;
	gpio = (struct exynos5_gpio_part1 *) EXYNOS5_GPIO_PART1_BASE;
	usb = (struct exynos_usb_phy *)samsung_get_base_usb_phy();

	s5p_gpio_direction_output(&gpio->x3, 5, 0);
	s5p_gpio_direction_output(&gpio->d1, 7, 0);

	setup_usb_phy(usb);

	s5p_gpio_direction_output(&gpio->x3, 5, 1);
	s5p_gpio_direction_output(&gpio->d1, 7, 1);

	*hccr = (struct ehci_hccr *)samsung_get_base_usb_ehci();
	*hcor = (struct ehci_hcor *)((uint32_t) hccr
				+ HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));
	debug("Exynos5-ehci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	struct exynos_usb_phy *usb;

	usb = (struct exynos_usb_phy *)samsung_get_base_usb_phy();
	reset_usb_phy(usb);

	return 0;
}
