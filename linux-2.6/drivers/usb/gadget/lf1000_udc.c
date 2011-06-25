/*
 * linux/drivers/usb/gadget/lf1000_udc.c
 *
 * Based on:
 * linux/drivers/usb/gadget/s3c2410_udc.c
 * Samsung on-chip full speed USB device controllers
 *
 * Copyright (C) 2004-2007 Herbert Pötzl - Arnaud Patard
 *	Additional cleanups by Ben Dooks <ben-linux@fluff.org>
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
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/debugfs.h>
#include <linux/input.h>

#include <linux/usb.h>
#include <linux/usb/gadget.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>

#include <asm/mach-types.h>

#include <mach/gpio.h>

#include "lf1000_udc.h"

#define DRIVER_NAME		"lf1000-usbgadget"
#define RESSIZE(res)		(((res)->end - (res)->start)+1)

static const char gadget_name[] = "lf1000_udc";

static inline void set_mask(struct lf1000_udc *udc, u16 bits, u32 reg)
{
	u16 tmp = readw(udc->base_addr + reg);
	
	writew(tmp | bits, udc->base_addr + reg);
}

static inline void clear_mask(struct lf1000_udc *udc, u16 bits, u32 reg)
{
	u16 tmp = readw(udc->base_addr + reg);
	
	writew(tmp & ~(bits), udc->base_addr + reg);
}

#define BIT_SET(v,b)    ((v) |= (1<<(b)))
#define BIT_CLR(v,b)    ((v) &= ~(1<<(b)))
#define IS_SET(v,b)     ((v) & (1<<(b)))
#define IS_CLR(v,b)     !((v) & (1<<(b)))

/*************************** DEBUG FUNCTION ***************************/
#define DEBUG_NORMAL	1
#define DEBUG_VERBOSE	2

#ifdef CONFIG_USB_LF1000_DEBUG
#define USB_LF1000_DEBUG_LEVEL 2

static uint32_t lf1000_ticks=0;

static int dprintk(int level, const char *fmt, ...)
{
	static char printk_buf[1024];
	static long prevticks;
	static int invocation;
	va_list args;
	int len;

	if (level > USB_LF1000_DEBUG_LEVEL)
		return 0;

	if (lf1000_ticks != prevticks) {
		prevticks = lf1000_ticks;
		invocation = 0;
	}

	len = scnprintf(printk_buf, \
			sizeof(printk_buf), "%1lu.%02d USB: ", \
			prevticks, invocation++);

	va_start(args, fmt);
	len = vscnprintf(printk_buf+len, \
			sizeof(printk_buf)-len, fmt, args);
	va_end(args);

	return printk(KERN_DEBUG "%s", printk_buf);
}
#else
static int dprintk(int level, const char *fmt, ...)
{
	return 0;
}
#endif

static ssize_t lf1000udc_vbus_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct lf1000_udc *udc = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", udc->vbus);
}

static DEVICE_ATTR(vbus, 0444, lf1000udc_vbus_show, NULL);

static void show_reg(struct seq_file *s, const char *nm, u32 reg)
{
	struct lf1000_udc *udc = s->private;

	seq_printf(s, "%10s:\t0x%04X\n", nm, readw(udc->base_addr + reg));
}

static int lf1000_udc_regs_show(struct seq_file *s, void *v)
{
	show_reg(s, "EPINDEX", UDC_EPINDEX);
	show_reg(s, "EPINT", UDC_EPINT);
	show_reg(s, "EPINTEN", UDC_EPINTEN);
	show_reg(s, "FUNCADDR", UDC_FUNCADDR);
	show_reg(s, "FRAMENUM", UDC_FRAMENUM);
	show_reg(s, "EPDIR", UDC_EPDIR);
	show_reg(s, "TEST", UDC_TEST);
	show_reg(s, "SYSSTAT", UDC_SYSSTAT);
	show_reg(s, "SYSCTL", UDC_SYSCTL);
	show_reg(s, "EP0STAT", UDC_EP0STAT);
	show_reg(s, "EP0CTL", UDC_EP0CTL);
	show_reg(s, "EPSTAT", UDC_EPSTAT);
	show_reg(s, "EPCTL", UDC_EPCTL);
	show_reg(s, "BRCR", UDC_BRCR);
	show_reg(s, "BWCR", UDC_BWCR);
	show_reg(s, "MPR", UDC_MPR);
	show_reg(s, "DCR", UDC_DCR);
	show_reg(s, "DTCR", UDC_DTCR);
	show_reg(s, "DFCR", UDC_DFCR);
	show_reg(s, "DTTCR", UDC_DTTCR);
	show_reg(s, "PLICR", UDC_PLICR);
	show_reg(s, "PCR", UDC_PCR);
	show_reg(s, "CIKSEL", UDC_CIKSEL);
	show_reg(s, "VBUSINTENB", UDC_VBUSINTENB);
	show_reg(s, "VBUSPEND", UDC_VBUSPEND);
	show_reg(s, "POR", UDC_POR);
	show_reg(s, "SUSPEND", UDC_SUSPEND);
	show_reg(s, "USER0", UDC_USER0);
	show_reg(s, "USER1", UDC_USER1);
	show_reg(s, "CLKEN", UDC_CLKEN);
	show_reg(s, "CLKGEN", UDC_CLKGEN);

	return 0;
}

static int lf1000_udc_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, lf1000_udc_regs_show, inode->i_private);
}

