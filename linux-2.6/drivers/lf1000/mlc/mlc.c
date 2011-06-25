/* 
 * drivers/lf1000/mlc/mlc.c
 *
 * LF1000 Multi-Layer Controller (MLC) Driver 
 *
 * Copyright 2007 Leapfrog Enterprises Inc.
 *
 * Andrey Yurovsky <andrey@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/mlc.h>
#include <mach/common.h>

#include "mlc_hal.h"
#include "mlc_priv.h"

extern struct mlc_device mlc;

static void *SelectLayerControl(u8 layer)
{
	void *ctl = mlc.mem;

	if(!mlc.mem) {
		return 0;
	}
	switch(layer) {
		case 0:
		ctl += MLCCONTROL0;
		break;
		case 1:
		ctl += MLCCONTROL1;
		break;
		case 2:
		ctl += MLCCONTROL2;
		break;
		default:
		return 0;
	}

	return ctl;
}

/****************
 * API Routines *
 ****************/

int mlc_SetAddress(u8 layer, u32 addr)
{
	void *reg = NULL;

	if(layer > MLC_NUM_LAYERS) 
		return -EINVAL;

	if(!mlc.mem)
		return -ENOMEM;
	
#if 0	/* MCU registers need to be mapped */
	if (layer == MLC_VIDEO_LAYER) {
		u32 tmp = ioread32(MEMCONTROL);
		if (addr & 0x20000000)
			tmp |= 1;
		else
			tmp &= ~1;
		iowrite32(tmp, MEMCONTROL);
	}
#endif		

	switch(layer) {
		case 0:
		reg = mlc.mem+MLCADDRESS0;
		break;
		case 1:
		reg = mlc.mem+MLCADDRESS1;
		break;
		case 2:
		reg = mlc.mem+MLCADDRESS3; /* note: weird datasheet naming */
		break;
	}
	iowrite32(addr, reg);
	return 0;
}

int mlc_SetAddressCb(u8 layer, u32 addr)
{
	if (layer != MLC_VIDEO_LAYER) 
		return -EINVAL;
	iowrite32(addr, mlc.mem+MLCADDRESSCB);
	return 0;
}

int mlc_SetAddressCr(u8 layer, u32 addr)
{
	if (layer != MLC_VIDEO_LAYER) 
		return -EINVAL;
	iowrite32(addr, mlc.mem+MLCADDRESSCR);
	return 0;
}

int mlc_GetAddress(u8 layer, int *addr) /* FIXME */
{
	void *reg = NULL;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;
	
	switch(layer) {
		case 0:
		reg = mlc.mem+MLCADDRESS0;
		break;
		case 1:
		reg = mlc.mem+MLCADDRESS1;
		break;
		case 2:
		reg = mlc.mem+MLCADDRESS3; /* note: weird datasheet naming */
		break;
	}

	*addr = ioread32(reg);
	return 0;
}

int mlc_SetHStride(u8 layer, u32 hstride)
{
	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	hstride &= 0x7FFFFFFF;
	iowrite32(hstride,mlc.mem+MLCHSTRIDE0+layer*0x34);
	return 0;
}

int mlc_GetHStride(u8 layer)
{
	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	return ioread32(mlc.mem+MLCHSTRIDE0+layer*0x34);
}

int mlc_SetVStride(u8 layer, u32 vstride)
{
	void *reg = NULL;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	switch(layer) {
		case 0:
		reg = mlc.mem+MLCVSTRIDE0;
		break;
		case 1:
		reg = mlc.mem+MLCVSTRIDE1;
		break;
		case 2:
		reg = mlc.mem+MLCVSTRIDE3; /* note: weird datasheet naming */
		break;
	}

	iowrite32(vstride, reg);
	if (layer == MLC_VIDEO_LAYER) {
		iowrite32(vstride, mlc.mem+MLCSTRIDECB);
		iowrite32(vstride, mlc.mem+MLCSTRIDECR);
	}
	return 0;
}

