/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 *
 * Aneesh V <aneesh@ti.com>
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
#include <common.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_DSU_SUPPORT
void spl_dsu_load_image(void)
{
	u32 loadaddr;
	u32 size;
	int err;
	struct image_header *header;

	err = dsudownload(&loadaddr, &size);

	if (err) {
		serial_printf("usb download failed");
		hang();
	}
	header = (struct image_header *)loadaddr;
	spl_parse_image_header(header);
	memcpy((void *)spl_image.load_addr, (void *)loadaddr, spl_image.size);
}
#endif
