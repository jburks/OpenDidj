/* drivers/mmc/host/lf1000_mmc.c - LF1000 SDIO Host Controller Driver
 *
 * Copyright (c) 2010 Leapfrog Enterprises Inc.
 *
 * Andrey Yurovsky <ayurovsky@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This host controller supports:
 * - MMC spec 4.2
 * - SD memory card spec 2.0
 * - SDIO card spec 1.10
 * - 1-bit and 4-bit data bus modes
 * - PIO and DMA (PIO not implemented)
 * - up to 50MHz bus clock
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/sizes.h>

#include <mach/platform.h>
#include <mach/common.h>
#include <mach/gpio.h>
#include <mach/dma.h>

#include "lf1000_mmc.h"

#if !defined(CONFIG_MMC_LF1000_CHANNEL0) && \
    !defined(CONFIG_MMC_LF1000_CHANNEL1)
#warning "No host controllers enabled.  Enable at least one."
#endif

#define RESSIZE(res) (((res)->end - (res)->start)+1)

#define MAX_CHANNELS	2

#define COMPLETION_TIMEOUT	(2*1000)
#define DRIVER_NAME		"lf1000-sdio"
#define SDIO_CLK_SRC		PLL1
#define	SDIO_CLK_DIV		3
#define LF1000_SDIO_DIV_400KHZ	62	/* divider for 400KHz */
#define LF1000_SDIO_DIV		0	/* divider for full speed */

#define SDIO_GPIO_PORT	GPIO_PORT_B
#define SDIO_GPIO_FUNC	GPIO_ALT1

/* TODO: these are platform resources, move them and/or integrate with the
 * GPIO framework and request them */
static u8 sdio_pins[MAX_CHANNELS][6] = {
	{
		2,  /* Data 0 */
		3,  /* Data 1 */
		4,  /* Data 2 */
		5,  /* Data 3 */
		0,  /* Clock */
		1,  /* Command */
	},
	{
		8,  /* Data 0 */
		9,  /* Data 1 */
		10, /* Data 2 */
		11, /* Data 3 */
		6,  /* Clock */
		7,  /* Command */
	},
};

struct lf1000_sdio_host {
	struct mmc_host		*mmc;
	struct resource 	*mem;
	void __iomem		*base;
	unsigned int		phys;
	struct platform_device	*pdev;

	char			name[6];	/* "sdioN" */
	u8			channel;
	int			dma_channel;
	int			dma_id;
	struct lf1000_dma_desc	dma_desc;
	bool			dma_active;
	enum dma_data_direction	dma_dir;
	int			dma_nents;
	u8			irq;
	int			div;
	u32			clock_hz;
	u8			bus_width;
	unsigned char		power_mode;
	bool			sdio_irq_en;

	struct mmc_request	*mrq;
	struct mmc_data		*data;

	struct completion	dma_transfer;

	/* debugging */
	struct dentry		*debug;
};

static void lf1000_regs_show_reg(struct seq_file *s, const char *nm, u32 reg)
{
	struct lf1000_sdio_host *host = s->private;

	seq_printf(s, "%10s:\t0x%08X\n", nm, readl(host->base + reg));
}

static int lf1000_regs_show(struct seq_file *s, void *v)
{
	lf1000_regs_show_reg(s, "CTRL", SDI_CTRL);
	lf1000_regs_show_reg(s, "CLKDIV", SDI_CLKDIV);
	lf1000_regs_show_reg(s, "CLKENA", SDI_CLKENA);
	lf1000_regs_show_reg(s, "TMOUT", SDI_TMOUT);
	lf1000_regs_show_reg(s, "CTYPE", SDI_CTYPE);
	lf1000_regs_show_reg(s, "BLKSIZ", SDI_BLKSIZ);
	lf1000_regs_show_reg(s, "BYTCNT", SDI_BYTCNT);
	lf1000_regs_show_reg(s, "INTMASK", SDI_INTMASK);
	lf1000_regs_show_reg(s, "CMDARG", SDI_CMDARG);
	lf1000_regs_show_reg(s, "CMD", SDI_CMD);
	lf1000_regs_show_reg(s, "RESP0", SDI_RESP0);
	lf1000_regs_show_reg(s, "RESP1", SDI_RESP1);
	lf1000_regs_show_reg(s, "RESP2", SDI_RESP2);
	lf1000_regs_show_reg(s, "RESP3", SDI_RESP3);
	lf1000_regs_show_reg(s, "MINTSTS", SDI_MINTSTS);
	lf1000_regs_show_reg(s, "RINTSTS", SDI_RINTSTS);
	lf1000_regs_show_reg(s, "STATUS", SDI_STATUS);
	lf1000_regs_show_reg(s, "FIFOTH", SDI_FIFOTH);
	lf1000_regs_show_reg(s, "TCBCNT", SDI_TCBCNT);
	lf1000_regs_show_reg(s, "TBBCNT", SDI_TBBCNT);
	lf1000_regs_show_reg(s, "DAT", SDI_DAT);
	lf1000_regs_show_reg(s, "SYSCLKENB", SDI_SYSCLKENB);
	lf1000_regs_show_reg(s, "CLKGEN", SDI_CLKGEN);

	return 0;
}