static const struct file_operations lf1000_udc_regs_fops = {
	.owner		= THIS_MODULE,
	.open		= lf1000_udc_regs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/*------------------------- I/O ----------------------------------*/

static void udc_disable(struct lf1000_udc *dev);
static void udc_enable(struct lf1000_udc *dev);

#define LF1000_UDC_VBUS_INIT 0
#define LF1000_UDC_VBUS_ENABLE 1
#define LF1000_UDC_VBUS_DISABLE 2
#define LF1000_UDC_VBUS_RESET 3
#define LF1000_UDC_VBUS_SHUTDOWN 4

static int lf1000_vbus_command(struct lf1000_udc *udc, unsigned char cmd)
{
	int retval = 0;
	u16 tmp;

	if (!udc)
		return -EINVAL;

	switch (cmd) {
	case LF1000_UDC_VBUS_INIT:
		udc->vbus = 0;
		break;
	case LF1000_UDC_VBUS_ENABLE:
		/* enable vbus detect interrupts */
		tmp = readw(udc->base_addr + UDC_SYSCTL);
		tmp |= ((1<<VBUSOFFEN)|(1<<VBUSONEN));
		writew(tmp, udc->base_addr + UDC_SYSCTL);

		/* set up the VBUSENB bit. */
		tmp = readw(udc->base_addr + UDC_USER1);
		writew(tmp | (1<<VBUSENB), udc->base_addr + UDC_USER1);
		break;
	case LF1000_UDC_VBUS_DISABLE:
		/* disable the VBUSENB bit. */
		tmp = readw(udc->base_addr + UDC_USER1) & ~(1<<VBUSENB);
		writew(tmp, udc->base_addr + UDC_USER1);

		/* disable vbus detect interrupts */
		tmp = readw(udc->base_addr + UDC_SYSCTL);
		tmp &= ~((1<<VBUSOFFEN)|(1<<VBUSONEN));
		writew(tmp, udc->base_addr + UDC_SYSCTL);
		break;
	case LF1000_UDC_VBUS_SHUTDOWN:
		lf1000_vbus_command(udc, LF1000_UDC_VBUS_DISABLE);
		break;
	case LF1000_UDC_VBUS_RESET:
		/* What to do here? */
		break;
	default:
		break;
	}
	return retval;
}

static int lf1000_udc_vbus_session(struct usb_gadget *_gadget, int is_active)
{
	struct lf1000_udc  *udc;

	udc = container_of(_gadget, struct lf1000_udc, gadget);
	
	/* FIXME: we currently generate a keyboard event to notify userspace,
	 * 	  find a better way to do this. */
	input_report_switch(udc->input, SW_LID, is_active);

	if (udc->driver && udc->driver->vbus_session)
		udc->driver->vbus_session(&udc->gadget, is_active);

	udc->vbus = is_active;
	
	if (udc->vbus == 0 &&
	    udc->gadget.speed != USB_SPEED_UNKNOWN &&
	    udc->driver && udc->driver->disconnect) {
		udc->driver->disconnect(&udc->gadget);
	}

	return 0;
}

static void done(struct lf1000_ep *ep, struct lf1000_request *req, int status)
{
	unsigned halted = ep->halted;

	list_del_init(&req->queue);

	if (likely(req->req.status == -EINPROGRESS))
		req->req.status = status;

	ep->halted = 1; /* do we need this? */
	if(req->req.complete)
		req->req.complete(&ep->ep, &req->req);
	ep->halted = halted;
}

static void nuke(struct lf1000_ep *ep, int status)
{
	struct lf1000_request  *req;

	if (&ep->queue == NULL) /* FIXME */
		return;

	while (!list_empty (&ep->queue)) {
		req = list_entry(ep->queue.next, struct lf1000_request, queue);
		done(ep, req, status);
	}
}

static inline void clear_ep_state (struct lf1000_udc *dev)
{
	unsigned i;

	for (i = 1; i < LF1000_ENDPOINTS; i++)
		nuke(&dev->ep[i], -ECONNRESET);
}

static void udc_reinit(struct lf1000_udc *dev);

static int write_fifo(struct lf1000_ep *ep, char *buf, int len)
{
	struct lf1000_udc *udc = ep->dev;
	int idx = ep->bEndpointAddress & 0x7F;
	unsigned long fifo_reg = UDC_EPBUFS + 2*idx;
	int bytes_written = 0;
	u16 tmp;
	int fifo_nonempty;

	writew(idx, udc->base_addr + UDC_EPINDEX);

	fifo_nonempty = (readw(udc->base_addr + UDC_EPSTAT)>>PSIF) & 0x3;
	if (ep->num != 0 && fifo_nonempty)
		return 0;

	if (len > ep->ep.maxpacket)
		len = ep->ep.maxpacket;

	writew(len, udc->base_addr + UDC_BWCR);

	if (unlikely((int)buf & 0x1)) {
		/* source buffer is not aligned.  Yuck. */
		while (len > 1) {
			tmp = (*buf++) & 0xff;
			tmp |= ((*buf++)<<8);
			writew(tmp, udc->base_addr + fifo_reg);
			bytes_written += 2;
			len -= 2;
		}
	} else {
		while (len > 1) {
			writew(*((u16 *)buf), udc->base_addr + fifo_reg);
			buf += 2;
			bytes_written += 2;
			len -= 2;
		}
	}

	if (len) {
		/* write odd byte */
		tmp = *buf++;
		writew(tmp & 0xFF, udc->base_addr + fifo_reg);
		bytes_written += 1;
		len -= 1;
	}

	return bytes_written;
}

static int read_fifo(struct lf1000_ep *ep, char *buf, int len)
{
	struct lf1000_udc *udc = ep->dev;
	int bytes_read = 0;
	int fifo_count = 0;
	int idx = ep->bEndpointAddress&0x7F;
	unsigned long fifo_reg = UDC_EPBUFS + 2*idx;
	unsigned short tmp;
	int flush = 0;
	int fifo_empty;

	writew(idx, udc->base_addr + UDC_EPINDEX);

	/* We can't read the BRCR unless there's a valid packet. */
	fifo_empty = !((readw(udc->base_addr + UDC_EPSTAT)>>PSIF) & 0x3);
	if (ep->num != 0 && fifo_empty)
		return 0;

	fifo_count = 2*readw(udc->base_addr + UDC_BRCR);

	if (fifo_count == 0)
		return 0;

	if ((ep->num == 0 &&
	    IS_SET(readw(udc->base_addr + UDC_EP0STAT), EP0LWO)) ||
	   (ep->num != 0 &&
	    IS_SET(readw(udc->base_addr + UDC_EPSTAT), EPLWO))) {
		fifo_count--;
	}

	if (len < fifo_count) {
		fifo_count = len;
		if (ep->num != 0)
			flush = 1;
	}

	if (unlikely((int)buf & 0x1)) {
		/* destination buffer is not aligned.  Yuck. */
		while (fifo_count > 1) {
			tmp = readw(udc->base_addr + fifo_reg);
			*buf++ = (char)(tmp & 0xff);
			*buf++ = (char)((tmp >> 8) & 0xff);
			bytes_read += 2;
			fifo_count -= 2;
		}
	} else {
		while (fifo_count > 1) {
			*((short *)buf) = readw(udc->base_addr + fifo_reg);
			buf += 2;
			bytes_read += 2;
			fifo_count -= 2;
		}
	}

	if (fifo_count) {
		/* read odd byte */
		tmp = readw(udc->base_addr + fifo_reg);
		*buf++ = (char)(tmp & 0xff);
		bytes_read++;
		fifo_count--;
	}

	/* A packet must not cross a req boundary.  Flush any extra data. */
	if (flush)
		writew(1<<FLUSH, udc->base_addr + UDC_EPCTL);

	return bytes_read;
}

static int lf1000_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gf);
static int lf1000_set_halt(struct usb_ep *_ep, int value);

static int lf1000_get_status(struct lf1000_udc *dev,
		struct usb_ctrlrequest  *crq)
{
	u8 ep_num = crq->wIndex & 0x7F;

	switch(crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			/* All interface status bits are reserved.  Return 0. */
			dev->ifstatus = 0;
			dev->statreq.req.buf = &dev->ifstatus;
			break;
		case USB_RECIP_DEVICE:
			dev->statreq.req.buf = &dev->devstatus;
			break;
		case USB_RECIP_ENDPOINT:
			if (ep_num > LF1000_ENDPOINTS || crq->wLength > 2)
				return 1;
			writew(ep_num, dev->base_addr + UDC_EPINDEX);

			dev->ep[ep_num].status = dev->ep[ep_num].halted;
			dev->statreq.req.buf = &dev->ep[ep_num].status;
			break;
		default:
			return 1;
	}

	dev->statreq.req.length = 2;
	dev->statreq.req.actual = 0;
	dev->statreq.req.status = -EINPROGRESS;
	dev->statreq.req.complete = NULL;
	dev->statreq.req.zero = 0;
	lf1000_queue(&dev->ep[0].ep, &dev->statreq.req, GFP_ATOMIC);
	return 0;
}

static int lf1000_set_feature(struct lf1000_udc *dev,
		struct usb_ctrlrequest *crq)
{
	struct lf1000_ep *ep;
	u8 ep_num = crq->wIndex & 0x7F;
 
	WARN_ON(ep_num >= LF1000_ENDPOINTS);
	ep = &dev->ep[ep_num];

	switch(crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			/* How will we support interface features? */
			return 1;
		case USB_RECIP_DEVICE:
			if (crq->wValue != USB_DEVICE_REMOTE_WAKEUP)
				return 1;
			/* What do we do here? */
			break;
		case USB_RECIP_ENDPOINT:
			if (crq->wValue != USB_ENDPOINT_HALT ||
			    ep_num > LF1000_ENDPOINTS)
				return 1;
			if (!ep->desc)
				return 1;
			if ((crq->wIndex & USB_DIR_IN)) {
				if (!ep->is_in)
					return 1;
			} else if (ep->is_in)
				return 1;
			lf1000_set_halt(&ep->ep, 1);
			break;

		default:
			return 1;
	}

	return 0;
}

static int lf1000_clear_feature(struct lf1000_udc *dev,
		struct usb_ctrlrequest *crq)
{
	struct lf1000_ep *ep;
	u8 ep_num = crq->wIndex & 0x7F;
 
	WARN_ON(ep_num >= LF1000_ENDPOINTS);
	ep = &dev->ep[ep_num];
	
