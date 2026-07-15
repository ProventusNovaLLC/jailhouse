/*
 * GENIO-720-EVK - ivshmem demo cell (stage-1 IPC bring-up)
 *
 * Peer 1 of the ivshmem device defined in genio-720-evk.c (root = peer 0).
 * Runs the stock inmates/demos/ivshmem-demo.bin to validate the emulated
 * vPCI, the state table, and doorbell interrupts on this SoC before any
 * Zephyr-side PCIe work. Console on UART1 (8250), same as the zephyr cell.
 *
 * Copyright (c) Proventus Nova, 2026
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[7];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.architecture = JAILHOUSE_ARM64,
		.name = "ivshmem-demo",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG |
			 JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED,

		.cpu_set_size       = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips       = ARRAY_SIZE(config.irqchips),
		.num_pci_devices    = ARRAY_SIZE(config.pci_devices),

		.cpu_reset_address = CONFIG_INMATE_BASE,

		/* Inmate INTx block: SPIs 576-579 = INTIDs 608-611 (free gap
		 * 554-596 per the 2026-07-15 DT audit; root peer uses 580).
		 */
		.vpci_irq_base = 576,

		.console = {
			.address = 0x11002000,
			.divider = 0x0e,		/* 26 MHz / 16 / 14 ~= 115200 */
			.type = JAILHOUSE_CON_TYPE_8250,
			.flags = JAILHOUSE_CON_ACCESS_MMIO | JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0b00100000,		/* CPU 5 (A55), same core the zephyr cell uses */
	},

	.mem_regions = {
		/* IVSHMEM regions - same order and addresses as the root cell,
		 * ROOTSHARED, with per-peer write permissions inverted:
		 * this peer owns output section 1.
		 */
		/* state table */
		{
			.phys_start = 0x47F00000,
			.virt_start = 0x47F00000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* read/write section */
		{
			.phys_start = 0x47F01000,
			.virt_start = 0x47F01000,
			.size = 0x9000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* output section peer 0 (root) - read-only for us */
		{
			.phys_start = 0x47F0A000,
			.virt_start = 0x47F0A000,
			.size = 0x2000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* output section peer 1 (this cell) */
		{
			.phys_start = 0x47F0C000,
			.virt_start = 0x47F0C000,
			.size = 0x2000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* UART1 (console) */
		{
			.phys_start = 0x11002000,
			.virt_start = 0x11002000,
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED |
				JAILHOUSE_MEM_IO_32,
		},
		/* RAM (first MB of the inmate window) */
		{
			.phys_start = 0x45000000,
			.virt_start = CONFIG_INMATE_BASE,
			.size = 0x00100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* communication region */
		{
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
	},

	.irqchips = {
		/* GIC: INTIDs 608-611 (the INTx pins of this peer) */
		{
			.address = 0x0c000000,
			.pin_base = 608,
			.pin_bitmap = {
				0x0000000f, 0x00000000, 0x00000000, 0x00000000
			},
		},
	},

	.pci_devices = {
		/* IVSHMEM 0001:00:00.0, inmate side (peer 1) */
		{
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.domain = 1,
			.bdf = 0 << 3,
			.bar_mask = JAILHOUSE_IVSHMEM_BAR_MASK_INTX,
			.shmem_regions_start = 0,
			.shmem_dev_id = 1,
			.shmem_peers = 2,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_UNDEFINED,
		},
	},
};
