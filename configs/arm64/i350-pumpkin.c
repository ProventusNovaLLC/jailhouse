/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Test configuration for i350-pumpkin (quad-core Cortex-A53, 2GB RAM)
 *
 * Copyright (c) MediaTek, 2024
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
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[9];
	struct jailhouse_irqchip irqchips[3];
	struct jailhouse_vendor vendors[4];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x66c00000,
			.size       = 0x00400000,
		},
		.debug_console = {
			.address = 0x11002000,
			.size = 0x1000,
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO | JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.arm = {
				.gic_version = 3,
				.gicd_base = 0x0c000000,
				.gicr_base = 0x0c080000,
				.maintenance_irq = 25,
			},
		},
		.root_cell = {
			.name = "i350-pumpkin",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_vendors = ARRAY_SIZE(config.vendors),
		},
	},

	.cpus = {
		0b1111,
	},

	.mem_regions = {
		/* MMIO:           0x00000000 - 0x0c000000 */
		/* GIC:            0x0c000000 - 0x0c100000 */
		/* MMIO:           0x0c100000 - 0x10005000 */
		/* GPIO:           0x10005000 - 0x10006000 */
		/* MMIO:           0x10006000 - 0x1000b000 */
		/* EINT:           0x1000b000 - 0x1000c000 */
		/* MMIO:           0x1000c000 - 0x20000000 */
		/* DRAM:           0x40000000 - 0xc0000000 */
		/* Secure Monitor: 0x43000000 - 0x43030000 */
		/* TEE:            0x43200000 - 0x43e00000 */
		/* DMA Pool:       0x60000000 - 0x64000000 */ 
		/* Linux kernel:   0x64000000 - 0x66bc0000 */
		/* Hypervisor:     0x66c00000 - 0x67000000 */

		/* MMIO:  0x00000000 - 0x0c000000 */
		{
			.phys_start = 0x00000000,
			.virt_start = 0x00000000,
			.size = 0x0c000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* GIC:  0x0c000000 - 0x0c100000 */
		/* MMIO:  0x0c100000 - 0x10005000 */
		{
			.phys_start = 0x0c100000,
			.virt_start = 0x0c100000,
			.size = 0x03f05000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* GPIO:  0x10005000 - 0x10006000 */
		/* MMIO:  0x10006000 - 0x1000b000 */
		{
			.phys_start = 0x10006000,
			.virt_start = 0x10006000,
			.size = 0x00005000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* EINT:  0x1000b000 - 0x1000c000 */
		/* MMIO:  0x1000c000 - 0x20000000 */
		{
			.phys_start = 0x1000c000,
			.virt_start = 0x1000c000,
			.size = 0x0fff4000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* DRAM:  0x40000000 - 0x60000000 */
		{
			.phys_start = 0x40000000,
			.virt_start = 0x40000000,
			.size = 0x20000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DMA Pool:  0x60000000 - 0x64000000 */
		{
			.phys_start = 0x60000000,
			.virt_start = 0x60000000,
			.size = 0x04000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE
		},
		/* DRAM:  0x64000000 - 0x66c00000 */
		{
			.phys_start = 0x64000000,
			.virt_start = 0x64000000,
			.size = 0x02c00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* JAILHOUSE Hypevisor:  0x66c00000 - 0x67000000 */
		/* Inmate memory: 0x67000000 - 0x67800000 */
		{
			.phys_start = 0x67000000,
			.virt_start = 0x67000000,
			.size = 0x00800000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DRAM:  0x67800000 - 0xc0000000 */
		{
			.phys_start = 0x67800000,
			.virt_start = 0x67800000,
			.size = 0x58800000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
	},

	.irqchips = {
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 32,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 160,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 288,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
	},
	.vendors = {
		{
			.type = JAILHOUSE_VENDOR_MTK_EINT,
			.mtk_eint.address    = 0x1000b000,
			.mtk_eint.pin_base   = 0,
			.mtk_eint.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			}
		},
		{
			.type = JAILHOUSE_VENDOR_MTK_EINT,
			.mtk_eint.address    = 0x1000b000,
			.mtk_eint.pin_base   = 128,
			.mtk_eint.pin_bitmap = {
				0xffffffff, 0x00000000, 0x00000000, 0x00000000
			}
		},
		{
			.type = JAILHOUSE_VENDOR_MTK_GPIO,
			.mtk_gpio.address    = 0x10005000,
			.mtk_gpio.pin_base   = 0,
			.mtk_gpio.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			}
		},
		{
			.type = JAILHOUSE_VENDOR_MTK_GPIO,
			.mtk_gpio.address    = 0x10005000,
			.mtk_gpio.pin_base   = 128,
			.mtk_gpio.pin_bitmap = {
				0xffffffff, 0x00000000, 0x00000000, 0x00000000
			}
		}
	}
};