	switch(crq->bRequestType & USB_RECIP_MASK) {
		case USB_RECIP_INTERFACE:
			return 1;
		case USB_RECIP_DEVICE:
			if (crq->wValue != USB_DEVICE_REMOTE_WAKEUP)
				return 1;
			break;
		case USB_RECIP_ENDPOINT:
			if (crq->wValue != USB_ENDPOINT_HALT ||
			    ep_num > LF1000_ENDPOINTS)
				return 1;
			
			writew(ep_num, dev->base_addr + UDC_EPINDEX);
			
			if (!ep->desc)
				return 1;
			
			if ((crq->wIndex & USB_DIR_IN)) {
				if (!ep->is_in)
					return 1;
			} else if (ep->is_in)
				return 1;

			lf1000_set_halt(&ep->ep, 0);
			break;

		default:
			return 1;
	}

	return 0;
}

/*------------------------- usb helpers -------------------------------*/

static void clear_ep_stall(struct lf1000_udc *udc, int ep)
{
	if (ep == 0) {
		writew(1<<SHT, udc->base_addr + UDC_EP0STAT);
		clear_mask(udc, 1<<EP0ESS, UDC_EP0CTL);
		writew(1<<SHT, udc->base_addr + UDC_EP0STAT);
	} else if(ep < LF1000_ENDPOINTS) {
		writew(ep, udc->base_addr + UDC_EPINDEX);
		set_mask(udc, (1<<FLUSH), UDC_EPCTL);
		clear_mask(udc, (1<<ESS)|(1<<INPKTHLD)|(1<<OUTPKTHLD),
				UDC_EPCTL);
		writew((1<<FSC)|(1<<FFS), udc->base_addr + UDC_EPSTAT);
		writew((1<<ep), udc->base_addr + UDC_EPINT);
	} else {
		printk(KERN_ERR "No such endpoint: %d\n", ep);
	}
}

static void set_ep_stall(struct lf1000_udc *udc, int ep)
{
	if (ep == 0) {
		set_mask(udc, (1<<EP0ESS), UDC_EP0CTL);
	} else if(ep < LF1000_ENDPOINTS) {
		writew(ep, udc->base_addr + UDC_EPINDEX);
		set_mask(udc, (1<<ESS), UDC_EPCTL);
	} else {
		printk(KERN_ERR "No such endpoint: %d\n", ep);
	}
}

/*------------------------- usb state machine -------------------------------*/

/* return 1 for complete, 0 for incomplete */
int write_req(struct lf1000_ep *ep, struct lf1000_request *req)
{
	int res = req->req.length - req->req.actual;
	char *src = req->req.buf + req->req.actual;
	
	if (res > (ep->ep.maxpacket & 0x7ff))
		res = ep->ep.maxpacket & 0x7ff;
	
	dprintk(DEBUG_VERBOSE, "Writing %d/%d bytes.\n", res + req->req.actual,
		req->req.length);

	req->req.actual += write_fifo(ep, src, res);

        /* If we've sent the requested # of bytes, we're done unless the
         * most-recently-sent packet was max-size and the transfer must end
         * with a short packet (less than max-size). */
	if(req->req.actual == req->req.length) {
		if ((res >= (ep->ep.maxpacket & 0x7ff)) && req->req.zero) {
			dprintk(DEBUG_VERBOSE,
				"Full final pkt (%d); will send zlp\n", res);
			return 0;
		}

		dprintk(DEBUG_VERBOSE, "Transfer complete.\n");

		/* FIXME: We should call done on this packet when we see its TX
		 * interrupt.  But when we try that, we end up with some weird
		 * inconsistent list state.  Why?
		 */
		done(ep, req, 0);
		return 1;
	}
	return 0;
}

/* return 1 for complete, 0 for incomplete */
/* do not call unless you're holding the eplock */
int read_req(struct lf1000_ep *ep, struct lf1000_request *req)
{
	int res = req->req.length - req->req.actual;
	int pkt_len;
	char *dst = req->req.buf + req->req.actual;
	unsigned is_short;

	pkt_len = read_fifo(ep, dst, res);
	req->req.actual += pkt_len;
	is_short = (pkt_len < ep->ep.maxpacket);
	dprintk(DEBUG_VERBOSE, "\"%s\" read %d/%d bytes [%d] (z=%d, s=%d).\n", 
			ep->ep.name,
			req->req.actual, req->req.length, pkt_len,
			req->req.zero, is_short);

	if (pkt_len == 0 && req->req.actual == 0)
		return 0;

	if (is_short || req->req.actual == req->req.length) {
		dprintk(DEBUG_VERBOSE, "Transfer complete.\n");
		done(ep, req, 0);
		return 1;
	}
	return 0;
}

/* Return the recommended next state */
static int handle_setup(struct lf1000_udc *udc, struct lf1000_ep *ep, u32 csr)
{
	unsigned rxcount;
	struct usb_ctrlrequest creq;
	int next_state = EP0_IDLE, status = 0;
	u8 is_in;

	/* read SETUP; hard-fail for bogus packets */
	rxcount = read_fifo(ep, (char *)&creq, 8);
	if(unlikely(rxcount != 8)) {
		// REVISIT this happens sometimes under load; why??
		printk(KERN_ERR "setup: len %d, csr %08x\n", rxcount, csr);
		/* set stall */
		return EP0_STALL;
	}

	dprintk(DEBUG_VERBOSE, "setup: bRequestType: %02x\n"
		"bRequest: %02x\nwValue: %04x\nwIndex: %04x\nwLength: %04x\n",
		creq.bRequestType, creq.bRequest, creq.wValue, creq.wIndex,
		creq.wLength);

	is_in = creq.bRequestType & USB_DIR_IN;
	/* either invoke the driver or handle the request. */
	switch (creq.bRequest) {

	case USB_REQ_GET_STATUS:
		dprintk(DEBUG_VERBOSE, "get device status\n");
		if(lf1000_get_status(udc, &creq))
			return EP0_STALL;
		return EP0_IN_DATA_PHASE;

	case USB_REQ_CLEAR_FEATURE:
		dprintk(DEBUG_VERBOSE, "clear feature\n");
		if(lf1000_clear_feature(udc, &creq))
			return EP0_STALL;
		return EP0_STATUS_PHASE;

	case USB_REQ_SET_FEATURE:
		dprintk(DEBUG_VERBOSE, "set feature\n");
		if(lf1000_set_feature(udc, &creq))
			return EP0_STALL;
		return EP0_STATUS_PHASE;

	case USB_REQ_SET_ADDRESS:
		dprintk(DEBUG_VERBOSE, "set address\n");
		/* do we take action here?  Confirm address? */
		return EP0_STATUS_PHASE;

	case USB_REQ_GET_DESCRIPTOR:
	case USB_REQ_GET_CONFIGURATION:
	case USB_REQ_GET_INTERFACE:
		dprintk(DEBUG_VERBOSE, "get descriptor/config/interface\n");
		next_state = EP0_IN_DATA_PHASE;
		/* Invoke driver */
		break;

	case USB_REQ_SET_DESCRIPTOR:
		dprintk(DEBUG_VERBOSE, "set descriptor\n");
		next_state = EP0_OUT_DATA_PHASE;
		/* Invoke driver */
		break;

	case USB_REQ_SET_CONFIGURATION:
	case USB_REQ_SET_INTERFACE:
		dprintk(DEBUG_VERBOSE, "set config/interface\n");
		/* Invoke driver */
		next_state = EP0_STATUS_PHASE;
		break;
	default:
		/* Possibly a vendor-specific request. */
		if(is_in)
			next_state = EP0_IN_DATA_PHASE;
		else
			next_state = EP0_STATUS_PHASE;
	}

	/* pass request up to the gadget driver */
	if(udc->driver) {
		status = udc->driver->setup(&udc->gadget, &creq);
		if(status < 0) {
			if(status != -EOPNOTSUPP) {
				printk(KERN_ERR "Unknown USB request: ERR: %d\n",
				       status);
			}
			next_state = EP0_STALL;
		}
	} else {
		next_state = EP0_STALL;
	}

	return next_state;
}

