/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 * Copyright (c) Siemens AG, 2014-2017
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <inmate.h>
#include <gic.h>


#define GPIO_BASE           (0x10005000)
#define GPIO_DATA_IN        (GPIO_BASE + 0x00000000)
#define GPIO_DATA_OUT       (GPIO_BASE + 0x00000100)
#define GPIO_DATA_OUT_SET   (GPIO_BASE + 0x00000104)
#define GPIO_DATA_OUT_CLR   (GPIO_BASE + 0x00000108)

#define EINT_BASE           (0x1000b000)
#define EINT_INT_STATUS     (EINT_BASE + 0x00000000)
#define EINT_INT_ACK        (EINT_BASE + 0x00000040)


#define GPIO_GIC_IRQ   267
#define GPIO_EINT_IRQ  0

#define GPIO_OUT  1

#define LOOP_CNT_INIT       (1000000000)
#define GPIO_OUT_CNT_INIT        (20000)


static unsigned int cnt = 0;

static unsigned int gpioOutCnt = 0;


inline static void ack_EINT (void)
{
    void*         addr = (void*) (EINT_INT_ACK + ((GPIO_EINT_IRQ / 32) * 4));
    unsigned long mask = (1 << (GPIO_EINT_IRQ % 32));


    asm volatile("dmb oshst" : : : "memory");
	asm volatile("str %w0, [%1]" : : "rZ" (mask), "r" (addr));
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

	asm volatile("str %w0, [%1]" : : "rZ" (mask), "r" (addr));
}

static void handle_IRQ (unsigned int irqNum)
{
    if (irqNum == GPIO_GIC_IRQ)
    {
        set_GPIO (true);

        gpioOutCnt = GPIO_OUT_CNT_INIT;
        cnt++;
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
    unsigned int loopCnt;


    /* GPIO & UART memory mapping ... */
	printk ("Setup MMU ...\n");
	map_range ((void*)0x10005000, 0x1000, MAP_UNCACHED);
	map_range ((void*)0x1000b000, 0x1000, MAP_UNCACHED);
	map_range ((void*)0x11001200, 0x0100, MAP_UNCACHED);

	printk ("Initializing the interrupt handler ...\n");
	irq_init (handle_IRQ);

	printk ("Enable the interrupt handler ...\n");
	irq_enable (GPIO_GIC_IRQ);

	printk ("Initialize the GPIO output value ...\n");
    set_GPIO (false);

	printk ("Waiting for interrupts ...\n");
    loopCnt = LOOP_CNT_INIT;
	while (1)
    {
        /* Print the interrupt count. */
        if (loopCnt > 0)
        {
            loopCnt--;
            if (loopCnt == 0)
            {
    		    printk ("INT Latency  Cnt: %d\n", cnt);
                loopCnt = LOOP_CNT_INIT;
            }
        }
        else
        {
            loopCnt = LOOP_CNT_INIT;
        }

        /* Clear the GPIO output value. */
        if (gpioOutCnt > 0)
        {
            gpioOutCnt--;
            if (gpioOutCnt == 0)
            {
                set_GPIO (false);
            }
        }
	}
}