int mlc_GetVStride(u8 layer)
{
	void *reg = NULL;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	switch(layer) {
		case 0:
		reg = mlc.mem+MLCVSTRIDE0;
		break;
		case 1:
		reg = mlc.mem+MLCVSTRIDE1;
		break;
		case 2:
		reg = mlc.mem+MLCVSTRIDE3; /* note: weird datasheet naming */
		break;
	}
	return ioread32(reg);
}

int mlc_SetLockSize(u8 layer, u32 locksize)
{
	u32 tmp;
	void *reg;

	/* make sure we're working with a valid lock size */
	if(!(locksize == 4 || locksize == 8 || locksize == 16))
		return -EINVAL;

	if(layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	tmp &= ~(3<<LOCKSIZE);
	tmp |= ((locksize/8)<<LOCKSIZE);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetLockSize(u8 layer, int *locksize) /*FIXME*/
{
	u32 tmp;
	void *reg;

	if(layer == MLC_VIDEO_LAYER || layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	*locksize = ((tmp & (3<<LOCKSIZE))>>LOCKSIZE)*8;
	return 0;
}

int mlc_SetBlendEnable(u8 layer, u8 en)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	en ? BIT_SET(tmp,BLENDENB) : BIT_CLR(tmp,BLENDENB);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetBlendEnable(u8 layer, int *en) /*FIXME*/
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	*en = IS_SET(tmp,BLENDENB) ? 1 : 0;
	return 0;
}

int mlc_SetTransparencyEnable(u8 layer, u8 en)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	en ? BIT_SET(tmp,TPENB) : BIT_CLR(tmp,TPENB);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetTransparencyEnable(u8 layer, int *en)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);

	tmp = ioread32(reg);
	*en = IS_SET(tmp,TPENB) ? 1 : 0;
	return 0;
}

int mlc_SetInvertEnable(u8 layer, u8 en)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER) 
		return -EINVAL;

	reg = SelectLayerControl(layer);

	tmp = ioread32(reg);
	en ? BIT_SET(tmp,INVENB) : BIT_CLR(tmp,INVENB);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetInvertEnable(u8 layer, int *en)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);

	tmp = ioread32(reg);
	*en = IS_SET(tmp,INVENB) ? 1 : 0;
	return 0;
}

int mlc_SetTransparencyColor(u8 layer, u32 color)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer) + MLCTPCOLOR0 - MLCCONTROL0;

	tmp = ioread32(reg);
	tmp &= ~(0xFFFFFF<<TPCOLOR);
	tmp |= ((0xFFFFFF & color)<<TPCOLOR);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetTransparencyColor(u8 layer, int *color)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer) + MLCTPCOLOR0 - MLCCONTROL0;
	tmp = ioread32(reg);
	*color = ((tmp & (0xFFFFFF<<TPCOLOR))>>TPCOLOR);
	return 0;
}

int mlc_SetTransparencyAlpha(u8 layer, u8 alpha)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	if (layer == MLC_VIDEO_LAYER) 
		reg = mlc.mem+MLCTPCOLOR3;
	else
		reg = mlc.mem+MLCTPCOLOR0+layer*0x34;

	tmp = ioread32(reg);
	tmp &= ~(0xF<<ALPHA);
	tmp |= ((0xF & alpha)<<ALPHA);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetTransparencyAlpha(u8 layer)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	if (layer == MLC_VIDEO_LAYER) 
		reg = mlc.mem+MLCTPCOLOR3;
	else
		reg = mlc.mem+MLCTPCOLOR0+layer*0x34;
	
	tmp = ioread32(reg);
	return ((tmp & (0xF<<ALPHA))>>ALPHA);
}

int mlc_SetInvertColor(u8 layer, u32 color)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = mlc.mem+MLCINVCOLOR0+layer*0x34;
	
	tmp = ioread32(reg);
	tmp &= ~(0xFFFFFF<<INVCOLOR);
	tmp |= ((0xFFFFFF & color)<<INVCOLOR);
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetInvertColor(u8 layer, int *color)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = mlc.mem+MLCINVCOLOR0+layer*0x34;

	tmp = ioread32(reg);
	*color = ((tmp & (0xFFFFFF<<INVCOLOR))>>INVCOLOR);
	return 0;
}

