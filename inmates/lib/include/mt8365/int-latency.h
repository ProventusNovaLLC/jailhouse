/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) MediaTek, 2025
 *
 * The bare-metal interrupt latency test program MT8365 SoC specific
 * definitions.
 * 
 * Authors:
 *  Felix Freimann <felix.freimann@mediatek.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#ifndef _MT8365_INT_LATENCY_H
#define _MT8365_INT_LATENCY_H


#define GPIO_BASE          (0x10005000)
#define GPIO_DATA_IN       (GPIO_BASE + 0x00000000)
#define GPIO_DATA_OUT      (GPIO_BASE + 0x000000a0)
#define GPIO_DATA_OUT_SET  (GPIO_BASE + 0x000000a4)
#define GPIO_DATA_OUT_CLR  (GPIO_BASE + 0x000000a8)

#define EINT_BASE          (0x1000b000)
#define EINT_INT_STATUS    (EINT_BASE + 0x00000000)
#define EINT_INT_ACK       (EINT_BASE + 0x00000040)


#define GPIO_GIC_IRQ   147
#define GPIO_EINT_IRQ  106

#define GPIO_OUT  107


#define MAP_GPIO  map_range ((void*) GPIO_BASE, 0x1000, MAP_UNCACHED)
#define MAP_EINT  map_range ((void*) EINT_BASE, 0x1000, MAP_UNCACHED)
#define MAP_UART  map_range ((void*)0x11003000, 0x0100, MAP_UNCACHED)


#endif /* _MT8365_INT_LATENCY_H */
