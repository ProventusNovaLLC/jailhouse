/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Test configuration for GENION-510-EVK (4 * Cortex-A55 and 2 * Cortex-A78, 4GB RAM)
 *
 * Copyright (c) MediaTek, 2025
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
	struct jailhouse_memory mem_regions[16];
	struct jailhouse_irqchip irqchips[8];
	struct jailhouse_vendor vendors[4];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x6ac00000,
			.size       = 0x00400000,
		},
		.debug_console = {
			.address = 0x11001100,
			.size = 0x100,
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO | JAILHOUSE_CON_REGDIST_4,
		},
		.platform_info = {
			.arm = {
				.gic_version = 3,
				.gicd_base = 0x0c000000,
				.gicr_base = 0x0c040000,
                .gicr_size = 0x00200000,
				.maintenance_irq = 25,
			},
		},
		.root_cell = {
			.name = "genio-510-evk",

			.cpu_set_size = sizeof(config.cpus),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_vendors = ARRAY_SIZE(config.vendors),
		},
	},

    /* 4 * A55 (0b001111) and 2 * A78 (0b110000) */
	.cpus = {
		0b111111,
	},

	.mem_regions = {
		/* MMIO:           0x0000'0000'0000'0000 - 0x0000'0000'0c00'0000 */
		/* GIC:            0x0000'0000'0c00'0000 - 0x0000'0000'0c24'0000 */
		/* MMIO:           0x0000'0000'0c24'0000 - 0x0000'0000'1000'5000 */
		/* GPIO:           0x0000'0000'1000'5000 - 0x0000'0000'1000'6000 */
		/* MMIO:           0x0000'0000'1000'6000 - 0x0000'0000'1000'c000 */
		/* EINT:           0x0000'0000'1000'b000 - 0x0000'0000'1000'c000 */
		/* MMIO:           0x0000'0000'1000'c000 - 0x0000'0000'2000'0000 */
		/* DRAM:           0x0000'0000'2000'0000 - 0x0000'0000'4000'0000 */
		/* DRAM:           0x0000'0000'4000'0000 - 0x0000'0001'4000'0000 */
		/* TEE:            0x0000'0000'4320'0000 - 0x0000'0000'43e0'0000 */
		/* DMA Pool:       0x0000'0000'5000'0000 - 0x0000'0000'5290'0000 */
		/* Secure Monitor: 0x0000'0000'5460'0000 - 0x0000'0000'5480'0000 */
		/* DMA Pool:       0x0000'0000'5500'0000 - 0x0000'0000'5640'0000 */
		/* DMA Pool:       0x0000'0000'5700'0000 - 0x0000'0000'5840'0000 */
		/* DMA Pool:       0x0000'0000'6000'0000 - 0x0000'0000'60f0'0000 */
		/* DMA Pool:       0x0000'0000'60f0'0000 - 0x0000'0000'6100'0000 */
		/* DMA Pool:       0x0000'0000'6100'0000 - 0x0000'0000'6110'0000 */
		/* Linux kernel:   0x0000'0000'6400'0000 - 0x0000'0000'66bc'0000 */
		/* Hypervisor:     0x0000'0000'6ac0'0000 - 0x0000'0000'6b00'0000 */

		/* MMIO:  0x0000'0000'0000'0000 - 0x0000'0000'0c00'0000 */
		{
			.phys_start = 0x00000000,
			.virt_start = 0x00000000,
			.size = 0x0c000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* GIC:   0x0000'0000'0c00'0000 - 0x0000'0000'0c24'0000 */
		/* MMIO:  0x0000'0000'0c24'0000 - 0x0000'0000'1000'5000 */
		{
			.phys_start = 0x0c240000,
			.virt_start = 0x0c240000,
			.size = 0x03dc5000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* GPIO:  0x0000'0000'1000'5000 - 0x0000'0000'1000'6000 */
		/* MMIO:  0x0000'0000'1000'6000 - 0x0000'0000'1000'b000 */
		{
			.phys_start = 0x10006000,
			.virt_start = 0x10006000,
			.size = 0x00005000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* EINT:  0x0000'0000'1000'b000 - 0x0000'0000'1000'c000 */
		/* MMIO:  0x0000'0000'1000'c000 - 0000'0000'0x2000'0000 */
		{
			.phys_start = 0x1000c000,
			.virt_start = 0x1000c000,
			.size = 0x0fff4000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_IO
		},
		/* DRAM:  0x0000'0000'2000'0000 - 0x0000'0000'4000'0000 */
		{
			.phys_start = 0x20000000,
			.virt_start = 0x20000000,
			.size = 0x20000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DRAM:  0x0000'0000'4000'0000 - 0x0000'0000'5000'0000 */
		{
			.phys_start = 0x40000000,
			.virt_start = 0x40000000,
			.size = 0x10000000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DMA Pool:  0x0000'0000'5000'0000 - 0x0000'0000'5290'0000 */
		{
			.phys_start = 0x50000000,
			.virt_start = 0x50000000,
			.size = 0x02900000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE
		},
		/* DRAM:  0x0000'0000'5290'0000 - 0x0000'0000'5500'0000 */
		{
			.phys_start = 0x52900000,
			.virt_start = 0x52900000,
			.size = 0x02700000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DMA Pool:  0x0000'0000'5500'0000 - 0x0000'0000'5640'0000 */
		{
			.phys_start = 0x55000000,
			.virt_start = 0x55000000,
			.size = 0x01400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE
		},
		/* DRAM:  0x0000'0000'5640'0000 - 0x0000'0000'5700'0000 */
		{
			.phys_start = 0x56400000,
			.virt_start = 0x56400000,
			.size = 0x00c00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DMA Pool:  0x0000'0000'5700'0000 - 0x0000'0000'5840'0000 */
		{
			.phys_start = 0x57000000,
			.virt_start = 0x57000000,
			.size = 0x01400000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE
		},
		/* DRAM:  0x0000'0000'5840'0000 - 0x0000'0000'6000'0000 */
		{
			.phys_start = 0x58400000,
			.virt_start = 0x58400000,
			.size = 0x07c00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DMA Pool:  0x0000'0000'6000'0000 - 0x0000'0000'6110'0000 */
		{
			.phys_start = 0x60000000,
			.virt_start = 0x60000000,
			.size = 0x01100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE
		},
		/* DRAM:  0x0000'0000'6110'0000 - 0x0000'0000'6ac0'0000 */
		{
			.phys_start = 0x61100000,
			.virt_start = 0x61100000,
			.size = 0x09b00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* JAILHOUSE Hypevisor:  0x0000'0000'6ac0'0000 - 0x0000'0000'6b00'0000 */
		/* Inmate memory:        0x0000'0000'6b00'0000 - 0x0000'0000'6b80'0000 */
		{
			.phys_start = 0x6b000000,
			.virt_start = 0x6b000000,
			.size = 0x00800000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE
		},
		/* DRAM:  0x0000'0000'6b80'0000 - 0x0000'0001'4000'0000 */
		{
			.phys_start = 0x6b800000,
			.virt_start = 0x6b800000,
			.size = 0xd4800000,
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
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 416,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 544,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 672,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 800,
			.pin_bitmap = {
				0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
			},
		},
		/* GIC */
		{
			.address = 0x0c000000,
			.pin_base = 928,
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
				0xffffffff, 0xffffffff, 0x00000000, 0x00000000
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
				0xffffffff, 0xffffffff, 0x00000000, 0x00000000
			}
		}
	}
};