static void handle_ep0(struct lf1000_udc *udc)
{
	struct lf1000_ep *ep = &udc->ep[0];
	struct lf1000_request *req;
	u32 ep0csr;

	writew(0, udc->base_addr + UDC_EPINDEX);
	ep0csr = readw(udc->base_addr + UDC_EP0STAT);

	if(unlikely(IS_SET(ep0csr, SHT))) {
		dprintk(DEBUG_VERBOSE, "Clearing stall...\n");
		nuke(ep, -EPIPE);
	    	dprintk(DEBUG_NORMAL, "... clear SENT_STALL ...\n");
	    	clear_ep_stall(udc, 0);
		udc->ep0state = EP0_IDLE;
	}

	if((!IS_SET(ep0csr, RSR)) && (!IS_SET(ep0csr, TST))) {
		dprintk(DEBUG_NORMAL,
			"Unknown ep0 interrupt: ep0 state: 0x%04x\n", ep0csr);
		return;
	}

	while(1) {

		if(list_empty(&ep->queue))
			req = NULL;
		else
			req = list_entry(ep->queue.next,
					 struct lf1000_request, queue);

		dprintk(DEBUG_VERBOSE, "ep0state %s\n",
			ep0states[udc->ep0state]);

		switch (udc->ep0state) {
		case EP0_IDLE:
			if (IS_SET(ep0csr, TST)) {
				/* just finished an IN data transfer */
				writew(1<<TST, udc->base_addr + UDC_EP0STAT);
				return;
			} else if (IS_SET(ep0csr, RSR)) {
				udc->ep0state = handle_setup(udc, ep, ep0csr);
				writew(1<<RSR, udc->base_addr + UDC_EP0STAT);
			} else {
				/* This means all TSTs and RSRs have been
				 * handled, and somebody changed the state to
				 * IDLE.  Time to return from interrupt.
				 */
				return;
			}
			break;

		case EP0_IN_DATA_PHASE:
			/* We're either here because we are launching or
			 * continuing a control data transer to the host.
			 */
			if (IS_SET(ep0csr, TST)) {
				/* successfully tx'd a fragment of
				 * current req. */
				writew(1<<TST, udc->base_addr + UDC_EP0STAT);
			}

			if(req != NULL) {
				if (write_req(ep, req))
					udc->ep0state = EP0_STATUS_PHASE;
				else
					/* wait until TST interrupt */
					return;
			} else {
				dev_info(&udc->pdev->dev,
					"probably don't wanna be here.\n");
				return;
			}
			break;

		case EP0_OUT_DATA_PHASE:
			/* We're either here because we are launching or
			 * continuing a control data transer from the host.
			 */
			/* Note: If we receive a setup packet in this phase, we
			 * must handle it.  This may be a bit tricky.
			 */
			dev_warn(&udc->pdev->dev,
					"unsupported OUT transfer.\n");
			udc->ep0state = EP0_STALL;
			break;

		case EP0_STATUS_PHASE:
			if (IS_SET(ep0csr, TST)) {
				/* successfully tx'd a fragment of current req. */
				writew(1<<TST, udc->base_addr + UDC_EP0STAT);
			}
			udc->ep0state = EP0_IDLE;
			return;

		case EP0_STALL:
			/* If we stall, next invocation should be either a new
			 * setup transaction or a stall sent interrupt.
			 */
			set_ep_stall(udc, 0);
			udc->ep0state = EP0_IDLE;
			return;
		}
	}
}

static void handle_ep(struct lf1000_ep *ep)
{
	struct lf1000_udc *udc = ep->dev;
	struct lf1000_request *req;
	u32 ep_status; /* name ambiguous with ep int status */
	u32 idx;

	req = NULL;
	if (likely(!list_empty(&ep->queue)))
		req = list_entry(ep->queue.next,
				 struct lf1000_request, queue);

	idx = (u32)(ep->bEndpointAddress&0x7F);
	writew(idx, udc->base_addr + UDC_EPINDEX);
	ep_status = readw(udc->base_addr + UDC_EPSTAT);
	dprintk(DEBUG_VERBOSE, "ep%01d status:%04x.  Req pending: %s\n",
		idx, ep_status, req ? "yes" : "no");
	if (unlikely(IS_SET(ep_status, FUDR))) {
		/* handle fifo underflow */
		/* Only for ISO endpoints.  Not supported yet. */
		writew(1<<FUDR, udc->base_addr + UDC_EPSTAT);
	}

	if (unlikely(IS_SET(ep_status, FOVF))) {
		/* handle fifo overflow */
		/* Only for ISO endpoints.  Not supported yet. */
		writew(1<<FOVF, udc->base_addr + UDC_EPSTAT);
	}

	if (unlikely(IS_SET(ep_status, FFS))) {
		/* handle fifo flushed.  We probably just init'd an endpoint */
		writew(1<<FFS, udc->base_addr + UDC_EPSTAT);
	}

	if (IS_SET(ep_status, TPS)) {
		/* handle IN success */
		writew(1<<TPS, udc->base_addr + UDC_EPSTAT);
		if(req != NULL) {
			if (write_req(ep, req)) {
				/* if we just completed a req, and there is
				 * another pending, we must launch it.
				 */
				if(!list_empty(&ep->queue)) {
					req = list_entry(ep->queue.next,
							 struct lf1000_request,
							 queue);
					write_req(ep, req);
				}
			}
		} else {
			/* should we stall? */
		}
	}

	if(IS_SET(ep_status, RPS)) {
		/* handle OUT success */
		if(req != NULL)
			read_req(ep, req);

		if(ep->halted == 1) {
			dprintk(DEBUG_VERBOSE,
				"Dumping packet for halted ep\n");
			/* stall the ep and flush the data */
			set_ep_stall(udc, ep->num);
			writew(idx, udc->base_addr + UDC_EPINDEX);
			set_mask(udc, (1<<FLUSH), UDC_EPCTL);
		}

		/* If the request is NULL, but the ep is configured, we leave
		 * the data in the queue. */
		writew(1<<RPS, udc->base_addr + UDC_EPSTAT);
		return;
	}
}