void mlc_SetTopDirtyFlag(void)
{
	u32 tmp = ioread32(mlc.mem+MLCCONTROLT);

	BIT_SET(tmp,DITTYFLAG);
	iowrite32(tmp,mlc.mem+MLCCONTROLT);
}

int mlc_Set3DEnable(u8 layer, u8 en)
{
	void *reg;
	u32 tmp;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);

	en ? BIT_SET(tmp,GRP3DENB) : BIT_CLR(tmp,GRP3DENB);
	iowrite32(tmp, reg);
	return 0;
}

int mlc_Get3DEnable(u8 layer, int *en) /*FIXME*/
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	*en = IS_SET(tmp,GRP3DENB) ? 1 : 0;
	return 0;
}

void mlc_SetMLCEnable(u8 en)
{
	u32 tmp = ioread32(mlc.mem+MLCCONTROLT);

	BIT_CLR(tmp,DITTYFLAG);

	if(en) {
		BIT_SET(tmp,PIXELBUFFER_PWD); 	/* power up */
		iowrite32(tmp, mlc.mem+MLCCONTROLT);
		BIT_SET(tmp,PIXELBUFFER_SLD); 	/* disable sleep */
		iowrite32(tmp, mlc.mem+MLCCONTROLT);
		BIT_SET(tmp,MLCENB);		/* enable */
		iowrite32(tmp, mlc.mem+MLCCONTROLT);
		BIT_SET(tmp,DITTYFLAG);
	}
	else {
		BIT_CLR(tmp,MLCENB);		/* disable */
		BIT_SET(tmp,DITTYFLAG);
		iowrite32(tmp, mlc.mem+MLCCONTROLT);
		do { /* wait for MLC to turn off */
			tmp = ioread32(mlc.mem+MLCCONTROLT);
		} while(IS_SET(tmp,DITTYFLAG));
		BIT_CLR(tmp,PIXELBUFFER_SLD);	/* enable sleep */
		iowrite32(tmp, mlc.mem+MLCCONTROLT);
		BIT_CLR(tmp,PIXELBUFFER_PWD);	/* power down */
	}

	iowrite32(tmp,mlc.mem+MLCCONTROLT);
}

int mlc_SetScreenSize(u32 width, u32 height)
{
	if( width-1 >= 4096 || height-1 >= 4096 )
		return -EINVAL;

	iowrite32((((height-1)<<SCREENHEIGHT)|((width-1)<<SCREENWIDTH)),
				mlc.mem+MLCSCREENSIZE);
	return 0;
}

void mlc_GetScreenSize(struct mlc_screen_size *size)
{
	u32 tmp = ioread32(mlc.mem+MLCSCREENSIZE);

	size->width  = ((tmp & (0x7FF<<SCREENWIDTH))>>SCREENWIDTH)+1;
	size->height = ((tmp & (0x7FF<<SCREENHEIGHT))>>SCREENHEIGHT)+1;
}

void mlc_SetBackground(u32 color)
{
	iowrite32((0xFFFFFF & color),mlc.mem+MLCBGCOLOR);
}

u32 mlc_GetBackground(void)
{
	return ioread32(mlc.mem+MLCBGCOLOR);
}

void mlc_SetClockMode(u8 pclk, u8 bclk)
{
	u32 tmp = ioread32(mlc.mem+MLCCLKENB);

	tmp &= ~(0xF);
	tmp |= ((pclk<<_PCLKMODE)|(bclk<<BCLKMODE));
	iowrite32(tmp,mlc.mem+MLCCLKENB);
}

