/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) MediaTek, 2025
 *
 * The bare-metal interrupt latency test program.
 *
 * Authors:
 *  Felix Freimann <felix.freimann@mediatek.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <inmate.h>
#include <gic.h>

#include "int-latency.h"


#define GPIO_OUT_CNT_INIT  (20000)


static volatile unsigned int gpio_out_cnt = 0;


inline static void ack_EINT (void)
{
    void*         addr = (void*) (EINT_INT_ACK + ((GPIO_EINT_IRQ / 32) * 4));
    unsigned long mask = (1 << (GPIO_EINT_IRQ % 32));


    asm volatile ("dmb oshst" : : : "memory");
	asm volatile ("str %w0, [%1]" : : "rZ" (mask), "r" (addr));
}

inline static void set_GPIO (bool  value)
{
    void*         addr;
    unsigned long mask = (1 << (GPIO_OUT % 32));


    if (value)
    {
        addr = (void*) (GPIO_DATA_OUT_SET + ((GPIO_OUT / 32) * 16));
    }
    else
    {
        addr = (void*) (GPIO_DATA_OUT_CLR + ((GPIO_OUT / 32) * 16));
    }

	asm volatile ("str %w0, [%1]" : : "rZ" (mask), "r" (addr));
}

static void handle_IRQ (unsigned int irqNum)
{
    if (irqNum == GPIO_GIC_IRQ)
    {
        set_GPIO (true);

        gpio_out_cnt = GPIO_OUT_CNT_INIT;
    }
    else
    {
    	printk ("Received unknown interrupt %d ...\n", irqNum);
    }

    /* Acknowlede EINT interrupt */
    ack_EINT ();
}

void inmate_main (void)
{
    /* GPIO & UART memory mapping ... */
	printk ("Setup MMU ...\n");
    MAP_GPIO;
    MAP_EINT;    
    MAP_UART;

	printk ("Initializing the interrupt handler ...\n");
	irq_init (handle_IRQ);

	printk ("Enable the interrupt handler ...\n");
	irq_enable (GPIO_GIC_IRQ);

	printk ("Initialize the GPIO output value ...\n");
    set_GPIO (false);

	printk ("Waiting for interrupts ...\n");

    while (1)
    {
        /* Clear the GPIO output value. */
        if (gpio_out_cnt > 0)
        {
            gpio_out_cnt--;
            if (gpio_out_cnt == 0)
            {
                set_GPIO (false);
            }
        }
	}
}