static irqreturn_t lf1000_udc_irq(int irq, void *_dev)
{

	struct lf1000_udc *dev = _dev;
	void __iomem *base = dev->base_addr;
	int usb_status, tmp;
	int ep_int, ep_status;
	int test_reg;
	int ep0csr;
	int     i;
	u32	idx;

	/* Driver connected ? */
	if (!dev->driver) {
		/* Clear interrupts */
		tmp = readw(base + UDC_EPINT);
		writew(tmp, base + UDC_EPINT);
		tmp = readw(base + UDC_TEST);
		writew(tmp, base + UDC_TEST);
		tmp = readw(base + UDC_SYSSTAT);
		writew(tmp, base + UDC_SYSSTAT);
	}

	idx = readw(base + UDC_EPINDEX);

	/* Read status registers */
	usb_status = readw(base + UDC_SYSSTAT);
	ep_int = readw(base + UDC_EPINT);
	test_reg = readw(base + UDC_TEST);
	ep_status = readw(base + UDC_EPSTAT);
	ep0csr = readw(base + UDC_EP0STAT);

	dprintk(DEBUG_NORMAL, "usbs=%04x, ep_int=%04x, test=%04x "
		"ep0csr=%04x epstat=%04x\n",
		usb_status, ep_int, test_reg, ep0csr, ep_status);

	/* count VBUS interrupts separate from processing them */
	if (IS_SET(usb_status, VBUSOFF))
		if (IS_SET(usb_status, VBUSON))
			dev->vbus_int_high_low++; /* VBUS high and low set */
		else
			dev->vbus_int_low++;	  /* VBUS low only */
	else if (IS_SET(usb_status, VBUSON))
		dev->vbus_int_high++;		  /* VBUS high only */

	// may have one or both VBUSOFF and VBUSON interrupts set
	if (IS_SET(usb_status, VBUSOFF) || IS_SET(usb_status, VBUSON)) {
		udelay(20); /* debounce */
		if (IS_SET(usb_status, VBUSOFF)) {	// VBUSOFF
			dprintk(DEBUG_VERBOSE, "VBUS off IRQ\n");
			if(dev->vbus == 1 && IS_CLR(test_reg, VBUS)) {
				lf1000_udc_vbus_session(&dev->gadget, 0);
				dev->vbus_report_low++;  // processed low
			}
			writew(1<<VBUSOFF, base + UDC_SYSSTAT);
		}

		if (IS_SET(usb_status, VBUSON)) {	// VBUSON
			dprintk(DEBUG_VERBOSE, "VBUS on IRQ\n");
			if(dev->vbus == 0 && IS_SET(test_reg, VBUS)) {
				lf1000_udc_vbus_session(&dev->gadget, 1);
				dev->vbus_report_high++;  // processed high
			}
			writew(1<<VBUSON, base + UDC_SYSSTAT);
		}
		goto irq_done;
	}

	/* RESET */
	if (IS_SET(usb_status, HFRES)) {
		dprintk(DEBUG_NORMAL, "USB reset csr %x\n", ep0csr);
		if (dev->driver && dev->driver->disconnect)
			dev->driver->disconnect(&dev->gadget);
		nuke(&dev->ep[0], -ECONNRESET);
		writew(1<<TST, base + UDC_EP0STAT);
		dev->gadget.speed = USB_SPEED_UNKNOWN;
		writew(0x00, base + UDC_EPINDEX);
		writew(dev->ep[0].ep.maxpacket & 0x7FF, base + UDC_MPR);
		dev->address = 0;
		dev->ep0state = EP0_IDLE;
		clear_ep_state(dev);
		/* clear interrupt */
		writew(1<<HFRES, base + UDC_SYSSTAT);
		goto irq_done;
	}

	/* Speed Detect */
	if (IS_SET(usb_status, SDE)) {
		dprintk(DEBUG_NORMAL, "USB speed detect\n");
		if(dev->gadget.speed == USB_SPEED_UNKNOWN) {
			if(IS_SET(usb_status, HSP))
				dev->gadget.speed = USB_SPEED_HIGH;
			else
				dev->gadget.speed = USB_SPEED_FULL;
		}
		/* clear interrupt */
		writew(1<<SDE, base + UDC_SYSSTAT);
	}

	/* RESUME */
	if (IS_SET(usb_status, HFRM)) {

		dprintk(DEBUG_NORMAL, "USB resume\n");

		/* clear interrupt */
		writew(1<<HFRM, base + UDC_SYSSTAT);
		if (dev->gadget.speed != USB_SPEED_UNKNOWN
		    && dev->driver
		    && dev->driver->resume)
			dev->driver->resume(&dev->gadget);
	}

	/* SUSPEND */
	if (IS_SET(usb_status, HFSUSP)) {
		dprintk(DEBUG_NORMAL, "USB suspend\n");

		/* clear interrupt */
		writew(1<<HFSUSP, base + UDC_SYSSTAT);
		if (dev->gadget.speed != USB_SPEED_UNKNOWN
				&& dev->driver
				&& dev->driver->suspend)
			dev->driver->suspend(&dev->gadget);

		dev->ep0state = EP0_IDLE;
	}

	/* EP */
	if (IS_SET(ep_int, EP0INT)) {
		dprintk(DEBUG_VERBOSE, "USB ep0 irq\n");
		writew(1<<EP0INT, base + UDC_EPINT);
		handle_ep0(dev);
		goto irq_done;
	}
	/* endpoint data transfers */
	for (i = 1; i < LF1000_ENDPOINTS; i++) {
		if(IS_SET(ep_int, i)) {
			dprintk(DEBUG_VERBOSE, "USB ep%d irq\n", i);
			writew(1<<i, base + UDC_EPINT);
			handle_ep(&dev->ep[i]);
		}
	}

 irq_done:
	dprintk(DEBUG_VERBOSE,"irq: %d done.\n\n\n", irq);
	writew(idx, base + UDC_EPINDEX);

	return IRQ_HANDLED;
}

static int lf1000_ep_enable(struct usb_ep *_ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct lf1000_udc *dev;
	struct lf1000_ep *ep;
	unsigned int max, tmp;
	unsigned long flags;

	ep = container_of(_ep, struct lf1000_ep, ep);
	if (!_ep || !desc || ep->desc || _ep->name == ep0name ||
			desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;

	dev = ep->dev;
	if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	local_irq_save(flags);

	max = le16_to_cpu(desc->wMaxPacketSize) & 0x1fff;

	_ep->maxpacket = max & 0x7ff;
	ep->desc = desc;
	ep->halted = 0;
	ep->bEndpointAddress = desc->bEndpointAddress;

	/* disable interrupts */
	tmp = readw(dev->base_addr + UDC_EPINTEN);
	writew(tmp & ~(1<<ep->num), dev->base_addr + UDC_EPINTEN);
	writew(1<<ep->num, dev->base_addr + UDC_EPINT);

	/* reset the endpoint */
	writew(ep->num, dev->base_addr + UDC_EPINDEX);
	tmp = readw(dev->base_addr + UDC_EPDIR);
	writew(tmp & ~(1<<ep->num), dev->base_addr + UDC_EPDIR);
	tmp = readw(dev->base_addr + UDC_DCR);
	writew(tmp & ~(1<<DEN), dev->base_addr + UDC_DCR);
	writew((1<<FLUSH)|(1<<CDP)|(3<<TNPMF), dev->base_addr + UDC_EPCTL);
	writew(0, dev->base_addr + UDC_EPCTL);

	/* set type, direction, address; reset fifo counters */
	if (desc->bEndpointAddress & USB_DIR_IN) {
		tmp = readw(dev->base_addr + UDC_EPDIR);
		writew(tmp | 1<<ep->num, dev->base_addr + UDC_EPDIR);
		ep->is_in = 1;
	} else {
		tmp = readw(dev->base_addr + UDC_EPDIR);
		writew(tmp & ~(1<<ep->num), dev->base_addr + UDC_EPDIR);
		ep->is_in = 0;
	}

	tmp = readw(dev->base_addr + UDC_EPCTL);
	writew(tmp | (1<<FLUSH), dev->base_addr + UDC_EPCTL);

	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_ISOC:
		tmp = readw(dev->base_addr + UDC_EPCTL);
		tmp |= (1<<IME)|(3<<TNPMF);
		writew(tmp, dev->base_addr + UDC_EPCTL);
		break;

	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
	default:
		tmp = readw(dev->base_addr + UDC_EPCTL);
		writew(tmp & ~(1<<IME), dev->base_addr + UDC_EPCTL);
	}

	writew(max, dev->base_addr + UDC_MPR);

	clear_ep_stall(dev, ep->num);

	/* clear any pending interrupts, and enable */
	tmp = (1<<FUDR)|(1<<FOVF)|(1<<OSD)|(1<<DTCZ)|(1<<SPT)|(1<<FFS);
	tmp |= (1<<FSC)|(1<<TPS);
	/* I think we should clear this, but ME recommends otherwise. */
	/* tmp |= (1<<RPS); */
	set_mask(dev, tmp, UDC_EPSTAT);

	/* enable interrupts */
	set_mask(dev, (1<<ep->num), UDC_EPINT);
	set_mask(dev, (1<<ep->num), UDC_EPINTEN);

	/* print some debug message */
	tmp = desc->bEndpointAddress;
	dprintk (DEBUG_NORMAL, "enable %s(%d) ep%x%s-blk max %02x\n",
		_ep->name,ep->num, tmp, desc->bEndpointAddress & USB_DIR_IN ? "in" : "out", max);

	local_irq_restore(flags);

	return 0;
}