static void lf1000_status_show_bit(struct seq_file *s, const char *nm, u32 v)
{
	seq_printf(s, "%10s:\t%d\n", nm, !!v);
}

static void lf1000_status_show_hex(struct seq_file *s, const char *nm, u32 v)
{
	seq_printf(s, "%10s:\t0x%X\n", nm, v);
}

static int lf1000_status_show(struct seq_file *s, void *v)
{
	struct lf1000_sdio_host *host = s->private;
	u32 status = readl(host->base + SDI_STATUS);

	lf1000_status_show_bit(s, "DMAREQ", status & (1<<DMAREQ));
	lf1000_status_show_bit(s, "DMAACK", status & (1<<DMAACK));
	lf1000_status_show_hex(s, "FIFOCOUNT", (status>>FIFOCOUNT) & 0x1F);
	lf1000_status_show_hex(s, "RSPINDEX", (status>>RSPINDEX) & 0x3F);
	lf1000_status_show_bit(s, "FSMBUSY", status & (1<<FSMBUSY));
	lf1000_status_show_bit(s, "DATBUSY", status & (1<<DATBUSY));
	lf1000_status_show_bit(s, "CPRESENT", status & (1<<CPRESENT));
	lf1000_status_show_hex(s, "CMDFSM", (status>>CMDFSM) & 0xF);
	lf1000_status_show_bit(s, "FIFOFULL", status & (1<<FIFOFULL));
	lf1000_status_show_bit(s, "FIFOEMPTY", status & (1<<FIFOEMPTY));
	lf1000_status_show_bit(s, "TXWMARK", status & (1<<TXWMARK));
	lf1000_status_show_bit(s, "RXWMARK", status & (1<<RXWMARK));

	return 0;
}

static int lf1000_sdio_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, lf1000_regs_show, inode->i_private);
}

static int lf1000_sdio_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, lf1000_status_show, inode->i_private);
}

