/*
 *  linux/arch/arm/common/lf1000-ic.c
 *
 *  Copyright (C) 2007	Kosta Demirev <kdemirev@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/init.h>
#include <linux/list.h>

#include <asm/io.h>
#include <asm/mach/irq.h>
#include <asm/hardware/vic.h>
#include <mach/platform.h>
#include <mach/ic.h>

struct lf1000_ic* get_ic_base(unsigned int irq)
{
    return get_irq_chip_data(irq);
}

static void ic_mask_irq(unsigned int irq)
{
struct lf1000_ic* base = get_irq_chip_data(irq);

    if( irq < 32) base->intmaskl |= 1<< irq;
    else	  base->intmaskh |= 1<<(irq-32);
}

static void ic_pend_irq(unsigned int irq)
{
struct lf1000_ic* base = get_irq_chip_data(irq);

    if( irq < 32) base->intpendl |= 1<< irq;
    else	  base->intpendh |= 1<<(irq-32);
}

static void ic_unmask_irq(unsigned int irq)
{
struct lf1000_ic* base = get_irq_chip_data(irq);

    if( irq < 32) base->intmaskl &= ~(1<< irq);
    else	  base->intmaskh &= ~(1<<(irq-32));
}

static struct irq_chip ic_chip = {
	.name	= "IC",
	.ack	= ic_pend_irq,
	.mask	= ic_mask_irq,
	.unmask	= ic_unmask_irq,
};

/**
 * ic_init - initialise a interrupt controller
 * @base: iomem base address
 * @irq_start: starting interrupt number, must be muliple of 32
 * @vic_sources: bitmask of interrupt sources to allow
 */
void __init ic_init(void __iomem *base, unsigned int irq_start,
		     u64 ic_sources)
{
int i;
u64 shift;

	/* Disable all interrupts initially. */
	((struct lf1000_ic*) base)->intmodel =
	((struct lf1000_ic*) base)->intmodeh =  0;	/* all interrupts are IRQ */
	((struct lf1000_ic*) base)->intmaskl =
	((struct lf1000_ic*) base)->intmaskh = ~0;	/* all interrupts are masked */
	((struct lf1000_ic*) base)->intpendl =
	((struct lf1000_ic*) base)->intpendh = ~0;	/* all interrupts are cleared */
	((struct lf1000_ic*) base)->priorder =
	((struct lf1000_ic*) base)->primode  =  0;	/* priority : standard, no rotation */

	for (i = 0, shift=1; i < NR_IRQS; i++, shift<<=1) {
		unsigned int irq = irq_start + i;

		set_irq_chip(irq, &ic_chip);
		set_irq_chip_data(irq, base);

		if (ic_sources & shift) {
			set_irq_handler(irq, handle_level_irq);
			set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
		}
	}
}