static int lf1000_ep_disable(struct usb_ep *_ep)
{
	struct lf1000_ep *ep = container_of(_ep, struct lf1000_ep, ep);
	unsigned long tmp, flags;

	if (!_ep || !ep->desc) {
		dprintk(DEBUG_NORMAL, "%s not enabled\n",
			_ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	local_irq_save(flags);

	dprintk(DEBUG_NORMAL, "ep_disable: %s\n", _ep->name);

	ep->desc = NULL;
	ep->halted = 1;

	nuke(ep, -ESHUTDOWN);

	/* disable irqs */
	tmp = readw(ep->dev + UDC_EPINTEN);
	writew(tmp & ~(1<<ep->num), ep->dev + UDC_EPINTEN);

	dprintk(DEBUG_NORMAL, "%s disabled\n", _ep->name);

	local_irq_save(flags);
	return 0;
}

static struct usb_request *lf1000_alloc_request (struct usb_ep *_ep,
		gfp_t mem_flags)
{
	struct lf1000_ep	*ep;
	struct lf1000_request	*req;

    	dprintk(DEBUG_VERBOSE,"lf1000_alloc_request(ep=%p,flags=%d)\n", _ep, mem_flags);

	ep = container_of (_ep, struct lf1000_ep, ep);
	if (!_ep)
		return NULL;

	req = kzalloc (sizeof *req, mem_flags);
	if (!req)
		return NULL;
	INIT_LIST_HEAD (&req->queue);
	return &req->req;
}

static void lf1000_free_request (struct usb_ep *_ep, struct usb_request *_req)
{
	struct lf1000_ep	*ep;
	struct lf1000_request	*req;

	dprintk(DEBUG_VERBOSE, "lf1000_free_request(ep=%p,req=%p)\n", _ep, _req);

	ep = container_of (_ep, struct lf1000_ep, ep);
	if (!ep || !_req || (!ep->desc && _ep->name != ep0name))
		return;

	req = container_of (_req, struct lf1000_request, req);
	WARN_ON (!list_empty (&req->queue));
	kfree(req);
}

static int lf1000_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gf)
{
	struct lf1000_request	*req;
	struct lf1000_ep	*ep;
	struct lf1000_udc	*udc;
	int			launch = 0;
	unsigned long flags;

	ep = container_of(_ep, struct lf1000_ep, ep);
	if (unlikely (!_req || !_ep || (!ep->desc && ep->ep.name != ep0name))) {
		dprintk(DEBUG_NORMAL, "lf1000_queue: invalid arguments.\n");
		return -EINVAL;
	}
	req = container_of(_req, struct lf1000_request, req);

	udc = ep->dev;
	if(unlikely (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN)) {
		return -ESHUTDOWN;
	}

	if (unlikely(!_req->buf)) {
		dprintk(DEBUG_NORMAL, "lf1000_queue: Buffer not allocated!\n");
		return -EINVAL;
	}

	_req->status = -EINPROGRESS;
	_req->actual = 0;

	local_irq_save(flags);

	dprintk(DEBUG_VERBOSE,"lf1000_queue: ep%x len %d\n",
		ep->bEndpointAddress, _req->length);

	/* pio or dma irq handler advances the queue.  Don't bother with
	 * zero-length packets.  Those are handled automatically by our
	 * hardware.
	 */
	if(_req->length == 0) {
		if(_req->complete) {
			_req->status = 0;
			_req->complete(_ep, _req);
		}
	} else {
		if(list_empty(&ep->queue) &&
		   (ep->bEndpointAddress != 0)) {
			/* In this case we must launch the first tranfer.
			 * Otherwise, the interrupts that result from other
			 * transfers will launch ours.
			 */
			launch = 1;
		}
		list_add_tail(&req->queue, &ep->queue);
		if(launch == 1) {
			if(ep->bEndpointAddress & USB_DIR_IN) {
				write_req(ep, req);
			} else {
				read_req(ep, req);
			}
		}
	}

	local_irq_restore(flags);
	return 0;
}

static int lf1000_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	int retval = -EINVAL;
	struct lf1000_ep *ep = container_of(_ep, struct lf1000_ep, ep);
	struct lf1000_udc *udc = ep->dev;
	struct lf1000_request *req = NULL;

    	dprintk(DEBUG_VERBOSE,"lf1000_dequeue(ep=%p,req=%p)\n", _ep, _req);

	if (!udc->driver)
		return -ESHUTDOWN;

	if (!_ep || !_req)
		return retval;

	list_for_each_entry (req, &ep->queue, queue) {
		if (&req->req == _req) {
			list_del_init (&req->queue);
			_req->status = -ECONNRESET;
			retval = 0;
			break;
		}
	}

	if (retval == 0) {
		dprintk(DEBUG_VERBOSE, "dequeued req %p from %s, len %d buf %p\n",
				req, _ep->name, _req->length, _req->buf);

		done(ep, req, -ECONNRESET);
	}

	return retval;
}

static int lf1000_set_halt(struct usb_ep *_ep, int value)
{
	int ret = 0;
	struct lf1000_ep *ep;
	struct lf1000_udc *udc;
	unsigned long len, flags;

	WARN_ON(!_ep);
	if (!_ep)
		return -EINVAL;
	ep = container_of(_ep, struct lf1000_ep, ep);

	WARN_ON(!ep->gadget);
	if (!ep->gadget)
		return -EINVAL;
	udc = container_of(ep->gadget, struct lf1000_udc, gadget);
	
	WARN_ON(!udc->driver);
	if (!udc->driver)
		return -ESHUTDOWN;

	local_irq_save(flags);

	ep->halted = value;

	writew(ep->num, udc->base_addr + UDC_EPINDEX);
	len = 2*readw(udc->base_addr +UDC_BRCR);

	if(ep->is_in && (!list_empty(&ep->queue) || len))
		ret = -EAGAIN;
	else {
		if (value) {
			dprintk(DEBUG_VERBOSE, "setting halt on ep%d\n",
				ep->num);
			set_ep_stall(udc, ep->num);
		} else {
			dprintk(DEBUG_VERBOSE, "clearing halt on ep%d\n",
				ep->num);
			clear_ep_stall(udc, ep->num);
		}
	}
	
	local_irq_restore(flags);
	return ret;
}

static const struct usb_ep_ops lf1000_ep_ops = {
	.enable         = lf1000_ep_enable,
	.disable        = lf1000_ep_disable,

	.alloc_request  = lf1000_alloc_request,
	.free_request   = lf1000_free_request,


	.queue          = lf1000_queue,
	.dequeue        = lf1000_dequeue,

	.set_halt       = lf1000_set_halt,
};

/*------------------------- usb_gadget_ops ----------------------------------*/

static int lf1000_get_frame(struct usb_gadget *_gadget)
{
	struct lf1000_udc *udc;
       
	udc = container_of(_gadget, struct lf1000_udc, gadget);
	
	return (int)(readw(udc->base_addr + UDC_FRAMENUM) & 0x7ff);
}

static int lf1000_wakeup(struct usb_gadget *_gadget)
{
	return 0;
}

static int lf1000_set_selfpowered(struct usb_gadget *_gadget, int value)
{
	struct lf1000_udc  *udc;
	unsigned long flags;

	dprintk(DEBUG_NORMAL, "lf1000_set_selfpowered()\n");

	udc = container_of (_gadget, struct lf1000_udc, gadget);

	local_irq_save(flags);

	if (value)
		udc->devstatus |= (1 << USB_DEVICE_SELF_POWERED);
	else
		udc->devstatus &= ~(1 << USB_DEVICE_SELF_POWERED);
	
	local_irq_restore(flags);

	return 0;
}

static const struct usb_gadget_ops lf1000_ops = {
	.get_frame          = lf1000_get_frame,
	.wakeup             = lf1000_wakeup,
	.set_selfpowered    = lf1000_set_selfpowered,
	.vbus_session	    = lf1000_udc_vbus_session,
};

/*------------------------- gadget driver handling---------------------------*/
/*
 * udc_disable
 */