int mlc_SetLayerPriority(u32 priority)
{
	u32 tmp;

	if(priority >= VID_PRIORITY_INVALID)
		return -EINVAL;

	tmp = ioread32(mlc.mem+MLCCONTROLT);
	tmp &= ~(0x3<<PRIORITY);
	tmp |= (priority<<PRIORITY);
	iowrite32(tmp,mlc.mem+MLCCONTROLT);
	return 0;
}

u32 mlc_GetLayerPriority(void)
{
	u32 tmp = ioread32(mlc.mem+MLCCONTROLT);
	return ((tmp & (0x3<<PRIORITY))>>PRIORITY);
}

void mlc_SetFieldEnable(u8 en)
{
	u32 tmp = ioread32(mlc.mem+MLCCONTROLT);
	en ? BIT_SET(tmp,FIELDENB) : BIT_CLR(tmp,FIELDENB);
	iowrite32(tmp,mlc.mem+MLCCONTROLT);
}

/* Note: on the LF1000, we can't set the format for the video layer */
int mlc_SetFormat(u8 layer, enum RGBFMT format)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER || 
			format > 0xFFFF)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	tmp &= ~(0xFFFF<<FORMAT); /* clear format bits */
	tmp |= (format<<FORMAT); /* set format */
	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetFormat(u8 layer, int *format)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	*format = ((tmp & (0xFFFF<<FORMAT))>>FORMAT);
	return 0;
}

int mlc_SetPosition(u8 layer, s32 top, s32 left, s32 right, s32 bottom)
{
	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	right--;
	bottom--;

	top &= 0x7FF;
	left &= 0x7FF;
	right &= 0x7FF;
	bottom &= 0x7FF;

	iowrite32(((left<<LEFT)|(right<<RIGHT)),
			mlc.mem+MLCLEFTRIGHT0+0x34*layer);
	iowrite32(((top<<TOP)|(bottom<<BOTTOM)),
			mlc.mem+MLCTOPBOTTOM0+0x34*layer);
	return 0;
}

int mlc_GetPosition(u8 layer, struct mlc_layer_position *p)
{
	u32 tmp;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	tmp = ioread32(mlc.mem+MLCLEFTRIGHT0+0x34*layer);
	p->left = ((tmp & (0x7FF<<LEFT))>>LEFT);
	p->right  = ((tmp & (0x7FF<<RIGHT))>>RIGHT);

	tmp = ioread32(mlc.mem+MLCTOPBOTTOM0+0x34*layer);
	p->top  = ((tmp & (0x7FF<<TOP))>>TOP);
	p->bottom = ((tmp & (0x7FF<<BOTTOM))>>BOTTOM);
	return 0;
}

int mlc_SetLayerEnable(u8 layer, u8 en)
{
	void *reg;
	u32 tmp;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	BIT_SET(tmp,PALETTEPWD); /* power up */
	iowrite32(tmp,reg);
	en ? BIT_SET(tmp,PALETTESLD) : BIT_CLR(tmp,PALETTESLD); /* disable sleep mode */
	iowrite32(tmp,reg);
	en ? BIT_SET(tmp,LAYERENB) : BIT_CLR(tmp,LAYERENB);

	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetLayerEnable(u8 layer)
{
	void *reg;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);

	return  !!IS_SET(ioread32(reg), LAYERENB);
}

int mlc_SetDirtyFlag(u8 layer)
{
	void *reg;
	u32 tmp;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	BIT_SET(tmp,DIRTYFLAG);

	iowrite32(tmp,reg);
	return 0;
}

int mlc_GetDirtyFlag(u8 layer)
{
	void *reg;
	u32 tmp, ret=false;

	if(layer > MLC_NUM_LAYERS)
		return -EINVAL;

	reg = SelectLayerControl(layer);
	tmp = ioread32(reg);
	ret = IS_SET(tmp,DIRTYFLAG) ? 1 : 0;

	return ret;
}

