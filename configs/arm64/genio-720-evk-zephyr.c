/*
 * GENIO-720-EVK target - zephyr
 *
 * Copyright 2025 MediaTek
 *
 * Authors:
 *   Felix Freimann <felix.freimann@mediatek.com>
 *   Andres Campos <andres@proventusnova.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[4];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_vendor vendors[3];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name = "zephyr",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG | JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.cpu_set_size       = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips       = ARRAY_SIZE(config.irqchips),
		.num_vendors        = ARRAY_SIZE(config.vendors),

		.cpu_reset_address = CONFIG_INMATE_BASE,

		.console = {
			.address = 0x11002000,
			.divider = 0x2a,			/* baudrate = 38400 */
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO | JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0b00100000,
	},

	.mem_regions = {
		/* UART1 */
        {
			.phys_start = 0x11002000,
			.virt_start = 0x11002000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED | JAILHOUSE_MEM_IO_32,
		},
		/* SPI1 to the LoRa radio (controller at page offset 0x800;
		 * whole page mapped, nothing else lives at 0x11011000)
		 */
		{
			.phys_start = 0x11011000,
			.virt_start = 0x11011000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED | JAILHOUSE_MEM_IO_32,
		},
		/* Inmate memory: 0x45000000 - 0x48000000 (48 MB) */
		{
			.phys_start = 0x45000000,
			.virt_start = CONFIG_INMATE_BASE,
			.size = 0x03000000,   // 48 MB
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* Communication region */
		{
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_COMM_REGION,
		},
	},

	.irqchips = {
		/* GIC: UART1 (SPI 398 -> INTID 430) and SPI1 (SPI 460 ->
		 * INTID 492, word 2 bit 12). The inmate polls the SPI status
		 * instead of taking the interrupt, but owning the INTID here
		 * revokes it from the root cell so the still-bound Linux
		 * spi-mt65xx ISR cannot race the inmate for the read-to-clear
		 * status register. No EINT interrupt is routed: on this SoC
		 * (mt8189) all five EINT instances share one GIC line the
		 * Linux root cell depends on; the inmate detects GPIO edges
		 * by polling instead.
		 */
		{
			.address = 0x0c000000,
			.pin_base = 416,
			.pin_bitmap = {
				0x00004000, 0x00000000, 0x00001000, 0x00000000
			},
		},
	},

	.vendors = {
		/* EINT on mt8189 is five per-cluster instances with a per-pad
		 * (instance, index) map, NOT the single mt8390-style block at
		 * 0x1000b000 this config used to name. This entry grants
		 * instance 0 ("eint-e" @ 0x11ce0000) local index 46 = pad 110
		 * (LoRa DIO1) for the future dedicated-EINT port; the current
		 * inmate polls the GPIO instead and never touches it.
		 */
		{
			.type = JAILHOUSE_VENDOR_MTK_EINT,
			.mtk_eint.address    = 0x11ce0000,
			.mtk_eint.pin_base   = 32,
			.mtk_eint.pin_bitmap = {
				0x00004000, 0x00000000, 0x00000000, 0x00000000	/* local line 46 = pad 110 DIO1 */
			}
		},
		{
			.type = JAILHOUSE_VENDOR_MTK_GPIO,
			.mtk_gpio.address    = 0x10005000,
			.mtk_gpio.pin_base   = 32,
			.mtk_gpio.pin_bitmap = {
				0x00000146, 0x00000000, 0x00000000, 0x00000000	/* Pins 33 & 34 for UART1; GPIO 38 & 40 */
			}
		},
		{
			.type = JAILHOUSE_VENDOR_MTK_GPIO,
			.mtk_gpio.address    = 0x10005000,
			.mtk_gpio.pin_base   = 64,
			.mtk_gpio.pin_bitmap = {
				0x00001e00, 0x00007000, 0x00000000, 0x00000000	/* 73-76 SPI1 CS(GPIO)/CLK/MOSI/MISO; 108-110 LoRa reset/busy/dio1 */
			}
		}
/*
		,
		{
			.type = JAILHOUSE_VENDOR_MTK_GPIO,
			.mtk_gpio.address    = 0x10005000,
			.mtk_gpio.pin_base   = 96,
			.mtk_gpio.pin_bitmap = {
				0x00000140, 0x00000000, 0x00000000, 0x00000000	/ * GPIO 38 & 40 * /
			}
		}
*/
	},
};