static void udc_disable(struct lf1000_udc *dev)
{
	int tmp;
	int i;

	dprintk(DEBUG_NORMAL, "udc_disable called\n");

	lf1000_vbus_command(dev, LF1000_UDC_VBUS_DISABLE);

	/* disable phy block */
	tmp = readw(dev->base_addr + UDC_PCR) | (1<<PCE);
	writew(tmp, dev->base_addr + UDC_PCR);

	/* Disable all interrupts */
	writew(1<<RRDE, dev->base_addr + UDC_SYSCTL);
	writew(0, dev->base_addr + UDC_EPINTEN);

	/* Clear the interrupt registers */
	writew(0xFFFF, dev->base_addr + UDC_SYSSTAT);
	writew(0xFFFF, dev->base_addr + UDC_EPINT);
	tmp = readw(dev->base_addr + UDC_TEST);
	writew(tmp, dev->base_addr + UDC_TEST);

	/* Reset endpoint 0 */
	writew((1<<RSR)|(1<<TST)|(1<<SHT), dev->base_addr + UDC_EP0STAT);
	writew(1<<TTS, dev->base_addr + UDC_EP0CTL); /* TODO: Why? */
	writew(dev->ep[0].ep.maxpacket & 0x7ff, dev->base_addr + UDC_MPR);

	/* Reset endpoints */
	writew(0, dev->base_addr + UDC_EPINTEN);
	writew(0xFFFF, dev->base_addr + UDC_EPINT);
	writew(0, dev->base_addr + UDC_EPDIR);
	for (i = 1; i < LF1000_ENDPOINTS; i++) {
		writew(i, dev->base_addr + UDC_EPINDEX);
		tmp = readw(dev->base_addr + UDC_DCR) & ~(1<<DEN);
		writew(tmp, dev->base_addr + UDC_DCR);
		writew((1<<FLUSH)|(1<<CDP)|(3<<TNPMF),
				dev->base_addr + UDC_EPCTL);
	}

	/* Set speed to unknown */
	dev->gadget.speed = USB_SPEED_UNKNOWN;
}

static void udc_reinit(struct lf1000_udc *dev)
{
	int i;

	/* device/ep0 records init */
	INIT_LIST_HEAD (&dev->gadget.ep_list);
	INIT_LIST_HEAD (&dev->gadget.ep0->ep_list);
	dev->ep0state = EP0_IDLE;

	for (i = 0; i < LF1000_ENDPOINTS; i++) {
		struct lf1000_ep *ep = &dev->ep[i];

		if (i != 0)
			list_add_tail(&ep->ep.ep_list, &dev->gadget.ep_list);

		ep->dev = dev;
		ep->desc = NULL;
		ep->halted = 1;
		INIT_LIST_HEAD (&ep->queue);
	}
}

/*
 * udc_enable
 */
static void udc_enable(struct lf1000_udc *dev)
{
	int i;
	int tmp;

	dprintk(DEBUG_NORMAL, "udc_enable called\n");

	/* dev->gadget.speed = USB_SPEED_UNKNOWN; */
	dev->gadget.speed = USB_SPEED_FULL;

	/* Reverse byte order */
	tmp = readw(dev->base_addr + UDC_SYSCTL) | (1<<RRDE);
	writew(tmp, dev->base_addr + UDC_SYSCTL);

	/* Set MAXP for all endpoints */
	for (i = 0; i < LF1000_ENDPOINTS; i++) {
		writew(i, dev->base_addr + UDC_EPINDEX);
		if (i == 0) {
			writew((1<<RSR)|(1<<TST)|(1<<SHT),
					dev->base_addr + UDC_EP0STAT);
		} else {
			tmp = readw(dev->base_addr + UDC_DCR);
			BIT_CLR(tmp, DEN);
			writew(tmp, dev->base_addr + UDC_DCR);
			writew((1<<CDP)|(1<<FLUSH)|(3<<TNPMF),
					dev->base_addr + UDC_EPCTL);
		}
		writew(dev->ep[i].ep.maxpacket & 0x7FF,
				dev->base_addr + UDC_MPR);
	}

	/* disable phy block */
	tmp = readw(dev->base_addr + UDC_PCR) | (1<<PCE);
	writew(tmp, dev->base_addr + UDC_PCR);

	/* enable phy block */
	tmp = readw(dev->base_addr + UDC_PCR) & ~(1<<PCE);
	writew(tmp, dev->base_addr + UDC_PCR);

	/* Clear and enable reset and suspend interrupt interrupts */
	writew(0xFF, dev->base_addr + UDC_SYSSTAT);
	tmp = readw(dev->base_addr + UDC_SYSCTL);
	tmp |= (1<<HSUSPE)|(1<<HRESE)|(1<<SPDEN);
	writew(tmp, dev->base_addr + UDC_SYSCTL);

	/* Clear and enable ep0 interrupt */
	writew(0xFF, dev->base_addr + UDC_EPINT);
	tmp = readw(dev->base_addr + UDC_EPINTEN) | (1<<EP0INTEN);
	writew(tmp, dev->base_addr + UDC_EPINTEN);

	/* enable the vbus interrupt and let the vbus signal through */
	lf1000_vbus_command(dev, LF1000_UDC_VBUS_ENABLE);
}

static void lf1000_udc_setup_drvdata(struct lf1000_udc *udc)
{
	udc->gadget.ops			= &lf1000_ops;
	udc->gadget.ep0			= &udc->ep[0].ep;
	udc->gadget.name		= gadget_name;
	udc->gadget.dev.init_name	= "gadget";

	/* control endpoint */
	udc->ep[0].num			= 0;
	udc->ep[0].ep.name		= ep0name;
	udc->ep[0].ep.ops		= &lf1000_ep_ops;
	udc->ep[0].ep.maxpacket		= EP0_FIFO_SIZE;
	udc->ep[0].dev 			= udc;
	udc->ep[0].is_in 		= 1;

	udc->ep[1].num			= 1;
	udc->ep[1].ep.name		= "ep1";
	udc->ep[1].ep.ops		= &lf1000_ep_ops;
	udc->ep[1].ep.maxpacket		= EP_FIFO_SIZE;
	udc->ep[1].dev			= udc;
	udc->ep[1].fifo_size		= EP_FIFO_SIZE;
	udc->ep[1].bEndpointAddress	= 1;
	udc->ep[1].bmAttributes		= USB_ENDPOINT_XFER_BULK;

	udc->ep[2].num			= 2;
	udc->ep[2].ep.name		= "ep2";
	udc->ep[2].ep.ops		= &lf1000_ep_ops;
	udc->ep[2].ep.maxpacket		= EP_FIFO_SIZE;
	udc->ep[2].dev			= udc;
	udc->ep[2].fifo_size		= EP_FIFO_SIZE;
	udc->ep[2].bEndpointAddress	= 2;
	udc->ep[2].bmAttributes		= USB_ENDPOINT_XFER_BULK;
};

static struct lf1000_udc *udc_dev = NULL;

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct lf1000_udc *udc = udc_dev;
	int retval;

	/* Sanity checks */
	if (!udc)
		return -ENODEV;
	if (udc->driver)
		return -EBUSY;

	dev_info(&udc->pdev->dev, "%s '%s'\n", __FUNCTION__,
			driver->driver.name);

	if (!driver->bind || !driver->setup
			|| driver->speed < USB_SPEED_FULL) {
		dev_err(&udc->pdev->dev,
			"invalid driver: bind %p setup %p speed %d\n",
			driver->bind, driver->setup, driver->speed);
		return -EINVAL;
	}

#if defined(MODULE)
	if (!driver->unbind) {
		dev_err(&udc->pdev->dev, "invalid driver: no unbind method\n");
		return -EINVAL;
	}
#endif

	/* Hook the driver */
	udc->driver = driver;
	udc->gadget.dev.driver = &driver->driver;

	/* Bind the driver */
	if ((retval = device_add(&udc->gadget.dev)) != 0) {
		dev_err(&udc->pdev->dev, "failed to add device: %d\n", retval);
		goto register_error;
	}
	
	dev_info(&udc->pdev->dev, "binding gadget driver '%s'\n",
			driver->driver.name);

	if ((retval = driver->bind (&udc->gadget)) != 0) {
		device_del(&udc->gadget.dev);
		goto register_error;
	}

	/* Enable udc */
	udc_enable(udc);

	return 0;