int mlc_SetOverlaySize(u8 layer, u32 srcwidth, u32 srcheight, u32 dstwidth, 
		u32 dstheight)
{
	/* Enable adjusted ratio with bilinear filter for upscaling */
	if (srcwidth < dstwidth)
		iowrite32((1<<28) | (((srcwidth-1)<<11)/(dstwidth-1)), mlc.mem+MLCHSCALE);
	else
		iowrite32((srcwidth<<11)/(dstwidth), mlc.mem+MLCHSCALE);
	/* Ditto for height which scales independently of width */
	if (srcheight < dstheight)	
		iowrite32((1<<28) | (((srcheight-1)<<11)/(dstheight-1)), mlc.mem+MLCVSCALE);
	else
		iowrite32((srcheight<<11)/(dstheight), mlc.mem+MLCVSCALE);
	return 0;
}

int mlc_GetOverlaySize(u8 layer, struct mlc_overlay_size *psize)
{
	u32 hscale = ioread32(mlc.mem+MLCHSCALE);
	u32 vscale = ioread32(mlc.mem+MLCVSCALE);

	psize->srcwidth = (hscale>>11) & 0x7FF;
	psize->srcheight = hscale & 0x7FF;
	psize->dstwidth = (vscale>>11) & 0x7FF;
	psize->dstheight = vscale & 0x7FF;

	return 0;
}


int mlc_SetLayerInvisibleAreaEnable(u8 layer, u8 en)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = mlc.mem+MLCLEFTRIGHT0_0+layer*0x34;
	tmp = ioread32(reg);
	en ? BIT_SET(tmp, UNVALIDENB) : BIT_CLR(tmp, UNVALIDENB);
	iowrite32(tmp, reg);

	return 0;
}
EXPORT_SYMBOL(mlc_SetLayerInvisibleAreaEnable);

int mlc_GetLayerInvisibleAreaEnable(u8 layer)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	reg = mlc.mem+MLCLEFTRIGHT0_0+layer*0x34;
	tmp = ioread32(reg);

	return IS_SET(tmp, UNVALIDENB) ? 1 : 0;
}
EXPORT_SYMBOL(mlc_GetLayerInvisibleAreaEnable);

int mlc_SetLayerInvisibleArea(u8 layer, s32 top, s32 left, s32 right, s32 bottom)
{
	u32 tmp;
	void *reg;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	top &= 0x7FF;
	left &= 0x7FF;
	right &= 0x7FF;
	bottom &= 0x7FF;

	reg = mlc.mem+MLCLEFTRIGHT0_0+layer*0x34;
	tmp = ioread32(reg);
	tmp &= ~((0x7FF<<UNVALIDLEFT)|(0x7FF<<UNVALIDRIGHT));
	tmp |= (left<<UNVALIDLEFT)|(right<<UNVALIDRIGHT);
	iowrite32(tmp, reg);

	reg = mlc.mem+MLCTOPBOTTOM0_0+layer*0x34;
	tmp = ioread32(reg);
	tmp &= ~((0x7FF<<UNVALIDTOP)|(0x7FF<<UNVALIDBOTTOM));
	tmp |= (left<<UNVALIDTOP)|(right<<UNVALIDBOTTOM);
	iowrite32(tmp, reg);

	return 0;
}
EXPORT_SYMBOL(mlc_SetLayerInvisibleArea);

int mlc_GetLayerInvisibleArea(u8 layer, struct mlc_layer_position *p)
{
	u32 tmp;

	if(layer > MLC_NUM_LAYERS || layer == MLC_VIDEO_LAYER)
		return -EINVAL;

	tmp = ioread32(mlc.mem+MLCLEFTRIGHT0_0+0x34*layer);
	p->left = ((tmp & (0x7FF<<LEFT))>>LEFT);
	p->right  = ((tmp & (0x7FF<<RIGHT))>>RIGHT);

	tmp = ioread32(mlc.mem+MLCTOPBOTTOM0_0+0x34*layer);
	p->top  = ((tmp & (0x7FF<<TOP))>>TOP);
	p->bottom = ((tmp & (0x7FF<<BOTTOM))>>BOTTOM);

	return 0;
}
EXPORT_SYMBOL(mlc_GetLayerInvisibleArea);
