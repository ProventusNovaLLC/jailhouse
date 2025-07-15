/*
 * i350-Pumpkin target - int-latency
 *
 * Copyright 2025 MediaTek
 *
 * Authors:
 *   Felix Freimann <felix.freimann@mediatek.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[3];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_vendor vendors[3];
} __attribute__((packed)) config = {
	.cell = {
		.signature    = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision     = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name         = "int-latency",
		.flags        = JAILHOUSE_CELL_PASSIVE_COMMREG | JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.cpu_set_size       = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips       = ARRAY_SIZE(config.irqchips),
		.num_vendors        = ARRAY_SIZE(config.vendors),

		.cpu_reset_address = CONFIG_INMATE_BASE,

		.console = {
			.address = 0x11003000,
			.divider = 0x2a,			/* baudrate = 38400 */
			.type    = JAILHOUSE_CON_TYPE_8250,
			.flags   = JAILHOUSE_CON_ACCESS_MMIO | JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0b1000,
	},

	.mem_regions = {
		/* UART1 */
        {
			.phys_start = 0x11003000,
			.virt_start = 0x11003000,
			.size       = 0x1000,
			.flags      = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* Inmate memory: 0x67000000 - 0x67010000 */
		{
			.phys_start = 0x67000000,
			.virt_start = CONFIG_INMATE_BASE,
			.size       = 0x00010000,
			.flags      = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* Communication region */
		{
			.virt_start = 0x80000000,
			.size       = 0x00001000,
			.flags      = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_COMM_REGION,
		},
	},

	.irqchips = {
		/* GIC */
		{
			.address    = 0x0c000000,
			.pin_base   = 32,
			.pin_bitmap = {
				0x00000000, 0x00000000, 0x00000000, 0x00080000, /* SPI 147 */
			},
		},
	},
    
	.vendors = {
		{
			.type = JAILHOUSE_VENDOR_MTK_EINT,
			.mtk_eint.address    = 0x1000b000,
			.mtk_eint.pin_base   = 96,
			.mtk_eint.pin_bitmap = {
				0x00000c00, 0x00000000, 0x00000000, 0x00000000	/* EINT 106 & 107 */
			}
		},
		{
			.type = JAILHOUSE_VENDOR_MTK_GPIO,
			.mtk_gpio.address    = 0x10005000,
			.mtk_gpio.pin_base   = 32,
			.mtk_gpio.pin_bitmap = {
				0x00000060, 0x00000000, 0x00000c00, 0x00000000	/* Pins 37 & 38 for UART1, Pins 106 & 107 for GPIO */
			}
		},
	}
};
