/*
 *	Real Time Clock interface for LeapFrog LF1000
 *
 *      Scott Esters <sesters@leapfrog.com>
 *
 *      Based on rtc-lf1000rm9200.c driver
 *
 *	Converted to RTC class model by Andrew Victor
 *
 *	Ported to Linux 2.6 by Steven Scholz
 *	Based on s3c2410-rtc.c Simtec Electronics
 *
 *	Based on sa1100-rtc.c by Nils Faerber
 *	Based on rtc.c by Paul Gortmaker
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/completion.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <asm/mach/time.h>
#include <mach/common.h>

#define LF1000_RTC_FREQ		1
#define LF1000_RTC_EPOCH	1900UL	/* just like arch/arm/common/rtctime.c*/
#define RTC_FREE_TIMER		LF1000_FREE_TIMER
#define RTC_FREE_TIMER_SIZE	32	/* timer size in bits */
#define RTC_FREE_TIMER_MAX	0xFFFFFFFF /* max timer value */

typedef struct tagRTC_REGS {
	volatile u32 RTCCNTWRITE;
	volatile u32 RTCCNTREAD;
	volatile u32 RTCALARM;
	volatile u32 RTCCTRL;
	volatile u32 RTCINTENB;
	volatile u32 RTCINTPND;
} RTC_REGS;

RTC_REGS *rtcRegs = NULL;		// RTC registers

struct rtc_pll_info rtc_timer;

/********************
 * RTCCTRL Register *
 *******************/
#define RTCCNTWAIT	4
#define ALARMCNTWAIT	3
#define RTCCNTWRITEENB	0

/**********************
 * RTCINTENB Register *
 *********************/
#define ALARMINTENB	1
#define _RTCINTENB	0

/**********************
 * RTCINTPND Register *
 *********************/
#define ALARMINTPEND	1
#define RTCINTPEND	0


DECLARE_COMPLETION(lf1000_rtc_updated);

/*
 * Read current time and date in RTC
 */
