/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Test configuration for GENION-720-EVK (6 * Cortex-A55 and 2 * Cortex-A78, 8GB RAM)
 *
 * Copyright (c) MediaTek, 2025
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
	struct jailhouse_system header;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[15];
	struct jailhouse_irqchip irqchips[8];
	struct jailhouse_vendor vendors[4];
} __attribute__((packed)) config = {
	.header = {
		.signature = JAILHOUSE_SYSTEM_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.flags = JAILHOUSE_SYS_VIRTUAL_DEBUG_CONSOLE,
		.hypervisor_memory = {
			.phys_start = 0x44000000,
			.size       = 0x01000000,   // 16 MB
		},
		.debug_console = {
			.address = 0x11001000,
			.size = 0x1000,
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
			.name = "genio-720-evk",

			.cpu_set_size = sizeof(config.cpus),
			//.smc_ids_size = ARRAY_SIZE(config.smc_ids),
			.num_memory_regions = ARRAY_SIZE(config.mem_regions),
			.num_irqchips = ARRAY_SIZE(config.irqchips),
			.num_vendors = ARRAY_SIZE(config.vendors),
		},
	},

    /* 6 * A55 (0b00111111) and 2 * A78 (0b11000000) */
	.cpus = {
		0b11111111,
	},

	.mem_regions = {
		/*
		* ============================
		*  LOW PERIPHERAL / IO REGION
		* ============================
		*
		* 0x00000000 - 0x3FFFFFFF
		* SoC peripherals, GIC, UART, IOMMU, GPU, display, PCIe window, etc.
		*
		* This must be:
		*   - READ | WRITE
		*   - IO
		*   - NOT executable
		*
		* Linux never executes from MMIO.
		*/
		{
			.phys_start = 0x00000000,
			.virt_start = 0x00000000,
			.size       = 0x0c000000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_IO,
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

		/*
		* ============================
		*  SYSTEM RAM BLOCK 1
		* 40000000-431fffff
		* ============================
		*
		* Normal DRAM
		* Must be RWX because:
		*  - Kernel text may execute here
		*  - Modules may execute here
		*/
		{
			.phys_start = 0x40000000,
			.virt_start = 0x40000000,
			.size       = 0x03200000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 2
		* 43e00000-43ffffff
		*/
		{
			.phys_start = 0x43e00000,
			.virt_start = 0x43e00000,
			.size       = 0x00200000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/* Inmate memory: 0x45000000 - 0x48000000 (48 MB) */
		{
			.phys_start = 0x45000000,
			.virt_start = 0x45000000,
			.size = 0x03000000,   // 48 MB
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE | JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 3
		* 48000000-4fffffff
		*/
		{
			.phys_start = 0x48000000,
			.virt_start = 0x48000000,
			.size       = 0x08000000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 4
		* 52900000-545fffff
		*/
		{
			.phys_start = 0x52900000,
			.virt_start = 0x52900000,
			.size       = 0x01d00000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 5
		* 54800000-54ffffff
		*/
		{
			.phys_start = 0x54800000,
			.virt_start = 0x54800000,
			.size       = 0x00800000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 6
		* 56400000-5affffff
		*/
		{
			.phys_start = 0x56400000,
			.virt_start = 0x56400000,
			.size       = 0x04c00000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 7
		* 5b200000-5fffffff
		*/
		{
			.phys_start = 0x5b200000,
			.virt_start = 0x5b200000,
			.size       = 0x04e00000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 8
		* 60800000-13fffdfff
		*
		* This contains:
		*   - Kernel code
		*   - Kernel data
		*   - Page allocator memory
		*
		* Must be RWX.
		*/
		{
			.phys_start = 0x60800000,
			.virt_start = 0x60800000,
			.size       = 0x0F9F7E000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
		},

		/*
		* SYSTEM RAM BLOCK 9 (HIGH MEMORY BANK)
		* 140000000-23fffdfff
		*
		* This is the high DRAM bank.
		* Your crash address lived here.
		*
		* Must be RWX.
		*/
		{
			.phys_start = 0x140000000,
			.virt_start = 0x140000000,
			.size       = 0x0FFFFE000,
			.flags      = JAILHOUSE_MEM_READ |
						JAILHOUSE_MEM_WRITE |
						JAILHOUSE_MEM_EXECUTE,
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