static const struct file_operations lf1000_sdio_regs_fops = {
	.owner		= THIS_MODULE,
	.open		= lf1000_sdio_regs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations lf1000_sdio_status_fops = {
	.owner		= THIS_MODULE,
	.open		= lf1000_sdio_status_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void lf1000_sdio_init_debugfs(struct lf1000_sdio_host *host)
{
	struct dentry *dir;
       
	dir = debugfs_create_dir("lf1000-sdio", NULL);
	if (!dir || IS_ERR(dir)) {
		host->debug = NULL;
		return;
	}

	debugfs_create_file("registers", S_IRUSR, dir, host,
			&lf1000_sdio_regs_fops);
	debugfs_create_file("status", S_IRUSR, dir, host,
			&lf1000_sdio_status_fops);

	host->debug = dir;
	
}

/*
 * Hardware helpers.
 */

static void lf1000_sdio_reset_controller(struct lf1000_sdio_host *host)
{
	u32 tmp = readl(host->base + SDI_CTRL);

	tmp &= ~((1<<DMARST)|(1<<FIFORST));
	tmp |= (1<<CTRLRST);
	writel(tmp, host->base + SDI_CTRL);

	while (readl(host->base + SDI_CTRL) & (1<<CTRLRST));
}

static void lf1000_sdio_set_dma(struct lf1000_sdio_host *host, bool en)
{
	u32 tmp = readl(host->base + SDI_CTRL) & ~(1<<DMA_ENA);

	writel(tmp | (en<<DMA_ENA), host->base + SDI_CTRL);
}

static void lf1000_sdio_reset_dma(struct lf1000_sdio_host *host)
{
	u32 tmp = readl(host->base + SDI_CTRL);

	tmp &= ~((1<<CTRLRST)|(1<<FIFORST));
	tmp |= (1<<DMARST);
	writel(tmp, host->base + SDI_CTRL);

	while (readl(host->base + SDI_CTRL) & (1<<DMARST));
}

static void lf1000_sdio_reset_fifo(struct lf1000_sdio_host *host)
{
	u32 tmp = readl(host->base + SDI_CTRL);

	tmp &= ~((1<<DMARST)|(1<<CTRLRST));
	tmp |= (1<<FIFORST);
	writel(tmp, host->base + SDI_CTRL);

	while (readl(host->base + SDI_CTRL) & (1<<FIFORST));
}

/* Set the RX and TX FIFO thresholds.  The FIFOs are 64 bytes (16 words) long
 * and the recommended values are a TX threshold of (length/2) or 8 and an RX
 * threshold of ((length/2)-1) or 7. */
static void lf1000_sdio_set_fifo_th(struct lf1000_sdio_host *host)
{
	u32 tmp = readl(host->base + SDI_FIFOTH) & ~((0xF<<RXTH)|(0x3<<TXTH));

	tmp |= (7<<RXTH)|(8<<TXTH);

	writel(tmp, host->base + SDI_FIFOTH);
}

static void lf1000_sdio_set_width(struct lf1000_sdio_host *host, u8 width)
{
	u32 tmp = readl(host->base + SDI_CTYPE);

	if (width == MMC_BUS_WIDTH_4)
		tmp |= (1<<WIDTH);
	else
		tmp &= ~(1<<WIDTH);

	writel(tmp, host->base + SDI_CTYPE);
}

static inline void lf1000_sdio_clear_int_status(struct lf1000_sdio_host *host)
{
	writel(0xFFFFFFFF, host->base + SDI_RINTSTS);
}

static void lf1000_sdio_interrupt_enable(struct lf1000_sdio_host *host)
{
	u32 tmp = readl(host->base + SDI_CTRL);
	tmp |= (1<<INT_ENA)/*|(1<<SEND_IRQ_RESP)*/;
	writel(tmp, host->base + SDI_CTRL);
}

static int check_command_error(struct lf1000_sdio_host *host, u32 irqm)
{
	if (irqm & (1<<HLEINT)) {
		dev_err(&host->pdev->dev, "HW locked error\n");
		return -EIO;
	}

	if (irqm & (1<<REINT)) {
		dev_err(&host->pdev->dev, "response error\n");
		return -EIO;
	}

	if (irqm & (1<<RCRCINT)) {
		dev_err(&host->pdev->dev, "bad CRC\n");
		return -EILSEQ;
	}

	if (irqm & (1<<RTOINT)) {
		/* This happens when the stack probes for different types of
		 * cards and is expected.  For example, an SD card will time
		 * out when probed as an SDIO card. */
		dev_dbg(&host->pdev->dev, "response timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int check_data_error(struct lf1000_sdio_host *host, u32 irqm)
{
	if (irqm & (1<<FRUNINT)) {
		dev_err(&host->pdev->dev, "FIFO error\n");
		return -EIO;
	}
	
	if (irqm & (1<<DRTOINT)) {
		dev_err(&host->pdev->dev, "data read response timeout\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static void lf1000_sdio_setup_controller(struct lf1000_sdio_host *host)
{
	/* Turn off clock output, turn off clock low power mode.  The clock
	 * output will be enabled later by the MMC subsystem. */
	writel(0, host->base + SDI_CLKENA);

	/* use PLL1/3, 147MHz/3 = 49MHz */
	writel((2<<CLKSRCSEL0)|((SDIO_CLK_DIV-1)<<CLKDIV0),
			host->base + SDI_CLKGEN);
	writel((1<<PCLKMODE)|(1<<CLKGENENB), host->base + SDI_SYSCLKENB);
	/* set SDIO clock to "detect" rate, ~400KHz:
	 * 49MHz/(62*2) = ~400KHz */
	writel(62, host->base + SDI_CLKDIV);

	lf1000_sdio_interrupt_enable(host);

	lf1000_sdio_reset_controller(host);
	lf1000_sdio_reset_dma(host);
	lf1000_sdio_reset_fifo(host);

	lf1000_sdio_set_dma(host, 1);

	lf1000_sdio_set_width(host, MMC_BUS_WIDTH_1);

	/* Set host data and response timeouts. */
	writel((0xFFFFFF<<DTMOUT)|(0xFF<<RSPTMOUT), host->base + SDI_TMOUT);

	/* Set a defult block size: 512B is typical for MMC/SD cards. */
	writel(512, host->base + SDI_BLKSIZ);

	lf1000_sdio_set_fifo_th(host);

	/* Disable interrupts and clear any pending. */
	writel(0, host->base + SDI_INTMASK);
	lf1000_sdio_clear_int_status(host);
}

static void lf1000_sdio_clock_disable(struct lf1000_sdio_host *host)
{
	u32 reg = readl(host->base + SDI_SYSCLKENB);
	
	reg &= ~(1<<CLKGENENB);

	writel(reg, host->base + SDI_SYSCLKENB);
}

static void lf1000_sdio_enable_irq(struct mmc_host *mmc, int enable)
{
	struct lf1000_sdio_host *host = mmc_priv(mmc);

	host->sdio_irq_en = enable;
}

/*
 * Command and data transfer.
 */

/* Read the command result and, if there is a data stage and we are writing,
 * launch a DMA request.  If the command resulted in an error or if there is
 * no data stange, we are done handling this command and we report that to the
 * stack.  Otherwise we need to wait for the data stage to complete before we
 * report to the stack that the request is complete. */
static int lf1000_sdio_command_complete(struct lf1000_sdio_host *host)
{
	struct mmc_request *mrq = host->mrq;
	
	if (mrq->cmd->flags & MMC_RSP_PRESENT) {
		if (mrq->cmd->flags & MMC_RSP_136) {
			mrq->cmd->resp[0] = readl(host->base + SDI_RESP3);
			mrq->cmd->resp[1] = readl(host->base + SDI_RESP2);
			mrq->cmd->resp[2] = readl(host->base + SDI_RESP1);
			mrq->cmd->resp[3] = readl(host->base + SDI_RESP0);
		} else
			mrq->cmd->resp[0] = readl(host->base + SDI_RESP0);
	}

	if (mrq->data && (mrq->data->flags & MMC_DATA_WRITE)) {
		dev_dbg(&host->pdev->dev, "launching DMA TX\n");
		lf1000_dma_int_en(host->dma_channel);
		WARN_ON(lf1000_dma_launch(host->dma_channel,
					&host->dma_desc));
	}

	if (mrq->cmd->error || !mrq->data) {
		host->mrq = NULL;
		mmc_request_done(host->mmc, mrq);
		
		return 1;
	}

	return 0;
}

static void lf1000_sdio_dma_done(struct lf1000_sdio_host *host)
{
	lf1000_dma_int_dis(host->dma_channel);
	host->dma_active = 0;
	dma_unmap_sg(mmc_dev(host->mmc), host->data->sg, host->dma_nents,
			host->dma_dir);
	host->data = NULL;
	complete(&host->dma_transfer);
}

static void lf1000_sdio_transfer_complete(struct lf1000_sdio_host *host)
{
	struct mmc_data *data = host->mrq->data;

	dev_dbg(&host->pdev->dev, "data transfer complete\n");

	if (!data)
		return;

	if (data->error) {
		dev_err(&host->pdev->dev, "data error\n");
		if (host->dma_active)
			lf1000_sdio_dma_done(host);
	} else {
		data->bytes_xfered = data->blocks * data->blksz;
	}

	/* Complete the command now that the data transfer portion has
	 * finished. */
	host->mrq = NULL;
	mmc_request_done(host->mmc, data->mrq);
	return;
}

/* DMA transfer completion callback: mark the transfer as complete and turn off
 * DMA until we need it again. */
static void lf1000_sdio_dma(int channel, void *priv, unsigned int pending)
{
	struct lf1000_sdio_host *host = (struct lf1000_sdio_host *)priv;

	WARN_ON(channel != host->dma_channel);
	WARN_ON(!host->dma_active);
	WARN_ON(pending != 0);

	if (host->dma_active && channel == host->dma_channel) {
		dev_dbg(&host->pdev->dev, "DMA transfer completed\n");
		lf1000_sdio_dma_done(host);
	}
}

static irqreturn_t lf1000_sdio_irq(int irq, void *dev_id)
{
	struct lf1000_sdio_host *host = (struct lf1000_sdio_host *)dev_id;
	u32 irqm = readl(host->base + SDI_MINTSTS);

	dev_dbg(&host->pdev->dev, "IRQ: 0x%08X\n", irqm);

	if (host->mrq) {
		host->mrq->cmd->error = check_command_error(host, irqm);
		if (host->mrq->cmd->error) {
			dev_dbg(&host->pdev->dev, "command error\n");
			lf1000_sdio_command_complete(host);
			goto out_req;
		}

		if (irqm & (1<<CDINT)) {
			dev_dbg(&host->pdev->dev, "command done\n");
			if (lf1000_sdio_command_complete(host))
				goto out_req;
		}

		if (host->mrq->data && (irqm & (1<<DTOINT))) {
			dev_dbg(&host->pdev->dev, "data transfer over\n");
			host->mrq->data->error = check_data_error(host, irqm);
			lf1000_sdio_transfer_complete(host);
		}
	}

out_req:
	if (irqm & (1<<SDIOINT)) {
		dev_dbg(&host->pdev->dev, "SDIO interrupt occured\n");
		mmc_signal_sdio_irq(host->mmc);
	}

	writel(irqm, host->base + SDI_RINTSTS);
	return IRQ_HANDLED;
}

/* Set up and launch a transfer from the controller to memory. */
static void lf1000_sdio_start_dma_rx(struct lf1000_sdio_host *host)
{
	struct lf1000_dma_desc *desc = &host->dma_desc;
	struct scatterlist *sg = host->mrq->data->sg;
	u8 *buffer = page_address(sg_page(sg)) + sg->offset;

	dev_dbg(&host->pdev->dev, "%s\n", __FUNCTION__);

	if (host->dma_active)
		wait_for_completion(&host->dma_transfer);

	init_completion(&host->dma_transfer);
	host->dma_active = 1;

	lf1000_dma_initdesc(desc);

	host->dma_dir = DMA_FROM_DEVICE;
	host->dma_nents = dma_map_sg(mmc_dev(host->mmc), host->mrq->data->sg,
			host->mrq->data->sg_len, host->dma_dir);

	desc->src = (dma_addr_t)(host->mem->start);
	desc->dst = (dma_addr_t)virt_to_phys(buffer);
	desc->len = sg_dma_len(sg);
	desc->flags = (DMA_SRC_IO|DMA_DST_MEM|DMA_SRC_32BIT|DMA_DST_32BIT|
			DMA_SRC_NOINC);
	desc->id = host->dma_id;

	lf1000_dma_int_en(host->dma_channel);
	
	WARN_ON(lf1000_dma_launch(host->dma_channel, desc));
}

/* Set up a transfer from memory to the controller.  The transfer will be 
 * launched once the hardware is ready. */
static void lf1000_sdio_start_dma_tx(struct lf1000_sdio_host *host)
{
	struct lf1000_dma_desc *desc = &host->dma_desc;
	struct scatterlist *sg = host->mrq->data->sg;
	u8 *buffer = page_address(sg_page(sg)) + sg->offset;

	dev_dbg(&host->pdev->dev, "%s\n", __FUNCTION__);

	if (host->dma_active)
		wait_for_completion(&host->dma_transfer);

	init_completion(&host->dma_transfer);
	host->dma_active = 1;
	
	lf1000_dma_initdesc(desc);

	host->dma_dir = DMA_TO_DEVICE;
	host->dma_nents = dma_map_sg(mmc_dev(host->mmc), host->mrq->data->sg,
			host->mrq->data->sg_len, host->dma_dir);

	desc->src = (dma_addr_t)virt_to_phys(buffer);
	desc->dst = (dma_addr_t)(host->mem->start);
	desc->len = sg_dma_len(sg);
	desc->flags = (DMA_SRC_MEM|DMA_DST_IO|DMA_SRC_32BIT|DMA_DST_32BIT|
			DMA_DST_NOINC);
	desc->id = host->dma_id;
}

static void lf1000_sdio_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	u32 irqm = 0;
	u32 flags = 0;
	struct lf1000_sdio_host *host = mmc_priv(mmc);
	u8 resp = mmc_resp_type(mrq->cmd);

	WARN_ON(host->mrq != NULL);
	host->mrq = mrq;
	
	flags |= (1<<STARTCMD)|(mrq->cmd->opcode & 0x3F);

	dev_dbg(&host->pdev->dev, "cmd resp flags: 0x%X\n", resp);

	/* Send 80 clocks before the first command for initialization */
	if (mrq->cmd->opcode == MMC_GO_IDLE_STATE ||
	    mrq->cmd->opcode == SD_IO_SEND_OP_COND)
		flags |= (1<<SENDINIT);

	if (mrq->cmd->opcode != MMC_SEND_STATUS)
		flags |= (1<<WAITPRVDAT);

	if (resp & MMC_RSP_PRESENT) { /* expect a response */
		flags |= (1<<RSPEXP);
		irqm |= (1<<MSKRTO);
		if (resp & MMC_RSP_136) /* expect a long response */
			flags |= (1<<RSPLEN);
	}

	if (resp & MMC_RSP_CRC) /* expect valid CRC */
		flags |= (1<<CHKRSPCRC);

	/* We always want to know about command completion, HW errors, and 
	 * response errors. */
	irqm |= (1<<MSKHLE)|(1<<MSKRE)|(1<<MSKCD);

	/* Preserve the SDIO Interrupt setting. */
	irqm |= (host->sdio_irq_en<<MSKSDIOINT);

	if (mrq->data) {
		u32 length = mrq->data->blocks * mrq->data->blksz;

		dev_dbg(&host->pdev->dev, "transferring %d bytes\n", length);

		host->data = mrq->data;

		if (mrq->data->blocks > 1 &&
			(mrq->cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
			 mrq->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK)) {
			dev_dbg(&host->pdev->dev, "setting autostop\n");
			flags |= (1<<SENDAUTOSTOP);
			irqm |= (1<<MSKACD);
		}

		writel(mrq->data->blksz, host->base + SDI_BLKSIZ);
		writel(length, host->base + SDI_BYTCNT);

		irqm |= (1<<MSKDTO)|(1<<MSKHTO)|(1<<MSKFRUN);
		flags |= (1<<DATEXP);

		if (mrq->data->flags & MMC_DATA_STREAM) {
			dev_dbg(&host->pdev->dev, "data stream requested\n");
			flags |= (1<<TRMODE);
		}

		if (mrq->data->flags & MMC_DATA_READ) {
			irqm |= (1<<MSKDRTO);
			lf1000_sdio_start_dma_rx(host);
		}
		
		if (mrq->data->flags & MMC_DATA_WRITE) {
			flags |= (1<<RW);
			lf1000_sdio_start_dma_tx(host);
		}
	} else if (mrq->stop) {
		flags |= (1<<STOPABORT);
	}

	dev_dbg(&host->pdev->dev, "submitting cmd: 0x%04X irqm: 0x%04X\n",
			flags, irqm);

	/* Submit the command. */
	lf1000_sdio_clear_int_status(host);
	writel(irqm, host->base + SDI_INTMASK);
	writel(mrq->cmd->arg, host->base + SDI_CMDARG);
	writel(flags, host->base + SDI_CMD);
}

static int lf1000_sdio_get_ro(struct mmc_host *mmc)
{
	/* we don't support RO detection */
	return -ENOSYS;
}

static void lf1000_sdio_set_clock_out(struct lf1000_sdio_host *host, bool en)
{
	writel((1<<CLKENA), host->base + SDI_CLKENA);
}

static int lf1000_sdio_update_clock(struct lf1000_sdio_host *host)
{
	u32 tout, tmp;

	/* send a clock update command and wait for it to complete, repeat if
	 * a HLEINT occurs */
	while (1) {
		writel((1<<STARTCMD)|(1<<UPDATECLKONLY)|(1<<WAITPRVDAT),
				host->base + SDI_CMD);

		tout = 0;
		while (readl(host->base + SDI_CMD) & (1<<STARTCMD))
			if (++tout > 0x1000000)
				return 1;

		tmp = readl(host->base + SDI_RINTSTS);
		if (!(tmp & (1<<HLEINT)))
			break;

		tmp |= (1<<HLEINT);
		writel(tmp, host->base + SDI_RINTSTS);
	}

	return 0;
}

static int lf1000_sdio_set_clock_hz(struct lf1000_sdio_host *host, u32 hz)
{
	int div; 

	if (hz == 400000)
		div = LF1000_SDIO_DIV_400KHZ;
	else if (hz == host->mmc->f_max)
		div = LF1000_SDIO_DIV;
	else {
		u32 clk_hz = get_pll_freq(SDIO_CLK_SRC)/SDIO_CLK_DIV;
		div = lf1000_CalcDivider(clk_hz, hz);
		if (div < 0)
			return 1;
		div >>= 1;
	}
	dev_dbg(&host->pdev->dev, "setting DIV=%d\n", div);

	/* disable the SDIO clock and set the divider */
	lf1000_sdio_set_clock_out(host, 0);
	writel(div & 0xFF, host->base + SDI_CLKDIV);
	host->div = div;

	if (lf1000_sdio_update_clock(host)) {
		dev_err(&host->pdev->dev, "can't set clock: disabled\n");
		return 1;
	}

	lf1000_sdio_set_clock_out(host, 1);

	if (lf1000_sdio_update_clock(host)) {
		dev_err(&host->pdev->dev, "can't set clock\n");
		return 1;
	}

	writel((1<<PCLKMODE)|(1<<CLKGENENB), host->base + SDI_SYSCLKENB);

	return 0;
}

static void lf1000_sdio_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct lf1000_sdio_host *host = mmc_priv(mmc);

	if (ios->bus_width != host->bus_width) {
		lf1000_sdio_set_width(host, ios->bus_width);
		host->bus_width = ios->bus_width;
	}

	if (ios->clock != host->clock_hz) {
		if (ios->clock > 0) {
			if (lf1000_sdio_set_clock_hz(host, ios->clock))
				dev_err(&host->pdev->dev,
						"can't set clock rate\n");
			else {
				host->clock_hz = ios->clock;
				mdelay(100);
			}
		} else
			lf1000_sdio_clock_disable(host);
	}

	if (ios->power_mode != host->power_mode) {
		switch (ios->power_mode) {
			case MMC_POWER_OFF:
				lf1000_sdio_set_clock_out(host, 0);
				break;

			case MMC_POWER_ON:
				lf1000_sdio_set_clock_out(host, 1);
				lf1000_sdio_reset_dma(host);
				lf1000_sdio_reset_fifo(host);
				mdelay(5);
				break;

			case MMC_POWER_UP:
				mdelay(5);
				break;

			default:
				break;
		}
		host->power_mode = ios->power_mode;
	}
}

static struct mmc_host_ops lf1000_sdio_ops = {
	.request 		= lf1000_sdio_request,
	.get_ro			= lf1000_sdio_get_ro,
	.set_ios		= lf1000_sdio_set_ios,
	.enable_sdio_irq	= lf1000_sdio_enable_irq,
};

static void lf1000_sdio_setup_pins(struct lf1000_sdio_host *host)
{
	int i;

	if (host->channel >= MAX_CHANNELS) {
		dev_err(&host->pdev->dev, "invalid SDIO channel\n");
		return;
	}

	for (i = 0; i < 6; i++)
		gpio_configure_pin(SDIO_GPIO_PORT,
				   sdio_pins[host->channel][i],
				   SDIO_GPIO_FUNC, 1, 0, 1);
}

static int lf1000_sdio_probe_channel(struct platform_device *pdev, u8 channel)
{
	struct mmc_host *mmc;
	struct lf1000_sdio_host *host = NULL;
	struct resource *res;
	int ret, irq;

	res = platform_get_resource(pdev, IORESOURCE_MEM, channel);
	irq = platform_get_irq(pdev, channel);
	if (!res || irq < 0)
		return -ENXIO;

	mmc = mmc_alloc_host(sizeof(struct lf1000_sdio_host), &pdev->dev);
	if (!mmc)
		return -ENOMEM;

	mmc->ops = &lf1000_sdio_ops;

	mmc->max_hw_segs = 1;
	mmc->max_phys_segs = 16;
	mmc->max_blk_size = 512;
	mmc->max_blk_count = 4096-1;
	mmc->max_req_size = mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_seg_size = mmc->max_req_size;

	mmc->f_min = 400000;
	mmc->f_max = get_pll_freq(SDIO_CLK_SRC)/SDIO_CLK_DIV;
	mmc->ocr_avail = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 |
	       MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 |
	       MMC_VDD_34_35 | MMC_VDD_35_36;

	mmc->caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED |
		MMC_CAP_SDIO_IRQ | MMC_CAP_NONREMOVABLE;

	res = request_mem_region(res->start, RESSIZE(res), DRIVER_NAME);
	if (!res) {
		dev_err(&pdev->dev, "failed to get IO memory region\n");
		ret = -EBUSY;
		goto out_mem;
	}

	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->mem = res;
	host->phys = res->start;
	host->pdev = pdev;
	host->channel = channel;
	host->div = 0;
	host->clock_hz = 0;
	host->dma_id = channel == 0 ? DMA_SD0RW : DMA_SD1RW;
	host->dma_active = 0;
	host->bus_width = MMC_BUS_WIDTH_1;
	host->sdio_irq_en = 0;

	snprintf(host->name, sizeof(host->name), "sdio%1d", host->channel);

	host->base = ioremap(res->start, res->end - res->start + 1);
	if (!host->base) {
		dev_err(&pdev->dev, "failed to remap\n");
		ret = -ENOMEM;
		goto out_mem;
	}

	host->irq = platform_get_irq(pdev, channel);
	if (host->irq == 0) {
		dev_err(&pdev->dev, "failed to get an IRQ number\n");
		ret = -EINVAL;
		goto out_remap;
	}

	ret = request_irq(host->irq, lf1000_sdio_irq, 0, DRIVER_NAME, host);
	if (ret) {
		dev_err(&pdev->dev, "can't get SDIO%d IRQ\n", host->channel);
		ret = -ENOENT;
		goto out_irq;
	}

	host->dma_channel = lf1000_dma_request(host->name, DMA_PRIO_HIGH,
			lf1000_sdio_dma, (void *)host);
	if (host->dma_channel < 0) {
		dev_err(&pdev->dev, "can't get DMA channel\n");
		goto out_dma;
	}

	/* prepare the hardware */
	lf1000_sdio_setup_pins(host);
	lf1000_sdio_setup_controller(host);

	platform_set_drvdata(pdev, mmc);

	lf1000_sdio_init_debugfs(host);

	mmc_add_host(mmc);

	dev_dbg(&host->pdev->dev, "\"%s\" probe complete\n", host->name);

	return 0;

out_dma:
	free_irq(host->irq, host);
out_irq:
	iounmap(host->base);
out_remap:
	release_mem_region(host->mem->start, RESSIZE(host->mem));
out_mem:
	mmc_free_host(mmc);

	return ret;
}

static int lf1000_sdio_probe(struct platform_device *pdev)
{
	int ret;

#ifdef CONFIG_MMC_LF1000_CHANNEL0
	ret = lf1000_sdio_probe_channel(pdev, 0);
	if (ret)
		return ret;
#endif
#ifdef CONFIG_MMC_LF1000_CHANNEL1
	ret = lf1000_sdio_probe_channel(pdev, 1);
#endif
	return ret;
}

static int lf1000_sdio_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	struct lf1000_sdio_host *host = NULL;

	if (mmc) {
		host = mmc_priv(mmc);

		if (host->debug)
			debugfs_remove_recursive(host->debug);

		mmc_remove_host(mmc);
		lf1000_sdio_clock_disable(host);
		writel(0, host->base + SDI_INTMASK);
		lf1000_sdio_clear_int_status(host);
		free_irq(host->irq, host);
		lf1000_dma_int_dis(host->dma_channel);
		lf1000_dma_free(host->dma_channel);
		if (host->dma_active)
			complete(&host->dma_transfer);
		iounmap(host->base);
		release_mem_region(host->mem->start, RESSIZE(host->mem));
		release_resource(host->mem);
	}
	
	return 0;
}

#ifdef CONFIG_PM
static int lf1000_sdio_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int lf1000_sdio_resume(struct platform_device *dev)
{
	return 0;
}
#else
#define lf1000_sdio_suspend	NULL
#define lf1000_sdio_resume	NULL
#endif

struct platform_driver lf1000_sdio_driver = {
	.probe		= lf1000_sdio_probe,
	.remove		= lf1000_sdio_remove,
	.suspend	= lf1000_sdio_suspend,
	.resume		= lf1000_sdio_resume,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init lf1000_sdio_init(void)
{
	return platform_driver_register(&lf1000_sdio_driver);
}

static void __exit lf1000_sdio_exit(void)
{
	platform_driver_unregister(&lf1000_sdio_driver);
}

module_init(lf1000_sdio_init);
module_exit(lf1000_sdio_exit);

MODULE_DESCRIPTION("LF1000 SDIO Host Controller Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrey Yurovsky");
MODULE_ALIAS("platform:lf1000-sdio");