static int lf1000_rtc_readtime(struct device *dev, struct rtc_time *tm)
{
	unsigned long ulTime;
	u32 reg32;

	reg32 = ioread32(&rtcRegs->RTCCTRL);
	BIT_SET(reg32, RTCCNTWRITEENB);		// enable access
	iowrite32(reg32, &rtcRegs->RTCCTRL);	// enable RTC
	ulTime = ioread32(&rtcRegs->RTCCNTREAD);
	BIT_CLR(reg32, RTCCNTWRITEENB);		// read-only
	iowrite32(reg32, &rtcRegs->RTCCTRL);	// disable access
	
	rtc_time_to_tm(ulTime, tm);
	
	pr_debug("%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

/*
 * Set current time and date in RTC
 */
static int lf1000_rtc_settime(struct device *dev, struct rtc_time *tm)
{
	unsigned int uiTime;
	unsigned long ulTime;
	u32 reg32;
	int ret = 0;

	pr_debug("%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm->tm_year, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	rtc_tm_to_time(tm, &ulTime);

	// ensure value fits in 32 bits
	if (ulTime > 0xFFFFFFFF)
		printk(KERN_ALERT "%s: ulTime value (%lu) out of range\n",
			__FUNCTION__, ulTime);
	uiTime = ulTime  & 0xFFFFFFFF;	// keep just lower 32 bits

	reg32 = ioread32(&rtcRegs->RTCCTRL);
	BIT_SET(reg32, RTCCNTWRITEENB);		// enable access
	iowrite32(reg32, &rtcRegs->RTCCTRL);	// enable RTC
	iowrite32(uiTime, &rtcRegs->RTCCNTWRITE);

	// wait for RTC write to complete (2/32768 sec ~ 61 us)
	msleep(1);				// let write finish
	reg32 = ioread32(&rtcRegs->RTCCTRL);	// write should have finished
	if (IS_SET(reg32, RTCCNTWAIT))
		ret = -EDEADLOCK;		// write failed

	reg32 = ioread32(&rtcRegs->RTCCTRL);
	BIT_CLR(reg32, RTCCNTWRITEENB);		// read-only
	iowrite32(reg32, &rtcRegs->RTCCTRL);	// disable access
	//wait_for_completion(&lf1000_rtc_updated);	/* wait for ACKUPD interrupt */
	
	return ret;
}

/*
 * Read alarm time and date in RTC
 */
static int lf1000_rtc_readalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	unsigned long ulTime;
	struct rtc_time *tm = &alrm->time;

	ulTime = ioread32(&rtcRegs->RTCALARM);
	rtc_time_to_tm(ulTime, tm);

	tm->tm_mday = -1;
	tm->tm_mon = -1;
	tm->tm_year = -1;
	tm->tm_wday = -1;
	tm->tm_yday = -1;
	tm->tm_isdst = -1;

	pr_debug("%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm->tm_year, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	return 0;
}

/*
 * Set alarm time and date in RTC
 * Note only hour, minute, second set.  Other values are -1
 */
static int lf1000_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_time *tm = &alrm->time;
	unsigned int uiTime;
	unsigned long ulTime;
	u32 reg32;
	int ret=0;

	rtc_tm_to_time(tm, &ulTime);

	// ensure value fits in 32 bits
	if (ulTime > 0xFFFFFFFF)
		printk(KERN_ALERT "%s: ulTime value (%lu) out of range\n",
			__FUNCTION__, ulTime);
	uiTime = ulTime  & 0xFFFFFFFF;	// keep just lower 32 bits

	iowrite32(uiTime, &rtcRegs->RTCALARM);
	
	// wait for ALARM write to complete (2/32768 sec ~ 61 us)
	msleep(1);			// 1000us long, let write finish
	reg32 = ioread32(&rtcRegs->RTCCTRL);	// write should have finished
	if (IS_SET(reg32, ALARMCNTWAIT))
		ret = -EDEADLOCK;		// write failed

	pr_debug("%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		alrm->time.tm_year, alrm->time.tm_mon,
		alrm->time.tm_mday, alrm->time.tm_hour,
		alrm->time.tm_min, alrm->time.tm_sec);

	return ret;
}

/*
 * Handle commands from user-space
 */
static int lf1000_rtc_ioctl(struct device *dev, unsigned int cmd,
			unsigned long arg)
{
	int ret = 0;
	u32 reg32;
	unsigned int uiTime;
	unsigned long ulTime;

	pr_debug("%s(): cmd=%08x, arg=%08lx.\n", __FUNCTION__, cmd, arg);

	switch (cmd) {
	case RTC_RD_TIME:	/* will use rd_time function */
		ret = -ENOIOCTLCMD;
		break;
	case RTC_SET_TIME:	/* will use set_time function */
		ret = -ENOIOCTLCMD;
		break;
	case RTC_AIE_OFF:	/* alarm off */
		reg32 = ioread32(&rtcRegs->RTCINTENB);
		BIT_CLR(reg32, ALARMINTENB);
		iowrite32(reg32, &rtcRegs->RTCINTENB);
		break;
	case RTC_AIE_ON:	/* alarm on */
		reg32 = ioread32(&rtcRegs->RTCINTENB);
		BIT_SET(reg32, ALARMINTENB);
		iowrite32(reg32, &rtcRegs->RTCINTENB);
		break;
	case RTC_UIE_OFF:	/* update off */
	case RTC_PIE_OFF:	/* periodic off */
		// FIXME: check write completion bit
		reg32 = ioread32(&rtcRegs->RTCINTENB);
		BIT_CLR(reg32, _RTCINTENB);
		iowrite32(reg32, &rtcRegs->RTCINTENB);
		break;
	case RTC_UIE_ON:	/* update on */
	case RTC_PIE_ON:	/* periodic on */
		// FIXME: check write completion bit
		reg32 = ioread32(&rtcRegs->RTCINTENB);
		BIT_SET(reg32, _RTCINTENB);
		iowrite32(reg32, &rtcRegs->RTCINTENB);
		break;
	case RTC_IRQP_READ:	/* read periodic alarm frequency */
		uiTime = ioread32(&rtcRegs->RTCALARM);
		ret = put_user((unsigned long)uiTime, (unsigned long *) arg);
		break;
	case RTC_IRQP_SET:	/* set periodic alarm frequency */
		ret = get_user(ulTime, (unsigned long *) arg);
		if (ulTime > 0xFFFFFFFF) {
			printk(KERN_ALERT "ulTime (%lu) out of range\n",
				ulTime);
			return -ENOIOCTLCMD;
		}
		uiTime = ulTime & 0xFFFFFFFF;
		reg32 = iowrite32(uiTime, &rtcRegs->RTCALARM);
		// wait for ALARM write to complete (2/32768 sec ~ 61 us)
		msleep(1);		// 1000us long, let write finish
		// write should have finished
		reg32 = ioread32(&rtcRegs->RTCCTRL);
		if (IS_SET(reg32, ALARMCNTWAIT))
			ret = -EDEADLOCK;	// write failed
		break;
	
        case RTC_WIE_ON:
		printk(KERN_ALERT "%s():%d RTC_WIE_ON not implemented\n",
			       	__FUNCTION__, __LINE__);
		ret = -ENOIOCTLCMD;
		break;

	case RTC_WIE_OFF:
		printk(KERN_ALERT "%s():%d RTC_WIE_OFF not implemented\n",
			__FUNCTION__, __LINE__);
		ret = -ENOIOCTLCMD;
		break;

	case RTC_PLL_SET:
		printk(KERN_ALERT "%s():%d RTC_PLL_SET not implemented\n",
			__FUNCTION__, __LINE__);
		ret = -ENOIOCTLCMD;
		break;

	/*
	 * reuse exiting RTC IOCTL call, allowing access to high-speed timer
	 */

	/* read timer value */	
	case RTC_PLL_GET:
		/* read raw seconds from RTC	*/
		reg32 = ioread32(&rtcRegs->RTCCTRL);
		BIT_SET(reg32, RTCCNTWRITEENB);		// enable access
		iowrite32(reg32, &rtcRegs->RTCCTRL);	// enable RTC
		rtc_timer.pll_min = ioread32(&rtcRegs->RTCCNTREAD);
		BIT_CLR(reg32, RTCCNTWRITEENB);		// read-only
		iowrite32(reg32, &rtcRegs->RTCCTRL);	// disable access
		
		/* get high-speed timer values */
		rtc_timer.pll_value = get_timer_cnt(RTC_FREE_TIMER);
		rtc_timer.pll_clock = get_timer_freq(RTC_FREE_TIMER);
		rtc_timer.pll_ctrl = 
			RTC_FREE_TIMER_MAX / get_timer_freq(RTC_FREE_TIMER); 
		ret = copy_to_user((struct rtc_pll_info *)arg, &rtc_timer, 
			sizeof(struct rtc_pll_info));
		break;

	default:
		printk(KERN_ALERT "%s(): RTC: default ioctl _IOC_NR(cmd)=%d\n",
		       	__FUNCTION__, _IOC_NR(cmd));
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

/*
 * Provide additional RTC information in /proc/driver/rtc
 */
#ifdef CONFIG_PROC_FS
static int lf1000_rtc_proc(struct device *dev, struct seq_file *seq)
{
	u32 reg32;

	reg32 = ioread32(&rtcRegs->RTCCTRL);
	BIT_SET(reg32, RTCCNTWRITEENB);		// enable access
	iowrite32(reg32, &rtcRegs->RTCCTRL);
	seq_printf(seq, "RTCCNTWRITE    : 0x%08X\n",
			ioread32(&rtcRegs->RTCCNTWRITE));
	seq_printf(seq, "RTCCNTREAD     : 0x%08X\n",
			ioread32(&rtcRegs->RTCCNTREAD));
	BIT_CLR(reg32, RTCCNTWRITEENB);		// read-only
	iowrite32(reg32, &rtcRegs->RTCCTRL);	// disable access
	seq_printf(seq, "RTCALARM       : 0x%08X\n",
			ioread32(&rtcRegs->RTCALARM));
	seq_printf(seq, "RTCCTRL        : 0x%08X\n",
			ioread32(&rtcRegs->RTCCTRL));
	seq_printf(seq, "RTCINTENB      : 0x%08X\n",
			ioread32(&rtcRegs->RTCINTENB));
	seq_printf(seq, "RTCINTPND      : 0x%08X\n",
			ioread32(&rtcRegs->RTCINTPND));
	seq_printf(seq, "RTC_FREE_TIMER_FREQ  : 0x%p\n",
			(void *)get_timer_freq(RTC_FREE_TIMER));
	seq_printf(seq, "RTC_FREE_TIMER_COUNT : 0x%p\n",
			(void *)get_timer_cnt(RTC_FREE_TIMER));
	seq_printf(seq, "RTC_FREE_TIMER_SECS  : %d\n",
			RTC_FREE_TIMER_MAX / get_timer_freq(RTC_FREE_TIMER));
	return 0;
}
#endif

/*
 * IRQ handler for the RTC
 */
static irqreturn_t lf1000_rtc_interrupt(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct rtc_device *rtc = platform_get_drvdata(pdev);
	u32 reg32;
	u32 intReg32;
	u32 tmpReg32;

	unsigned long events = 0;

	reg32 = ioread32(&rtcRegs->RTCINTPND);
	intReg32 = ioread32(&rtcRegs->RTCINTENB);		// save reg

	if (reg32) { // our interrupt occurred	
		if (IS_SET(reg32,RTCINTPEND)) {
			events |= (RTC_UF | RTC_IRQF);
			complete(&lf1000_rtc_updated);
			tmpReg32 = intReg32;			// clear int
			BIT_CLR(tmpReg32, _RTCINTENB);
			iowrite32(tmpReg32, &rtcRegs->RTCINTENB);
		}
		if (IS_SET(reg32,ALARMINTPEND)) {
			events |= (RTC_AF | RTC_IRQF);
			tmpReg32 = intReg32;			// clear int
			BIT_CLR(tmpReg32, ALARMINTENB);
			iowrite32(tmpReg32, &rtcRegs->RTCINTENB);
		}
		rtc_update_irq(rtc, 1, events);

		iowrite32(intReg32, &rtcRegs->RTCINTENB);	// restore reg
		return IRQ_HANDLED;
	}

	printk(KERN_ALERT "RTC: Interrupt unhandled!\n");
	return IRQ_NONE;		/* not handled */
}

static const struct rtc_class_ops lf1000_rtc_ops = {
	.ioctl		= lf1000_rtc_ioctl,
	.read_time	= lf1000_rtc_readtime,
	.set_time	= lf1000_rtc_settime,
	.read_alarm	= lf1000_rtc_readalarm,
	.set_alarm	= lf1000_rtc_setalarm,
#ifdef CONFIG_PROC_FS
	.proc		= lf1000_rtc_proc,
#endif
};

/*
 * Initialize and install RTC driver
 */
static int __init lf1000_rtc_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct rtc_device *rtc;
	int ret;
	u32 reg32;

	struct resource *mem_rtc = NULL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!res) {
		printk(KERN_ERR "RTC: failed to get resource\n");
		return -ENXIO;
	}

	mem_rtc = request_mem_region(res->start, (res->end - res->start)+1, 
				     "lf1000_rtc");
	if (mem_rtc == NULL) {
		printk(KERN_ERR "RTC: failed to map RTC region\n");
		return -EBUSY;
	}

	rtcRegs = ioremap_nocache(res->start, (res->end - res->start)+1);
	if(rtcRegs == NULL) {
		printk(KERN_ERR "RTC: failed to ioremap\n");
		ret = -ENOMEM;
		goto fail_remap;
	}

	/* initialize RTC registers */
	reg32 = ioread32(&rtcRegs->RTCCTRL);	// disable access
	BIT_CLR(reg32, RTCCNTWRITEENB);
	iowrite32(reg32, &rtcRegs->RTCCTRL);
	iowrite32(0, &rtcRegs->RTCALARM);
	iowrite32(0, &rtcRegs->RTCINTENB);
	iowrite32(0, &rtcRegs->RTCINTPND);

	ret = request_irq(LF1000_RTC_IRQ, lf1000_rtc_interrupt,
				IRQF_DISABLED | IRQF_SHARED,
				"lf1000-rtc", pdev);
	if (ret) {
		printk(KERN_ERR "lf1000-rtc: IRQ %d already in use.\n",
				LF1000_RTC_IRQ);
		goto fail_irq;
	}


	rtc = rtc_device_register(pdev->name, &pdev->dev,
				&lf1000_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		ret = PTR_ERR(rtc);
		goto fail_dev;
	}
	platform_set_drvdata(pdev, rtc);
	device_init_wakeup(&pdev->dev, 1);

	printk(KERN_INFO "LF1000 Real Time Clock driver.\n");
	return 0;

fail_dev:
	free_irq(LF1000_RTC_IRQ, pdev);
fail_irq:
	iounmap(rtcRegs);
fail_remap:
	release_mem_region(res->start, (res->end - res->start) + 1);

	return ret;
}

/*
 * Disable and remove the RTC driver
 */
static int __exit lf1000_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);
	struct resource *res;
       
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	/* Disable all interrupts */
	iowrite32(1 << ALARMINTENB | 1 << _RTCINTENB, &rtcRegs->RTCINTENB);
	free_irq(LF1000_RTC_IRQ, pdev);

	iounmap(rtcRegs);
	release_mem_region(res->start, (res->end - res->start) + 1);

	rtc_device_unregister(rtc);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM

/* LF1000R RTC Power management control */


static int lf1000_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
#if 0
	struct rtc_time tm;
	struct timespec time;

	time.tv_nsec = 0;

	/* calculate time delta for suspend */
	lf1000_rtc_readtime(&pdev->dev, &tm);
	rtc_tm_to_time(&tm, &time.tv_sec);
	save_time_delta(&lf1000_rtc_delta, &time);

	/* this IRQ is shared with DBGU and other hardware which isn't
	 * necessarily doing PM like we are...
	 */
	lf1000_rtc_imr = lf1000_sys_read(LF1000_RTC_IMR)
			& (LF1000_RTC_ALARM|LF1000_RTC_SECEV);
	if (lf1000_rtc_imr) {
		if (device_may_wakeup(&pdev->dev))
			enable_irq_wake(LF1000_ID_SYS);
		else
			lf1000_sys_write(LF1000_RTC_IDR, lf1000_rtc_imr);
	}
	pr_debug("%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
	return 0;
}

static int lf1000_rtc_resume(struct platform_device *pdev)
{
#if 0
	struct rtc_time tm;
	struct timespec time;

	time.tv_nsec = 0;

	lf1000_rtc_readtime(&pdev->dev, &tm);
	rtc_tm_to_time(&tm, &time.tv_sec);
	restore_time_delta(&lf1000_rtc_delta, &time);

	if (lf1000_rtc_imr) {
		if (device_may_wakeup(&pdev->dev))
			disable_irq_wake(LF1000_ID_SYS);
		else
			lf1000_sys_write(LF1000_RTC_IER, lf1000_rtc_imr);
	}

	pr_debug("%s(): %4d-%02d-%02d %02d:%02d:%02d\n", __FUNCTION__,
		1900 + tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
	return 0;
}
#else
#define lf1000_rtc_suspend NULL
#define lf1000_rtc_resume  NULL
#endif

static struct platform_driver lf1000_rtc_driver = {
	.remove		= __exit_p(lf1000_rtc_remove),
	.suspend	= lf1000_rtc_suspend,
	.resume		= lf1000_rtc_resume,
	.driver		= {
		.name	= "lf1000-rtc",
		.owner	= THIS_MODULE,
	},
};

static int __init lf1000_rtc_init(void)
{
	return platform_driver_probe(&lf1000_rtc_driver, lf1000_rtc_probe);
}

static void __exit lf1000_rtc_exit(void)
{
	platform_driver_unregister(&lf1000_rtc_driver);
}

module_init(lf1000_rtc_init);
module_exit(lf1000_rtc_exit);

MODULE_AUTHOR("Scott Esters");
MODULE_DESCRIPTION("RTC driver for LF1000");
MODULE_LICENSE("GPL");