register_error:
	udc->driver = NULL;
	udc->gadget.dev.driver = NULL;
	return retval;
}
EXPORT_SYMBOL(usb_gadget_register_driver);

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct lf1000_udc *udc = udc_dev;

	if (!udc)
		return -ENODEV;

	if (!driver || driver != udc->driver || !driver->unbind)
		return -EINVAL;

	dev_info(&udc->pdev->dev, "%s '%s'\n", __FUNCTION__,
		driver->driver.name);

	if (driver->disconnect)
		driver->disconnect(&udc->gadget);

	driver->unbind(&udc->gadget);

	device_del(&udc->gadget.dev);
	udc->driver = NULL;

	/* Disable udc */
	udc_disable(udc);

	return 0;
}
EXPORT_SYMBOL(usb_gadget_unregister_driver);

static int lf1000_udc_remove(struct platform_device *pdev)
{
	struct lf1000_udc *udc = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "remove\n");

	if (udc->debug)
		debugfs_remove(udc->debug);

	device_remove_file(&pdev->dev, &dev_attr_vbus);

	if(udc->input > 0)
		input_unregister_device(udc->input);
	
	lf1000_vbus_command(udc, LF1000_UDC_VBUS_SHUTDOWN);

	if(udc->base_addr) {
		writew(0, udc->base_addr + UDC_CLKEN);
		iounmap(udc->base_addr);
		release_mem_region(udc->res->start, RESSIZE(udc->res));
	}

	platform_set_drvdata(pdev, NULL);

	usb_gadget_unregister_driver(udc->driver);

	free_irq(udc->irq, udc);

	kfree(udc);
	udc_dev = NULL;

	return 0;
}

static int lf1000_udc_probe(struct platform_device *pdev)
{
	struct lf1000_udc *udc;
	int retval = 0;

	dev_info(&pdev->dev, "probe\n");

	udc = kzalloc(sizeof(struct lf1000_udc), GFP_KERNEL);
	if (!udc) {
		dev_err(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}

	lf1000_udc_setup_drvdata(udc);
	platform_set_drvdata(pdev, udc);
	udc->pdev = pdev;
	udc_dev = udc;

	udc->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(!udc->res) {
		dev_err(&pdev->dev, "failed to get resource\n");
		retval = -ENXIO;
		goto fail;
	}

	if(!request_mem_region(udc->res->start, RESSIZE(udc->res),
				"lf1000_udc")) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		retval = -ENXIO;
		retval = -EBUSY;
		goto fail;
	}

	udc->base_addr = ioremap(udc->res->start, RESSIZE(udc->res));
	if(!udc->base_addr) {
		dev_err(&pdev->dev, "failed to ioremap\n");
		retval = -ENOMEM;
		goto fail;
	}

	/* initialize vbus sensing stats */
	udc->vbus_int_high = 0;
	udc->vbus_int_high_low = 0;
	udc->vbus_int_low = 0;
	udc->vbus_report_high = 0;
	udc->vbus_report_low = 0;

	/* Set up clock */
	writew((0<<UDC_CLKDIV)|(0x3<<UDC_CLKSRCSEL),
			udc->base_addr + UDC_CLKGEN);
	writew((1<<UDC_PCLKMODE)|(1<<UDC_CLKGENENB)|(3<<UDC_CLKENB),
			udc->base_addr + UDC_CLKEN);

	/* Add the usb gadget bus */
	device_initialize(&udc->gadget.dev);
	udc->gadget.dev.parent = &pdev->dev;
	udc->gadget.dev.dma_mask = pdev->dev.dma_mask;
	udc->gadget.is_dualspeed = 1;
	udc->devstatus = 0;

	udc_disable(udc);
	udc_reinit(udc);

	/* irq setup after old hardware state is cleaned up */
	udc->irq = platform_get_irq(pdev, 0);
	if(udc->irq < 0) {
		dev_err(&pdev->dev, "can't get IRQ\n");
		retval = udc->irq;
		goto fail;
	}
	retval = request_irq(udc->irq, lf1000_udc_irq,
			IRQF_DISABLED, gadget_name, udc);

	if (retval != 0) {
		dev_err(&pdev->dev, "can't get irq %i, err %d\n",
				udc->irq, retval);
		retval = -EBUSY;
		goto fail;
	}
	dprintk(DEBUG_VERBOSE, "%s: got irq %i\n", gadget_name, udc->irq);

	/* init the vbus signal */
	retval = lf1000_vbus_command(udc, LF1000_UDC_VBUS_INIT);
	if(retval != 0) {
		dev_err(&pdev->dev, "failed to initialize vbus pin");
		goto fail;
	}

	/* set up input device for reporting VBUS */
	udc->input = input_allocate_device();
	if(!udc->input) {
		dev_err(&pdev->dev, "can't get device for vbus key hack\n");
		retval = -ENOMEM;
		goto fail;
	}
	udc->input->name = "LF1000 USB";
	udc->input->phys = "lf1000/usb";
	udc->input->id.bustype = BUS_HOST;
	udc->input->id.vendor = 0x0001;
	udc->input->id.product = 0x0001;
	udc->input->id.version = 0x0001;

	/* we only support a 'switch' event */
	udc->input->evbit[0] = BIT(EV_SW);
	/* we don't offer any keys */
	udc->input->keycode = NULL;
	udc->input->keycodesize = 0;
	udc->input->keycodemax = 0;
	
	/* reusing 'lid' for our switch */
	set_bit(SW_LID, udc->input->swbit);

	retval = input_register_device(udc->input);
	if(retval) {
		dev_err(&pdev->dev, "can't register dev for vbus key hack\n");
		goto fail;
	}

	device_create_file(&pdev->dev, &dev_attr_vbus);

	udc->debug = debugfs_create_dir("lf1000-usb-gadget", NULL);
	if (udc->debug && udc->debug != ERR_PTR(-ENODEV)) {
		debugfs_create_bool("vbus", S_IRUGO, udc->debug,
				(u32 *)&udc->vbus);
		debugfs_create_u32("vbus_int_high", S_IRUGO, udc->debug,
				&udc->vbus_int_high);
		debugfs_create_u32("vbus_int_low", S_IRUGO, udc->debug,
				&udc->vbus_int_low);
		debugfs_create_u32("vbus_int_high_low", S_IRUGO, udc->debug,
				&udc->vbus_int_high_low);
		debugfs_create_u32("vbus_report_high", S_IRUGO, udc->debug,
				&udc->vbus_report_high);
		debugfs_create_u32("vbus_report_low", S_IRUGO, udc->debug,
				&udc->vbus_report_low);
		debugfs_create_file("regs", S_IRUGO, udc->debug, udc,
				&lf1000_udc_regs_fops);
	} else {
		dev_err(&pdev->dev, "can't create debugfs files\n");
		udc->debug = NULL;
	}

	return 0;

fail:
	lf1000_udc_remove(pdev);
	return retval;
}

#ifdef CONFIG_PM
static int lf1000_udc_suspend(struct platform_device *pdev, pm_message_t message)
{
#if 0
	struct lf1000_udc *udc = platform_get_drvdata(pdev);

	if (udc_info && udc_info->udc_command)
		udc_info->udc_command(LF1000_UDC_P_DISABLE);
#endif
	return 0;
}

static int lf1000_udc_resume(struct platform_device *pdev)
{
#if 0
	struct lf1000_udc *udc = platform_get_drvdata(pdev);

	if (udc_info && udc_info->udc_command)
		udc_info->udc_command(LF1000_UDC_P_ENABLE);
#endif
	return 0;
}
#else
#define lf1000_udc_suspend      NULL
#define lf1000_udc_resume       NULL
#endif

static struct platform_driver lf1000_udc_driver = {
	.driver		= {
		.name 	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe          = lf1000_udc_probe,
	.remove         = lf1000_udc_remove,
	.suspend	= lf1000_udc_suspend,
	.resume		= lf1000_udc_resume,
};

static int __init udc_init(void)
{
	return platform_driver_register(&lf1000_udc_driver);
}

static void __exit udc_exit(void)
{
	platform_driver_unregister(&lf1000_udc_driver);
}


module_init(udc_init);
module_exit(udc_exit);

MODULE_AUTHOR("Scott Esters <sesters@leapfrog.com>");
MODULE_DESCRIPTION("LF1000 USB Device Controller Gadget");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:lf1000_udc");
